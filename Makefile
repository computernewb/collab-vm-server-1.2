# Master makefile of CollabVM Server

ifeq ($(OS), Windows_NT)

# Allow selection of w64 or w32 target

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

else

# Assume Linux or other *nix-likes
$(info Compiling targeting *nix)
MKCONFIG=mk/linux.mkc
BINDIR=bin/
ARCH=$(shell uname -m)

endif

# Set defaults for DEBUG and JPEG builds

ifeq ($(DEBUG),)
DEBUG = 0
endif

ifeq ($(JPEG),)
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

.PHONY: all clean help

all:
	@$(MAKE) -f $(MKCONFIG) DEBUG=$(DEBUG) JPEG=$(JPEG)
	@./scripts/build_site.sh $(ARCH)
	-@ [[ -d "$(BINDIR)/http" ]] && rm -rf $(BINDIR)/http
	-@mv -f http/ $(BINDIR)
ifeq ($(OS), Windows_NT)
	@./scripts/copy_dlls_mw.sh $(ARCH) $(BINDIR)
endif

clean:
	@$(MAKE) -f $(MKCONFIG) clean

help:
	@echo -e "CollabVM Server 1.2.9 Makefile help:\n"
	@echo "make - Build release"
	@echo "make DEBUG=1 - Build a debug build (Adds extra trace information and debug symbols)"
	@echo "make JPEG=1 - Build with JPEG support (Useful for slower internet connections)"
