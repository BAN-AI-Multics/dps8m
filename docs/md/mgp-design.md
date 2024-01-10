
<!-- SPDX-License-Identifier: LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2024 The DPS8M Development Team -->
<!-- scspell-id: 5d4185d0-fefd-11ed-b27a-80ee73e9b8e7 -->

<!-- pagebreak -->

# How the MGP-with-cbridge is supposed to work

Multics originally connected to Chaosnet by means of a PDP11 which implemented window management, retransmissions etc. For communicating with the PDP11, Multics used higher-level packets (see `mgp_packet.incl.pl1`) similar to those in the `chaos_packet` NCP interface of [cbridge](https://github.com/bictorv/chaosnet-bridge/blob/master/NCP.md#chaos_packet). Below they are referred to as "cbridge packets".

Making Multics use cbridge in place of the PDP11 seems rather straight-forward, and the principles are described below. The code in `dps8_mgp.pl1` is referred to as "the MGP driver". The MGP driver communicates with cbridge using "unix sockets".

## MGP packets
Some findings about MGP packets:

  - `frame_number` and `receipt_number` count frames sent/received between the MGP driver and Multics, while
  - `packet_number` and `ack_number` count packets sent/acked on a particular connection

## Packet translation: Multics to cbridge

Multics MGP packets are translated to cbridge packets as follows.
All 9-bit data is translated to 8-bit data.

### CONNECT
An MGP CONNECT packet has a string payload, for example `CHAOS 3443 TELNET ` or `CHAOS 3405 NAME /W BV`, i.e. a network name (here "CHAOS"), the host address (here 3443 or 3405), the Chaosnet "contact name" (here TELNET or NAME), followed by any arguments to the contact (here "/W BV").

This is translated to a cbridge RFC packet where the contents are the host address, contact name, and args.

### OPEN
An MGP OPEN packet has two forms, depending on its `chaos_opcode` field: it can open a stream connection and then corresponds to a cbridge OPN packet, or it can be a "simple" protocol (see end of [Section 4.1](https://chaosnet.net/amber.html#Connection-Establishment) in MIT AIM 628) answer, corresponding to an cbridge ANS packet.

### CLOSE
An MGP CLOSE packet matches a Chaosnet CLS packet. However, to play nicely and try to ensure that previous data reaches its destination, it is translated to a "controlled close sequence": first a special form of a cbridge EOF packet is sent, making cbridge wait for it to be acked from the other host, resulting in a cbridge ACK packet sent to the MGP driver. It then sends a cbridge CLS packet and closes the socket.

### LOSE
An MGP LOSE packet matches a cbridge LOS packet, indicating lossage. When the MGP driver has sent the LOS, it closes the socket.

### STATUS
An MGP STATUS packet is not related to a specific connection, but informs the recipient about the last frame received. This is handled locally in the MGP driver, which keeps track of what frames it has sent and received from Multics.

### SEND STATUS
This is also handled locally in the MGP driver, and results in sending a STATUS back to Multics.

### ORDER
This is not handled, as the Multics code in `mgp_queue_manager_.pl1` doesn't use it.

### DATA
Similarly to CLOSE, an MGP DATA packet can correspond to two types of Chaosnet packets, depending on the `chaos_opcode` field. Either it is a DAT packet, or an EOF packet (without data).

### Flags
Every MGP packet received from Multics seems to have the `loopback` bit set, which is confusing since the code in `mgp_queue_manager_.pl1` seems to only set it if the "net" is MULTICS (rather than CHAOS).

Packets typically also have the `reply_NOW` bit set. I interpret this as "we should send some reply right away", and try to generate either a STATUS packet for the connection, or a NOOP packet (connectionless), unless a packet is already sent on the connection.

## Packet translation: cbridge to Multics

Similar, but the other way around. Note that several Chaosnet packet types never occur on in the cbridge `chaos_packet` NCP protocol, since they are handled only in cbridge itself: SNS, STS, RUT, MNT, BRD. Chaosnet packet types FWD and UNC are not handled (yet).
All 8-bit data is translated to 9-bit data.

  - RFC becomes CONNECT
  - OPN and ANS becomes OPEN
  - CLS becomes CLOSE
  - LOS becomes LOSE
  - EOF and DAT becomes DATA

### Flags
None generated.

## Connections

A connection between Multics and the MGP driver is identified by a pair of <source,destination> processes, similar to Chaosnet indices. When a connection is opened from Multics, the MGP driver opens a socket to cbridge, and keeps track of what Multics connection it corresponds to.

No retransmissions etc should be necessary (I hope) since the communication is (should be) flawless between Multics and the MGP driver, and between the MGP driver and cbridge.

Starting a Chaosnet server in Multics is not yet supported (see below).

## Writing packets from Multics

Multics issues an IOM command to write packets to the MGP driver. The MGP driver parses the packet, and typically translates it to a cbridge packet which is sent to cbridge.

No errors are reported to Multics. They should be.

## Reading packets to Multics

Multics issues an IOM command to read packets from the MGP driver. For this to happen, it needs to be told when packets are available to read.

The emulator "poll loop" calls a routine in the MGP driver every 100 Hz, which does a `select()` call to see if any of its sockets open to cbridge has data available. If so, it signals Multics using `send_terminate_interrupt`.

When Multics asks to read packet from the MGP driver, the driver also does a `select()` but this time also reads a packet from one of the sockets (in a round-robin fashion). If there was data, the driver returns `IOM_CMD_DISCONNECT` which makes a "terminate interrupt" happen and Multics read use the read data.

However Multics is very slow to react to packets. It takes a minute or three before the first data packet is displayed in a `chtn` session. Some interrupt or other? **Multicians, please help?**

## To do

### Enable tracing in `mgp_queue_manager`
Solved by adding `mgp_queue_manager_$trace_on` to `start_up.ec`.
Note: the result is in `mgp_trace_segment`, which needs `abc mgp_trace_segment` to adjust the byte count before reading.

### Read the trace generated, and find problems
Two possible aspects: MGP-to-Multics communication, and Chaosnet. For the first, **Multicians are needed**, and after that BV can help with the second.

### Handle errors
Passing errors to Multics, somehow. **Multicians, please help.** Look for `@@@@` in the code.

### Use reasonable status codes
Again, **Multicians** might want to look at this (rather than using just 04000 or some "arbitrary error code")?

### Listening
Setting up a server listening on a contact name in the `chaos_packet` cbridge NCP interface is currently done by sending a `LSN contact` packet to cbridge.

This would require changes to `mgp_queue_manager` and defining a new MGP packet type, which should be doable (**by Multicians**).

Another option would be to add a "wildcard listen" option to cbridge, e.g. `LSN *`. This would pass all RFCs from Chaosnet over the cbridge socket. These cbridge RFC packets would need to include the contact name used, which currently is not the case.

Perhaps cbridge should be configurable to *avoid* handling some contact names which are currently built-in (status, time, uptime, ...)

### cbridge configuration
Make a "standard config" for a cbridge which is only used as a front-end for Multics. What is needed is essentially `ncp enabled yes`, and links to other nodes (see [examples](https://github.com/bictorv/chaosnet-bridge/blob/master/EXAMPLES.md)). The cbridge is the Chaosnet front-end of Multics, so its address is the Multics Chaosnet address. You might want to use something like `myname Multics-name` for cosmetics.

If you want to run one cbridge as a front-end for Multics, and another one on the same host for other purposes, the socket names will collide. Perhaps make a suffix configurable?
