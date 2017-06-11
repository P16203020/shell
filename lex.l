%{
#include "bison.tab.h"
#include "global.h"
%}

%%
"<" {return INPUT;}
">" {return OUTPUT;}
"|" {return PIPE;}
"&" {return BACKGROUND;}
[^<>|&\t\r ]+ {yylval.string=strdup(yytext); if(YYDEBUG) printf("yylval.str=%s\n",yylval.string); add_args(yylval.string); return STRING;}
.    { }
%%
