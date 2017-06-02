%{
	#include <stdio.h>
	#include "global.h"
	void yyerror (char const *);
	int yylex( void );
	#define YYDEBUG 1
	#define YYERROR_VERBOSE 1
%}

%union {
    char *str;   
}

%token INPUT	
%token OUTPUT
%token PIPE
%right BACKGROUND
%token PAR
%token <string> STRING

%%

line :
     |cmd { execute(); if(YYDEBUG) { printf ("execute cmd done\n");} }
;

cmd: exec_cmd
     |exec_cmd BACKGROUND { background(); if(YYDEBUG) { printf ("BACKGROUND\n");} }
;

exec_cmd:simple_cmd input_re pipe_re output_re
;

pipe_re:/* empty */  
	| pipe_re PIPE simple_cmd { pipe_cmd(); if(YYDEBUG) { printf ("pipe_re\n");} }
;

input_re: /* empty */  
	| input_re INPUT STRING { input_cmd(); if(YYDEBUG) { printf ("input_re\n");} }
;

output_re: /* empty */ 
	| output_re OUTPUT STRING  { output_cmd(); if(YYDEBUG) { printf ("output_re\n");} }
;

simple_cmd: STRING args { simple_cmd(); if(YYDEBUG) { printf ("simple_cmd\n");} }
;	

args : /* empty */
	| args STRING
;

%%

void yyerror (char const *s )
{ printf( "%s" , s ); }

