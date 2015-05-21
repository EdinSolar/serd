#include <sys/types.h>  /* Types */
#include <sys/stat.h>   /* File Status */
#include <stdio.h>      /* Standard Input/Output functions & macros */
#include <stdlib.h>     /* Standard library functions & macros */
#include <fcntl.h>      /* (UART) File control */
#include <errno.h>      /* Contains error numbers for process exit */
#include <signal.h>     /* Signal handling (e.g. SIGTERM, SIGINT) */
#include <unistd.h>     /* (UART) Contains POSIX symbolic constants */
#include <syslog.h>     /* System log library */
#include <string.h>     /* Strings, duh */
#include <termios.h>    /* (UART) Serial line control */
#include <czmq.h>       /* C ZeroMQ Binding */

#define SERIALPORT "/dev/ttyAMA0"
/*#define SERIALPORT "/dev/pts/11"*/
#define SERD_SOCKET "tcp://127.0.0.1:5000"
#define SER_READ_TIMEOUT 20
#define SERIAL_BUFFER_SIZE 4000
#define DAEMON_NAME "serd"

#define SERD_LOG_LEVEL LOG_DEBUG

/* zeromq publisher socket */
zsock_t *pub;
/* File descripter for serial port */
int tty_file_desc = -1;

/* Buffer to yank from serial and pipe to zeromq */
unsigned char serialbuff[SERIAL_BUFFER_SIZE+1];

/* Serial port config struct */
struct termios io;

/* Helper prototypes: */
void rundaemon(void);
void dofork(void);
void initserial(void);
void _sig_handler(int);


int main (int argc, char *argv[])
{
        setlogmask (LOG_UPTO (SERD_LOG_LEVEL));
        openlog (DAEMON_NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

        syslog(LOG_INFO, "Beginning init...");

	dofork();

	/* Initialise serial: */
	initserial();

	/* Initialise socket: */
        pub = zsock_new_pub (">"SERD_SOCKET);
	
        /* Handle kill signals */
	signal(SIGINT, _sig_handler);
	signal(SIGTERM, _sig_handler);

	syslog(LOG_INFO, "Initialised successfully on %s using %s", SERD_SOCKET, SERIALPORT);

	while(1) rundaemon();
}




void rundaemon(void)
{
	int n = read(tty_file_desc, serialbuff, SERIAL_BUFFER_SIZE);
	if(n > SERIAL_BUFFER_SIZE) syslog(LOG_ERR, "Serial buffer size exceeded!");

	if(n != 0){
                /* Null terminate at the end of this read given we don't clean the buffer: */
		serialbuff[n-1] = '\0';
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


void initserial(void)
{
	/* 
           O_RDWR      --  Read & Write enabled
           O_NOCTTY    --  Do NOT use the TTY as this process' controlling terminal
           o_NONBLOCK  --  Do not allow operations on this file to cause blocking
        */
	tty_file_desc = open(SERIALPORT, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (tty_file_desc < 0){
                syslog(LOG_ERR, "Can't open serial port %s (errno %i)", SERIALPORT, tty_file_desc);
		exit(EXIT_FAILURE);
	}

	/* Set file flags for serial port */
	fcntl(tty_file_desc, F_SETFL, SER_READ_TIMEOUT);

	/* load and set serial line conf */
	tcgetattr(tty_file_desc, &io);
	
	/* Set baud rates for in and out */
	cfsetispeed(&io, B115200);
	cfsetospeed(&io, B115200);

	/* Enable reciever and set local mode
           with bitwise operation */
	io.c_cflag |= (CLOCAL | CREAD);
	
        /* Set termios attributes. TCSANOW means the descriptor 
           will change as soon as a write occurs. */
	tcsetattr(tty_file_desc, TCSANOW, &io);
}


/* Handle system signals (Terminate, Interrupt) */
void _sig_handler(int signum)
{
	if(signum == SIGTERM || signum == SIGINT){
		zsock_destroy(&pub);
		syslog(LOG_NOTICE, "Exiting...");
		closelog();
                close(tty_file_desc);
		exit(EXIT_SUCCESS);
	}
}
