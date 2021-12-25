// DN355 TCP PACKET
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

// CS generated
#define DN_CMD_BOOTLOAD 'B'
#define DN_CMD_DATA 'D'
#define DN_CMD_XFER_TO_L6 'T'
#define DN_CMD_XFER_FROM_L6 'F'

// FNP generated
#define DN_CMD_READ 'R'
#define DN_CMD_WRITE 'W'
#define DN_CMD_DISCONNECT 'E'
#define DN_CMD_SET_EXEC_CELL 'S'

// CS and FNP generated
#define DN_CMD_CONNECT 'C'


// Read:
//   sprintf (pkt.cmdR.addr, "%08o", addr);
// Data
//   sprintf (pkt.cmdD.data, "%08o:%012llo", addr, data);

struct udpPktStruct {
  char cmd;
  union {
    struct {
      char data[21 + 1]; // %08o:%012llo addr data
    } cmdW;
    struct {
      char addr[8 + 1]; // %08o addr
    } cmdR;
    struct {
      char addr[8 + 1]; // %08o addr
    } cmdT;
    struct {
      char data[21 + 1]; // %08o:%012llo addr data
    } cmdD;
    struct {
      char cell[2 + 1]; // %012llo data
    } cmdS;
  };
};

typedef struct udpPktStruct dnPkt;
