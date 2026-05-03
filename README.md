# Nucleo_F439ZI
Just a example as a base for future projects.

# What to be done in this project:
* Cmake as a build
* ETH with LwIP
* ERPC communication as server
    * Interface for reading every possible variable from RAM
    * Interface for reading DET
* ERPC as a client
* CAN for communication with VESC
* DET (development error tracer) - To log any issues. It should be configured as event and a list of variables to be stored.


CAN communication module:
- receive message from interrupt and put in into queue
- it has a array of callback function (functions are registered when class object is created)
- it run the callbacks until callback returns true or out of callbacks
- the object class check if the message is in defined range (check out the virtual option for doing it - maybe there should be option to overwrite it with custom checking function)
- the object have a methed that will wait for the message arrival
- Add option for timeout - callback when message did not arrived at time

CanDriverInstance - this is global object
when CanListener is created - I will need to pass there a pointer to CanDriverInstance to assign the listener to specific instance