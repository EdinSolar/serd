#!/bin/sh
echo "Compiling serd..."
gcc serd.c -o serd -lczmq -lzmq
echo "Compiling subscriber test..."
gcc czmq_subscriber.c -o stest -lczmq -lzmq
echo "Compiling publisher test..."
gcc czmq_publisher.c -o ptest -lczmq -lzmq
echo "Done."
