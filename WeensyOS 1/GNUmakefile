all: mpos.img

# '$(V)' controls whether the lab makefiles print verbose commands (the
# actual shell commands run by Make), as well as the "overview" commands
# (such as '+ cc lib/readline.c').
#
# For overview commands only, run 'make all'.
# For overview and verbose commands, run 'make V=1 all'.
V = 0
ifeq ($(V),1)
VV =
else
VV = @
endif

# This Makefile will automatically use the cross-compiler toolchain
# installed as 'i386-jos-elf-*', if one exists.  If the host tools ('gcc',
# 'objdump', and so forth) compile for a 32-bit x86 ELF target, that will
# be detected as well.  If you have the right compiler toolchain installed
# using a different name, set GCCPREFIX explicitly with
#
#	make 'GCCPREFIX=i386-jos-elf-' [WHATEVER]
#
# or define GCCPREFIX below.

-include conf/rules.mk

# Generic rules for making object files

$(OBJDIR)/%.o: %.c
	@echo + cc $<
	@mkdir -p $(@D)
	$(VV)$(CC) -nostdinc $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.S
	@echo + as $<
	@mkdir -p $(@D)
	$(VV)$(CC) -nostdinc $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%-bootsector: $(OBJDIR)/bootstart.o $(OBJDIR)/%-boot.o
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o $@.out $^
	$(VV)$(OBJDUMP) -S $@.out >$@.asm
	$(VV)$(OBJCOPY) -S -O binary $@.out $@


# miniprocos

MPOS_KERN_FILES = $(OBJDIR)/mpos-int.o $(OBJDIR)/mpos-kern.o \
	$(OBJDIR)/mpos-x86.o $(OBJDIR)/mpos-loader.o \
	$(OBJDIR)/lib.o mpos-symbols.ld

MPOS_BIN_FILES = $(OBJDIR)/mpos-app $(OBJDIR)/mpos-app2

# mpos-kern is linked at address 0x100000.
$(OBJDIR)/mpos-kern: $(MPOS_KERN_FILES) $(MPOS_BIN_FILES)
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -e multiboot_start -Ttext 0x100000 -o $@ $(MPOS_KERN_FILES) -b binary $(MPOS_BIN_FILES)
	$(VV)$(OBJDUMP) -S $@ >$@.asm
	$(VV)$(NM) -n $@ >$@.sym
	$(VV)$(STRIP) -g $@

# mpos-app is linked at address 0x200000.
$(OBJDIR)/mpos-app: $(OBJDIR)/mpos-app.o $(OBJDIR)/lib.o mpos-symbols.ld
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -e start -Ttext 0x200000 -o $@ $^
	$(VV)$(OBJDUMP) -S $@ >$@.asm
	$(VV)$(NM) -n $@ >$@.sym

# So is mpos-app2.
$(OBJDIR)/mpos-app2: $(OBJDIR)/mpos-app2.o $(OBJDIR)/lib.o mpos-symbols.ld
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -e start -Ttext 0x200000 -o $@ $^
	$(VV)$(OBJDUMP) -S $@ >$@.asm
	$(VV)$(NM) -n $@ >$@.sym

mpos.img: mkbootdisk $(OBJDIR)/mpos-bootsector $(OBJDIR)/mpos-kern
	@echo + mk $@
	$(VV)./mkbootdisk $(OBJDIR)/mpos-bootsector \
		$(OBJDIR)/mpos-kern > $@

/boot/weensyos: obj/mpos-kern
	cp obj/mpos-kern /boot/weensyos
