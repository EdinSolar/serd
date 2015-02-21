#!/bin/sh
echo "Compiling serd..."
gcc serial2zmqsockd.c -o serd -lczmq -lzmq
echo "Compiling test..."
gcc czmq_subscriber.c -o test -lczmq -lzmq
echo "Done."
