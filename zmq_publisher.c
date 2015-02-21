#include <zmq.h>
#include <czmq.h>

void rundaemon()
{
	/* Initialise socket: */
	zsock_t* pub = zsock_new(ZMQ_PUB);
	zsock_bind("tcp://*:5000");

	zstr_send(pub, "FUCK SOCKETS.");
}
