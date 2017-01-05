SUBNAME = trajectory
LIB = smartmet-$(SUBNAME)
SPEC = smartmet-$(SUBNAME)
INCDIR = smartmet/$(SUBNAME)
PROG = qdtrajectory

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

#
# To build serially (helps get the error messages right): make debug SCONS_FLAGS=""
#
SCONS_FLAGS=-j 4

# Installation directories

processor := $(shell uname -p)

ifeq ($(processor), x86_64)
  libdir = $(PREFIX)/lib64
else
  libdir = $(PREFIX)/lib
endif

bindir = $(PREFIX)/bin
includedir = $(PREFIX)/include
datadir = $(PREFIX)/share
objdir = obj

# rpm variables

rpmsourcedir=/tmp/$(shell whoami)/rpmbuild

rpmexcludevcs := $(shell tar --help | grep -m 1 -o -- '--exclude-vcs')

# Templates

TEMPLATES = $(wildcard tmpl/*.tmpl)
BYTECODES = $(TEMPLATES:%.tmpl=%.c2t)

# What to install

LIBFILE = lib$(LIB).so

# How to install

INSTALL_PROG = install -m 775
INSTALL_DATA = install -m 664

.PHONY: test rpm

#
# The rules
#
SCONS_FLAGS += objdir=$(objdir) prefix=$(PREFIX)

all release: $(BYTECODES)
	scons $(SCONS_FLAGS) $(LIBFILE) $(PROG)

debug:  $(BYTECODES)
	scons $(SCONS_FLAGS) debug=1 $(LIBFILE) $(PROG)

profile: $(BYTECODES)
	scons $(SCONS_FLAGS) profile=1 $(LIBFILE) $(PROG)

clean:
	@#scons -c objdir=$(objdir)
	-rm -f $(LIBFILE) *~ source/*~ include/*~ main/*~ main/*.o $(PROG) .sconsign.dblite tmpl/*.c2t
	-rm -rf $(objdir)

format:
	clang-format -i -style=file include/*.h source/*.cpp

install:
	mkdir -p $(includedir)/$(INCDIR)
	@list=`cd include && ls -1 *.h`; \
	for hdr in $$list; do \
	  echo $(INSTALL_DATA) include/$$hdr $(includedir)/$(INCDIR)/$$hdr; \
	  $(INSTALL_DATA) include/$$hdr $(includedir)/$(INCDIR)/$$hdr; \
	done
	@mkdir -p $(libdir)
	$(INSTALL_PROG) $(LIBFILE) $(libdir)/$(LIBFILE)
	mkdir -p $(bindir)
	@list='$(PROG)'; \
	for prog in $$list; do \
	  echo $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	  $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	done
	mkdir -p $(datadir)/smartmet/$(SUBNAME)
	@list=`ls -1 tmpl/*.c2t`; \
	echo $(INSTALL_DATA) $$list $(datadir)/smartmet/$(SUBNAME)/; \
	$(INSTALL_DATA) $$list $(datadir)/smartmet/$(SUBNAME)/

test:
	cd test && make test

rpm: clean
	if [ -e $(SPEC).spec ]; \
	then \
	  mkdir -p $(rpmsourcedir) ; \
	  tar $(rpmexcludevcs) -C ../ -cf $(rpmsourcedir)/$(SPEC).tar $(SUBNAME) ; \
	  gzip -f $(rpmsourcedir)/$(SPEC).tar ; \
	  TAR_OPTIONS=--wildcards rpmbuild -ta $(rpmsourcedir)/$(SPEC).tar.gz ; \
	  rm -f $(rpmsourcedir)/$(SPEC).tar.gz ; \
	else \
	  echo $(SPEC).spec file missing; \
	fi;

cppcheck:
	cppcheck -DUNIX -I include -I $(includedir) source

headertest:
	@echo "Checking self-sufficiency of each header:"
	@echo
	@for hdr in `cd include && ls -1 *.h`; do \
	echo $$hdr; \
	echo "#include \"$$hdr\"" > /tmp/$(LIB).cpp; \
	echo "int main() { return 0; }" >> /tmp/$(LIB).cpp; \
	$(CC) $(CFLAGS) $(INCLUDES) -o /dev/null /tmp/$(LIB).cpp $(LIBS); \
	done

%.c2t: %.tmpl
	ctpp2c $< $@

