/* -*-Mode: c++; -*-
   Copyright (c) 2003-2008 John Plevyak, All Rights Reserved
*/
#include "plib.h"

#include "conn.h"

static pthread_mutex_t date_mutex = PTHREAD_MUTEX_INITIALIZER;
static time_t date_time = 0;
static char date_string[128] = "";
static int date_string_len = 0;
static cchar *const weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static cchar *const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

Conn::Conn() {
  ifd = ofd = -1;
  factory = 0;
  rbuf.buf = rbuf.cur = rbuf.end = rbuf.bufend = 0;
  rbuf.next = 0;
  wbuf.buf = wbuf.cur = wbuf.end = wbuf.bufend = 0;
  wbuf.next = 0;
}

void Conn::init(int rbuf_size, int wbuf_size) {
  rbuf.buf = (byte *)MALLOC(rbuf_size);
  rbuf.end = rbuf.cur = rbuf.buf;
  rbuf.bufend = rbuf.buf + rbuf_size;
  wbuf.buf = (byte *)MALLOC(wbuf_size);
  wbuf.end = wbuf.cur = wbuf.buf;
  wbuf.bufend = wbuf.buf + wbuf_size;
}

int Conn::get_line() {
  byte *p = rbuf.cur, *x = 0;
  while (1) {
    int lcur = rbuf.end - p;
    if (lcur && (x = (byte *)memchr((char *)p, '\n', lcur))) {
      rbuf.line = rbuf.cur;
      rbuf.cur = x + 1;
      return 0;
    }
    p = rbuf.end;
    int lend = rbuf.bufend - rbuf.end;
    if (lend <= 0) return -1;
    int r = read(ifd, rbuf.end, lend);
    if (r <= 0) {
      switch (errno) {
        case EINTR:
          continue;
        default:
          return -1;
      }
    }
    rbuf.end += r;
  }
}

int Conn::get(int n) {
  while (1) {
    int lcur = rbuf.end - rbuf.cur;
    if (lcur >= n) return 0;
    int lend = rbuf.bufend - rbuf.end;
    if (lend <= 0) return -1;
    int r = read(ifd, rbuf.end, lend);
    if (r <= 0) {
      switch (errno) {
        case EINTR:
          continue;
        default:
          return -1;
      }
    }
    rbuf.end += r;
  }
}

int Conn::get_some() {
Lagain:
  int lend = rbuf.bufend - rbuf.end;
  if (lend <= 0) return -1;
  int r = read(ifd, rbuf.end, lend);
  if (r <= 0) {
    switch (errno) {
      case EINTR:
        goto Lagain;
      default:
        return -1;
    }
  }
  rbuf.end += r;
  return 0;
}

int Conn::put_string(cchar *str, int len) {
  cchar *s = str;
  while (1) {
    int r = write(ofd, s, len);
    if (r < 0) {
      switch (errno) {
        case EINTR:
          continue;
        default:
          return -1;
      }
    }
    if (len == r) return 0;
    s += r;
    len -= r;
  }
}

int Conn::put() {
  byte *str = wbuf.cur;
  (void)str;
  while (1) {
    int r = 0;
    if (!wbuf.next) {
      int len = wbuf.end - wbuf.cur;
      r = write(ofd, wbuf.cur, len);
    } else {
      int n = 2;
      buffer_t *b = wbuf.next;
      while (b) {
        n++;
        b = b->next;
      }
      struct iovec *iovec = (struct iovec*)malloc(sizeof(struct iovec) * n);
      b = &wbuf;
      int i = 0;
      while (b) {
        iovec[i].iov_base = b->cur;
        int l = b->end - b->cur;
        iovec[i].iov_len = l;
        b = b->next;
        r = writev(ofd, iovec, n);
      }
      ::free(iovec);
    }
    if (r < 0) {
      switch (errno) {
        case EINTR:
          continue;
        default:
          return -1;
      }
    }
    reset_wbuf();
    return 0;
  }
}

int Conn::done() {
  if (ifd != -1 && ifd >= STDERR_FILENO) {
    ::close(ifd);
  }
  if (ofd != ifd && ofd != -1 && ofd >= STDERR_FILENO) {
    ::close(ofd);
  }
  pthread_mutex_t *l = &factory->lock;
  pthread_mutex_lock(l);
  free();
  pthread_mutex_unlock(l);
  return 0;
}

void Conn::free() {
  if (factory->conn_freelist) factory->conn_freelist->free(this);
  FREE(rbuf.buf);
  FREE(wbuf.buf);
}

int Conn::error(cchar *s) {
  if (s) elog(s);
  return done();
}

int Conn::append_print(cchar *str, ...) {
  char nstr[2048];
  va_list ap;
  va_start(ap, str);
  vsnprintf(nstr, 2048, str, ap);
  return append_string(nstr);
}

void add_string_buffer(Vec<buffer_t> &bufs, byte *buf, int len) {
  int i = bufs.n;
  bufs.add();
  bufs.v[i].buf = bufs.v[i].cur = buf;
  bufs.v[i].end = bufs.v[i].bufend = buf + len;
}

char *Conn::alloc_xml_value(char *s, char *e, char **r) {
  char *p = (char *)memchr(s, '>', e - s);
  if (!p) return 0;
  p++;
  char *pe = (char *)memchr(s, '<', e - p);
  if (!pe) return 0;
  *r = pe;
  return alloc_str(p, pe - p);
}

char *Conn::alloc_date_str() {
  time_t t = time(0);
  pthread_mutex_lock(&date_mutex);
  if (t != date_time) {
    date_time = t;
    struct tm m;
    gmtime_r(&t, &m);
    date_string_len = sprintf(date_string, "%s, %02u %s %04u %02u:%02u:%02u GMT", weekdays[m.tm_wday], m.tm_mday,
                              months[m.tm_mon], 1900 + m.tm_year, m.tm_hour, m.tm_min, m.tm_sec);
  }
  char *ret = alloc_str(date_string, date_string_len);
  pthread_mutex_unlock(&date_mutex);
  return ret;
}

ConnFactory::ConnFactory(cchar *aname) : name(aname) {
  fd = -1;
  conn_freelist = 0;
  pthread_mutex_init(&lock, 0);
  int_config(DYNAMIC_CONFIG, &port, 0, name, "port");
  int_config(DYNAMIC_CONFIG, &thread_pool.maxthreads, DEFAULT_FACTORY_THREADS, name, "threads");
  int_config(DYNAMIC_CONFIG, &thread_pool.stacksize, DEFAULT_FACTORY_STACKSIZE, name, "stacksize");
}

Server::Server(cchar *aname) : ConnFactory(aname) {}
