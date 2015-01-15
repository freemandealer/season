#include "kernel.h"
#include "lib.h"
#include "interupt.h"

#define MEMCHK_NUM_ADDR	0x7e00
#define ADDR_RANGE_DESC_TBL_ADDR	(MEMCHK_NUM_ADDR+4)
#define GRAPHIC_ADDR	0xb800
#define PAGE_DIR_TBL_ADDR	0x100000
#define PAGE_TBL_ADDR	(PAGE_DIR_TBL_ADDR+0x1000)

struct addr_range_desc {
	u32 baselow;
	u32 basehigh;
	u32 lenlow;
	u32 lenhigh;
	u32 type;
};

/*
 * do not put any function defination upon cstart
 */
static void init_global_var();
static int get_total_mem();
static int setup_paging();

void cstart()
{
	int ret;

	init_global_var();
	ret = setup_paging();
	if (ret < 0) {
		goto out;
	}
	init_idtr();
	init_8259A();
	setup_idt();
	set_interupt();
out:
	for(;;) {};
}

static void init_global_var()
{
	tmp_dbg = (int *)0x7dfc; //XXX
}

static int get_total_mem()
{
	struct addr_range_desc *ar;
	int *memchk_num;
	int total_mem = 0;
	int i;

	ar = (struct addr_range_desc *)ADDR_RANGE_DESC_TBL_ADDR;
	memchk_num = (int *)MEMCHK_NUM_ADDR;
	for (i=0; i<*memchk_num; i++) {
		total_mem += ar[i].lenlow;
	}
	return total_mem;
}

static int setup_paging()
{
	int total_mem;
	int page_tbl_num;
	int page_dir_tbl_num;
	pde_t *pde;
	pte_t *pte;
	long pt_base;
	long pte_base;
	int i;

	total_mem = get_total_mem();
	*tmp_dbg = total_mem;

	if (total_mem <= 0) {
		return ERR;
	}
	pde = (pde_t *)PAGE_DIR_TBL_ADDR;
	pte = (pte_t *)PAGE_TBL_ADDR;
	page_tbl_num = (total_mem+4095) / 4096;
	page_dir_tbl_num = (page_tbl_num+1023) / 1024;
	/* initiate page directory table */
	pt_base = PAGE_TBL_ADDR;
	for (i=0; i<page_dir_tbl_num; i++) {
		pde[i] = pt_base | 0x7;
		pt_base += 4096;
	}
	/* initiate page tables */
	pte_base = 0;
	for (i=0; i<page_tbl_num; i++) {
		pte[i] = pte_base | 0x7;
		pte_base += 4096;
	}
	apply_paging();
	return OK;
}
