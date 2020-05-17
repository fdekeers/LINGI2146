# LINGI2146 - Project

This repository contains our files for the simulation of a air quality sensor network.

# Project structure
mote : source code of the different types of mote (sensor, computation, and border router)\
server : source code of the Python server


# Simulation
A Cooja simulation file, ```simulation.csc```, is given. The simulation already contains the 3 types of motes.\
To run the simulation :
- Check that the serial socket (server) of the root mote is activated
- Run the python server :
```
python3 server/server.py 127.0.0.1 [serial-socket-port] (optional slope threshold)
```
- Start the simulation

Motes can be removed and added to the network.
