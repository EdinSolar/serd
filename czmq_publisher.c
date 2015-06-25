#include <czmq.h>
#include <signal.h>

zsock_t *pub;

void _sig_handler(int);

int main (int argc, char *argv[])
{
	pub = zsock_new_pub(">tcp://127.0.0.1:5001");
	signal(SIGINT, _sig_handler);
	signal(SIGTERM, _sig_handler);

	while(1){
		zstr_send(pub, "Hello world!\n\0");
                printf("Hello world!\n");
                sleep(10);
	}
	
	zsock_destroy(&pub);
	return 0;
}


void _sig_handler(int signum)
{
	if(signum == SIGTERM || signum == SIGINT){
		zsock_destroy(&pub);
		exit(EXIT_SUCCESS);
	}
}
