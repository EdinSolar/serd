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
#include <termios.h>
#include <czmq.h>

#define SERIALPORT "/dev/ttyAMA0"
#define SER_READ_TIMEOUT 20
#define SERIAL_BUFFER_SIZE 4000
#define DAEMON_NAME "serd"

/* zeromq publisher */
zsock_t *pub;
/* File descripter for serial port */
int file_desc = -1;

/* Buffer to yank from serial and pipe to zeromq */
void* serialbuff[SERIAL_BUFFER_SIZE+1];

/* Serial port config struct */
struct termios io;


void rundaemon(void)
{
	int n = read(file_desc, serialbuff, SERIAL_BUFFER_SIZE);
	if(n > SERIAL_BUFFER_SIZE) syslog(LOG_ERR, "Serial buffer size exceeded!");

	if(n != 0){
		serialbuff[n] = '\0';
		zstr_send(pub, (char*) serialbuff);
	}
}

void dofork(void)
{
	pid_t pid;
        pid_t sid;

        /* Fork the parent process: */
        pid = fork();

        if (pid < 0) exit(EXIT_FAILURE);
        if (pid > 0) exit(EXIT_SUCCESS);

	/* Adopt perms of initialising user */
        umask(0);

        /* Set a new signature ID for the child process */
        sid = setsid();

        if (sid < 0) exit(EXIT_FAILURE);

        /*
         * Change DIR
         * If we can't change to the root directory then die
         */
        if ((chdir("/")) < 0) exit(EXIT_FAILURE);

	/* Close standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
}


void sig_handler(int signum)
{
	if(signum == SIGTERM || signum == SIGINT){
		zsock_destroy(&pub);
		close(file_desc);
		syslog(LOG_NOTICE, "Exiting successfully...");
		exit(EXIT_SUCCESS);
	}
}


void initserial(void)
{
	/* Open dev file with no delay on the _reading_ */
	file_desc = open(SERIALPORT, O_RDWR | O_NOCTTY | O_NDELAY);

	if (file_desc < 0){
		syslog(LOG_ERR, "Can't open serial port");
		exit(EXIT_FAILURE);
	}

	/* Set file flags for serial port */
	fcntl(file_desc, F_SETFL, SER_READ_TIMEOUT);

	/* load and set serial line conf */
	tcgetattr(file_desc, &io);
	
	/* Set baud */
	cfsetispeed(&io, B115200);
	cfsetospeed(&io, B115200);

	/* Enable reciever and set local mode */
	io.c_cflag |= (CLOCAL | CREAD);
	
	tcsetattr(file_desc, TCSANOW, &io);
}


int main (int argc, char *argv[])
{
        setlogmask (LOG_UPTO (LOG_INFO));
        openlog (DAEMON_NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

        syslog(LOG_INFO, "Beginning init...");

	dofork();

	/* Initialise serial: */
	initserial();

	/* Initialise socket: */
        pub = zsock_new_pub (">tcp://127.0.0.1:5000");
	/* Handle kill signals */
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	syslog(LOG_INFO, "Initialised successfully");

	while(1){
		rundaemon();
		//sleep(20);
		//write(file_desc, "ping\n", 5);
	}

        closelog();
}
