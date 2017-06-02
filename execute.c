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

typedef struct {
  char args[10][128];
  char input[128];
  char output[128];
  int args_count;
  int is_bg;
} COMMAND;


COMMAND g_cmd[MAX_CMD];
int current_cmd_index=0;
int is_simple_cmd=0;
//int pipe_fd[MAX_CMD][2];
int pipe_fd[MAX_CMD][2];
void background(void)
{
  	
}


int execue_init()
{
  memset(g_cmd,0,sizeof(g_cmd));
  current_cmd_index=0;
  is_simple_cmd=-1;

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
		 	printf("OUTPUT:stdout");
		 else
		 	printf("OUTPUT:pipe");			
	}
	else
	{
		printf("OUTPUT:%s",g_cmd[i].output);
	}
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
	waitpid(pid, NULL, 0);
//	printf("execute pid=%d down\n",pid);
   } 

}

