#ifndef HELPERS_H
#define HELPERS_H

#include "icssh.h"

void d_bgentry(void *P);

int c_bgentry(void *P1, void *P2);

bgentry_t *bgentry_make(pid_t pid, job_info *j, time_t t);

bool check_files(char *in, char *out, char *err);

#endif