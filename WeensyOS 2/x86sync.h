#ifndef WEENSYOS_X86SYNC_H
#define WEENSYOS_X86SYNC_H
#include "types.h"

/*****************************************************************************
 * x86sync.h
 *
 *   C code implementing x86 atomic instructions.
 *
 *****************************************************************************/

/*****************************************************************************
 * atomic_swap(addr, val)
 *
 *   Executes the following as a single atomic operation:
 *
 *      void *oldval = *addr;
 *      *addr = val;
 *      return oldval;
 *
 *   You may need to type-cast some of the arguments or the return value to
 *   use this function, or the other functions in this file.
 *
 *****************************************************************************/

static inline uint32_t atomic_swap(uint32_t *addr, uint32_t val) __attribute__((always_inline));

static inline uint32_t
atomic_swap(uint32_t *addr, uint32_t val)
{
	asm volatile("xchgl %0, %1"
		     : "=g" (val), "=m" (*addr)
		     : "0" (val), "m" (*addr));
	return val;
}


/*****************************************************************************
 * compare_and_swap(addr, expected, desired)
 *
 *   Executes the following as a single atomic operation:
 *
 *      uint32_t actual = *addr;
 *      if (actual == expected)
 *          *addr = desired;
 *      return actual;
 *
 *****************************************************************************/

static inline uint32_t compare_and_swap(uint32_t *addr, uint32_t expected, uint32_t desired) __attribute__((always_inline));

static inline uint32_t
compare_and_swap(uint32_t *addr, uint32_t expected, uint32_t desired)
{
	asm volatile("lock cmpxchg %2,%1"
		     : "=a" (expected), "=m" (*addr)
		     : "r" (desired), "0" (expected), "m" (*addr));
	return expected;
}



/*****************************************************************************
 * fetch_and_add(addr, delta)
 *
 *   Executes the following as a single atomic operation:
 *
 *      uint32_t oldval = *addr;
 *      *addr += delta;
 *      return oldval;
 *
 *   Note that the OLD value is returned.
 *
 *****************************************************************************/

static inline uint32_t fetch_and_add(uint32_t *addr, uint32_t delta) __attribute__((always_inline));

static inline uint32_t
fetch_and_add(uint32_t *addr, uint32_t delta)
{
	asm volatile("lock xaddl %0, %1"
		     : "=a" (delta), "=m" (*addr)
		     : "0" (delta), "m" (*addr));
	return delta;
}

#endif /* !WEENSYOS_X86SYNC_H */
