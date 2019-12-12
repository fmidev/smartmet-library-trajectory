MODULE = trajectory
LIB = smartmet-trajectory
SPEC = smartmet-trajectory
INCDIR = smartmet/trajectory

MAINFLAGS = -MD -Wall -W -Wno-unused-parameter

ifeq (6, $(RHEL_VERSION))
  MAINFLAGS += -std=c++0x -fPIC
else
  MAINFLAGS += -std=c++11 -fPIC -fdiagnostics-color=always
endif

# mdsplib does not declare things correctly

MAINFLAGS += -fpermissive


EXTRAFLAGS = \
	-Werror \
	-Winline \
	-Wpointer-arith \
	-Wcast-qual \
	-Wcast-align \
	-Wwrite-strings \
	-Wnon-virtual-dtor \
	-Wno-pmf-conversions \
	-Wsign-promo \
	-Wchar-subscripts \
	-Wredundant-decls \
	-Woverloaded-virtual

DIFFICULTFLAGS = \
	-Wunreachable-code \
	-Wconversion \
	-Wctor-dtor-privacy \
	-Weffc++ \
	-Wold-style-cast \
	-pedantic \
	-Wshadow

CC = g++

# Default compiler flags

DEFINES = -DUNIX -DWGS84

CFLAGS = $(DEFINES) -O2 -DNDEBUG $(MAINFLAGS)
LDFLAGS = 

# Templates

TEMPLATES = $(wildcard tmpl/*.tmpl)
BYTECODES = $(TEMPLATES:%.tmpl=%.c2t)

# What to install

LIBFILE = lib$(LIB).so


# Special modes

CFLAGS_DEBUG = $(DEFINES) -O0 -g $(MAINFLAGS) $(EXTRAFLAGS) -Werror
CFLAGS_PROFILE = $(DEFINES) -O2 -g -pg -DNDEBUG $(MAINFLAGS)

LDFLAGS_DEBUG =
LDFLAGS_PROFILE =

INCLUDES = -I$(includedir) \
	-I$(includedir)/smartmet \
	-I$(includedir)/smartmet/newbase \
	-I$(includedir)/smartmet/smarttools \
	-I$(PREFIX)/gdal30/include

LIBS = -L$(libdir) \
	-lsmartmet-smarttools \
	-lsmartmet-newbase \
	-lsmartmet-locus \
	-lsmartmet-macgyver \
	-lboost_date_time \
	-lboost_filesystem \
	-lboost_iostreams \
	-lboost_locale \
	-lboost_program_options \
	-lboost_regex \
	-lboost_thread \
	-lboost_system \
	-L$(PREFIX)/gdal30/lib -lgdal \
	-lctpp2 \
	-lpqxx \
	-lrt

# Common library compiling template

# Installation directories

processor := $(shell uname -p)

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

ifeq ($(processor), x86_64)
  libdir = $(PREFIX)/lib64
else
  libdir = $(PREFIX)/lib
endif

objdir = obj
includedir = $(PREFIX)/include

ifeq ($(origin BINDIR), undefined)
  bindir = $(PREFIX)/bin
else
  bindir = $(BINDIR)
endif

ifeq ($(origin DATADIR), undefined)
  datadir = $(PREFIX)/share
else
  datadir = $(DATADIR)
endif

# Special modes

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_DEBUG)
  LDFLAGS = $(LDFLAGS_DEBUG)
endif

ifneq (,$(findstring profile,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_PROFILE)
  LDFLAGS = $(LDFLAGS_PROFILE)
endif

# Compilation directories

vpath %.cpp trajectory main
vpath %.h trajectory
vpath %.o $(objdir)
vpath %.d $(objdir)

# How to install

INSTALL_PROG = install -m 775
INSTALL_DATA = install -m 664

# The files to be compiled

HDRS = $(patsubst trajectory/%,%,$(wildcard *.h trajectory/*.h))

MAINSRCS     = $(patsubst main/%,%,$(wildcard main/*.cpp))
MAINPROGS    = $(MAINSRCS:%.cpp=%)
MAINOBJS     = $(MAINSRCS:%.cpp=%.o)
MAINOBJFILES = $(MAINOBJS:%.o=obj/%.o)

SRCS     = $(patsubst trajectory/%,%,$(wildcard trajectory/*.cpp))
OBJS     = $(SRCS:%.cpp=%.o)
OBJFILES = $(OBJS:%.o=obj/%.o)

INCLUDES := -Itrajectory $(INCLUDES)

# For make depend:

ALLSRCS = $(wildcard main/*.cpp source/*.cpp)

.PHONY: test rpm

# The rules

all: objdir $(BYTECODES) $(MAINPROGS) $(LIBFILE)
debug: objdir $(BYTECODES) $(MAINPROGS) $(LIBFILE)
release: objdir $(BYTECODES) $(MAINPROGS) $(LIBFILE)
profile: objdir $(BYTECODES) $(MAINPROGS) $(LIBFILE)

.SECONDEXPANSION:
$(MAINPROGS): % : obj/%.o $(OBJFILES)
	$(CC) $(LDFLAGS) -o $@ obj/$@.o $(OBJFILES) $(LIBS)

clean:
	rm -f $(MAINPROGS) source/*~ include/*~
	rm -rf obj

format:
	clang-format -i -style=file include/*.h source/*.cpp main/*.cpp

install:
	mkdir -p $(includedir)/$(INCDIR)
	@list=`ls -1 trajectory | grep '\.h'`; \
	for hdr in $$list; do \
	  echo $(INSTALL_DATA) trajectory/$$hdr $(includedir)/$(INCDIR)/$$hdr; \
	  $(INSTALL_DATA) trajectory/$$hdr $(includedir)/$(INCDIR)/$$hdr; \
	done
	@mkdir -p $(libdir)
	$(INSTALL_PROG) $(LIBFILE) $(libdir)/$(LIBFILE)
	mkdir -p $(bindir)
	@list='$(MAINPROGS)'; \
	for prog in $$list; do \
	  echo $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	  $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	done
	mkdir -p $(datadir)/smartmet/$(MODULE)
	@list=`ls -1 tmpl/*.c2t`; \
	echo $(INSTALL_DATA) $$list $(datadir)/smartmet/$(MODULE)/; \
	$(INSTALL_DATA) $$list $(datadir)/smartmet/$(MODULE)/

test:
	cd test && make test

objdir:
	@mkdir -p $(objdir)

$(LIBFILE): $(OBJFILES)
	$(CXX) $(CFLAGS) -shared -rdynamic -o $(LIBFILE) $(OBJFILES) $(LIBS)

rpm: clean $(SPEC).spec
	rm -f $(SPEC).tar.gz # Clean a possible leftover from previous attempt
	tar -czvf $(SPEC).tar.gz --transform "s,^,$(SPEC)/," *
	rpmbuild -ta $(SPEC).tar.gz
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o : %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.c2t: %.tmpl
	ctpp2c $< $@

-include obj/*.d
