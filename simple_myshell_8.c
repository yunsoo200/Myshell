#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_CMD_ARG 10

const char *prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];

void fatal(char *str){
	perror(str);
	exit(1);
}

int makelist(char *s, const char *delimiters, char** list, int MAX_LIST){	
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;

  if( (s==NULL) || (delimiters==NULL) ) return -1;

  snew = s + strspn(s, delimiters);	/* delimiters¸¦ skip */
  if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
    return numtokens;
	
  numtokens = 1;
  
  while(1){
     if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
	break;
     if(numtokens == (MAX_LIST-1)) return -1;
     numtokens++;
  }
  return numtokens;
}

int main(int argc, char**argv){
  int i=0;
  pid_t pid;
  while (1) {
      
  	fputs(prompt, stdout);
	fgets(cmdline, BUFSIZ, stdin);
	cmdline[ strlen(cmdline) - 1] = '\0';

	int argnum = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);
	
	int backflag = 0;
	if(!strcmp(cmdvector[argnum - 1], "&")) {
		cmdvector[argnum - 1] = NULL;
		backflag = 1;
	}

	if(!strcmp(cmdvector[0], "exit"))
		break;
	else if(!strcmp(cmdvector[0], "cd")) {
		if(argnum != 2) {
			fprintf(stderr, "Usage : cd directory_name\n");
			continue;
		}
		
		if(chdir(cmdvector[1]) == -1) {
			fprintf(stderr, "working directory change error\n");
		}
	}
	else {
		switch(pid=fork()){
		case 0: 
			if(backflag == 1) {
				pid_t tmp_pid;
				tmp_pid = fork();
				if(tmp_pid == 0) {
					execvp(cmdvector[0], cmdvector);
				}
				else if(tmp_pid == -1)
					fatal("child");
				else
					return 0;
			}
			else {
				execvp(cmdvector[0], cmdvector);
			}
			fatal("main()");
		case -1:
  			fatal("main()");
		default:
			wait(NULL);
		}
	}
  }

  return 0;
}
