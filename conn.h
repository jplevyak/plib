/* -*-Mode: c++; -*-
   Copyright (c) 2003-2007 John Plevyak, All Rights Reserved
*/
#ifndef _conn_h_
#define _conn_h_

#define DEFAULT_FACTORY_THREADS          100
#define DEFAULT_FACTORY_STACKSIZE        0 // default
#define DEFAULT_IOBUF_SIZE               512000

#define APPEND_STRING(_s) append_string(_s "", sizeof(_s)-1)
#define APPEND_TO_BUF(_b, _s) do { memcpy(_b.end, _s "", sizeof(_s)-1); _b.end += sizeof(_s)-1; } while (0)
#define ALLOC_STRING(_s) alloc_str(_s, sizeof(_s)-1)
#define LINECMP(_const_string) (((int)sizeof(_const_string ""))-1 > rbuf.line_len() || STRCMP(rbuf.line, _const_string))
#define LINECASECMP(_const_string) (((int)sizeof(_const_string ""))-1 > rbuf.line_len() || STRCASECMP(rbuf.line, _const_string))
#define LINEPCMP(_p, _const_string)  (((int)sizeof(_const_string ""))-1 > rbuf.cur - (_p) || STRCMP(_p, _const_string))
#define LINEPCASECMP(_p, _const_string)  (((int)sizeof(_const_string ""))-1 > rbuf.cur - (_p) || STRCASECMP(_p, _const_string))
#define LINEPSKIPSPACE(_p)  while ((_p) < rbuf.cur && isspace(*(_p))) (_p)++;
#define LINEPSKIP(_p, _const_string) ((_p) += ((int)sizeof(_const_string "")-1))

struct buffer_t {
  byte *buf;
  byte *cur;
  byte *end;
  byte *bufend;
  byte *line;
  struct buffer_t *next;

  int rlen() { return end - cur; }
  int wlen() { return bufend - end; }

  int line_len() { return cur - line; }
  int empty_line() { return (line[0] == '\r' && line[1] == '\n') || line[0] == '\n'; }

  void reset() { cur = end = buf; }
};

int str_len(cchar *s);

class ConnFactory;

class Conn : public ThreadPoolJob { public:
  int fd;
  ConnFactory *factory;
  buffer_t rbuf, wbuf;

  char *alloc_str(char *s, int l = 0) { 
    if (!l) l = strlen(s); char *x = (char*)MALLOC(l); memcpy(x,s,l); return x; 
  }
  char *alloc_xml_value(char *s, char *e, char **r);
  char *alloc_date_str();

  int get_line();
  int get(int n);
  int get_some();

  int append_to_buf(buffer_t &b, cchar *s) { b.end = (byte*)scpy((char *)b.end, s); assert(b.end <= b.bufend); return 0; }
  int append_to_buf_n(buffer_t &b, byte *s, int n) { 
    memcpy(b.end, s, n); b.end += n; assert(b.end <= b.bufend); return 0; 
  }
  int append_string(cchar *str) { append_to_buf(wbuf, str); return 0; }
  int append_string(cchar *str, int len) { append_to_buf_n(wbuf, (byte*)str, len); return 0; }
  int append_bytes(void *b, int len) { append_to_buf_n(wbuf, (byte*)b, len); return 0; }
  int append_str(cchar *str) { return append_string(str, str_len(str)); }

  int put_string(cchar *str, int len);
#define PUT_STRING(_s) put_string(_s, sizeof(_s)-1)
  int put_str(char *str) { return put_string(str, str_len(str)); }
  int put_bytes(void *b, int len) { int r; if ((r = append_bytes(b, len))) return r; return put(); }
  int put();

  void reset_rbuf() { rbuf.reset(); }
  void reset_wbuf() { wbuf.reset(); }
  void check_rbuf(); // move data to top of the buffer

  int error(cchar *errlogmsg = 0);
  virtual int done();
  virtual void free();
  void init(int rbuf_size = DEFAULT_IOBUF_SIZE, int wbuf_size = DEFAULT_IOBUF_SIZE);

  Conn();
};

class ConnFactory { public:
  cchar *name;
  int fd;
  int port;
  int threads;
  int stacksize;
  ThreadPool thread_pool;
  pthread_mutex_t lock;
  FreeList *conn_freelist;

  ConnFactory(cchar *aname);
};

class Server : public ConnFactory { public:
  Server(cchar *aname);
};

void init_conn();
void add_string_buffer(Vec<buffer_t> &bufs, char *str, int len);
void copy_str_to_buf(char *s, char *dest, int *dest_len); // dest_len read for limit and set

// INLINE FUNCTIONS

inline int str_len(char *s) { return *(int*)(s - sizeof(int)); }

inline void Conn::check_rbuf() {
  int l = rbuf.end - rbuf.cur;
  if (l && rbuf.cur != rbuf.buf)
    memmove(rbuf.buf, rbuf.cur, l);
  rbuf.end = rbuf.buf + l;
  rbuf.cur = rbuf.buf;
}

inline void copy_str_to_buf(char *s, char *dest, int *dest_len) {
  int l = str_len(s);
  assert(l < *dest_len);
  *dest_len = l;
  memcpy(dest, s, l);
  dest[l] = 0;
}

#endif
