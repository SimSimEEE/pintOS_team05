/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "userprog/syscall.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {

	off_t init_ofs = offset;
	off_t ofs = offset;
	uint32_t read_bytes = length;
	void *init_addr = addr;
   	while (read_bytes > 0)
   	{
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;

		struct info *aux = (struct info *)malloc(sizeof(struct info));
		aux->file = file;
		aux->offset = ofs;
		aux->read_bytes = page_read_bytes;
		vm_alloc_page_with_initializer(VM_FILE, addr,
											writable, lazy_load_segment, aux);

		/* Advance. */
		read_bytes -= page_read_bytes;
		addr += PGSIZE;
		ofs += page_read_bytes;
	}
	return init_addr;
}

/* Do the munmap */
void do_munmap(void *addr)
{
	struct thread *cur = thread_current();
	struct page *page = spt_find_page(&cur->spt, addr);
	if(page == NULL)
		return;
	struct info *file_info = page->uninit.aux;
	struct file *file = file_info->file;
	if(file != NULL)
		return;
	while(page != NULL){
		if(pml4_is_dirty(cur->pml4, addr)){
			lock_acquire(&filesys_lock);
			file_seek(file_info->file,file_info->offset);
			file_write(file_info->file,page->frame->kva,file_info->read_bytes);
			lock_release(&filesys_lock);
			pml4_set_dirty(cur->pml4,addr,false);
		}
		// pml4_clear_page(cur->pml4, page->va);
		pml4_clear_page(cur->pml4, addr); //@@@@@@@@@@@@
		addr += PGSIZE;
		page = spt_find_page(&cur->spt,addr);
	}
}