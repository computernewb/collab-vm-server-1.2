# Proxy Makefile, this picks what makefile to use for each configuration
# E.g on Linux use mk/linux.mk, so on...

ifeq ($(OS), Windows_NT)
$(info [>] Compiling for Windows)

# Allow selection of w64 or w32 target
ifneq ($(WARCH), w32)
$(info [>] Compiling win64.)
DEFMAKE=mk/windows64.mk
ARCH=amd64
BINDIR=bin/windows64/
else
$(info [>] Compiling win32.)
DEFMAKE=mk/windows32.mk
BINDIR=bin/windows32/
ARCH=x86
endif

else
# just go with what we have for now
$(info [>] Compiling for Linux)
DEFMAKE=mk/linux.mk
BINDIR=bin/linux/
ARCH=$(shell uname -m)

endif

all: pre_common
	@$(MAKE) -f $(DEFMAKE)
	@./scripts/build_site.sh $(ARCH)
	@mv -f http/ $(BINDIR)

# common mkdir
pre_common:
	@if [ ! -d "bin" ]; then echo "[>] MKDIR 'bin'" && mkdir bin; fi
	@if [ ! -d "obj" ]; then echo "[>] MKDIR 'obj'" && mkdir obj; fi

clean:
	@$(MAKE) -f $(DEFMAKE) clean
	@echo "[>] RMDIR 'bin/'"
	@rm -rf bin/
	@echo "[>] RMDIR 'obj/'"
	@rm -rf obj/
help:
	@echo -e "CollabVM Server 1.2.7 Makefile help:\n"
	@echo "make - Build release"
