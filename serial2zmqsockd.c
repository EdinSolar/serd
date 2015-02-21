#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <czmq.h>

#define DAEMON_NAME "serd"

zsock_t *pub;

void rundaemon(void)
{
	syslog(LOG_DEBUG, "zstring sent");
	zstr_send (pub, "FUCK SOCKETS.\n");
}

void sig_handler(int signum)
{
	if(signum == SIGTERM || signum == SIGINT){
		zsock_destroy(&pub);
		exit(EXIT_SUCCESS);
	}
}

int main (int argc, char *argv[])
{
        setlogmask (LOG_UPTO (LOG_NOTICE));
        openlog (DAEMON_NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

        syslog(LOG_INFO, "Beginning ", DAEMON_NAME);

        pid_t pid;
        pid_t sid;

        /* Fork the parent process: */
        pid = fork();

        if (pid < 0) exit(EXIT_FAILURE);
        if (pid > 0) exit(EXIT_SUCCESS);

        umask(0);

        /* Set a new signature ID for the child process */
        sid = setsid();

        if (sid < 0) exit(EXIT_FAILURE);

        /*
         * Change DIR
         * If we can't change to the root directory
         */
        if ((chdir("/")) < 0) exit(EXIT_FAILURE);


	/* Initialise socket: */
        pub = zsock_new_pub (">tcp://127.0.0.1:5000");
	/* Handle kill signals */
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

        /* Close standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

	while(true){
		rundaemon();
		sleep(20);
	}

        closelog();
}
