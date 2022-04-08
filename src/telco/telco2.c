// telco Connect two slave lines together
//   telco addr1 port1 addr2 port2 [retry_rate]
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <uv.h>

#if defined (__MINGW64__) || \
    defined (__MINGW32__) || \
    defined (__GNUC__) || \
    defined (__clang_version__)
# define NO_RETURN __attribute__ ((noreturn))
# define UNUSED    __attribute__ ((unused))
#else
# define NO_RETURN
# define UNUSED
#endif

static char * addrs[2];
static int ports[2];
static int retryRate = 1;

static uv_loop_t* loop;
static uv_tcp_t socks[2];
static uv_connect_t connects[2];
static int linenos[2] = { 0, 1};
static int otherlinenos[2] = { 1, 0};
static uv_stream_t * streams[2] = { NULL, NULL };

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

static void allocCB (UNUSED uv_handle_t * handle, size_t size, uv_buf_t * buf) {
  * buf = uv_buf_init (malloc (size), size);
}

static void onWrite (uv_write_t * req, int status) {
  int lineno = * (int *) req->data;
  //fprintf (stderr, "\nonWrite %d\n", lineno);
  if (status) {
    fprintf (stderr, "\nuv_write error line %d errno %d\n", lineno, errno);
    return;
  }
  free (req);
}

static void onRead (uv_stream_t * tcp, ssize_t nread, const uv_buf_t * buf) {
  int lineno = * (int *) tcp->data;
  int otherlineno = otherlinenos[lineno];
  if (nread >= 0) {
    //fprintf (stderr, "\nread %d nread %ld buflen %ld\n", lineno, nread, buf->len);
    if (nread) {
      if (streams[otherlineno]) {
        uv_buf_t buffer[] = {{.base = buf->base, .len = nread}};
        uv_write_t * req = malloc (sizeof (uv_write_t));
        req->data = & linenos[otherlineno];
        uv_write (req, streams[otherlineno], buffer, 1, onWrite);
      } else {
        // fprintf (stderr, "dropped %ld\n", nread);
        fprintf (stderr, "d");
      }
    }
  } else {
    //we got an EOF
    fprintf (stderr, "\nslave %d closed\n", lineno);
    uv_close ((uv_handle_t *) tcp, NULL /* onClose */);
    streams[lineno] = NULL;
  }

  free (buf->base);
}

static void onConnect (uv_connect_t * connection, int status) {
  if (status < 0) {
    //fprintf (stderr, "failed to connect\n");
    return;
  }
  int lineno = * (int *) connection->data;
  fprintf (stderr, "\nSlave %d connected. %d\n", lineno, status);

  streams[lineno] = connection->handle;
  streams[lineno]->data = & linenos[lineno];
  uv_read_start (streams[lineno], allocCB, onRead);
}

static void startConn (int lineno) {
  char * host = addrs[lineno];
  int port = ports[lineno];
  uv_tcp_t * pSock = & socks[lineno];
  uv_connect_t * pConn = & connects[lineno];

  uv_tcp_init (loop, pSock);
  pConn->data = & linenos[lineno];
  uv_tcp_keepalive (pSock, 1, 60);

  struct sockaddr_in dest;
  uv_ip4_addr (host, port, & dest);

  uv_tcp_connect (pConn, pSock, (const struct sockaddr *) & dest, onConnect);
}

static void timerCB (UNUSED uv_timer_t* timer) {
  fprintf (stderr, ".");
  if (! streams[0])
    startConn (0);
  if (! streams[1])
    startConn (1);
}

///////////////////
///
/// main
///

int main (int argc, char * argv[]) {
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

  loop = uv_default_loop();

  uv_timer_t timer;
  uv_timer_init (loop, & timer);
  uv_timer_start (& timer, (uv_timer_cb) & timerCB, retryRate * 1000, retryRate * 1000);

  startConn (0);
  startConn (1);

  uv_run (loop, UV_RUN_DEFAULT);
}

