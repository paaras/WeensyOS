OBJDIR	= obj

# Cross-compiler toolchain
CC	= $(GCCPREFIX)gcc
CXX	= $(GCCPREFIX)c++
AS	= $(GCCPREFIX)as
AR	= $(GCCPREFIX)ar
LD	= $(GCCPREFIX)ld
OBJCOPY	= $(GCCPREFIX)objcopy
OBJDUMP	= $(GCCPREFIX)objdump
NM	= $(GCCPREFIX)nm
STRIP	= $(GCCPREFIX)strip

# Native commands
HOSTCC	= gcc
TAR	= tar
PERL	= perl

# Check for i386-jos-elf compiler
ifndef GCCPREFIX
GCCPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1 && i386-jos-elf-gcc -E -x c /dev/null >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your GCCPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake GCCPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# Compiler flags
# -Os is required for the boot loader to fit within 512 bytes;
# -ffreestanding means there is no standard library.
CFLAGS	:= $(CFLAGS) $(DEFS) $(LABDEFS) -Os -ffreestanding -fomit-frame-pointer -I. -MD -Wall -Wno-format -Wno-unused -Werror -ggdb -nostdinc
# Include -fno-stack-protector if the option exists.
CFLAGS	+= $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# Include -m32 if the option exists (x86_64).
CFLAGS	+= $(shell $(CC) -m32 -E -x -c /dev/null >/dev/null 2>&1 && echo -m32)
ifdef SOL
CFLAGS	+= -DSOL=$(SOL)
endif

# Linker flags
LDFLAGS	:= $(LDFLAGS)
# Link for 32-bit targets if on x86_64.
LDFLAGS	+= $(shell $(LD) -m elf_i386 --help >/dev/null 2>&1 && echo -m elf_i386)
# LDFLAGS += $(shell $(LD) -b elf32-i386 --help >/dev/null 2>&1 && echo -b elf32-i386)


# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(wildcard $(OBJDIR)/*.d)
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps
-include conf/date.mk


# The mkbootdisk program
mkbootdisk: mkbootdisk.c
	@echo + hostcc mkbootdisk.c
	$(VV)$(HOSTCC) mkbootdisk.c -o mkbootdisk


# For deleting the build
clean:
	@echo + clean
	$(VV)rm -rf $(OBJDIR) *.img mkbootdisk bochs.log core *.core

realclean: clean
	$(VV)rm -rf $(DISTDIR)-$(USER).tar.gz $(DISTDIR)-$(USER)

distclean: realclean


# Distribute weensyos
DISTDIR = weensyos1

distdir:
	@/bin/rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	perl mklab.pl 1 0 $(DISTDIR) .bochsrc COPYRIGHT GNUmakefile bootstart.S elf.h mergedep.pl mkbootdisk.c mpos-app.c mpos-app.h mpos-app2.c lib.c lib.h mpos-boot.c mpos-kern.c mpos-kern.h mpos-loader.c mpos-symbols.ld mpos-int.S mpos-x86.c mpos.h types.h x86.h answers.txt conf/rules.mk
	echo >$(DISTDIR)/conf/date.mk "PACKAGEDATE="`date`

dist: distdir
	$(TAR) czf $(DISTDIR).tar.gz $(DISTDIR)
	/bin/rm -rf $(DISTDIR)

tarball: realclean
	@echo + mk $(DISTDIR)-$(USER).tar.gz
	$(VV)mkdir $(DISTDIR)-$(USER)
	$(VV)tar cf - `ls -a | grep -v '^\.*$$' | grep -v '^$(DISTDIR)*'` | (cd $(DISTDIR)-$(USER) && tar xf -)
	$(VV)/bin/rm -rf `find $(DISTDIR)-$(USER) \( -name CVS -o -name .svn -o -name .git \) -print`
	$(VV)date > $(DISTDIR)-$(USER)/tarballstamp
	$(VV)tar cf $(DISTDIR)-$(USER).tar $(DISTDIR)-$(USER)
	$(VV)gzip $(DISTDIR)-$(USER).tar
	$(VV)/bin/rm -rf $(DISTDIR)-$(USER)

bochsrc-%:
	$(VV)/bin/mv .bochsrc .bochsrc~
	$(VV)i=`echo $@ | sed s/bochsrc-//`; sed 's/path="[^"]*"/path="'$$i'.img"/' <.bochsrc~ >.bochsrc
	$(VV)/bin/rm .bochsrc~

run-%: %.img bochsrc-%
	@echo + bochs $<
	$(VV)bochs -q

run: bochsrc-mpos
	@echo + make
	$(VV)$(MAKE) clean
	$(VV)$(MAKE) mpos.img
	$(VV)bochs -q


# Patch targets
# Create a patch from ../$(DISTDIR).tar.gz.
patch-extract-tarball:
	@test -r ../$(DISTDIR).tar.gz || (echo "***" 1>&2; \
	echo "*** Can't find '../$(DISTDIR).tar.gz'.  Download it" 1>&2; \
	echo "*** into my parent directory and try again." 1>&2; \
	echo "***" 1>&2; false)
	@(gzcat ../$(DISTDIR).tar.gz 2>/dev/null || zcat ../$(DISTDIR).tar.gz) | tar xf -

patch-check-date: patch-extract-tarball
	@pkgdate=`grep PACKAGEDATE $(DISTDIR)/conf/date.mk | sed 's/.*=//'`; \
	test "$(PACKAGEDATE)" = "$$pkgdate" || (echo "***" 1>&2; \
	echo "*** The ../$(DISTDIR).tar.gz tarball was created on $$pkgdate," 1>&2; \
	echo "*** but your work directory was expanded from a tarball created" 1>&2; \
	echo "*** on $(PACKAGEDATE)!  I can't tell the difference" 1>&2; \
	echo "*** between your changes and the changes between the tarballs," 1>&2; \
	echo "*** so I won't create an automatic patch." 1>&2; \
	echo "***" 1>&2; false)

patch.diff: patch-extract-tarball always
	@rm -f patch.diff
	@for f in `cd $(DISTDIR) && find . -type f -print`; do \
	if diff -u $(DISTDIR)/"$$f" "$$f" >patch.diffpart || [ "$$f" = ./boot/lab.mk ]; then :; else \
	echo "*** $$f differs; appending to patch.diff" 1>&2; \
	echo diff -u $(DISTDIR)/"$$f" "$$f" >>patch.diff; \
	cat patch.diffpart >>patch.diff; \
	fi; done
	@for f in `find . -name $(DISTDIR) -prune -o -name obj -prune -o -name "patch.diff*" -prune -o -name '*.rej' -prune -o -name '*.orig' -prune -o -type f -print`; do \
	if [ '(' '!' -f $(DISTDIR)/"$$f" ')' -a '(' "$$f" != ./kern/appkernbin.c ')' ]; then \
	echo "*** $$f is new; appending to patch.diff" 1>&2; \
	echo New file: "$$f" >>patch.diff; \
	echo diff -u $(DISTDIR)/"$$f" "$$f" >>patch.diff; \
	echo '--- $(DISTDIR)/'"$$f"'	Thu Jan 01 00:00:00 1970' >>patch.diff; \
	diff -u /dev/null "$$f" | tail +2 >>patch.diff; \
	fi; done
	@test -n patch.diff || echo "*** No differences found" 1>&2
	@rm -rf $(DISTDIR) patch.diffpart

diff: patch-check-date patch.diff

patch:
	@test -r patch.diff || (echo "***" 1>&2; \
	echo "*** No 'patch.diff' file found!  Did you remember to" 1>&2; \
	echo "*** run 'make diff'?" 1>&2; \
	echo "***" 1>&2; false)
	@x=`grep "^New file:" patch.diff | head -n 1 | sed 's/New file: //'`; \
	if test -n "$$x" -a -f "$$x"; then \
	echo "*** Note: File '$$x' found in current directory;" 1>&2; \
	echo "*** not applying new files portion of patch.diff." 1>&2; \
	echo "awk '/^New file:/ { exit } { print }' <patch.diff | patch -p0"; \
	awk '/^New file:/ { exit } { print }' <patch.diff | patch -p0; \
	else echo 'patch -p0 <patch.diff'; \
	patch -p0 <patch.diff; \
	fi


# Always build something
always:
	@:

# These are phony targets
.PHONY: all always clean realclean distclean dist distdir tarball handin \
	diff patch patch-extract-tarball patch-check-date patch-make-diff

# Eliminate default suffix rules
.SUFFIXES:

# Delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# no intermediate .o files should be deleted
.PRECIOUS: %.o $(OBJDIR)/%.o $(OBJDIR)/%-bootsector
