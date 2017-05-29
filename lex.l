%{
#include "bison.tab.h"
%}

%%
"<" {return INPUT;}
">" {return OUTPUT;}
"|" {return PIPE;}
"&" {return BACKGROUND;}
[ \t]  { /* ignore */   }
[^<>|&\t\r ]+ {yylval.str=strdup(yytext); return STRING;}
.    { }
%%
