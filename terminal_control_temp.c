#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
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

static void handler(int signo) {
	switch(signo) {
		case SIGINT:
		case SIGQUIT:
			printf("\n");
			break;
		case SIGCHLD:
			while((waitpid(-1, NULL, WNOHANG)) > 0);
	}
}

int main(int argc, char**argv){
  int i=0;
  pid_t pid;

  static struct sigaction act;
  act.sa_handler = handler;
  sigemptyset(&act.sa_mask);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);

  /*static struct sigaction chld_act;
  chld_act.sa_handler = handler;
  sigemptyset(&chld_act.sa_mask);
  chld_act.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &chld_act, NULL);*/

  while (1) {
  	fputs(prompt, stdout);
	fgets(cmdline, BUFSIZ, stdin);
	cmdline[ strlen(cmdline) - 1] = '\0';
	
	if(cmdline[0] == '\0')
		continue;

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
			setpgid(0, 0);
			if(backflag == 0) {
				tcsetpgrp(STDIN_FILENO, getpgid(0));
			}
			signal(SIGINT, SIG_DFL);
                        signal(SIGQUIT, SIG_DFL);
			execvp(cmdvector[0], cmdvector);
			fatal("main()");
		case -1:
  			fatal("main()");
		default:
			if(backflag == 0) {
				waitpid(pid, NULL, 0);
				tcsetpgrp(STDIN_FILENO, getpgid(0));
			}
		}
	}
	fflush(NULL);
  }

  return 0;
}