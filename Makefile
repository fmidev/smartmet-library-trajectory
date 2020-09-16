MODULE = trajectory
LIB = smartmet-trajectory
SPEC = smartmet-trajectory
INCDIR = smartmet/trajectory

MAINFLAGS = -MD -Wall -W -Wno-unused-parameter

# Compiler options                                                                                                                                           

-include $(HOME)/.smartmet.mk
GCC_DIAG_COLOR ?= always
CXX_STD ?= c++11

MAINFLAGS = -fPIC -std=$(CXX_STD) -fdiagnostics-color=$(GCC_DIAG_COLOR)

EXTRAFLAGS = \
	-Werror \
	-Winline \
	-Wpointer-arith \
	-Wcast-qual \
	-Wcast-align \
	-Wwrite-strings \
	-Wno-pmf-conversions \
	-Wsign-promo \
	-Wchar-subscripts

DIFFICULTFLAGS = \
	-Wunreachable-code \
	-Wconversion \
	-Wctor-dtor-privacy \
	-Weffc++ \
	-Wold-style-cast \
	-pedantic \
	-Wshadow

# Default compiler flags

DEFINES = -DUNIX

CFLAGS = $(DEFINES) -O2 -DNDEBUG $(MAINFLAGS) -g
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

# Boost 1.69

ifneq "$(wildcard /usr/include/boost169)" ""
  INCLUDES += -isystem /usr/include/boost169
  LIBS += -L/usr/lib64/boost169
endif

ifneq "$(wildcard /usr/gdal30/include)" ""
  INCLUDES += -isystem /usr/gdal30/include
  LIBS += -L$(PREFIX)/gdal30/lib
else
  INCLUDES += -isystem /usr/include/gdal
endif

INCLUDES += \
	-I$(includedir)/smartmet \
	-I$(includedir)/smartmet/newbase \
	-I$(includedir)/smartmet/smarttools \

LIBS += -L$(libdir) \
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
	-lgdal \
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
	$(CXX) $(LDFLAGS) -o $@ obj/$@.o $(OBJFILES) $(LIBS)

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
	tar -czvf $(SPEC).tar.gz --exclude test --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -ta $(SPEC).tar.gz
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.c2t: %.tmpl
	ctpp2c $< $@

-include obj/*.d
