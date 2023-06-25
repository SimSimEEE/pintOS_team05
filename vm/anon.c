/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

#include "threads/vaddr.h"
#include "include/lib/kernel/bitmap.h"

/* DISC_INDEX는 PGSIZE(4096 bytes)를 DISK_SECTOR_SIZE(512 bytes)로 나눠준 값(=8) */
size_t DISC_INDEX = PGSIZE / DISK_SECTOR_SIZE;

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in(struct page *page, void *kva);
static bool anon_swap_out(struct page *page);
static void anon_destroy(struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void vm_anon_init(void)
{
	/* TODO: Set up the swap_disk. */
	/* disk_get()은 해당 인자에 맞는 디스크를 반환해주는 함수 */
	swap_disk = disk_get(1, 1);
	/*  한 페이지 당 몇 개의 섹터가 들어가는지를 나눈 값을 dsize로 지칭 */
	disk_sector_t dsize = disk_size(swap_disk) / DISC_INDEX;
	/*
	 *주어진 dsize 크기만큼 swap_table을 비트맵으로 생성한다.
	 *이 스왑 테이블은 할당받은 스왑 디스크를 관리하는 테이블
	 */
	swap_table = bitmap_create(dsize);
}

/* Initialize the file mapping */
bool anon_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
	anon_page->index = -1;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in(struct page *page, void *kva)
{
	struct anon_page *anon_page = &page->anon;
	/* anon_page에 들어있는 index */
	size_t index = anon_page->index;
	if (!bitmap_test(swap_table, index))
		return false;
	for (int i = 0; i < DISC_INDEX; i++)
		disk_read(swap_disk, index * DISC_INDEX + i, kva + i * DISK_SECTOR_SIZE);
	bitmap_set(swap_table, index, false);
}

/* Swap out the page by writing contents to the swap disk. */
/* anonymous page를 디스크 내 swap 공간으로 내리는 작업을 수행하는 함수 */
static bool
anon_swap_out(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
	/* 비트맵을 처음부터 순회해 false 값을 갖는 비트를 하나 찾는다.
	즉, 페이지를 할당받을 수 있는 swap slot을 하나 찾는다. */
	size_t index = bitmap_scan(swap_table, 0, 1, false);
	if (index == BITMAP_ERROR)
	{
		return false;
	}
	/*
	 * 한 페이지를 디스크에 써주기 위해 SECTORS_PER_PAGE 개의 섹터에 저장해야 한다.
	 * 이때 디스크에 각 섹터 크기의 DISK_SECTOR_SIZE만큼 써준다.
	 */
	for (int i = 0; i < DISC_INDEX; i++)
	{
		disk_write(swap_disk, index * DISC_INDEX + i, page->va + i * DISK_SECTOR_SIZE);
	}
	/* swap table의 해당 페이지에 대한 swap slot의 비트를 true로 바꿔주고
	해당 페이지의 PTE에서 present bit을 0으로 바꿔준다.
	이제 프로세스가 이 페이지에 접근하면 page fault가 뜬다.	*/
	bitmap_set(swap_table, index, true);
	pml4_clear_page(thread_current()->pml4, page->va);
	/* 페이지의 swap_index 값을 이 페이지가 저장된 swap slot의 번호로 써준다.*/
	anon_page->index = index;
	return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy(struct page *page)
{
	struct anon_page *anon_page = &page->anon;
}
