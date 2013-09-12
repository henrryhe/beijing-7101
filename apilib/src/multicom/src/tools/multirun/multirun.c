/*
 * multirun.c
 *
 * Copyright (C) STMicroelectronics Ltd. 2001
 *
 * Spawn several stXXrun/stXXload processes under the control of one
 * master process.
 */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE pid_t;
#define sleep(t) Sleep(t*1000)
#else
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define MAX_PROCESSES 8
#define MAX_ARGUMENTS 32
typedef char *arguments_t[MAX_ARGUMENTS];

static int verbose = 0;
#define VERBOSE(x) if (verbose) { x; }

static int launchDelay = 0;

/* HACK: this is a *really* hacky means to try to reap sh4xrun processes */
static int doFreakyKill = 0;
static int ignoreSlave = 0;

enum error_codes_e {
	error_code_bad_argument = 1,
	error_code_cannot_fork,
	error_code_cannot_exec
};

void fprintcmd(FILE *stream, char *fmt, char *argv[])
{
	int i, len;
	char *buf, *pBuf;

	len = 0;
	for (i=0; argv[i] != 0; i++) {
		len += strlen(argv[i]) + 2;
	}

	if (len != 0) {
		pBuf = buf = malloc(len);

		pBuf += sprintf(pBuf, "%s", argv[0]);
		for (i=1; argv[i] != 0; i++) {
			pBuf += sprintf(pBuf, " %s", argv[i]);
		}

		fprintf(stream, fmt, buf);
		free(buf);
	}
}
void printcmd(char *fmt, char *argv[]) { fprintcmd(stdout, fmt, argv); }


pid_t background(char *file, char *argv[])
{
	pid_t pid;

	VERBOSE(printcmd("multirun: backgrounding - %s\n", argv));

#ifdef _WIN32
	{
		char *cmd, *pCmd;
		int i, cmdlen;
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };

		/* convert list of arguments into a command line */
		cmdlen = 1;
		for (i=0; argv[i] != 0; i++) {
			cmdlen += strlen(argv[i]) + 1;
		}
		pCmd = cmd = malloc(cmdlen); /* 1 */
		for (i=0; argv[i] != 0; i++) {
			pCmd += sprintf(pCmd, "%s ", argv[i]);
		}

		/* start the sub-process */
		if (CreateProcess(NULL, cmd, NULL, NULL,
				   TRUE, 0, NULL, NULL, &si, &pi)) {
			pid = pi.hProcess;
		} else {
			pid = (HANDLE) -1;
		}

		free(cmd); /* 1 */
	}
#else
	{
		int res;

		if (NULL != strstr(file, "sh4xrun")) {
			doFreakyKill = 1;
		}

		pid = fork();
		if (0 == pid) {
			res = execvp(file, argv);
			assert(res == -1); /* must have error to run this */
			fprintcmd(stderr, 
			          "Error - cannot run sub-process %s", argv);
			fprintf(stderr, " - %s\n", strerror(errno));
			exit(error_code_cannot_exec);
		}
	}
#endif

	VERBOSE(printf("multirun: pid of %s - %d\n", file, pid));

	return pid;
}

int main(int argc, char *argv[])
{
	arguments_t argvList[MAX_PROCESSES];
	pid_t pidList[MAX_PROCESSES], pid;
	int i, iArgs, iArgvList;
	int iMaster = -1;
	int res, status;

	/* argument processing */
	for (i=1, iArgs=0, iArgvList=-1; i<argc; i++) {
		if        (0 == strcmp("--master", argv[i])) {
			/* start the next arg list as a master */
			argvList[iArgvList++][iArgs] = 0;
			iMaster = iArgvList;
			iArgs = 0;
		} else if (0 == strcmp("--slave", argv[i])) {
			/* start the next arg list as a slave */
			argvList[iArgvList++][iArgs] = 0;
			iArgs = 0;
		} else if (iArgvList >= 0) {
			/* add the current argument to the current arg list */
			argvList[iArgvList][iArgs++] = argv[i];
		} else {
			/* this block contains arguments that must be presented
			 * before --master or --slave
			 */
			if        (0 == strcmp("--verbose", argv[i])) {
				verbose = 1;
			} else if (0 == strcmp("--ignore-slave", argv[i])) {
				ignoreSlave = 1;
			} else if (0 == strcmp("--launch-delay", argv[i])) {
				launchDelay = 20;
			} else {
				/* error (no --master or --slave) */
				fprintf(stderr, 
				        "Error - expect --master or --slave\n");
				exit(error_code_bad_argument);
			}
		}

		/* range checking */
		if (iArgs >= MAX_ARGUMENTS) {
			fprintf(stderr, 
			        "Error - too many arguments for \'%s\'\n",
				argvList[iArgvList][0]);
			exit(error_code_bad_argument);
		}
		if (iArgvList >= MAX_PROCESSES) {
			fprintf(stderr, "Error - too many processes\n");
			exit(error_code_bad_argument);
		}
	}

	/* null terminate the last set of arguments */
	argvList[iArgvList++][iArgs] = 0;

	if (0 == iArgvList) {
		fprintf(stderr, "Usage: multirun [--verbose] <[--master|--slave] args ...>\n");
		exit(error_code_bad_argument);
	}

	if (-1 == iMaster) {
		fprintf(stderr, "Error - no master process\n");
		exit(error_code_bad_argument);
	}

	VERBOSE(printf("multirun: master process is %s (%d)\n",
	               argvList[iMaster][0], iMaster));

	/* start the processes */
	for (i=0; i<iArgvList; i++) {
		if (i && launchDelay) {
			sleep(launchDelay);
		}

		pidList[i] = background(argvList[i][0], argvList[i]);
		if (-1 == (int) pidList[i]) {
			fprintf(stderr, 
			        "Error - could not fork sub-process - %s\n",
				strerror(errno));
			exit(error_code_cannot_fork);
		}

	}

	/* wait for the master or an unsuccessful slave */
	res = 0;
#ifdef _WIN32
	do {
		status = WaitForMultipleObjects(iArgvList, pidList, 
					     FALSE, INFINITE);
		status -= WAIT_OBJECT_0;

		if ((status < 0) || (status > iArgvList)) {
			fprintf(stderr, "Error - wait failed\n");
			break; /* cause the reap on all processes */
		} else {
			pid = pidList[status];

			/* remove the current pid from pidList */
			pidList[status] = pidList[--iArgvList];

			if (!GetExitCodeProcess(pid, &res)) {
				assert(0); /* this should never happen */
			}

			VERBOSE(printf("multirun: %d (%s) returned %s (%d)\n",
		               pid, (status == iMaster ? "master" : "slave"),
			       (0 == res ? "success" : "failure"), res));
		}

		if (ignoreSlave && (iMaster != status)) {
			res = 0;
		}
	} while ((iMaster != status) && (res == 0));
#else
	do {
		pid = wait(&status);
		res = (WIFEXITED(status) ? WEXITSTATUS(status) : -11);

		/* remove this pid from the list */
		for (i=0; i<iArgvList; i++) {
			if (pid == pidList[i]) {
				pidList[i] = 0;
				break;
			}
		}

		VERBOSE(printf("multirun: %d (%s) returned %s (%d)\n",
		               (int) pid, (i == iMaster ? "master" : "slave"),
			       (0 == res ? "success" : "failure"), res));

		if (ignoreSlave && (0 != pidList[iMaster])) {
			res = 0;
		}
	} while ((0 != pidList[iMaster]) && (res == 0));
#endif

	/* allow time for our children to terminate themselves */
	sleep(3);

	/* tidy up the pid list so that we do not attempt to kill any tasks
	 * have actually finished
	 */
#ifndef _WIN32
	while (0 < (pid = waitpid(0, &status, WNOHANG))) {
		/* remove this pid from the list */
		for (i=0; i<iArgvList; i++) {
			if (pid == pidList[i]) {
				pidList[i] = 0;
				break;
			}
		}

		res = (WIFEXITED(status) ? WEXITSTATUS(status) : -11);
		VERBOSE(printf("multirun: %d (%s) returned %s (%d)\n",
		               (int) pid, (i == iMaster ? "master" : "slave"),
			       (0 == res ? "success" : "failure"), res));

	}
#endif

	/* reap the unterminated processes (if there are any) */	
	for (i=0; i<iArgvList; i++) {
		if (0 != pidList[i]) {
			VERBOSE(printf("multirun: reaping process %ld\n",
		               pidList[i]));
#ifdef _WIN32
			TerminateProcess(pidList[i], 0);
#else
			if (doFreakyKill) {
				kill(pidList[i]+1, SIGINT);
				sleep(2);
			}
                        kill(pidList[i], SIGINT);
#endif
		}
	}

	VERBOSE(printf("multirun: returning %s (%d)\n", 
	               (res == 0 ? "success" : "failure"), res));
	return res;
}
