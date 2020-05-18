# LINGI2146 - Project

This repository contains our files for the simulation of a air quality sensor network.

# Project structure
Let us describe the different important files contained in the two folders of this repository :

- [`mote`](mote) : folder containing the source code of the different types of motes (sensor, computation, and border router/root mote)
  - [`routing.c`](mote/routing.c) : this file contains the routing functions that are helpful for the different nodes, regardless of their type (initialize the (potentially root) mote, choose/change/update parent, detach from the tree, send/forward the different types of messages);
  - [`root-mote.c`](mote/root-mote.c) : root mote (border router) source code, containing the communication with the python server and the handling of the different types of messages;
  - [`sensor-mote.c`](mote/sensor-mote.c) : sensor motes source code, containing the specific handling of the different types of messages and the different timers;
  - [`computation-mote.c`](mote/computation-mote.c) : computation motes source code, containing the specific handling of the different types of messages and the different timers, the only difference with `sensor-mote.c` being the handling of DATA messages;
  - [`computation.c`](mote/computation.c) : this contains the needed functions by the computation motes to know if there is enough place to add a new mote or if it has to send an OPEN message regarding the data value it just received;
  - [`hashmap.c`](mote/hashmap.c) : this contains the code of the linear-probing hashmap we adapted from an open-sourced implementation of a generic hashmap. This is used by the different motes as their routing table;
  - [`trickle-timer.c`](mote/trickle-timer.c) : this contains the implementation of the trickle timer we saw during the courses, which is useful for every mote;
- [`server`](server) : folder containing the Python files needed to run the server
  - [`Packet.py`](server/Packet.py) : this python file contains classes and functions to encode the packets to send and decode the different packets received;
  - [`server.py`](server/server.py) : this is the source code of the Python server, it handles the received data, makes the needed computations and can also send OPEN packets to the different motes by sending a message to the root-mote.

# Definition of the different constants
Multiple constants are defined to make our implementation work. Let us list here the header files that contain the constants you might want to change to suit your needs :
- [`mote/routing.h`](mote/routing.h) : general constants needed for the routing
  - `RSS_THRESHOLD` : signal strength threshold, in dB, for a mote to change parent to one with a better signal strength;
  - `MAX_RETRANSMISSIONS` : maximum number of retransmissions for reliable unicast transport;
  - `TIMEOUT_PARENT` : timeout value, in seconds, to detach from parent, if the parent has not sent a message during this time;
- [`mote/hashmap.h`](mote/hashmap.h) :
  - `TIMEOUT_CHILDREN` : timeout value, in seconds, after which we should erase a child from the hashmap;
  - `DEBUG_MODE` : turns on debug messages if set to 1;
- [`mote/trickle-timer.h`](mote/trickle-timer.h) : constants related to the trickle timer
  - `T_MIN` : minimum value for T;
  - `T_MAX` : maximum value for T;
- [`mote/sensor-mote.c`](mote/sensor-mote.c) : constants only needed for sensor motes, related to DATA messages
  - `DATA_PERIOD` : sending period, in seconds, of DATA messages;
  - `OPEN_TIME` : opening duration of the valve, in seconds, upon reception of an OPEN message;
- [`mote/computation.h`](mote/computation.h) : constants related to the computation made by the computation nodes
  - `MAX_NB_VALUES` : maximum number of stored values for one sensor node;
  - `MAX_NB_COMPUTED` : maximum number of sensor nodes that a computation node can do computations for;
  - `SLOPE_THRESHOLD` : threshold for the slope value, over which the sensor node should open its valve;
  - `TIMEOUT_DATA` : timeout to erase an unresponsive sensor node from the computation buffer;
  - `MIN_NB_VALUES_COMPUTE` : minimum number of values required to compute the slope of the least square regression of the data values, this number should be contained in [1, `MAX_NB_VALUES`].

Other constants in these files define return values, and should not be changed.


# Simulation
A Cooja simulation file, [`simulation.csc`](simulation.csc), is given. The simulation already contains the 3 types of motes.\
To run the simulation :
- Check that the serial socket (server) of the root mote is activated
- Run the python server :
```
python3 server/server.py 127.0.0.1 [serial-socket-port] (optional slope threshold)
```
- Start the simulation

Motes can be removed and added to the network.
