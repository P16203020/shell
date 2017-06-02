%{
#include "bison.tab.h"
#include "global.h"
%}

%%
"<" {return INPUT;}
">" {return OUTPUT;}
"|" {return PIPE;}
"&" {return BACKGROUND;}
[^<>|&\t\r ]+ {yylval.str=strdup(yytext); printf("yylval.str=%s\n",yylval.str); add_args(yylval.str); return STRING;}
.    { }
%%
