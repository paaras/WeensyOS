all: schedos.img

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

SCHEDOS_KERN_FILES = $(OBJDIR)/schedos-int.o $(OBJDIR)/schedos-kern.o \
	$(OBJDIR)/schedos-x86.o $(OBJDIR)/schedos-loader.o \
	$(OBJDIR)/lib.o schedos-symbols.ld

SCHEDOS_BIN_FILES = $(OBJDIR)/schedos-1 $(OBJDIR)/schedos-2 \
	$(OBJDIR)/schedos-3 $(OBJDIR)/schedos-4

# schedos-kern is linked at address 0x100000.
$(OBJDIR)/schedos-kern: $(SCHEDOS_KERN_FILES) $(SCHEDOS_BIN_FILES)
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -e multiboot_start -Ttext 0x100000 -o $@ $(SCHEDOS_KERN_FILES) -b binary $(SCHEDOS_BIN_FILES)
	$(VV)$(OBJDUMP) -S $@ >$@.asm
	$(VV)$(NM) -n $@ >$@.sym
	$(VV)$(STRIP) -g $@

# schedos-1 is linked at address 0x200000.
$(OBJDIR)/schedos-1: $(OBJDIR)/schedos-1.o schedos-symbols.ld
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -e start -Ttext 0x200000 -Tdata 0x210000 -o $@ $^
	$(VV)$(OBJDUMP) -S $@ >$@.asm
	$(VV)$(NM) -n $@ >$@.sym

# schedos-2 is linked at address 0x300000.
$(OBJDIR)/schedos-2: $(OBJDIR)/schedos-2.o schedos-symbols.ld
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -e start -Ttext 0x300000 -Tdata 0x310000 -o $@ $^
	$(VV)$(OBJDUMP) -S $@ >$@.asm
	$(VV)$(NM) -n $@ >$@.sym

# schedos-3 is linked at address 0x400000.
$(OBJDIR)/schedos-3: $(OBJDIR)/schedos-3.o schedos-symbols.ld
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -e start -Ttext 0x400000 -Tdata 0x410000 -o $@ $^
	$(VV)$(OBJDUMP) -S $@ >$@.asm
	$(VV)$(NM) -n $@ >$@.sym

# schedos-4 is linked at address 0x500000.
$(OBJDIR)/schedos-4: $(OBJDIR)/schedos-4.o schedos-symbols.ld
	@echo + ld $(@F)
	$(VV)$(LD) $(LDFLAGS) -e start -Ttext 0x500000 -Tdata 0x510000 -o $@ $^
	$(VV)$(OBJDUMP) -S $@ >$@.asm
	$(VV)$(NM) -n $@ >$@.sym

schedos.img: mkbootdisk $(OBJDIR)/schedos-bootsector $(OBJDIR)/schedos-kern
	@echo + mk $@
	$(VV)./mkbootdisk $(OBJDIR)/schedos-bootsector \
		$(OBJDIR)/schedos-kern > $@

DEPFILES := $(wildcard $(OBJDIR)/*.d)
ifneq ($(DEPFILES),)
-include $(DEPFILES)
endif

/boot/weensyos: obj/schedos-kern
	cp obj/schedos-kern /boot/weensyos
