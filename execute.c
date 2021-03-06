#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "global.h"
#include "bison.tab.h"

#define MAX_CMD 10

#define JOB_RUNNING 	1
#define JOB_STOPPED	2
extern void yy_scan_string(char *s); 

typedef struct {
  char args[10][128];
  char input[128];
  char output[128];
  int args_count;
} COMMAND;

typedef struct {
  int pid;
  int state;
  char args[10][128];
  int args_count;
} JOB;

COMMAND g_cmd[MAX_CMD];
JOB g_job[MAX_CMD];

int current_cmd_index=0;
int current_job_index=0;
int is_bg=0;
int now_pid=-1;

//int pipe_fd[MAX_CMD][2];
int pipe_fd[MAX_CMD][2];

int com_pwd(char *ignore)
{
  char dir[1024], *s;

  s = getcwd (dir, sizeof(dir) - 1);
  if (s == 0)
    {
      printf ("Error getting pwd: %s\n", dir);
      return 1;
    }

  printf ("Current directory is %s\n", dir);
  return 0;
}

int com_cd(char *arg)
{
  if (chdir (arg) == -1)
    {
      perror (arg);
      return 1;
    }

  com_pwd ("");
  return 0;
}


void background(void)
{
  is_bg=1;	
}

int add_jobs(int pid,int state,int cmd_index)
{
	g_job[current_job_index].pid=pid;
	g_job[current_job_index].state=state;
	memcpy(g_job[current_job_index].args,g_cmd[cmd_index].args,sizeof(g_cmd[cmd_index].args));
	g_job[current_job_index].args_count=g_cmd[cmd_index].args_count;
	current_job_index++;
	
	return 0;
}

int dump_jobs(void)
{
	int i=0;
	int j=0;
	printf("PID\t\tSTATE\t\tCMD\n");
	for(i=0;i<current_job_index;i++)
	{
		if(g_job[i].state==1) 
		{
		 	printf("%d\t\t%s\t\t",g_job[i].pid, "running");
	
			for(j=0;j<g_job[i].args_count;j++)
				printf("%s ",g_job[i].args[j]);
			printf("&\n");
		}
		else if(g_job[i].state==2) 
		{
		 	printf("%d\t\t%s\t\t",g_job[i].pid, "stopped");
			for(j=0;j<g_job[i].args_count;j++)
				printf("%s ",g_job[i].args[j]);
			printf("&\n");
		}
	}
	   	
}

int find_top_job_pid(int *index)
{
	int i=0;
	for(i=0;i<current_job_index;i++)
	{
		if(g_job[i].state!=0) 
		{
			*index=i;
			return g_job[i].pid;
		}
	}
	return -1;
}

int find_index_by_job_pid(int pid)
{
	int i=0;
	for(i=0;i<current_job_index;i++)
	{
		if(g_job[i].state!=0) 
		{
			if(g_job[i].pid==pid)
				return i;
		}
	}

}
int fg_cmd(int pid)
{
	if(pid==0)
	{
		int index=0;
		int pid_find=find_top_job_pid(&index);
		if(pid_find!=-1)
		{
			kill(pid_find,SIGCONT);
			g_job[index].state=0;
			is_bg=0;
			now_pid=pid_find;
			while(!is_bg)
			{
				if(waitpid(pid_find, NULL, WNOHANG)>0) break;
			}	
			now_pid=-1;
		}
	}
	else 
	{
	   int index=find_index_by_job_pid(pid);
	   kill(pid,SIGCONT);
	   g_job[index].state=0;
	   now_pid=pid;
	   waitpid(pid, NULL, 0);	   
	}
	return 0;
}

int bg_cmd(int pid)
{
	if(pid==0)
	{
		int index=0;
		int pid=find_top_job_pid(&index);
		if(pid!=-1)
		{
			kill(pid,SIGCONT);
			g_job[index].state=JOB_RUNNING;
		}
	}
	else 
	{
	   int index=find_index_by_job_pid(pid);
	   kill(pid,SIGCONT);
	   g_job[index].state=JOB_RUNNING;	   
	}
	return 0;
}



void CTRL_Z_DEAL()
{
	if(now_pid!=-1)
	{
	  add_jobs(now_pid,JOB_STOPPED,0);
	  kill(now_pid,SIGSTOP);
	  is_bg=1;
	  now_pid=-1;
	}
}

void CTRL_C_DEAL()
{
	if(now_pid!=-1)
	{
	  kill(now_pid,SIGQUIT);
	  now_pid=-1;
	}
}

void execue_init()
{
  memset(g_cmd,0,sizeof(g_cmd));
  current_cmd_index=0;
  is_bg=0;
  now_pid=-1;
}


void add_args(char *s)
{
//   printf("add_args\n");
   int current_args_index=g_cmd[current_cmd_index].args_count;
   strcpy(g_cmd[current_cmd_index].args[current_args_index],s);	
   free(s);	
   g_cmd[current_cmd_index].args_count++;		
}

void add_args_local(char *s)
{
   int current_args_index=g_cmd[current_cmd_index].args_count;
   strcpy(g_cmd[current_cmd_index].args[current_args_index],s);	
   g_cmd[current_cmd_index].args_count++;
}

int simple_cmd()
{
	current_cmd_index++;	
}

int pipe_cmd()
{
   return 0;
}

int input_cmd()
{
   strcpy(g_cmd[current_cmd_index-1].input,g_cmd[current_cmd_index].args[0]);
   memset(&g_cmd[current_cmd_index],0,sizeof(g_cmd[current_cmd_index]));
   return 0;
}

int output_cmd()
{
   strcpy(g_cmd[current_cmd_index-1].output,g_cmd[current_cmd_index].args[0]);
   memset(&g_cmd[current_cmd_index],0,sizeof(g_cmd[current_cmd_index]));
   return 0;
}

void dump(void)
{
  printf("##############DUMP BEGIN#########\n");
  int i=0;
  int j=0;
  for(i=0;i<current_cmd_index;i++)
  {
   	int current_args_index=g_cmd[i].args_count;
	printf("###cmd[%d]:###\t",i);
	for(j=0;j<current_args_index;j++)
	{
	  printf("args[%d]:%s,",j,g_cmd[i].args[j]);	
	}
	if(g_cmd[i].input[0]=='\0')  
	{
		if(i==0)		
			printf("INPUT:stdin,");
		else
			printf("INPUT:pipe,");			
	}	
	else
	{
		printf("INPUT:%s,",g_cmd[i].input);
	}
	if(g_cmd[i].output[0]=='\0') 
	{
		 if(i==current_cmd_index-1)
		 	printf("OUTPUT:stdout,");
		 else
		 	printf("OUTPUT:pipe,");			
	}
	else
	{
		printf("OUTPUT:%s,",g_cmd[i].output);
	}
	printf("is_bg=%d",is_bg);
	printf("\n");
  }
  printf("##############DUMP END#########\n");
}


int execute_cmd(int i)
{	
//	printf("execute_cmd\n");
	int j=0;
	char **argv= (char**)malloc(sizeof(char*)*(g_cmd[i].args_count+1));
   	argv[g_cmd[i].args_count] = NULL;
	for(j=0;j<g_cmd[i].args_count;j++)
		 argv[j]=g_cmd[i].args[j];
//	for(j = 0; j<g_cmd[i].args_count+1; j++)
//		printf("argv[%d]=%s,",j,argv[j]);
//	printf("\n");
	
	char tmp[1024];
	memset(tmp,0,1024);
	sprintf(tmp,"%s/%s",dir_path,g_cmd[i].args[0]);
	if(access(tmp,F_OK)!=0)
	{
		for(j=0;j<env_count;j++)
		{
			memset(tmp,0,1024);
			sprintf(tmp,"%s/%s",env_data[j],g_cmd[i].args[0]);
//			printf("tmp:%s\n",tmp);
			if(access(tmp,F_OK)==0)
			{	
				memcpy(g_cmd[i].args[0],tmp,sizeof(g_cmd[i].args[0]));
				break;
			}
		}
	}

	if(execvp(g_cmd[i].args[0],argv)<0)
	{
		printf("execv error\n");
		free(argv);
		return -1;
	}
//	printf("execute_cmd done\n");
	free(argv);
	return 0;
}

int run_cmd(int i,int cmd_amount)
{
	int pid;
	FILE *input_fp=NULL;
	FILE *output_fp=NULL;
	if(i!=0) //not first cmd
	{
		pipe(pipe_fd[i-1]);
		pid=fork();
		if(pid==0) //child
		{
			if(run_cmd(i-1,cmd_amount)==-1) return -1;
		}
		else
		{
			waitpid(pid, NULL, 0);
		} 
	}	
//	printf ( "run_cmd i=%d\n",i);
	if ( i != 0 )    //not first cmd	
	{
//	    printf("read stdin from pile[%d] \n",i-1);
            close ( pipe_fd[i-1][1] );
            close ( fileno ( stdin ) );
            dup2 ( pipe_fd[i-1][0] , fileno(stdin));
            close ( pipe_fd[i-1][0] );
	}

	else //first cmd
	{
	
	    if(g_cmd[0].input[0]!='\0')
	    {
//	        printf("first cmd,g_cmd[cmd_amount-1].input=%s\n",g_cmd[cmd_amount-1].input);
		input_fp=fopen(g_cmd[0].input,"r");
		if(input_fp)
		{
		 	close ( fileno ( stdin ) );
		    	dup2 ( fileno(input_fp) , fileno(stdin));
		}
	    }
	}


	if ( i != cmd_amount-1 )  //not last cmd
	{
//		printf("write stdout to pile[%d] \n",i);
		close ( pipe_fd[i][0] ); //close read
		close ( fileno(stdout));
		dup2 ( pipe_fd[i][1] , fileno(stdout));
		close ( pipe_fd[i][1] );

	}
	
	else //last cmd
	{
	    if(g_cmd[cmd_amount-1].output[0]!='\0')
	    {
//		printf("last cmd,g_cmd[cmd_amount-1].output=%s\n",g_cmd[cmd_amount-1].output);
		output_fp=fopen(g_cmd[cmd_amount-1].output,"w");
		if(output_fp)
		{
		 	close ( fileno ( stdout ) );
		    	dup2 ( fileno(output_fp) , fileno(stdout));
		}
	    }	
	}

	if(execute_cmd(i)==-1) return -1;
	if(input_fp) fclose(input_fp);
	if(output_fp) fclose(output_fp);
}


int execute(void)		
{
  int cmd_amount=current_cmd_index;
//  printf("execute,cmd_amount=%d\n",cmd_amount);
  dump();
  int i=0;  
  int j=0;
  if(cmd_amount==1)
  {
	if(strcmp(g_cmd[0].args[0],"jobs")==0)
	{
		dump_jobs();		
		return 0;
	}
	else if(strcmp(g_cmd[0].args[0],"fg")==0)
	{
		if(g_cmd[0].args_count==1)
		{
			fg_cmd(0);
			return 0;
		}
		else if(strcmp(g_cmd[0].args[1],"%")==0 && g_cmd[0].args_count==3)
		{
			fg_cmd(atoi(g_cmd[0].args[2]));
			return 0;
		}
		else
		{
			printf("usage:fg;fg %% <pid>\n");
			return 0;
		}
			
	}
	else if(strcmp(g_cmd[0].args[0],"bg")==0)
	{
		if(g_cmd[0].args_count==1)
		{
			bg_cmd(0);
			return 0;
		}
		else if(strcmp(g_cmd[0].args[1],"%")==0 && g_cmd[0].args_count==3)
		{
			bg_cmd(atoi(g_cmd[0].args[2]));
			return 0;
		}
		else
		{
			printf("usage:bg;bg %% <pid>\n");
			return 0;
		}
	}
	else if(strcmp(g_cmd[0].args[0],"history")==0)
	{
		register HIST_ENTRY **the_list;
         	register int i;
         	the_list = history_list ();
          	if (the_list)
            		for (i = 0; the_list[i]; i++)
              			printf ("%d: %s\n", i + history_base, the_list[i]->line);
		return 0;
	}
	else if(strcmp(g_cmd[0].args[0],"cd")==0)
	{
		if(g_cmd[0].args_count==2)
		{
			com_cd(g_cmd[0].args[1]);
		}
		return 0;
	}	
	else if( (strcmp(g_cmd[0].args[0],"quit")==0) || (strcmp(g_cmd[0].args[0],"exit")==0) || (strcmp(g_cmd[0].args[0],"q")==0) )
	{
		exit(0);
	}
  }
  int pid=fork();
  if(pid==0) //child process
  {
//	printf("child process\n");
	run_cmd(cmd_amount-1,cmd_amount); 
//	printf("done \n");		  			  
	return 0;
  }
  else
  {	
	if(!is_bg)
	{
		now_pid=pid;
		while(!is_bg)
		{
			if(waitpid(pid, NULL, WNOHANG)>0) break;
		}	
		now_pid=-1;
	}	
	else
	{
		add_jobs(pid,JOB_RUNNING,0);
	}
//	printf("execute pid=%d down\n",pid);
   } 

}

int found_val_index_by_name(int var_total,char *name)
{

}

int exec_srcipt(char *path)
{
	char buf[256];
	char var_val[10][128];
	char var_name[10][128];	
	char value[128];
	memset(buf,0,sizeof(buf));
	memset(var_val,0,sizeof(var_val));
	memset(var_name,0,sizeof(var_name));	
	memset(value,0,sizeof(value));
	char *p_start=NULL;
	char *p_end=NULL;
	char cmd_buf[256];
	memset(cmd_buf,0,256);
	int var_index=0;
	FILE *fp=fopen(path,"r");
	if(fp==NULL)
	{
		return -1;
	}
	if(fgets(buf,sizeof(buf),fp)==NULL)
	{
		fclose(fp);
		return -1;
	}
	while(fgets(buf,sizeof(buf),fp)!=NULL)
	{			
//		printf("buf=%s\n",buf);
		if(strstr(buf,"=\"")!=NULL)    //found val=""
		{
			p_start=strstr(buf,"=\"");
			strcpy(var_val[var_index],p_start+1);
			memcpy(var_name[var_index],buf,p_start-buf);
//			printf("var_name=%s,var_val=%s\n",var_name[var_index],var_val[var_index]);
			var_index++;
		}
		else if(strstr(buf,"$")!=NULL)   //found $
		{
			p_start=strstr(buf,"$");
			if(p_start)
			{
				p_end=strstr(p_start+1," ");
				if(p_end)
				{
					memcpy(value,p_start+1,p_end-p_start-1);
				}
				else
				{
					memcpy(value,p_start+1,strlen(buf)-(p_start-buf)-2);
				}
//				printf("value=%s,strlen=%d\n",value,strlen(value));
				int i=0;
				for(i=0;i<var_index;i++)
				{	
					if(strcmp(var_name[i],value)==0) //found
					{
						execue_init();
						memset(cmd_buf,0,256);
						memcpy(cmd_buf,buf,p_start-buf-1);
						//printf("cmd_buf=%s\n",cmd_buf);
						add_args_local(cmd_buf);
						memset(cmd_buf,0,256);
						memcpy(cmd_buf,var_val[i],strlen(var_val[i])+1);
						//printf("cmd_buf=%s\n",cmd_buf);
						add_args_local(cmd_buf);
						int pid=fork();
						if(pid==0)
						{
							execute_cmd(0);
							break;
						}
						else
						{
							waitpid(pid, NULL, 0);
						 	break;
						}						
						break;
					}
				}			
			}	
		}
		else
		{
			execue_init();
			yy_scan_string(buf);
			yyparse();
		}
		memset(buf,0,sizeof(buf));
	}
	fclose(fp);
}
