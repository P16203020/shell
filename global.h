#ifndef __GLOBAL__
#define __GLOBAL__
#define YYDEBUG 0
#define YYERROR_VERBOSE 0
extern void execue_init();
extern void add_args(char *s);
extern int simple_cmd();
extern int pipe_cmd();	
extern int input_cmd();	
extern int output_cmd();	
extern int execute();
extern void background();
extern void CTRL_Z_DEAL();
extern void CTRL_C_DEAL();
extern char *env_argv[];
extern char env_data[5][256];
extern char dir_path[1024];
extern int env_count;
#endif
