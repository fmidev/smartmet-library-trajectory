MODULE = trajectory
LIB = smartmet-trajectory
SPEC = smartmet-trajectory
INCDIR = smartmet/trajectory

REQUIRES = gdal

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc
# Default compiler flags

DEFINES = -DUNIX

# Templates

TEMPLATES = $(wildcard tmpl/*.tmpl)
BYTECODES = $(TEMPLATES:%.tmpl=%.c2t)

# What to install

LIBFILE = lib$(LIB).so


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
	-lctpp2 \
	-lpqxx \
	$(REQUIRED_LIBS) \
	-lrt -lstdc++ -lm

# Common library compiling template
# Compilation directories

vpath %.cpp trajectory main
vpath %.h trajectory
vpath %.o $(objdir)
vpath %.d $(objdir)

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
	tar -czvf $(SPEC).tar.gz --exclude test --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -ta $(SPEC).tar.gz
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.c2t: %.tmpl
	ctpp2c $< $@

-include obj/*.d
