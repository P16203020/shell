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

#define MAX_CMD 10

#define JOB_RUNNING 	1
#define JOB_STOPPED	2

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
int is_simple_cmd=0;
int is_bg=0;
int now_pid=-1;

//int pipe_fd[MAX_CMD][2];
int pipe_fd[MAX_CMD][2];
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

int execue_init()
{
  memset(g_cmd,0,sizeof(g_cmd));
  current_cmd_index=0;
  is_simple_cmd=-1;
  is_bg=0;
  now_pid=-1;
}


void add_args(char *s)
{
   printf("add_args\n");
   int current_args_index=g_cmd[current_cmd_index].args_count;
   strcpy(g_cmd[current_cmd_index].args[current_args_index],s);	
   free(s);	
   g_cmd[current_cmd_index].args_count++;		
}

int simple_cmd()
{
	is_simple_cmd=1;
	current_cmd_index++;	
}

int pipe_cmd()
{
   return 0;
}

int input_cmd()
{
 
   if(is_simple_cmd==1)
   {
	   strcpy(g_cmd[current_cmd_index-1].input,g_cmd[current_cmd_index].args[0]);
	   memset(&g_cmd[current_cmd_index],0,sizeof(g_cmd[current_cmd_index]));
//	   current_cmd_index--;
   }
   else if(is_simple_cmd==0)
   {
   	  int current_args_index=g_cmd[current_cmd_index].args_count;
  	  strcpy(g_cmd[current_cmd_index].input,g_cmd[current_cmd_index].args[current_args_index-1]);	
	  memset(g_cmd[current_cmd_index].args[current_args_index-1],0,sizeof(g_cmd[current_cmd_index].args[current_args_index-1]));	
	  g_cmd[current_cmd_index].args_count--;
   }
   is_simple_cmd=0;
   return 0;
}

int output_cmd()
{

   if(is_simple_cmd==1)
   {
	   strcpy(g_cmd[current_cmd_index-1].output,g_cmd[current_cmd_index].args[0]);
	   memset(&g_cmd[current_cmd_index],0,sizeof(g_cmd[current_cmd_index]));
//	   current_cmd_index--;
   }
   else if(is_simple_cmd==0)
   {

   	  int current_args_index=g_cmd[current_cmd_index].args_count;
  	  strcpy(g_cmd[current_cmd_index].output,g_cmd[current_cmd_index].args[current_args_index-1]);	
	  memset(g_cmd[current_cmd_index].args[current_args_index-1],0,sizeof(g_cmd[current_cmd_index].args[current_args_index-1]));
	  g_cmd[current_cmd_index].args_count--;			
   }
  is_simple_cmd=0;
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
//	printf("execute_cmd before\n");
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

void run_cmd(int i,int cmd_amount)
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
			run_cmd(i-1,cmd_amount);
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

	execute_cmd(i);
	if(input_fp) fclose(input_fp);
	if(output_fp) fclose(output_fp);
}


int execute(void)		
{
  int cmd_amount=current_cmd_index;
  printf("execute,cmd_amount=%d\n",cmd_amount);
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
		else if(g_cmd[0].args[1][0]!='%' && g_cmd[0].args_count==3)
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
		else if(g_cmd[0].args[1][0]!='%' && g_cmd[0].args_count==3)
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
	else if(strcmp(g_cmd[0].args[0],"quit")==0)
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

