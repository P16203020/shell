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
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "bison.tab.h"

extern void yy_scan_string(char *s); 
extern void execue_init();
int done=0;

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


int main (int argc, char **argv)
{
  char *line, *s;
  initialize_readline ();	/* Bind our completer. */
  /* Loop reading and executing execue_initlines until the user quits. */
  for ( ; done==0; )
    {
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
      }
      free (line);
    }
  exit (0);
}
