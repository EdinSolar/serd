# SERd - Serial to socket daemon

* See ./dependencies for a breif list of stuff required for serd to run
* You will need to compile and install cZMQ

* serd comes with two test programs, `./stest` and `./ptest`.
  - stest has a ZMQ subscriber; It will listen to the output (port 5000) of serd and print it to stdout.
  - ptest has a publisher; It will write "Hello world" to port 5001 every 10 seconds or so.
  - `serialEmulator.sh` is a simple script generates a pair of emulated TTYs for local testing of a serial interface if a real one if unavailable.

* `./compile.sh` has the correct links for GCC to compile serd and the tests. You don't have to use it and at some point it may get replaced with a proper makefile.

## Usage

* By default serd will publish anything that comes over the serial port to 127.0.0.1:5000
  - The port can be changed wth the command line argument `-p PORT_NUM`

* Serd will listen on port 5001 by default, this is also changeable with the cli argument `-l PORT_NUM`. Any data recieved on this port will be written to the serial port.

* Serd will by default use `/dev/ttyAMA0` as the serial port; This is the hardware serial port on a RaspberryPi. It can be changed using `-s /file/path` although it is supposed to be a TTY.