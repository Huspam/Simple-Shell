#include "icssh.h"
#include "linkedlist.h"
#include "helpers.h"
#include <readline/readline.h>

volatile sig_atomic_t flag = 0;

void sigchld_handler(){
	flag = 1;
}

void sigusr2_handler(int sig){
	time_t epoch;
	struct tm *t;
	time(&epoch);
	t = localtime(&epoch);
	write(2, asctime(t), strlen(asctime(t)));
}

int main(int argc, char* argv[]) {	
    int max_bgprocs = -1;
	int exec_result;
	int exit_status; //exit status of last child process
	pid_t pid;
	pid_t wait_result;
	char* line;
	int total_procs = 0;
	int fdi;
	int fdo;
	int fde;
	list_t *background_ps = CreateList(&c_bgentry, &print_bgentry, &d_bgentry);
#ifdef GS
    rl_outstream = fopen("/dev/null", "w");
#endif

    // check command line arg
    if(argc > 1) {
        int check = atoi(argv[1]);
        if(check != 0)
            max_bgprocs = check;
        else {
            printf("Invalid command line argument value\n");
            exit(EXIT_FAILURE);
        }
    }

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, sigchld_handler);
	signal(SIGUSR2, sigusr2_handler);
	//int i = 0;
    	// print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {
		
        	// MAGIC HAPPENS! Command string is parsed into a job struct
        	// Will print out error message if command string is invalid
		    job_info* job = validate_input(line);

		// 	if (i== 1){
		// 	bgentry_t *t = (bgentry_t *)(background_ps->head->data);
		// 	debug_print_job(t->job);
		// 	print_bgentry(t);
		// }
        	if (job == NULL) { // Command was empty string or invalid
			free(line);
			continue;
		}

        	//Prints out the job linked list struture for debugging
        	#ifdef DEBUG   // If DEBUG flag removed in makefile, this will not longer print
            		debug_print_job(job);
        	#endif
		

		//i++;
		// check bg
		if (flag){
			node_t *cur = background_ps->head;
			int num = 0;
			while(cur != NULL){
				bgentry_t *proc = cur->data;
				wait_result = waitpid(proc->pid, &exit_status, WNOHANG);

				if (wait_result > 0){
					printf(BG_TERM, proc->pid, proc->job->line);
					cur = cur->next;
					proc = RemoveByIndex(background_ps, num);
					d_bgentry(proc);
					continue;
				}
				cur = cur->next;
				num++;
			}
			flag = 0;
		}

		//BUILTINS
		// exit
		proc_info* cur = job->procs;
		if (strcmp(cur->cmd, "exit") == 0) {
			// Terminating the shell
			node_t *iter = background_ps->head;
			while(iter != NULL){
				bgentry_t *proc = iter->data;
				kill(proc->pid, SIGKILL);
				wait_result = waitpid(proc->pid, &exit_status, WNOHANG);

				printf(BG_TERM, proc->pid, proc->job->line);

				iter = iter->next;
			}
			free(line);
			free_job(job);
			DeleteList(&background_ps);
            validate_input(NULL);   // calling validate_input with NULL will free the memory it has allocated
            return 0;
		}
		// changing directories
		else if (!strcmp(cur->cmd, "cd")) {

			if (cur->argc > 1){
				if (chdir(cur->argv[1])){
					fprintf(stderr, DIR_ERR);
					exit(EXIT_FAILURE);
				} else {
					char s[64];
					printf("%s\n", getcwd(s, 64));
				}
			} else {
				chdir(getenv("HOME"));
				char s[64];
				printf("%s\n", getcwd(s, 64));
			}
			free(line);
			free_job(job);
			continue;
		}
		// ascii53
		else if (!strcmp(cur->cmd, "ascii53")) {

			printf("_____8888888888_____________________\n");
			printf("____888888888888888_________________\n");
			printf("__888888822222228888________________\n");
			printf("_88888822222222288888_______________\n");
			printf("888888222222222228888822228888______\n");
			printf("888882222222222222288222222222888___\n");
			printf("8888822222222222222222222222222288__\n");
			printf("_8888822222222222222222222222222_88_\n");
			printf("__88888222222222222222222222222__888\n");
			printf("___888822222222222222222222222___888\n");
			printf("____8888222222222222222222222____888\n");
			printf("_____8888222222222222222222_____888_\n");
			printf("______8882222222222222222_____8888__\n");
			printf("_______888822222222222______888888__\n");
			printf("________8888882222______88888888____\n");
			printf("_________888888_____888888888_______\n");
			printf("__________88888888888888____________\n");
			printf("___________8888888888_______________\n");
			printf("____________8888888_________________\n");
			printf("_____________88888__________________\n");
			printf("______________888___________________\n");
			printf("_______________8____________________\n");
			free(line);
			free_job(job);
			continue;
		}
		// most recent child exit status
		else if (!strcmp(cur->cmd, "estatus")) {
			if (WIFEXITED(exit_status)){
				int status = WEXITSTATUS(exit_status);
				printf("%d\n", status);
			}
			free(line);
			free_job(job);
			continue;
		}
		// lists background processes
		else if (!strcmp(cur->cmd, "bglist")) {
			if (background_ps->length > 0){
				PrintLinkedList(background_ps);
			}
			free(line);
			free_job(job);
			continue;
		}

		// brings background process to foreground
		else if (!strcmp(cur->cmd, "fg")) {
			bgentry_t *b;
			if (background_ps->length == 0){
				fprintf(stderr, PID_ERR);
				free(line);
				free_job(job);
				continue;
			} else if (cur->argc > 1){
				bool flag = 1;
				int count = 0;
				node_t *iter = background_ps->head;
				
				while (iter != NULL){
					b = iter->data;
					if (b->pid == atoi(cur->argv[1])){
						flag = 0;
						break;
					}
					iter = iter->next;
					count++;
				}
				if (flag){
					fprintf(stderr, PID_ERR);
					free(line);
					free_job(job);
					continue;
				}
				
				b = RemoveByIndex(background_ps, count);
			} else {
				b = RemoveFromTail(background_ps);
			}

			wait_result = waitpid(b->pid, &exit_status, 0);
			if (wait_result < 0) {
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
			printf("%s\n",  b->job->line);
			d_bgentry(b);

			free(line);
			free_job(job);
			continue;
		}

		// invalid file combo checking
		else if (check_files(job->in_file, job->out_file, cur->err_file)){
			// printf("hello");
			fprintf(stderr, RD_ERR);
			free_job(job);  
			free(line);
				validate_input(NULL);
			exit(EXIT_FAILURE);
		}

		//running pipe procs
		if (job->nproc > 1){
			
			int pipes[2*(job->nproc-1)];
			int index = 0;
			for (int i = 0; i < (job->nproc-1); i++){
				if(pipe(pipes+ 2*i) < 0){
					free_job(job);  
					free(line);
						validate_input(NULL);
					exit(EXIT_FAILURE);
				}
			}

			//run child procs
			while (cur != NULL){
				if ((pid = fork()) < 0) {
					perror("fork error");
					free_job(job);  
					free(line);
						validate_input(NULL);
					exit(EXIT_FAILURE);
				}

				if (pid == 0){
					if (cur->next_proc != NULL){

						if (dup2(pipes[index+1], 1) < 0){
							free_job(job);  
							free(line);
								validate_input(NULL);
							exit(EXIT_FAILURE);
						}
					}

					if (index != 0){
						if(dup2(pipes[index-2], 0) < 0){
							fprintf(stderr, RD_ERR);
							free_job(job);  
							free(line);
								validate_input(NULL);
							exit(EXIT_FAILURE);
						}
					}

					for (int i = 0; i < 2*(job->nproc-1); i++){
						close(pipes[i]);
					}

					exec_result = execvp(cur->cmd, cur->argv);
					if (exec_result < 0) {
						printf(EXEC_ERR, cur->cmd);
						free_job(job);  
						free(line);
							validate_input(NULL);  
						exit(EXIT_FAILURE);
					}
				}
				index += 2;
				cur = cur->next_proc;
			}

			//close all pipes
			for (int i = 0; i < 2*(job->nproc-1); i++){
				close(pipes[i]);
			}
			
			//bg pipe
			if (job->bg){
				if (max_bgprocs < 0 || total_procs + job->nproc <= max_bgprocs){	
					total_procs += job->nproc;
					bgentry_t *add = bgentry_make(pid, job, time(NULL));
					InsertInOrder(background_ps, add);
				} else {fprintf(stderr, BG_ERR);}
			} else { //fg pipe
				for (int i = 0; i < job->nproc; i++){
					wait_result = wait(&exit_status);
					if (wait_result < 0) {
						printf(WAIT_ERR);
						exit(EXIT_FAILURE);
					}
				}
			}

		} else {
			// no pipe
			if ((pid = fork()) < 0) {
				perror("fork error");
				free_job(job);  
				free(line);
					validate_input(NULL);
				exit(EXIT_FAILURE);
			}
			if (pid == 0) { 
				//redirect infile
				if (job->in_file != NULL){
					if ((fdi = open(job->in_file, O_RDONLY, 0)) < 0){
						fprintf(stderr, RD_ERR);
						free_job(job);  
						free(line);
							validate_input(NULL);
						exit(EXIT_FAILURE);
					}
					if (dup2(fdi, STDIN_FILENO) < 0){
						fprintf(stderr, RD_ERR);
						free_job(job);  
						free(line);
							validate_input(NULL);
						exit(EXIT_FAILURE);
					}
				}
				//redirect outfile
				if (job->out_file != NULL){
					if ((fdo = open(job->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0)) < 0){
						fprintf(stderr, RD_ERR);
						free_job(job);  
						free(line);
							validate_input(NULL);
						exit(EXIT_FAILURE);
					}
					if (dup2(fdo, STDOUT_FILENO) < 0){
						fprintf(stderr, RD_ERR);
						free_job(job);  
						free(line);
							validate_input(NULL);
						exit(EXIT_FAILURE);
					}
				}
				//redirect errfile
				if (cur->err_file != NULL){
					if ((fde = open(cur->err_file, O_WRONLY | O_CREAT | O_TRUNC, 0)) < 0){
						fprintf(stderr, RD_ERR);
						free_job(job);  
						free(line);
							validate_input(NULL);
						exit(EXIT_FAILURE);
					}
					if (dup2(fde, STDERR_FILENO) < 0){
						fprintf(stderr, RD_ERR);
						free_job(job);  
						free(line);
							validate_input(NULL);
						exit(EXIT_FAILURE);
					}
				}
				proc_info* proc = job->procs;
				
				exec_result = execvp(proc->cmd, proc->argv);
				if (exec_result < 0) {  //Error checking
					printf(EXEC_ERR, proc->cmd);
					
					// Cleaning up to make Valgrind happy 
					// (not necessary because child will exit. Resources will be reaped by parent)
					free_job(job);  
					free(line);
						validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated
					exit(EXIT_FAILURE);
				}
			} else {
				//background
				if (job->bg){
					if (max_bgprocs < 0 || total_procs + job->nproc <= max_bgprocs){	
						total_procs += job->nproc;
						bgentry_t *add = bgentry_make(pid, job, time(NULL));
						InsertInOrder(background_ps, add);
					} else {fprintf(stderr, BG_ERR);}
				}
				//foreground
				else {
					wait_result = waitpid(pid, &exit_status, 0);
					if (wait_result < 0) {
						printf(WAIT_ERR);
						exit(EXIT_FAILURE);
					}
				}
			}		
		}
		

		free_job(job);  // if a foreground job, we no longer need the data
		free(line);

	}
    	// calling validate_input with NULL will free the memory it has allocated
    	validate_input(NULL);

#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}