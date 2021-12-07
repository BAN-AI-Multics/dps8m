// DN355 UDP PACKET
//

// UDP Packet
//
// From CS to FNP
//  Cmd:
//   B:   bootload
//   D addr data: reply to R; addr and data read from core
//
// From FNP to CS
//   R addr:  Read word from core
//   W addr data: Write word to core
//   C Connect
//   E Disconnet

#define DN_CMD_BOOTLOAD 'B'
#define DN_CMD_READ 'R'
#define DN_CMD_WRITE 'R'
#define DN_CMD_DATA 'D'
#define DN_CMD_CONNECT 'C'
#define DN_CMD_DISCONNECT 'E'

// Read:
//   sprintf (pkt.cmdR.addr, "%08o", addr);
// Data
//   sprintf (pkt.cmdD.data, "%08o:%012llo", addr, data);

struct udpPktStruct {
  char cmd;
  union {
    struct {
      char data[21 + 1];
    } cmdW;
    struct {
      char addr[8 + 1];
    } cmdR;
    struct {
      char data[21 + 1];
    } cmdD;
  };
};

typedef struct udpPktStruct dnPkt;
