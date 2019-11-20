#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
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

int child_exist = 0;
static void sh_int(int signo) {
	printf("\n");
	if(child_exist == 0)
	        write(fileno(stdin), prompt, strlen(prompt));
}

/*static void sh_chld(int signo) {
	while(waitpid(-1, NULL, WNOHANG) <= 0);
}*/

int redirect_in_fd[3] = {STDIN_FILENO, 0, 0};
int redirect_out_fd[3] = {STDOUT_FILENO, 0, 0};
void redirection(int type, int i) {
	if(type == 0) {
		redirect_in_fd[1] = dup(redirect_in_fd[0]);
		if((redirect_in_fd[2] = open(cmdvector[i+1], O_RDONLY, 0644)) == -1) {
                        fprintf(stderr, "file open error\n");
                        return;
                }
                dup2(redirect_in_fd[2], STDIN_FILENO);
		close(redirect_in_fd[2]);
	}
	else if(type == 1) {
		redirect_out_fd[1] = dup(redirect_out_fd[0]);
		if((redirect_out_fd[2] = open(cmdvector[i+1], O_WRONLY|O_CREAT, 0644)) == -1) {
			fprintf(stderr, "file open error\n");
			return;
		}
		dup2(redirect_out_fd[2], STDOUT_FILENO);
		close(redirect_out_fd[2]);
	}

	cmdvector[i] = NULL;
        cmdvector[i+1] = NULL;
        for(; cmdvector[i + 2] != NULL; i++)
                cmdvector[i] = cmdvector[i+2];
        cmdvector[i] = NULL;
}


int main(int argc, char**argv){
  int i=0;
  pid_t pid;

  static struct sigaction sig_int;
  sig_int.sa_handler = sh_int;
  sig_int.sa_flags = SA_RESTART;
  sigemptyset(&sig_int.sa_mask);

  sigaction(SIGINT, &sig_int, NULL);
  sigaction(SIGQUIT, &sig_int, NULL);

  /*static struct sigaction sig_chld;
  sig_chld.sa_handler = sh_chld;
  sig_chld.sa_flags = SA_RESTART;
  sigemptyset(&sig_chld.sa_mask);

  sigaction(SIGCHLD, &sig_chld, NULL);*/

  while (1) {
	child_exist = 0;
  	fputs(prompt, stdout);
	fgets(cmdline, BUFSIZ, stdin);
	cmdline[ strlen(cmdline) - 1] = '\0';
	
	if(cmdline[0] == '\0')
		continue;

	int redirection_flag = 0;
	i = 0;
	while(cmdline[i] != '\0') {
		if(cmdline[i] == '<' || cmdline[i] == '>')
			redirection_flag++;
		i++;
	}

	int argnum = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);
	
	int backflag = 0;
	if(!strcmp(cmdvector[argnum - 1], "&")) {
		cmdvector[argnum - 1] = NULL;
		backflag = 1;
	}

	if(redirection != 0) {
		for(i = 0; cmdvector[i] != NULL; i++) {
			char *tmp;
			if((tmp = strchr(cmdvector[i], '<'))) {
				redirection(0, i);
				i--;
			}
			else if((tmp = strchr(cmdvector[i], '>'))) {
				redirection(1, i);
				i--;
			}
		}
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
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);

			if(backflag == 1) {
				pid_t tmp_pid;
				tmp_pid = fork();
				if(tmp_pid == 0) {
					execvp(cmdvector[0], cmdvector);
				}
				else if(tmp_pid == -1)
					fatal("background");
			}
			else {
				execvp(cmdvector[0], cmdvector);
			}
			fatal("main()");
		case -1:
  			fatal("main()");
		default:
			child_exist = 1;
			wait(NULL);
		}
	}
	fflush(NULL);
	if(redirection_flag != 0) {
		if(redirect_in_fd[1] != 0) {
			dup2(redirect_in_fd[1], STDIN_FILENO);
			close(redirect_in_fd[1]);
			redirect_in_fd[1] = 0;
		}
		if(redirect_out_fd[1] != 0) {
			dup2(redirect_out_fd[1], STDOUT_FILENO);
			close(redirect_out_fd[1]);
			redirect_out_fd[1] = 0;
		}
	}
  }

  return 0;
}