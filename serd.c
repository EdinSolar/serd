#include <sys/types.h>  /* Types */
#include <sys/stat.h>   /* File Status */
#include <stdio.h>      /* Standard Input/Output functions & macros */
#include <string.h>     /*  */
#include <stdlib.h>     /* Standard library functions & macros */
#include <fcntl.h>      /* File control */
#include <errno.h>      /* Contains error numbers for process exit */
#include <signal.h>     /* Signal handling (e.g. SIGTERM, SIGINT) */
#include <unistd.h>     /* (UART) Contains POSIX symbolic constants */
#include <syslog.h>     /* System log library */
#include <termios.h>    /* (UART) Serial line config & control */
#include <czmq.h>       /* C ZeroMQ Binding */

/* Default TTY */
#define DEFAULT_TTY "/dev/ttyAMA0"

/* Fluff for 0MQ socket init */
#define SERD_DEF_PUBSOCK ">tcp://127.0.0.1:"
#define SERD_DEF_SUBSOCK "@tcp://127.0.0.1:"
/* Defauly publisher & subscriber ports */
#define SERD_DEF_PUB_PRT "5000"
#define SERD_DEF_SUB_PRT "5001"

#define SER_READ_TIMEOUT 20
/* Max size of input serial buffer, in chars */
#define SERIAL_BUFFER_SIZE 4000
#define DAEMON_NAME "serd"

#define SERD_LOG_LEVEL LOG_DEBUG

/* zeromq publisher & subscriber sockets */
zsock_t* pub;
zsock_t* sub;

/* File descripter for serial port, initialised to an error. */
int tty_file_desc = -1;

/* Buffer to yank from serial and pipe to zeromq */
unsigned char serialbuff[SERIAL_BUFFER_SIZE+1];

/* Serial port config struct */
struct termios io;

/* Strings to hold config from cli */
char pub_port[6] = SERD_DEF_PUB_PRT;
char sub_port[6] = SERD_DEF_SUB_PRT;
char use_tty[99] = DEFAULT_TTY;

/* Function prototypes: */
void rundaemon(void);
void dofork(void);
void initserial(void);
void initsocket(void);
void _sig_handler(int);


int main (int argc, char *argv[])
{
        setlogmask (LOG_UPTO (SERD_LOG_LEVEL));
        openlog (DAEMON_NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);

        syslog(LOG_INFO, "Beginning init...");

        /* Parse CLI Args: */
        char c;

        while (--argc > 0 && (*++argv)[0] == '-'){
                c = *++argv[0];
                switch (c) {
                        /* publisher port */
                case 'p':
                        if(argc-- >= 0) strcpy(pub_port, *++argv);
                        syslog(LOG_DEBUG, "Publisher socket port arg has value %s", pub_port);
                        break;
                
                        /* subscriber port */
                case 'l':
                        if(argc-- >= 0) strcpy(sub_port, *++argv);
                        syslog(LOG_DEBUG, "Subscriber socket port arg has value %s", sub_port);
                        break;
                        
                        /* tty to use */
                case 's':
                        if(argc-- >= 0) strcpy(use_tty, *++argv);
                        syslog(LOG_DEBUG, "TTY port arg has value %s", use_tty);
                        break;

                        /* Help text */
                case 'h':
                        printf("TODO: Help text.\n");
                        syslog(LOG_INFO, "Exiting with help text.");
                        exit(EXIT_SUCCESS);
                        break;
                
                default:
                        syslog(LOG_INFO, "Called with undefined switch '%c'", c);
                        break;
                }
        }
        

        if (argc != 0){
                syslog(LOG_INFO, "Undefined arguments given! Continuing.");
        }
        
        
	dofork();

	/* Initialise serial: */
	initserial();

        /* Initialise publisher and subscriber sockets: */
        initsocket();

        /* Handle kill signals */
	signal(SIGINT, _sig_handler);
	signal(SIGTERM, _sig_handler);

	syslog(LOG_INFO, "Initialised successfully, publishing to %s%s and listening on %s%s using %s", 
               SERD_DEF_PUBSOCK, pub_port, SERD_DEF_SUBSOCK, sub_port, use_tty);

	while(1) rundaemon();
}




void rundaemon(void)
{
	int n = read(tty_file_desc, serialbuff, SERIAL_BUFFER_SIZE);
        
	if(n >= SERIAL_BUFFER_SIZE) syslog(LOG_ERR, "Serial buffer size exceeded!");

	if(n != 0){
                /* Null terminate at the end of this read given we don't empty the buffer after each read: */
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

        /* Fork failed */
        if (pid < 0) exit(EXIT_FAILURE);
        
        /* Parent process exit */
        if (pid > 0) exit(EXIT_SUCCESS);

	/* Adopt perms of initialising user */
        umask(0);

        /* Set a new signature ID for the child process */
        sid = setsid();

        if (sid < 0) {
                syslog(LOG_ERR, "Could not fork!");
                killpg(getpid(),SIGINT);
        }

        /*
         * Change DIR
         * If we can't change to the root directory then die
         */
        if ((chdir("/")) < 0) exit(EXIT_FAILURE);
        
	/* Close standard file descriptors, can't use them as a daemon */
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
	tty_file_desc = open(use_tty, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (tty_file_desc < 0){
                syslog(LOG_ERR, "Can't open serial port %s (errno %i)", use_tty, tty_file_desc);
		killpg(getpid(),SIGINT);
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


void initsocket(void)
{
        char* ptmp = malloc(strlen(SERD_DEF_PUBSOCK) + strlen(pub_port) +1);
        sprintf(ptmp, "%s%s", SERD_DEF_PUBSOCK, pub_port);

	/* Initialise Publisher socket: */
        pub = zsock_new_pub(ptmp);
        free(ptmp);

        char* stmp = malloc(strlen(SERD_DEF_SUBSOCK) + strlen(sub_port) +1);
        sprintf(stmp, "%s%s", SERD_DEF_SUBSOCK, sub_port);

        /* Initialise Subscriber socket: */
        sub = zsock_new_sub(stmp, "");
        free(stmp);
}


/* Handle system signals (Terminate, Interrupt) */
void _sig_handler(int signum)
{
	if(signum == SIGTERM || signum == SIGINT){
		zsock_destroy(&pub);
		zsock_destroy(&sub);
                syslog(LOG_INFO, "Exiting... (sig %i)", signum);
		closelog();
                close(tty_file_desc);
                exit(EXIT_SUCCESS);
	}
        syslog(LOG_ERR, "Quit unexpectedly.");
}
