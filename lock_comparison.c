/*
 * Test performance of various locking primitives.
 *
 * Copyright (C) 2007 Anton Blanchard <anton@au.ibm.com>, IBM
 * Copyright (C) 2014 Michael Ellerman, IBM
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#define NR_LOOPS 100000000

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


static void inline spin_unlock(unsigned int *lock)
{
	asm volatile("lwsync":::"memory");
	*lock = 0;
}

static inline unsigned long spin_isync_trylock(unsigned int *lock)
{
	unsigned long tmp, token;

	token = 1;
	asm volatile(
"1:	lwarx	%0,0,%2,1\n\
	cmpwi	0,%0,0\n\
	bne-	2f\n\
	stwcx.	%1,0,%2\n\
	bne-	1b\n\
	isync\n\
2:"	: "=&r" (tmp)
	: "r" (token), "r" (lock)
	: "cr0", "memory");

	return tmp;
}

static void inline spin_isync_lock(unsigned int *lock)
{
	while (spin_isync_trylock(lock))
		;
}

void test_spin_isync_lock(unsigned long nr)
{
	unsigned int lock = 0;
	unsigned long i;

	for (i = 0; i < nr; i++) {
		spin_isync_lock(&lock);
		spin_unlock(&lock);
	}
}

static inline unsigned long spin_lwsync_trylock(unsigned int *lock)
{
	unsigned long tmp, token;

	token = 1;
	asm volatile(
"1:	lwarx	%0,0,%2,1\n\
	cmpwi	0,%0,0\n\
	bne-	2f\n\
	stwcx.	%1,0,%2\n\
	bne-	1b\n\
	lwsync\n\
2:"	: "=&r" (tmp)
	: "r" (token), "r" (lock)
	: "cr0", "memory");

	return tmp;
}

static void inline spin_lwsync_lock(unsigned int *lock)
{
	while (spin_lwsync_trylock(lock))
		;
}

void test_spin_lwsync_lock(unsigned long nr)
{
	unsigned int lock = 0;
	unsigned long i;

	for (i = 0; i < nr; i++) {
		spin_lwsync_lock(&lock);
		spin_unlock(&lock);
	}
}

static inline unsigned long spin_sync_trylock(unsigned int *lock)
{
	unsigned long tmp, token;

	token = 1;
	asm volatile(
"1:	lwarx	%0,0,%2,1\n\
	cmpwi	0,%0,0\n\
	bne-	2f\n\
	stwcx.	%1,0,%2\n\
	bne-	1b\n\
	sync\n\
2:"	: "=&r" (tmp)
	: "r" (token), "r" (lock)
	: "cr0", "memory");

	return tmp;
}

static void inline spin_sync_lock(unsigned int *lock)
{
	while (spin_sync_trylock(lock))
		;
}

void test_spin_sync_lock(unsigned long nr)
{
	unsigned int lock = 0;
	unsigned long i;

	for (i = 0; i < nr; i++) {
		spin_sync_lock(&lock);
		spin_unlock(&lock);
	}
}

#define TIME(job, name) \
	clock_gettime(CLOCK_MONOTONIC, &start); \
	job; \
	clock_gettime(CLOCK_MONOTONIC, &end); \
	printf("%s,0x%lx," name ",%1lu\n", hostname, pvr, \
		((end.tv_sec  - start.tv_sec) * 1000000000UL + \
		(end.tv_nsec - start.tv_nsec)));

int main()
{
	struct timespec start, end;
	unsigned long pvr;
	char hostname[64];

	asm volatile("mfspr %0, 0x11f" : "=r" (pvr));

	gethostname(hostname, sizeof(hostname));

	/* Get warmed up */

	test_spin_isync_lock(NR_LOOPS);
	test_spin_lwsync_lock(NR_LOOPS);
	test_spin_sync_lock(NR_LOOPS);

	/* Test all possible permutations */

	TIME(test_spin_isync_lock(NR_LOOPS), "spin_isync_lock");
	TIME(test_spin_lwsync_lock(NR_LOOPS), "spin_lwsync_lock");
	TIME(test_spin_sync_lock(NR_LOOPS), "spin_sync_lock");

	TIME(test_spin_isync_lock(NR_LOOPS), "spin_isync_lock");
	TIME(test_spin_sync_lock(NR_LOOPS), "spin_sync_lock");
	TIME(test_spin_lwsync_lock(NR_LOOPS), "spin_lwsync_lock");

	TIME(test_spin_lwsync_lock(NR_LOOPS), "spin_lwsync_lock");
	TIME(test_spin_isync_lock(NR_LOOPS), "spin_isync_lock");
	TIME(test_spin_sync_lock(NR_LOOPS), "spin_sync_lock");

	TIME(test_spin_lwsync_lock(NR_LOOPS), "spin_lwsync_lock");
	TIME(test_spin_sync_lock(NR_LOOPS), "spin_sync_lock");
	TIME(test_spin_isync_lock(NR_LOOPS), "spin_isync_lock");

	TIME(test_spin_sync_lock(NR_LOOPS), "spin_sync_lock");
	TIME(test_spin_isync_lock(NR_LOOPS), "spin_isync_lock");
	TIME(test_spin_lwsync_lock(NR_LOOPS), "spin_lwsync_lock");

	TIME(test_spin_sync_lock(NR_LOOPS), "spin_sync_lock");
	TIME(test_spin_lwsync_lock(NR_LOOPS), "spin_lwsync_lock");
	TIME(test_spin_isync_lock(NR_LOOPS), "spin_isync_lock");

	return 0;
}
