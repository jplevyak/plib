# Makefile for PLIB
# Copyright (C) 2004-2009 John Plevyak, All Rights Reserved

ifndef USE_PLIB
MODULE=plib
DEBUG=1
#OPTIMIZE=1
#PROFILE=1
#USE_GC=1
#LEAK_DETECT=1
#USE_READLINE=1
#USE_EDITLINE=1
#VALGRIND=1

MAJOR=0
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
  CFLAGS += -DHAS_32BIT=1
endif

ifeq ($(OS_TYPE),Darwin)
  AR_FLAGS = crvs
else
  AR_FLAGS = crv
endif

ifeq ($(OS_TYPE),CYGWIN)
#GC_CFLAGS += -L/usr/local/lib
else
GC_CFLAGS += -I/usr/include/gc -I/usr/local/include 
LIBS += -lrt -lpthread 
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
  LIBS += -leditline -ltermcap
endif
ifeq ($(OS_TYPE),CYGWIN)
  CFLAGS += -DUSE_EDITLINE
  LIBS += -leditline -ltermcap
endif
endif

BUILD_VERSION = $(shell svnversion .)
ifeq ($(BUILD_VERSION),exported)
  BUILD_VERSION = $(shell git show-ref 2> /dev/null | head -1 | cut -d ' ' -f 1)
  ifeq ($(BUILD_VERSION),)
    BUILD_VERSION = $(shell cat BUILD_VERSION)
  endif
endif
VERSIONCFLAGS += -DMAJOR_VERSION=$(MAJOR) -DMINOR_VERSION=$(MINOR) -DBUILD_VERSION=\"$(BUILD_VERSION)\"

CFLAGS += -Wall 
# debug flags
ifdef DEBUG
CFLAGS += -ggdb3 -DDEBUG=1
endif
# optimized flags
ifdef OPTIMIZE
CFLAGS += -O3 -march=core2 
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

LIB_SRCS = arg.cc config.cc misc.cc util.cc service.cc list.cc vec.cc map.cc thrpool.cc barrier.cc prime.cc mt19937-64.cc unit.cc log.cc conn.cc md5c.cc dlmalloc.cc persist.cc hash.cc
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

TEST_EXEC = test_$(MODULE)

ALL_SRCS = $(PLIB_SRCS) $(LIB_SRCS)

allplib: $(EXECUTABLES) $(LIBRARY)

version:
	@echo $(MODULE) $(MAJOR).$(MINOR).$(BUILD_VERSION) '('$(OS_TYPE) $(OS_VERSION)')'

version.o: version.cc
	$(CC) $(CFLAGS) $(VERSIONCFLAGS) -c version.cc

$(LIBRARY):  $(LIB_OBJS)
	ar $(AR_FLAGS) $@ $^

$(TEST_PLIB): $(TEST_PLIB_OBJS) $(LIB_OBJS) $(LIBRARIES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

LICENSE.i: LICENSE
	rm -f LICENSE.i
	cat $< | sed s/\"/\\\\\"/g | sed s/\^/\"/g | sed s/$$/\\\\n\"/g > $@

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
	./mkdep $(CFLAGS) $(ALL_SRCS)

plib.o: LICENSE.i COPYRIGHT.i

version.o: Makefile

-include .depend
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

mt19937-64.o: mt19937-64.cc mt64.h
md5c.o: md5c.cc md5.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
