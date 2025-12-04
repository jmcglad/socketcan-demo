# SocketCAN Demo Programs

These programs demonstrate how to use the SocketCAN API on Linux. Feel free to use these as a starting point for learning how to write your own SocketCAN applications.

See the [official documentation](https://docs.kernel.org/networking/can.html) on SocketCAN for more details. Also, see [can-utils](https://github.com/linux-can/can-utils) for more advanced SocketCAN user space applications.

## Raw Interface Demo

This program demonstrates reading and writing to a CAN bus using SocketCAN's raw interface. The intended behavior of this program is to read in any CAN message from the bus, add one to the value of each byte in the received message, and then write that message back out to the bus with the message ID defined by the macro MSGID.

## Broadcast Manager Interface Demo

This program demonstrates reading and writing to a CAN bus using SocketCAN's broadcast manager interface. The intended behavior of this program is to read in CAN messages which have an ID of 0x123, add one to the value of each data byte in the received message, and then write that message back out to the bus with the message ID defined by the macro MSGID.

## Broadcast Manager Cyclic Demo

This program demonstrates sending a set of cyclic messages out to the CAN bus using SocketCAN's broadcast manager interface. The intended behavior of this program is to send four cyclic messages out to the CAN bus. These messages have IDs ranging from 0x0C0 to 0x0C3. These messages will be sent out one at a time every 1200 milliseconds. Once all messages have been sent, transmission will begin again with message 0x0C0.
