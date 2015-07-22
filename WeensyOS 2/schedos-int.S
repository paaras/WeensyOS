###############################################################################
# INTERRUPT HANDLERS
#
#   This assembly code defines handlers for the software and hardware
#   interrupts understood by weensyos: that is, the clock interrupt and its
#   system calls.
#   It also contains some magic code that allows fancy boot loaders to boot
#   weensyos.
#
#   There is no need to understand this code in detail!
#
###############################################################################

.text

# First, some magic that makes WeensyOS follow the "Multiboot" standard.
# This magic must come first in the binary.

.globl multiboot
multiboot:
	.long 0x1BADB002
	.long 0
	.long -0x1BADB002

# The multiboot_start routine sets the stack pointer to the top of the
# SchedOS's kernel stack, then jumps to the 'start' routine in schedos-kern.c.

.globl multiboot_start
multiboot_start:
	movl $0x180000, %esp
	pushl $0
	popfl
	call start


# Interrupt handlers
.align 2

	.globl clock_int_handler
clock_int_handler:
	pushl $0		// error code
	pushl $32		// trap number
	jmp _generic_int_handler

sys_int48_handler:
	pushl $0
	pushl $48
	jmp _generic_int_handler

sys_int49_handler:
	pushl $0
	pushl $49
	jmp _generic_int_handler

sys_int50_handler:
	pushl $0
	pushl $50
	jmp _generic_int_handler

sys_int51_handler:
	pushl $0
	pushl $51
	jmp _generic_int_handler

sys_int52_handler:
	pushl $0
	pushl $52
	jmp _generic_int_handler

sys_int53_handler:
	pushl $0
	pushl $53
	jmp _generic_int_handler

sys_int54_handler:
	pushl $0
	pushl $54
	jmp _generic_int_handler

sys_int55_handler:
	pushl $0
	pushl $55
	jmp _generic_int_handler

sys_int56_handler:
	pushl $0
	pushl $56
	jmp _generic_int_handler

sys_int57_handler:
	pushl $0
	pushl $57
	jmp _generic_int_handler

	.globl default_int_handler
default_int_handler:
	pushl $0
	jmp _generic_int_handler

_generic_int_handler:
	# When we get here, the processor's interrupt mechanism has
	# pushed the old task status and stack registers onto the kernel stack.
	# Then one of the specific handlers pushed the trap number.
	# Now, we complete the 'registers_t' structure by pushing the extra
	# segment definitions and the general CPU registers.
	pushl %ds
	pushl %es
	pushal

	# Load the kernel's data segments into the extra segment registers
	# (although we don't use those extra segments!).
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es

	# Call the kernel's 'interrupt' function.
	pushl %esp
	call interrupt
	# 'interrupt' should never return.

	# An array of function pointers to the interrupt handlers.
	.globl sys_int_handlers
sys_int_handlers:
	.long sys_int48_handler
	.long sys_int49_handler
	.long sys_int50_handler
	.long sys_int51_handler
	.long sys_int52_handler
	.long sys_int53_handler
	.long sys_int54_handler
	.long sys_int55_handler
	.long sys_int56_handler
	.long sys_int57_handler
