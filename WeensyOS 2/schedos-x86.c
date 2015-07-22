#include "schedos-kern.h"
#include "x86.h"
#include "lib.h"


/*****************************************************************************
 * schedos-x86.c
 *
 *   This file contains x86-specific code for setting up processor state,
 *   especially interrupts.
 *
 *   YOU DO NOT NEED TO UNDERSTAND THIS!
 *
 *****************************************************************************/


/*****************************************************************************
 * segments_init
 *
 *   Set up the segment registers and interrupt descriptor table.
 *
 *   The segment registers distinguish the kernel from applications:
 *   the kernel runs with segments SEGSEL_KERN_CODE and SEGSEL_KERN_DATA,
 *   and applications with SEGSEL_APP_CODE and SEGSEL_APP_DATA.
 *   The kernel segment runs with full privilege (level 0), but application
 *   segments run with less privilege (level 3).
 *   However, both kernel and applications can access all parts of machine
 *   memory!  So there's no memory protection yet.
 *
 *   The interrupt descriptor table tells the processor where to jump
 *   when an interrupt or exception happens.
 *   In schedos, it should jump to the assembly code in 'schedos-int.S'.
 *
 *   The taskstate_t, segmentdescriptor_t, and pseduodescriptor_t types
 *   are defined by the x86 hardware.
 *
 *****************************************************************************/

// Segment selectors
#define SEGSEL_KERN_CODE	0x8		// kernel code segment
#define SEGSEL_KERN_DATA	0x10		// kernel data segment
#define SEGSEL_APP_CODE		0x18		// application code segment
#define SEGSEL_APP_DATA		0x20		// application data segment
#define SEGSEL_TASKSTATE	0x28		// task state segment

// The task descriptor defines the state the processor should set up
// when taking an interrupt.
static taskstate_t kernel_task_descriptor;

// Segments
static segmentdescriptor_t segments[] = {
	SEG_NULL,				// ignored
	SEG(STA_X | STA_R, 0, 0xFFFFFFFF, 0),	// SEGSEL_KERN_CODE
	SEG(STA_W, 0, 0xFFFFFFFF, 0),		// SEGSEL_KERN_DATA
	SEG(STA_X | STA_R, 0, 0xFFFFFFFF, 3),	// SEGSEL_APP_CODE
	SEG(STA_W, 0, 0xFFFFFFFF, 3),		// SEGSEL_APP_DATA
	SEG_NULL /* defined below */		// SEGSEL_TASKSTATE
};
pseudodescriptor_t global_descriptor_table = {
	sizeof(segments) - 1,
	(uintptr_t) segments
};

// Interrupt descriptors
static gatedescriptor_t interrupt_descriptors[256];	/* initialized below */
pseudodescriptor_t interrupt_descriptor_table = {
	sizeof(interrupt_descriptors) - 1,
	(uintptr_t) interrupt_descriptors
};

// Particular interrupt handler routines
extern void clock_int_handler(void);
extern void (*sys_int_handlers[])(void);
extern void default_int_handler(void);


void
segments_init(void)
{
	int i;

	// Set task state segment
	segments[SEGSEL_TASKSTATE >> 3]
		= SEG16(STS_T32A, (uint32_t) &kernel_task_descriptor,
			sizeof(taskstate_t), 0);
	segments[SEGSEL_TASKSTATE >> 3].sd_s = 0;

	// Set up kernel task descriptor, so we can receive interrupts
	kernel_task_descriptor.ts_esp0 = KERNEL_STACK_TOP;
	kernel_task_descriptor.ts_ss0 = SEGSEL_KERN_DATA;

	// Set up interrupt descriptor table.
	// Most interrupts are effectively ignored
	for (i = 0; i < sizeof(interrupt_descriptors) / sizeof(gatedescriptor_t); i++)
		SETGATE(interrupt_descriptors[i], 0,
			SEGSEL_KERN_CODE, default_int_handler, 0);

	// The clock interrupt gets special handling
	SETGATE(interrupt_descriptors[INT_CLOCK], 0,
		SEGSEL_KERN_CODE, clock_int_handler, 0);

	// System calls get special handling.
	// Note that the last argument is '3'.  This means that unprivileged
	// (level-3) applications may generate these interrupts.
	for (i = INT_SYS_YIELD; i < INT_SYS_YIELD + 10; i++)
		SETGATE(interrupt_descriptors[i], 0,
			SEGSEL_KERN_CODE, sys_int_handlers[i - INT_SYS_YIELD], 3);

	// Reload segment pointers
	asm volatile("lgdt global_descriptor_table\n\t"
		     "ltr %0\n\t"
		     "lidt interrupt_descriptor_table"
		     : : "r" ((uint16_t) SEGSEL_TASKSTATE));

	// Convince compiler that all symbols were used
	(void) global_descriptor_table, (void) interrupt_descriptor_table;
}



/*****************************************************************************
 * interrupt_controller_init
 *
 *   Set up the interrupt controller (Intel part number 8259A).
 *   Each interrupt controller supports up to 8 different kinds of interrupt.
 *   The first x86s supported only one controller; this was too few, so modern
 *   x86 machines can have more than one controller, a master and some slaves.
 *
 *   Much hoop-jumping is required to get the controllers to communicate!
 *
 *   Note: "IRQ" stands for "Interrupt ReQuest line", and stands for an
 *   interrupt number.
 *
 *****************************************************************************/

#define MAX_IRQS	16	// Number of IRQs

// I/O Addresses of the two 8259A programmable interrupt controllers
#define IO_PIC1		0x20	// Master (IRQs 0-7)
#define IO_PIC2		0xA0	// Slave (IRQs 8-15)

#define IRQ_SLAVE	2	// IRQ at which slave connects to master

// Timer-related constants
#define	IO_TIMER1	0x040		/* 8253 Timer #1 */
#define	TIMER_MODE	(IO_TIMER1 + 3)	/* timer mode port */
#define	  TIMER_SEL0	0x00		/* select counter 0 */
#define	  TIMER_RATEGEN	0x04		/* mode 2, rate generator */
#define   TIMER_16BIT	0x30		/* r/w counter 16 bits, LSB first */

// Timer frequency: (TIMER_FREQ/freq) generates a frequency of 'freq' Hz.
#define	TIMER_FREQ	1193182
#define TIMER_DIV(x)	((TIMER_FREQ+(x)/2)/(x))

void
interrupt_controller_init(bool_t allow_clock_interrupt)
{
	// mask all interrupts
	outb(IO_PIC1+1, 0xFF);
	outb(IO_PIC2+1, 0xFF);

	/* Set up master (8259A-1) */
	// ICW1:  0001g0hi
	//    g:  0 = edge triggering (1 = level triggering)
	//    h:  0 = cascaded PICs (1 = master only)
	//    i:  1 = ICW4 required (0 = no ICW4)
	outb(IO_PIC1, 0x11);

	// ICW2:  Trap offset. Interrupt 0 will cause trap INT_HARDWARE.
	outb(IO_PIC1+1, INT_HARDWARE);

	// ICW3:  On master PIC, bit mask of IR lines connected to slave PICs;
	//        on slave PIC, IR line at which slave connects to master (0-8)
	outb(IO_PIC1+1, 1<<IRQ_SLAVE);

	// ICW4:  000nbmap
	//    n:  1 = special fully nested mode
	//    b:  1 = buffered mode
	//    m:  0 = slave PIC, 1 = master PIC
	//	  (ignored when b is 0, as the master/slave role
	//	  can be hardwired).
	//    a:  1 = Automatic EOI mode
	//    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
	outb(IO_PIC1+1, 0x3);

	/* Set up slave (8259A-2) */
	outb(IO_PIC2, 0x11);			// ICW1
	outb(IO_PIC2+1, INT_HARDWARE + 8);	// ICW2
	outb(IO_PIC2+1, IRQ_SLAVE);		// ICW3
	// NB Automatic EOI mode doesn't tend to work on the slave.
	// Linux source code says it's "to be investigated".
	outb(IO_PIC2+1, 0x01);			// ICW4

	// OCW3:  0ef01prs
	//   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
	//    p:  0 = no polling, 1 = polling mode
	//   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
	outb(IO_PIC1, 0x68);             /* clear specific mask */
	outb(IO_PIC1, 0x0a);             /* read IRR by default */

	outb(IO_PIC2, 0x68);               /* OCW3 */
	outb(IO_PIC2, 0x0a);               /* OCW3 */

	// mask all interrupts again, except possibly for clock interrupt
	outb(IO_PIC1+1, (allow_clock_interrupt ? 0xFE : 0xFF));
	outb(IO_PIC2+1, 0xFF);

	// if the clock interrupt is allowed, initialize the clock
	if (allow_clock_interrupt) {
		outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
		outb(IO_TIMER1, TIMER_DIV(HZ) % 256);
		outb(IO_TIMER1, TIMER_DIV(HZ) / 256);
	}
}



/*****************************************************************************
 * special_registers_init
 *
 *   Set up the first process's registers to required values.  In particular,
 *   tell the first process to use the memory information set up by
 *   segments_init(), and tell it to run with interrupts enabled.
 *
 *****************************************************************************/

void
special_registers_init(process_t *proc)
{
	memset(&proc->p_registers, 0, sizeof(registers_t));
	proc->p_registers.reg_cs = SEGSEL_APP_CODE | 3;
	proc->p_registers.reg_ds = SEGSEL_APP_DATA | 3;
	proc->p_registers.reg_es = SEGSEL_APP_DATA | 3;
	proc->p_registers.reg_ss = SEGSEL_APP_DATA | 3;
	// Enable interrupts
	proc->p_registers.reg_eflags = EFLAGS_IF;
}



/*****************************************************************************
 * console_clear
 *
 *   Clear the console by writing spaces to it, and move the cursor to the
 *   upper left (row 0, column 0).
 *
 *****************************************************************************/

void
console_clear(void)
{
	int i;
	cursorpos = (uint16_t *) 0xB8000;

	for (i = 0; i < 80 * 25; i++)
		cursorpos[i] = ' ' | 0x0700;
	outb(0x3D4, 14);
	outb(0x3D5, 0 / 256);
	outb(0x3D4, 15);
	outb(0x3D5, 0 % 256);
}



/*****************************************************************************
 * console_read_digit
 *
 *   Read a single digit from the keyboard and return it.
 *
 *****************************************************************************/

#define KBSTATP 0x64
#define KBS_DIB 0x01
#define KBDATAP 0x60

int
console_read_digit(void)
{
	uint8_t data;

	if ((inb(KBSTATP) & KBS_DIB) == 0)
		return -1;

	data = inb(KBDATAP);
	if (data >= 0x02 && data <= 0x0A)
		return data - 0x02 + 1;
	else if (data == 0x0B)
		return 0;
	else if (data >= 0x47 && data <= 0x49)
		return data - 0x47 + 7;
	else if (data >= 0x4B && data <= 0x4D)
		return data - 0x4B + 4;
	else if (data >= 0x4F && data <= 0x51)
		return data - 0x4F + 1;
	else if (data == 0x53)
		return 0;
	else
		return -1;
}


/*****************************************************************************
 * run
 *
 *   Run the process with the supplied process descriptor.
 *   This means reloading all the relevant registers from the descriptor's
 *   p_registers member, using the 'popal', 'popl', and 'iret'
 *   instructions.
 *
 *****************************************************************************/

void
run(process_t *proc)
{
	current = proc;

	asm volatile("movl %0,%%esp\n\t"
		     "popal\n\t"
		     "popl %%es\n\t"
		     "popl %%ds\n\t"
		     "addl $8, %%esp\n\t"
		     "iret"
		     : : "g" (&proc->p_registers) : "memory");

	while (1)
		/* do nothing */;
}
