
#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <sys/mman.h>

#include "os.h"

/* 2^20 pages ought to be enough for anybody */
#define NPAGES	(1024*1024)

static void* pages[NPAGES];

uint64_t alloc_page_frame(void)
{
	static uint64_t nalloc;
	uint64_t ppn;
	void* va;

	if (nalloc == NPAGES)
		errx(1, "out of physical memory");

	/* OS memory management isn't really this simple */
	ppn = nalloc;
	nalloc++;

	va = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (va == MAP_FAILED)
		err(1, "mmap failed");

	pages[ppn] = va;	
	return ppn;
}

void* phys_to_virt(uint64_t phys_addr)
{
	uint64_t ppn = phys_addr >> 12;
	uint64_t off = phys_addr & 0xfff;
	void* va = NULL;

	if (ppn < NPAGES)
		va = pages[ppn] + off;

	return va;
}

int main(int argc, char **argv)
{
	uint64_t pt = alloc_page_frame();

	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);
	page_table_update(pt, 0xcafe, 0xf00d);
    printf("aaa  %lx\n", page_table_query(pt, 0xcafe));
    printf("bbb  %lx\n", page_table_query(pt, 0xcafe));
    assert(page_table_query(pt, 0xcafe) == 0xf00d);
	page_table_update(pt, 0xcafe, NO_MAPPING);
    printf("%lx\n", page_table_query(pt, 0xcafe));
	assert(page_table_query(pt, 0xcafe) == NO_MAPPING);

    printf("%ld\n",page_table_query(pt, 0xdebb1e));
    page_table_update(pt, 0xcafe, 0xf00d);
    printf("%lx\n",page_table_query(pt, 0xcafe));
    page_table_update(pt, 0xcafe, NO_MAPPING);
    printf("%ld\n",page_table_query(pt, 0xcafe));
    page_table_update(pt, 0x1fffffffffff, 0xc001);
    printf("%lx\n",page_table_query(pt,  0x1fffffffffff));
    page_table_update(pt, 0xdebb, 0xddd343);
    printf("%lx\n",page_table_query(pt, 0xdebb));
    page_table_update(pt, 0x1234, 0x5678);
    printf("%lx\n",page_table_query(pt, 0x1234));
    page_table_update(pt, 0x3ffff, 0xc0de);
    printf("%lx\n",page_table_query(pt, 0x3ffff));
    page_table_update(pt, 0xabcd, 0xdefe);
    printf("%lx\n",page_table_query(pt, 0xabcd));
    page_table_update(pt, 0xdebb1e, 0xdebb1e);
    printf("%lx\n",page_table_query(pt, 0xdebb1e));
    page_table_update(pt, 0xaba, 0x100);
    printf("%lx\n",page_table_query(pt, 0xaba));
    page_table_update(pt, 0xabcdef, 0xabcdef);
    printf("%lx\n",page_table_query(pt, 0xabcdef));
    page_table_update(pt, 0xc0ffe, 0xbed);
    printf("%lx\n",page_table_query(pt, 0xc0ffe));
    page_table_update(pt, 0xc01d, 0xbeef);
    printf("%lx\n",page_table_query(pt, 0xc01d));
    printf("%lx\n",page_table_query(pt, 0xdebb1e));
	/*page_table_update(pt, 0x1234, 0x5678);
	printf("%lx\n", page_table_query(pt, 0x1234));
    page_table_update(pt, 0xabcd, 0xdefe);
    printf("%lx\n", page_table_query(pt, 0xabcd));
    page_table_update(pt, 0x1abcdef123, 0xc001);
    printf("%lx\n", page_table_query(pt, 0x1abcdef123));*/

	return 0;
}

/*abcdef0125
bad
coo1
debb1e
c0ffee
f00d
bed
bad co1d
a5
a
beef
deaf
feed
dead
4, 2
1 8 food
1 d1d a code*/

