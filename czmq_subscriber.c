#include <czmq.h>

int main (int argc, char *argv[])
{
	zsock_t *sub = zsock_new_sub ("@tcp://127.0.0.1:5000","");

	while(1){
		char *string = zstr_recv(sub);
		printf(string);
		zstr_free(&string);
		fflush(stdout);
	}
	
	zsock_destroy(&sub);
	return 0;
}
