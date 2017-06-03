#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <signal.h>
#include <stddef.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "bison.tab.h"
#include "global.h"
extern void yy_scan_string(char *s); 
int done=0;
char env_data[5][256];
char dir_path[1024];
int env_count=0;
/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char * stripwhite (char *string)
{
  register char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;
    
  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void initialize_readline (void)
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = "MyShell";

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = NULL;
}

void read_env(void)
{
  memset(env_data,0,sizeof(env_data));
  char tmp[1024];
  memset(tmp,0,1024);
  int i=0;
  FILE *fp=fopen("env.conf","r");
  if(fp)
  {
	fread(tmp,1,sizeof(tmp),fp);
	fclose(fp);
	char *token=strtok(tmp,":");
	while(token!=NULL){
//		printf("%s\n",token);
		if(i<5)
			strcpy(env_data[i],token);
		i++;
		env_count++;
		token=strtok(NULL,":");
	}
		
  }
}
int main (int argc, char **argv)
{
  char *line, *s;
  initialize_readline ();	/* Bind our completer. */
  read_history(NULL);
  read_env();
  signal(SIGTSTP, CTRL_Z_DEAL);
  signal(SIGINT, CTRL_C_DEAL);
  /* Loop reading and executing execue_initlines until the user quits. */
  for ( ; done==0; )
    {
 	char dir[1024], *s;
	s = getcwd (dir, sizeof(dir) - 1);
	if(s)
	{
		memset(dir_path,0,1024);
		sprintf(dir_path,"MyShell@%s:",dir);
		line = readline (dir_path);
	}
	else
      		line = readline ("MyShell: ");

      if (!line)
        break;

      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      s = stripwhite (line);
	
      if (*s)
      {  
			execue_init();
			yy_scan_string(s);
			yyparse();
			add_history(s);
			write_history(NULL);
      }
      free (line);
    }
  exit (0);
}
