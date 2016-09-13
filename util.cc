/* -*-Mode: c++;-*-
   Copyright (c) 2003-2008 John Plevyak, All Rights Reserved
*/
#include "plib.h"

#define FREER_RUN_DELAY_SECONDS 300

class UtilService : public Service {
 public:
  pthread_t freer_thread;
  void start();
  void stop();
} util_service;

int bind_port(int port, int protocol, bool nolinger) {
  int fd = socket(AF_INET, protocol, 0);
  if (fd < 0) PERROR("socket");
  int one = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one))) PERROR("setsockopt");
  struct sockaddr_in name;
  memset(&name, 0, sizeof(name));
  name.sin_family = AF_INET;
  name.sin_port = htons((short)port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(fd, (struct sockaddr *)&name, sizeof(name))) PERROR("bind");
  int addrlen = sizeof(name);
  if (getsockname(fd, (struct sockaddr *)&name, (socklen_t *)&addrlen)) PERROR("getsockname");
  if (protocol == SOCK_STREAM) {
    if (nolinger) {
      struct linger linger;
      linger.l_onoff = 0;
      linger.l_linger = 0;
      if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(struct linger))) PERROR("setsockopt");
      exit(EXIT_FAILURE);
    }
    if (listen(fd, 1024)) PERROR("listen");
  }
  return fd;
}

int accept_socket(int socket, bool nolinger, bool tcp_nodelay, int set_buf_size) {
  struct sockaddr_in client_hostname;
  int size = sizeof(client_hostname);
  int fd = -1;
  while (1) {
    fd = accept(socket, (struct sockaddr *)&client_hostname, (socklen_t *)&size);
    if (fd < 0) {
      switch (errno) {
        case EINTR:
        case ECONNABORTED:
          continue;
        case EAGAIN:
        case ENOTCONN:
          return 0;
        default:
          PERROR("accept");
      }
    } else
      break;
  }
  if (set_buf_size) {
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&set_buf_size, sizeof(set_buf_size)) < 0)
      PERROR("setsockopt");
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&set_buf_size, sizeof(set_buf_size)) < 0)
      PERROR("setsockopt");
  }
  if (tcp_nodelay) {
    int one = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one))) PERROR("setsockopt");
  }
  if (nolinger) {
    struct linger linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof(struct linger)) < 0) PERROR("setsockopt");
  }
  return fd;
}

int connect_socket(in_addr_t addr, int port, int client_buf_size, bool client_nodelay, bool client_nolinger) {
  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd < 0) PERROR("socket");
  if (client_buf_size) {
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&client_buf_size, sizeof(client_buf_size)) < 0)
      PERROR("setsockopt");
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&client_buf_size, sizeof(client_buf_size)) < 0)
      PERROR("setsockopt");
  }
  if (client_nodelay) {
    int one = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)) < 0) PERROR("setsockopt");
  }
  if (client_nolinger) {
    struct linger linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof(struct linger)) < 0) PERROR("setsockopt");
  }

  struct sockaddr_in name;
  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = addr;
  while (connect(fd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    if (errno == EINTR) continue;
    return -1;
  }
  return fd;
}

pthread_t create_thread(void *(*fn)(void *), void *data, int thread_stacksize) {
  pthread_t t;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (thread_stacksize) pthread_attr_setstacksize(&attr, thread_stacksize);
  pthread_create(&t, &attr, fn, data);
  pthread_attr_destroy(&attr);
  return t;
}

uint get_inet_addr(char *hostname) {
  uint addr = inet_addr(hostname);
  struct hostent *hent = NULL;
  if (!addr || ((int)addr == -1)) {
    hent = gethostbyname(hostname);
    if (!hent) {
      fprintf(stderr, "gethostbyname failed: h_errno %d\n", h_errno);
      exit(1);
    }
    addr = *((uint *)hent->h_addr);
  }
  return addr;
}

char *memmem(char *m, char *e, char *p, int l) {
  if (l == 1) return (char *)memchr(m, *p, e - m);
  while (1) {
    m = (char *)memchr(m, *p, e - m);
    if (!m || m + l > e) return 0;
    if (!memcmp(m + 1, p + 1, l - 1)) return m;
    m++;
  }
  return 0;
}

hrtime_t hrtime() {
#if 0
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (ts.tv_sec * HRTIME_SEC + ts.tv_nsec * HRTIME_NSEC);
#else
  timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * HRTIME_SEC + tv.tv_usec * HRTIME_USEC);
#endif
}

void hrtime_to_ts(hrtime_t t, struct timespec *ts) {
  ts->tv_sec = t / HRTIME_SEC;
  ts->tv_nsec = (t - ((t / HRTIME_SEC) * HRTIME_SEC));
}

void wait_for(hrtime_t t) {
  struct timespec ts;
  hrtime_to_ts(t, &ts);
  nanosleep(&ts, NULL);
}

void wait_till(hrtime_t target) {
  hrtime_t now = hrtime();
  while (1) {
    if (now > target) break;
    struct timespec ts;
    hrtime_to_ts(target - now, &ts);
    nanosleep(&ts, NULL);
    now = hrtime();
  }
}

struct freer_t {
  void *p;
  hrtime_t t;
};

static pthread_mutex_t freer_mutex = PTHREAD_MUTEX_INITIALIZER;
static Vec<freer_t> freers;
static int freer_dirty = 0;

void free_in(void *p, hrtime_t t) {
  hrtime_t now = hrtime();
  t += now;
  pthread_mutex_lock(&freer_mutex);
  freer_t &f = freers.add();
  f.p = p;
  f.t = t;
  freer_dirty = 1;
  pthread_mutex_unlock(&freer_mutex);
}

void free_in(Vec<void *> &v, hrtime_t t) {
  hrtime_t now = hrtime();
  t += now;
  pthread_mutex_lock(&freer_mutex);
  forv_Vec(void *, x, v) {
    freer_t &f = freers.add();
    f.p = x;
    f.t = t;
  }
  freer_dirty = 1;
  pthread_mutex_unlock(&freer_mutex);
}

static int compar_freer(const void *a, const void *b) {
  freer_t *i = *(freer_t **)a;
  freer_t *j = *(freer_t **)b;
  if (i->t > j->t) return 1;
  if (i->t < j->t) return -1;
  return 0;
}

static void *freer_main(void *data) {
  while (1) {
    sleep(FREER_RUN_DELAY_SECONDS);
    hrtime_t now = hrtime();
    pthread_mutex_lock(&freer_mutex);
    if (freer_dirty) {
      qsort(&freers.v[0], freers.n, sizeof(freers.v[0]), compar_freer);
      freer_dirty = 0;
    }
    int i = 0;
    for (; i < freers.n; i++) {
      if (freers.v[i].t < now)
        FREE(freers.v[i].p);
      else
        break;
    }
    if (i > 0) {
      memmove(&freers.v[0], &freers.v[i], sizeof(freers.v[0]) * (freers.n - i));
      freers.n -= i;
    }
    pthread_mutex_unlock(&freer_mutex);
  }
  return 0;
}

void UtilService::start() { freer_thread = create_thread(freer_main, 0); }

void UtilService::stop() { pthread_cancel(freer_thread); }
