#ifndef __GLOBAL__
#define __GLOBAL__
extern void add_args(char *s);
extern int simple_cmd();
extern int pipe_cmd();	
extern int input_cmd();	
extern int output_cmd();	
extern int execute();
extern void background();
#endif
