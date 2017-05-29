%{
	#include <stdio.h>
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
//%token STRING
%right BACKGROUND
%token EOL

%token <string> STRING

%%

line :
     |cmd { if(YYDEBUG) { printf ("execute cmd done\n");} }
;

cmd: exec_cmd
     |exec_cmd BACKGROUND { if(YYDEBUG) { printf ("BACKGROUND\n");} }
;

exec_cmd:simple_cmd pile_re input_re output_re
;

simple_cmd: STRING args { if(YYDEBUG) {printf ("simple_cmd:%s\n",yylval.str);} }
;

args : /* empty */
	| args STRING
;

pile_re:/* empty */
	| pile_re PIPE STRING { if(YYDEBUG) {printf ("pile_re:%s\n",yylval.str);} }
;

input_re: /* empty */
	| input_re INPUT STRING { if(YYDEBUG) {printf ("input_re:%s\n",yylval.str);} }
;

output_re: /* empty */
	| output_re OUTPUT STRING { if(YYDEBUG) {printf ("output_re:%s\n",yylval.str);} }
;
%%

void yyerror (char const *s )
{ printf( "%s" , s ); }

