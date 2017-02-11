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
SCONS_FLAGS=-j 8

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
	-rm -f $(LIBFILE) *~ $(SUBNAME)/*~ main/*~ main/*.o $(PROG) .sconsign.dblite tmpl/*.c2t
	-rm -rf $(objdir)

format:
	clang-format -i -style=file $(SUBNAME)/*.h $(SUBNAME)/*.cpp

install:
	mkdir -p $(includedir)/$(INCDIR)
	@list=`cd trajectory && ls -1 *.h`; \
	for hdr in $$list; do \
	  echo $(INSTALL_DATA) trajectory/$$hdr $(includedir)/$(INCDIR)/$$hdr; \
	  $(INSTALL_DATA) trajectory/$$hdr $(includedir)/$(INCDIR)/$$hdr; \
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
	  tar -czvf $(SPEC).tar.gz --transform "s,^,$(SPEC)/," * ; \
	  rpmbuild -ta $(SPEC).tar.gz ; \
	  rm -f $(SPEC).tar.gz ; \
	else \
	  echo $(SPEC).spec file missing; \
	fi;

%.c2t: %.tmpl
	ctpp2c $< $@

