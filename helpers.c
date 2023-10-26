#include "helpers.h"
#include "icssh.h"

void d_bgentry(void *P){
	bgentry_t *p = P;
    if (p->job != NULL) {
		if (p->job->in_file != NULL) {free(p->job->in_file);}
		if (p->job->out_file != NULL) {free(p->job->out_file);}
		proc_info *pro = p->job->procs;
		
		while (pro != NULL){
			free(pro->cmd);
			for(int i = 0; i < pro->argc; i++){
				if (pro->err_file != NULL) {free(pro->err_file);}
				free(pro->argv[i]);
			}
			pro = pro->next_proc;
			
		}
		free_job(p->job);
	}
    free(p);
}

int c_bgentry(void *P1, void *P2){
	bgentry_t *p1 = P1;
	bgentry_t *p2 = P2;
	if (p1->seconds < p2->seconds){return -1;}
	else if (p1->seconds == p2->seconds){return 0;}
	return 1;
}

bgentry_t *bgentry_make(pid_t pid, job_info *j, time_t t){
	bgentry_t *add = malloc(sizeof(bgentry_t));
	int len;
	
	//deep copy job
	add->job = malloc(sizeof(job_info));
	add->job->bg = j->bg;

	len = strlen(j->line)+1;
	add->job->line = malloc(len);
	strcpy(add->job->line, j->line);

	add->job->nproc = j->nproc;

	if (j->in_file != NULL){
		len = strlen(j->in_file)+1;
		add->job->in_file = malloc(len);
		strcpy(add->job->in_file, j->in_file);
	} else {add->job->in_file = j->in_file;}
	
	if (j->out_file != NULL){
		len = strlen(j->out_file)+1;
		add->job->out_file = malloc(len);
		strcpy(add->job->out_file, j->out_file);
	} else {add->job->out_file = j->out_file;}
	
	
	//deep copy procs inside of job
	proc_info *tail = malloc(sizeof(proc_info));
	add->job->procs = tail;
	proc_info *cur = j->procs;
	while (cur->next_proc != NULL){
		if (cur->err_file != NULL){
			len = strlen(cur->err_file)+1;
			tail->err_file  = malloc(len);
			strcpy(tail->err_file, cur->err_file);
		} else {tail->err_file = cur->err_file;}
		
		tail->argc = cur->argc;
		
		//deep copy argv
		tail->argv = malloc(cur->argc * sizeof(char *));
		for (int i = 0; i < cur->argc; i++){
			len = strlen(cur->argv[i])+1;
			tail->argv[i] = malloc(len*sizeof(char));
			strcpy(tail->argv[i], cur->argv[i]);
		}

		
		len = strlen(cur->cmd)+1;
		tail->cmd = malloc(len*sizeof(char));
		strcpy(tail->cmd, cur->cmd);
		tail->next_proc = malloc(sizeof(proc_info));
		tail = tail->next_proc;
		cur = cur->next_proc;
	}

	//////////////tail for proc///////////////
	if (cur->err_file != NULL){
		len = strlen(cur->err_file)+1;
		tail->err_file  = malloc(len);
		strcpy(tail->err_file, cur->err_file);
	} else {tail->err_file = cur->err_file;}
	
	tail->argc = cur->argc;

	tail->argv = malloc(cur->argc * sizeof(char *));
	for (int i = 0; i < cur->argc; i++){
		len = strlen(cur->argv[i])+1;
		tail->argv[i] = malloc(len*sizeof(char));
		strcpy(tail->argv[i], cur->argv[i]);
	}

	len = strlen(cur->cmd)+1;
	tail->cmd = malloc(len*sizeof(char));
	strcpy(tail->cmd, cur->cmd);
	tail->next_proc = NULL;
	/////////////////////////////////////////

	add->pid = pid;
	add->seconds = t;

	return add;
}

bool check_files(char *in, char *out, char *err){
	if (in != NULL && out != NULL && !strcmp(in, out)){return 1;}
	if (in != NULL && err != NULL && !strcmp(in, err)){return 1;}
	if (out != NULL && err != NULL && !strcmp(out, err)){return 1;}
	return 0;
}