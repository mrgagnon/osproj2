// Maylee Gagnon
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <asm/current.h>
#include <asm-generic/cputime.h>
#include <linux/time.h>
#include <linux/list.h>
#include "processinfo.h"

unsigned long **sys_call_table;

asmlinkage long (*ref_sys_cs3013_syscall2)(void);

asmlinkage long new_sys_cs3013_syscall2(struct processinfo *info) {
	struct processinfo kinfo;
	struct task_struct *task;
	
	struct task_struct *temp;
	struct list_head *pos; 

	// initalizing a processinfo structure for within the kernel 
	kinfo.state = 0;
	kinfo.pid = 0;
	kinfo.parent_pid = 0;
	kinfo.youngest_child = 0;
	kinfo.younger_sibling = 0;
	kinfo.older_sibling = 0;
	kinfo.uid = 0;
	kinfo.start_time = 0;
	kinfo.user_time = 0;
	kinfo.sys_time = 0;
	kinfo.cutime = 0;
	kinfo.cstime = 0;

 	// get task_struct for current project 
	task = current;

	// setting kinfo values    
	kinfo.state = task->state;
	kinfo.pid = task->pid;

	kinfo.parent_pid = task->parent->pid;
	kinfo.uid  = task->cred->uid.val;
	
	kinfo.start_time = timespec_to_ns(&task->start_time);  
	kinfo.user_time = cputime_to_usecs(&task->utime);
	kinfo.sys_time = cputime_to_usecs(&task->stime);
		
	// Counting children time
	list_for_each(pos, &task->children) {  // loop through children 
		temp = list_entry(pos, struct task_struct, sibling);  // get each child entry
		printk(KERN_INFO "time %lld\n",cputime_to_usecs(temp->utime ));
		kinfo.cutime += cputime_to_usecs(temp->utime); // get time and add to total time 
		kinfo.cstime += cputime_to_usecs(temp->stime);
	}
/*if (!list_empty(&task->children)){  
		list_for_each(pos, &task->children) { 
			temp = list_entry(pos, struct task_struct, sibling);
			printk(KERN_INFO "time %lld\n",temp->utime );
			kinfo.cutime += cputime_to_usecs(temp->utime);
			kinfo.cstime += cputime_to_usecs(temp->stime);
		}
	}
	else { 
		kinfo.cutime = -1;
		kinfo.cstime = -1; 
	}*/
	
	
	struct task_struct *youngestChild;
	// youngest child 
	if (!list_empty(&task->children)){ // check to see if the list is empty
		youngestChild = list_last_entry(&task->children, struct task_struct, sibling); // get the child with last added
		kinfo.youngest_child = youngestChild->pid; 
		printk(KERN_INFO "child! %d\n", youngestChild->pid);  // keep printing  0
	}
	else {
		kinfo.youngest_child = -1;
	}

	// younger sibling 
	if(!list_empty(&task->sibling)){ // check to see if the list is empty, 
		temp = list_entry((&task->sibling)->next, struct task_struct, sibling); // get the next entry
		kinfo.younger_sibling = temp->pid; 
		printk(KERN_INFO "PID: %d\n", temp->pid);  // Keeps printing 0's 
	
	}
	else{	
		kinfo.younger_sibling=-1;
	}
	
	// older sibling 
	if(list_empty(&task->sibling)){
		kinfo.older_sibling=-1;
	}
	else{	
		temp = list_entry((&task->sibling)->prev, struct task_struct, sibling);
		if (temp->pid > 0) {
			kinfo.older_sibling = temp->pid; 
		}
		else {
			kinfo.older_sibling=-1;
		}
	}


	// check validiy of arguments & return values 
    	if (copy_to_user(info, &kinfo, sizeof kinfo)){  
		return EFAULT;
	}
	else {
		return 0;
	}
}

static unsigned long **find_sys_call_table(void) {
  unsigned long int offset = PAGE_OFFSET;
  unsigned long **sct;
  
  while (offset < ULLONG_MAX) {
    sct = (unsigned long **)offset;

    if (sct[__NR_close] == (unsigned long *) sys_close) {
      printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX",
                (unsigned long) sct);
      return sct;
    }
    
    offset += sizeof(void *);
  }
  
  return NULL;
}

static void disable_page_protection(void) {
  /*
    Control Register 0 (cr0) governs how the CPU operates.

    Bit #16, if set, prevents the CPU from writing to memory marked as
    read only. Well, our system call table meets that description.
    But, we can simply turn off this bit in cr0 to allow us to make
    changes. We read in the current value of the register (32 or 64
    bits wide), and AND that with a value where all bits are 0 except
    the 16th bit (using a negation operation), causing the write_cr0
    value to have the 16th bit cleared (with all other bits staying
    the same. We will thus be able to write to the protected memory.

    It's good to be the kernel!
   */
  write_cr0 (read_cr0 () & (~ 0x10000));
}

static void enable_page_protection(void) {
  /*
   See the above description for cr0. Here, we use an OR to set the 
   16th bit to re-enable write protection on the CPU.
  */
  write_cr0 (read_cr0 () | 0x10000);
}

static int __init interceptor_start(void) {
  /* Find the system call table */
  if(!(sys_call_table = find_sys_call_table())) {
    /* Well, that didn't work. 
       Cancel the module loading step. */
    return -1;
  }
  
  /* Store a copy of all the existing functions */
  ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];

  /* Replace the existing system calls */
  disable_page_protection();

  sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)new_sys_cs3013_syscall2;
  
  enable_page_protection();
  
  /* And indicate the load was successful */
  printk(KERN_INFO "Loaded interceptor!");

  return 0;
}

static void __exit interceptor_end(void) {
  /* If we don't know what the syscall table is, don't bother. */
  if(!sys_call_table)
    return;
  
  /* Revert all system calls to what they were before we began. */
  disable_page_protection();
  sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)ref_sys_cs3013_syscall2;
  enable_page_protection();

  printk(KERN_INFO "Unloaded interceptor!");
}

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);
