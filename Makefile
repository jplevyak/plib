# Makefile for PLIB
# Copyright (C) 2004-2012 John Plevyak, All Rights Reserved

ifndef USE_PLIB
MODULE=plib
#DEBUG=1
OPTIMIZE=1
#PROFILE=1
#USE_GC=1
#LEAK_DETECT=1
#USE_READLINE=1
#USE_EDITLINE=1
#VALGRIND=1

MAJOR=1
MINOR=0
endif

CC = g++

ifndef PREFIX
PREFIX=/usr/local
endif

.PHONY: all install

OS_TYPE = $(shell uname -s | \
  awk '{ split($$1,a,"_"); printf("%s", a[1]);  }')
OS_VERSION = $(shell uname -r | \
  awk '{ split($$1,a,"."); sub("V","",a[1]); \
  printf("%d%d%d",a[1],a[2],a[3]); }')
ARCH = $(shell uname -m)
ifeq ($(ARCH),i386)
  ARCH = x86
endif
ifeq ($(ARCH),i486)
  ARCH = x86
endif
ifeq ($(ARCH),i586)
  ARCH = x86
endif
ifeq ($(ARCH),i686)
  ARCH = x86
endif

ifeq ($(ARCH),x86)
ifneq ($(OS_TYPE),Darwin)
# Darwin lies
  CFLAGS += -DHAS_32BIT=1
endif
endif

ifeq ($(OS_TYPE),Darwin)
  AR_FLAGS = crvs
else
  AR_FLAGS = crv
endif

ifeq ($(OS_TYPE),CYGWIN)
#GC_CFLAGS += -L/usr/local/lib
else
ifeq ($(OS_TYPE),Darwin)
GC_CFLAGS += -I/usr/local/include
else
GC_CFLAGS += -I/usr/local/include 
LIBS += -lrt -lpthread 
endif
endif

ifdef USE_GC
CFLAGS += -DUSE_GC ${GC_CFLAGS}
LIBS += -lgc
endif
ifdef LEAK_DETECT
CFLAGS += -DLEAK_DETECT  ${GC_CFLAGS}
LIBS += -lleak
endif

ifdef USE_READLINE
ifeq ($(OS_TYPE),Linux)
  CFLAGS += -DUSE_READLINE
  LIBS += -lreadline
endif
ifeq ($(OS_TYPE),CYGWIN)
  CFLAGS += -DUSE_READLINE
  LIBS += -lreadline
endif
endif
ifdef USE_EDITLINE
ifeq ($(OS_TYPE),Linux)
  CFLAGS += -DUSE_EDITLINE
  LIBS += -leditline
endif
ifeq ($(OS_TYPE),CYGWIN)
  CFLAGS += -DUSE_EDITLINE
  LIBS += -ledit -ltermcap
endif
endif

BUILD_VERSION = $(shell git show-ref 2> /dev/null | head -1 | cut -d ' ' -f 1)
ifeq ($(BUILD_VERSION),)
  BUILD_VERSION = $(shell cat BUILD_VERSION)
endif
VERSIONCFLAGS += -DMAJOR_VERSION=$(MAJOR) -DMINOR_VERSION=$(MINOR) -DBUILD_VERSION=\"$(BUILD_VERSION)\"

CFLAGS += -std=c++11 -Wall -Wno-strict-aliasing
# debug flags
ifdef DEBUG
CFLAGS += -g -DDEBUG=1
endif
# optimized flags
ifdef OPTIMIZE
CFLAGS += -O3 -march=native
endif
ifdef PROFILE
CFLAGS += -pg
endif
ifdef VALGRIND
CFLAGS += -DVALGRIND_TEST
endif

CPPFLAGS += $(CFLAGS)

LIBS += -lm

AUX_FILES = $(MODULE)/Makefile $(MODULE)/LICENSE $(MODULE)/README
TAR_FILES = $(AUX_FILES) $(TEST_FILES) $(MODULE)/BUILD_VERSION 

LIB_SRCS = arg.cc config.cc stat.cc misc.cc util.cc service.cc list.cc vec.cc map.cc threadpool.cc barrier.cc prime.cc mt19937-64.cc unit.cc log.cc conn.cc md5c.cc dlmalloc.cc persist.cc hash.cc
LIB_OBJS = $(LIB_SRCS:%.cc=%.o)

TEST_PLIB_SRCS = plib.cc
TEST_PLIB_OBJS = $(TEST_PLIB_SRCS:%.cc=%.o)

EXECUTABLE_FILES =
ifdef USE_GC
LIBRARY = libplib_gc.a
else
LIBRARY = libplib.a
endif
INSTALL_LIBRARIES = plib.a
INCLUDES = 
#MANPAGES = plib.1

ifeq ($(OS_TYPE),CYGWIN)
EXECUTABLES = $(EXECUTABLE_FILES:%=%.exe)
TEST_PLIB = test_plib.exe
else
EXECUTABLES = $(EXECUTABLE_FILES)
TEST_PLIB = test_plib
endif

ifndef TEST_EXEC
TEST_EXEC = test_$(MODULE)
endif

ALL_SRCS = $(PLIB_SRCS) $(LIB_SRCS) $(TEST_PLIB_SRCS)
DEPEND_SRCS = $(ALL_SRCS)

allplib: $(EXECUTABLES) $(LIBRARY)

version:
	@echo $(MODULE) $(MAJOR).$(MINOR).$(BUILD_VERSION) '('$(OS_TYPE) $(OS_VERSION)')'

version.o: version.cc
	$(CC) $(CFLAGS) $(VERSIONCFLAGS) -c version.cc

$(LIBRARY):  $(LIB_OBJS)
	ar $(AR_FLAGS) $@ $^

$(TEST_PLIB): $(TEST_PLIB_OBJS) $(LIB_SRCS) $(LIBRARIES)
	$(CC) $(CFLAGS) -DTEST_LIB=1 $(TEST_PLIB_OBJS) $(LDFLAGS) $(LIB_SRCS) -o $@ $(LIBS)

LICENSE.i: LICENSE
	rm -f LICENSE.i
	cat $< | sed s/\"/\\\\\"/g | sed s/\^/\"/g | sed s/$$/\\\\n\"/g | sed 's/%/%%/g' > $@

COPYRIGHT.i: LICENSE
	rm -f COPYRIGHT.i
	head -1 LICENSE | sed s/\"/\\\\\"/g | sed s/\^/\"/g | sed s/$$/\\\\n\"/g > $@

tar:
	echo $(BUILD_VERSION) > BUILD_VERSION
	(cd ..;tar czf $(MODULE).tar.gz $(MODULE)/*.cc $(MODULE)/*.h $(TAR_FILES)) 

bintar:
	(cd ..;tar czf $(MODULE)-$(RELEASE)-$(OS_TYPE)-bin.tar.gz $(AUX_FILES) $(LIBRARY:%=$(MODULE)/%) $(INCLUDES:%=$(MODULE)/%) $(EXECUTABLES:%=$(MODULE)/%))

test: $(TEST_EXEC)
	$(TEST_EXEC)

clean:
	\rm -f *.o core *.core *.gmon $(EXEC_FILES) LICENSE.i COPYRIGHT.i $(EXECUTABLES) $(CLEAN_FILES) $(TEST_PLIB)

realclean: clean
	\rm -f *.a *.orig *.rej svn-commit.tmp

depend:
	./mkdep $(CFLAGS) $(DEPEND_SRCS)

plib.o: LICENSE.i COPYRIGHT.i

version.o: Makefile

-include .depend
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

arg.o: arg.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
config.o: config.cc plib.h tls.h arg.h barrier.h config.h stat.h \
  dlmalloc.h freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h \
  misc.h util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h \
  timer.h unit.h
stat.o: stat.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
misc.o: misc.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
util.o: util.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
service.o: service.cc plib.h tls.h arg.h barrier.h config.h stat.h \
  dlmalloc.h freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h \
  misc.h util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h \
  timer.h unit.h
list.o: list.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
vec.o: vec.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
map.o: map.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
threadpool.o: threadpool.cc plib.h tls.h arg.h barrier.h config.h stat.h \
  dlmalloc.h freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h \
  misc.h util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h \
  timer.h unit.h
barrier.o: barrier.cc barrier.h
prime.o: prime.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
mt19937-64.o: mt19937-64.cc mt64.h
unit.o: unit.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
log.o: log.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
conn.o: conn.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
md5c.o: md5c.cc md5.h
dlmalloc.o: dlmalloc.cc plib.h tls.h arg.h barrier.h config.h stat.h \
  dlmalloc.h freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h \
  misc.h util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h \
  timer.h unit.h
persist.o: persist.cc plib.h tls.h arg.h barrier.h config.h stat.h \
  dlmalloc.h freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h \
  misc.h util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h \
  timer.h unit.h
hash.o: hash.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h
plib.o: plib.cc plib.h tls.h arg.h barrier.h config.h stat.h dlmalloc.h \
  freelist.h defalloc.h list.h log.h vec.h map.h threadpool.h misc.h \
  util.h conn.h md5.h mt64.h hash.h persist.h prime.h service.h timer.h \
  unit.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
