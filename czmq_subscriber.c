#include <czmq.h>
#include <signal.h>

zsock_t *sub;

void sig_handler(int signum)
{
	if(signum == SIGTERM || signum == SIGINT){
		zsock_destroy(&sub);
		exit(EXIT_SUCCESS);
	}
}

int main (int argc, char *argv[])
{
	sub = zsock_new_sub ("@tcp://127.0.0.1:5000","");
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	while(1){
		char *string = zstr_recv(sub);
		printf(string);
		zstr_free(&string);
		fflush(stdout);
	}
	
	zsock_destroy(&sub);
	return 0;
}
