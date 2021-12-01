#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/mm_types.h>
#include<linux/module.h>
#include<linux/moduleparam.h>
#include<linux/unistd.h>
#include<linux/types.h>
#include<linux/sched.h>
#include<linux/signal.h>
#include<linux/highmem.h>
#include<linux/kallsyms.h>
#include<linux/syscalls.h>
#include<linux/proc_fs.h>
#include<asm/syscall_wrapper.h>
#include<asm/uaccess.h>

#define __NR_ftrace 336

void** syscall_table;
void* original_ftrace;


__SYSCALL_DEFINEx(1, process_trace, long, pid){
    	char *path;
	char buf[512];
	struct file* f;
	struct task_struct *task;
	struct mm_struct *vm_addr;
	struct vm_area_struct *vm;
	for (task = &init_task ; (task = next_task(task)) != &init_task && task->pid != pid;);

	vm_addr = task->mm;
	for (vm = vm_addr->mmap; vm != NULL; vm = vm->vm_next){
		f = vm->vm_file;
		if(f){
			path = d_path(&f->f_path, buf, 512);
			printk(KERN_INFO"mem(%lx~%lx) code(%lx~%lx) data(%lx~%lx) heap(%lx~%lx) %s",vm->vm_start ,vm->vm_end, vm->vm_mm->start_code, vm->vm_mm->end_code, vm->vm_mm->start_data, vm->vm_mm->end_data, vm->vm_mm->start_brk, vm->vm_mm->brk, path);
		}
		
	}
	return 0;
}

void make_rw(void* addr){
	unsigned int level;
	pte_t* pte = lookup_address((u64)addr, &level);
	
	if(pte->pte &~ _PAGE_RW)
		pte->pte |= _PAGE_RW;
}

void make_ro(void* addr){
	unsigned int level;
	pte_t* pte = lookup_address((u64)addr, &level);
	
	pte->pte = pte->pte &~ _PAGE_RW; 
}

static int __init hooking_init(void){
	syscall_table = (void**) kallsyms_lookup_name("sys_call_table");	
	make_rw(syscall_table);
	//save original table index of each syscall
    	original_ftrace = syscall_table[__NR_ftrace];
	//allocate new function to syscall table
    	syscall_table[__NR_ftrace] = __x64_sysprocess_trace;
	
	return 0;
}

static void __exit hooking_exit(void){
    	syscall_table[__NR_ftrace] = original_ftrace;
	make_ro(syscall_table);
}

module_init(hooking_init);
module_exit(hooking_exit);
MODULE_LICENSE("GPL");
