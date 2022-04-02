// telco Connect two slave lines together
//   telco addr1 port1 addr2 port2 [retry_rate]
//

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <arpa/inet.h>

static char * addrs[2];
static int ports[2];
static int retryRate = 1;

static int socks[2];
static struct pollfd fds[2];
static struct sockaddr_in serverAddrs[2];

#define BUFSZ 4096

//////////
//
// Usage
//

static void usage (void) {
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "  telco addr1 port1 addr2 port2 [retry_rate]\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "addr1:port1  TCP address of slave line 1\n");
  fprintf (stderr, "addr2:port2  TCP address of slave line 2\n");
  fprintf (stderr, "retry_rate Attempt to connect to slave line every n seconds; default 1\n");
}

static void setup (int lineno) {
  socks[lineno] = socket (AF_INET, SOCK_STREAM, 0);
  if (socks[lineno] == -1) {
    fprintf (stderr, "\nsocket %d creation failed...\n", lineno);
    exit (1);
  }
  memset (& serverAddrs[lineno], 0, sizeof (serverAddrs[lineno]));

  // assign IP, PORT
  serverAddrs[lineno].sin_family = AF_INET;
  serverAddrs[lineno].sin_addr.s_addr = inet_addr (addrs[lineno]);
  serverAddrs[lineno].sin_port = htons (ports[lineno]);
}

static void call (int lineno) {
  int rc;
  rc = connect (socks[lineno], (struct sockaddr *) & serverAddrs[lineno], sizeof (struct sockaddr_in));
  if (rc != 0) {
    //fprintf (stderr, "slave %d not connected %d %d\n", lineno, rc, errno);
    //perror ("call");
    return;
  }
  fprintf (stderr, "\nslave %d connected\n", lineno);
  fds[lineno].fd = socks[lineno];
  return;
}

static int relay (int lineno, int otherlineno) {
  uint8_t buffer[BUFSZ];
  ssize_t nread = read (fds[lineno].fd, buffer, sizeof (buffer));
  if (nread == -1) {
    if (errno == 0) { // EOF, I think
      fprintf (stderr, "slave %d disconnected\n", lineno);
      close (fds[lineno].fd);
      setup (lineno);
      fds[lineno].fd = -1;
      return -1;
    }
    fprintf (stderr, "\nread (%d) nread %ld errno %d\n", lineno, nread, errno);
    return -1;
  }
  if (nread == 0) {
    fprintf (stderr, "\nread (%d) nread %ld errno %d\n", lineno, nread, errno);
    //if (errno == 0) {  // Assume connection closed
      fprintf (stderr, "slave %d disconnected\n", lineno);
      close (fds[lineno].fd);
      setup (lineno);
      fds[lineno].fd = -1;
    //}
    return 0; // End of available data
  }
  write (fds[otherlineno].fd, buffer, nread);
  return 0;
}

static int ready (int lineno, int ev, int otherlineno) {
  // Ready for read?
  if (ev & POLLIN) {
    // Clear the read
    ev &= ~ POLLIN;
    // Read the data
    int rc = relay (lineno, otherlineno);
    if (rc)
       return rc;
  }
  // Any other events?
  if (ev) {
    fprintf (stderr, "\nunhandled ev %x\n", ev);
  }
  return 0;
}

///////////////////
///
/// main
///

int main (int argc, char * argv[]) {
  int rc;
  char * endptr;

  if (argc != 5 && argc != 6) {
    usage ();
    exit (1);
  }

  addrs[0] = argv[1];
  ports[0] = strtol (argv[2], & endptr, 0);
  if (* endptr || ports[0] < 0 || ports[0] > 65535) {
    fprintf (stderr, "Invalid value for port1 (0 - 65535)\n");
    usage ();
    exit (1);
  }

  addrs[1] = argv[3];
  ports[1] = strtol (argv[4], & endptr, 0);
  if (* endptr || ports[1] < 0 || ports[1] > 65535) {
    fprintf (stderr, "Invalid value for port2 (0 - 65535)\n");
    usage ();
    exit (1);
  }

  if (argc == 6) {
    retryRate = strtol (argv[5], & endptr, 0);
    if (* endptr || retryRate < 1) {
      fprintf (stderr, "Invalid value for retry rate (rate >= 1)\n");
      usage ();
      exit (1);
    }
  }

  fprintf (stderr, "slave 1 %s:%d\n", addrs[0], ports[0]);
  fprintf (stderr, "slave 2 %s:%d\n", addrs[1], ports[1]);
  fprintf (stderr, "retry rate %d\n", retryRate);

// Setup sockets

  setup (0);
  setup (1);

// Setup for I/O polling

  fds[0].fd = -1;
  fds[0].events = POLLIN;
  fds[1].fd = -1;
  fds[1].events = POLLIN;

// Poll...

  while (1) {
    rc = poll (fds, sizeof (fds) / sizeof (struct pollfd), retryRate * 1000 /* seconds */);
    if (rc) {
      if (fds[0].revents)
        ready (0, fds[0].revents, 1);
      if (fds[1].revents)
        ready (1, fds[1].revents, 0);
    } else { // Timeout
//fprintf (stderr, ".");
      if (fds[0].fd == -1)
        call (0);
      if (fds[1].fd == -1)
        call (1);
    }
  }
}

