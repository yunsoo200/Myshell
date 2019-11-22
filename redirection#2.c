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

int is_redirection(char **cmd) {
	for(int i = 0; cmd[i] != NULL; i++) {
		char *tmp;
		if((tmp = strchr(cmd[i], '<')) || (tmp = strchr(cmd[i], '>')))
			return 1;
		else continue;
	}
	return 0;
}

int redirect_in_fd[3] = {STDIN_FILENO, 0, 0};
int redirect_out_fd[3] = {STDOUT_FILENO, 0, 0};
void redirection(char **cmd) {
	int i;
	int type;
	for(int j = 0; cmd[j] != NULL; j++) {
		char *tmp;
		if((tmp = strchr(cmd[j], '<'))) {
			i = j;
			type = 0;
		}
		else if((tmp = strchr(cmd[j], '>'))) {
			i = j;
			type = 1;
		}
	}

	if(type == 0) {
		redirect_in_fd[1] = dup(redirect_in_fd[0]);
		if((redirect_in_fd[2] = open(cmd[i+1], O_RDONLY, 0644)) == -1) {
                        fprintf(stderr, "file open error\n");
                        return;
                }
                dup2(redirect_in_fd[2], STDIN_FILENO);
		close(redirect_in_fd[2]);
	}
	else if(type == 1) {
		redirect_out_fd[1] = dup(redirect_out_fd[0]);
		if((redirect_out_fd[2] = open(cmd[i+1], O_WRONLY|O_CREAT, 0644)) == -1) {
			fprintf(stderr, "file open error\n");
			return;
		}
		dup2(redirect_out_fd[2], STDOUT_FILENO);
		close(redirect_out_fd[2]);
	}

	cmd[i] = NULL;
        cmd[i+1] = NULL;
        for(; cmd[i + 2] != NULL; i++)
                cmd[i] = cmd[i+2];
        cmd[i] = NULL;
}

/*int join(char *com1[], char *com2[]) {
	int p[2], status;
	int in_fd, out_fd;
	in_fd = dup(STDIN_FILENO);
	out_fd = dup(STDOUT_FILENO);

	int redirect_in_com1_i = -1;
	int redirect_out_com1_i = -1;
	for(int i = 0; com1[i] != NULL; i++) {
		char *tmp;
		if((tmp = strchr(com1[i], '<')))
			redirect_in_com1_i = i;
		else if((tmp = strchr(com1[i], '>')))
			redirect_out_com1_i = i;
	}

	int redirect_in_com2_i = -1;
        int redirect_out_com2_i = -1;
        for(int i = 0; com2[i] != NULL; i++) {
                char *tmp;
                if((tmp = strchr(com2[i], '<')))
                        redirect_in_com2_i = i;
                else if((tmp = strchr(com2[i], '>')))
                        redirect_out_com2_i = i;
        }

	switch(fork()) {
		case -1: fprintf(stderr, "1st fork call in join\n");
			 return -1;
		case 0: break;
		default: wait(&status);
			 dup2(in_fd, STDIN_FILENO);
			 dup2(out_fd, STDOUT_FILENO);
			 return status;
	}

	if(pipe(p) == -1) {
		fprintf(stderr, "pipe call in join\n");
		return -2;
	}

	switch(fork()) {
		case -1: fprintf(stderr, "2nd fork call in join\n");
			 return -3;
		case 0: {
			if(redirect_in_com1_i != -1)
				redirection(0, com1, redirect_in_com1_i);
			if(redirect_out_com1_i != -1)
                      		redirection(0, com1, redirect_out_com1_i);
			dup2(p[1], STDOUT_FILENO);
			close(p[0]);
			close(p[1]);
			execvp(com1[0], com1);
			fprintf(stderr, "1st execvp call in join\n");
			return -4;
			}
		default: {
			 if(redirect_in_com2_i != -1)
                                redirection(0, com2, redirect_in_com2_i);
			 if(redirect_out_com2_i != -1)
                                redirection(0, com2, redirect_out_com2_i);
			 dup2(p[0], STDIN_FILENO);
			 close(p[0]);
			 close(p[1]);
			 execvp(com2[0], com2);
			 fprintf(stderr, "2nd execvp call in join\n");
			 return -5;
			 }
	}
}*/

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
	int pipe_flag = 0;
	i = 0;
	while(cmdline[i] != '\0') {
		if(cmdline[i] == '<' || cmdline[i] == '>')
			redirection_flag++;
		else if(cmdline[i] == '|')
			pipe_flag++;
		i++;
	}

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
	else if(pipe_flag != 0) {
		int pipe_done = 0;
		char *com1[MAX_CMD_ARG] = {NULL, }; char *com2[MAX_CMD_ARG] = {NULL, };
                int stop = 0;
    		int com_index = 0;

		for(int i = 0; cmdvector[i] != NULL; i++) {
			char *tmp;
			if((tmp = strchr(cmdvector[i], '|'))) {
				if(stop == 0) {
					stop++;
					com_index = 0;
				}
			}
			else {
				if(stop == 0)
					com1[com_index++] = cmdvector[i];
				else if(stop == 1)
					com2[com_index++] = cmdvector[i];
			}
		}
		//join(com1, com2);
	}
	else {
		if(redirection != 0) {
			while(is_redirection(cmdvector) == 1) 
				redirection(cmdvector);
        	}

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