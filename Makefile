# Master makefile of CollabVM Server

ifeq ($(OS), Windows_NT)

# Allow selection of w64 or w32 target

# Is it Cygwin?

ifeq ($(shell uname -s|cut -d _ -f 1), CYGWIN)

$(info Compiling targeting Cygwin)
MKCONFIG=mk/cygwin.mkc
BINDIR=bin/
ARCH=$(shell uname -m)
CYGWIN=1

else

CYGWIN=0

ifneq ($(WARCH), win32)

$(info Compiling targeting Win64)
MKCONFIG=mk/win64.mkc
ARCH=amd64
BINDIR=bin/win64/

else

$(info Compiling targeting x86 Windows)
MKCONFIG=mk/win32.mkc
BINDIR=bin/win32/
ARCH=x86

endif

endif

else

# Is it Termux?
$(shell command -v termux-setup-storage >/dev/null)

ifeq ($(.SHELLSTATUS),0)

TERMUX=1
$(info Compiling targeting Android (Termux))
MKCONFIG=mk/termux.mkc

else

TERMUX=0

# Assume Linux or other *nix-likes
$(info Compiling targeting *nix)
MKCONFIG=mk/linux.mkc

endif

BINDIR=bin/
ARCH=$(shell uname -m)

endif

# Set defaults for DEBUG, JPEG and STATIC builds

ifeq ($(DEBUG),)
DEBUG = 0
endif

ifeq ($(JPEG),)
JPEG = 0
endif

ifeq ($(STATIC),)
JPEG = 0
endif

ifeq ($(DEBUG),1)
$(info Building in debug mode)
else
$(info Building in release mode)
endif

ifeq ($(JPEG),1)
$(info Building JPEG support)
endif

ifeq ($(STATIC),1)
$(info Building as a static binary)
endif

.PHONY: all clean help

all:
	@$(MAKE) -f $(MKCONFIG) DEBUG=$(DEBUG) JPEG=$(JPEG)
	@./scripts/build_site.sh $(ARCH)
	-@ if [ -d "$(BINDIR)/http" ]; then rm -rf $(BINDIR)/http; fi;
	-@mv -f http/ $(BINDIR)
ifeq ($(OS), Windows_NT)

ifeq ($(CYGWIN), 1)

	@./scripts/copy_dlls_cyg.sh $(ARCH) $(BINDIR)

else

	@./scripts/copy_dlls_mw.sh $(ARCH) $(BINDIR)

endif

else

ifeq ($(TERMUX), 1)

	@./scripts/copy_dlls_tmux.sh $(ARCH) $(BINDIR)

endif

endif

clean:
	@$(MAKE) -f $(MKCONFIG) clean

help:
	@echo -e "CollabVM Server 1.2.9 Makefile help:\n"
	@echo "make - Build release"
	@echo "make DEBUG=1 - Build a debug build (Adds extra trace information and debug symbols)"
	@echo "make JPEG=1 - Build with JPEG support (Useful for slower internet connections)"
	@echo "make STATIC=1 - Build a static binary for those that need such"
