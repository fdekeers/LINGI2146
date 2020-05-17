# LINGI2146 - Project

This repository contains our files for the simulation of a air quality sensor network.

# Project structure
Let us describe the different important files contained in the two folders of this repository :

- [`mote`](mote) : contains the source code of the different types of motes (sensor, computation, and border router/root mote)
  - [`routing.c`](mote/routing.c) : this file contains the routing functions that are helpful for the different nodes, regardless of their type (initialize the (potentially root) mote, choose/change/update parent, detach from the tree, send/forward the different types of messages);
  - [`root-mote.c`](mote/root-mote.c) : this contains what is specific to the root-mote (or what is handled in a different way than the other types of motes) such as the communication with the python server and the handling of the different types of messages;
  - [`sensor-mote.c`](mote/sensor-mote.c) : this contains the specific handling of the different types of messages (to enter the tree/exit the tree) and the different timers needed for the children and the parents;
  - [`computation-mote.c`](mote/computation-mote.c) : this one is very similar to the sensor-mote, the only difference is what happens when a DATA packet is received. Indeed, it tries to make the computation of the slope of the least square regression for the mote sending the packet;
  - [`computation.c`](mote/computation.c) : this contains the needed functions by the computation-mote to know if there is enough place to add a new mote or if it has to send an OPEN message regarding the data value it just received;
  - [`hashmap.c`](mote/hashmap.c) : this contains the code of the linear-probing hashmap we adapted from an open-sourced implementation of a generic hashmap. This is used by the different motes as their routing table;
  - [`trickle-timer.c`](mote/trickle-timer.c) : this contains the implementation of the exponential backoff we saw during the courses, this is useful for every mote;
  - [`protocol.txt`](mote/protocol.txt) : this file contains different informations about how the motes send/handle the receiving of the DIS, DIO and DAO messages in our network.
- [`server`](server) : folder containing the Python files needed to run the server
  - [`Packet.py`](server/Packet.py) : this python file contains classes and functions to encode the packets to send and decode the different packets received;
  - [`server.py`](server/server.py) : this is the source code of the Python server, it handles the received data, makes the needed computations and can also send OPEN packets to the different motes by sending a message to the root-mote.

# Definition of the different constants
Multiple constants are defined to make our implementation work. Let us list here the header files that contain the constants you might want to change to suit your needs :
- [`mote/computation.h`](mote/computation.h) : this file contains everything that is related to the computation made by the computation nodes. This includes the maximum number of values a node can contain (MAX_NB_VALUES), the maximum number of sensor nodes that a computation node can do computations for (MAX_NB_COMPUTED), the slope threshold (SLOPE_THRESHOLD), the timeout to erase a sensor node from the computation buffer (TIMEOUT_DATA) and finally the the minimum number of values required to compute the slope of the least square regression of the data values (this number should be contained in [1, MAX_NB_VALUES]).
- [`mote/hashmap.h`](mote/hashmap.h) : apart from the constants defined for the debug mode and for the constants that are needed only by the hashmap/are returned as code values, there is only one interesting constant : the timeout value (in seconds) that helps knowing when we should erase a child from the hashmap (TIMEOUT_CHILDREN).
- [`mote/trickle-timer.h`](mote/trickle-timer.h) : this contains T_MIN and T_MAX, the constants that are used to know when to send the periodic DIO and DAO messages.


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
