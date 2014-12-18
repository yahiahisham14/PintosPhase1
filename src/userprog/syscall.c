#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
//#include "lib/user/syscall.h"


//typedef pid_t int;
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

static void syscall_handler (struct intr_frame *);

static void halt (void);
static void exit (int status);
static pid_t exec ( const char *cmd_line );
static int wait (pid_t pid);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);
static int open (const char *file);
static int filesize (int fd);
static int read (int fd, void *buffer, unsigned size);
static int write (int fd, const void *buffer, unsigned size);
static void seek (int fd, unsigned position);
static unsigned tell (int fd);
static void close (int fd);

static struct lock sync_lock;

static bool check(void *esp);
static void 
get_Args(void* esp , int args_count , char* arg_0, char * arg_1 , char * arg_2 );

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init ( &sync_lock );
}

/* Time line of the syscall_handler:
	1 - Fetch the stack ptr
	2 - Check that it's valid stack ptr --> validate that it's user stack ptr
	3 - Fetch the system call number (Top of stack)
	4 - Call the appropriate system call method
*/
static void
syscall_handler (struct intr_frame *f ) 
{
  
	/* 1 - Fetch Stack ptr */
	void *esp = f->esp;

	/* 2 - Validate the stack ptr */
	bool ptr_valid = check(esp);

	if (!ptr_valid){
		// Call exit passing 0
		exit(0);
	}

	/* 3 - Fetch system call number */
	int sys_call_number = (int) (*(int *)esp);

	//argument pointers.
	char *arg_0;
	char *arg_1;
	char *arg_2;

	/* 4 - Call the appropriate system call method */
	switch(sys_call_number){
		case SYS_HALT:

			get_Args(esp  , 0 ,arg_0 ,arg_1 ,arg_2);
			halt();

			break;
		case SYS_EXIT:

			get_Args(esp  , 1 ,arg_0 ,arg_1 ,arg_2);
			exit((int) *arg_0);

			break;
		case SYS_EXEC:

			get_Args(esp  , 1 ,arg_0 ,arg_1 ,arg_2);
			exec(arg_0);

			break;
		case SYS_WAIT:

			get_Args(esp  , 1 ,arg_0 ,arg_1 ,arg_2);
			wait((pid_t)*arg_0);

			break;
		case SYS_CREATE:

			get_Args(esp  , 2 ,arg_0 ,arg_1 ,arg_2);
			create(arg_0,(unsigned)*arg_1);

			break;		
		case SYS_REMOVE:

			get_Args(esp  , 1 ,arg_0 ,arg_1 ,arg_2);
			remove(arg_0);

			break;
		case SYS_OPEN:

			get_Args(esp  , 1 ,arg_0 ,arg_1 ,arg_2);
			open(arg_0);

			break;
		case SYS_FILESIZE:

			get_Args(esp  , 1 ,arg_0 ,arg_1 ,arg_2);
			filesize((int)*arg_0);

			break;
		case SYS_READ:

			get_Args(esp  , 3 ,arg_0 ,arg_1 ,arg_2);
			read ((int)*arg_0,   (void *) arg_1, (unsigned)*arg_1 );

			break;
		case SYS_WRITE:

			get_Args(esp  , 3 ,arg_0 ,arg_1 ,arg_2);
			write ((int) *arg_0, (void *) arg_1, (unsigned) *arg_2);

			break;
		case SYS_SEEK:

			get_Args(esp  , 2 ,arg_0 ,arg_1 ,arg_2);
			seek ((int)*arg_0, (unsigned)arg_1);

			break;
		case SYS_TELL:

			get_Args(esp  , 1 ,arg_0 ,arg_1 ,arg_2);
			tell ((int) *arg_0);

			break;
		case SYS_CLOSE:

			get_Args(esp  , 1 ,arg_0 ,arg_1 ,arg_2);
			close ((int) *arg_0);

			break;
	}
	

}

static void
halt (void){
	shutdown_power_off ();
}//end function

static void
exit (int status)
{
	thread_exit ();
}//end function.

static pid_t
exec ( const char *cmd_line )
{
	lock_acquire ( &sync_lock);
	pid_t id = (pid_t) process_execute ( cmd_line );
	lock_release ( &sync_lock );
	return id;
}//end function

static int
wait (pid_t pid)
{

}//end function.

static bool
create (const char *file, unsigned initial_size)
{

}//end function.

static bool
remove (const char *file)
{

}//end function.

static int
open (const char *file){

}//end function.

static int
filesize (int fd)
{

}//end function.

static int
read (int fd, void *buffer, unsigned size)
{

}//end function.

static int 
write (int fd, const void *buffer, unsigned size)
{
	if( fd == 1 ){
		putbuf (buffer, size);
	}
}//end function.

static void
seek (int fd, unsigned position)
{

}//end function.

static unsigned
tell (int fd)
{

}//end function.

static void 
close (int fd)
{

}//end function.



/*--------------------------------------------------ADDED METHODS----------------------------------*/


/* This method used to validate the stack ptr by :
	1 - Check if the ptr is null
	2 - Check that it's virtual address is not null and it's within user address space
*/
static bool
check(void *esp)
{

	/* First check if the stack ptr is null */
	if (esp == NULL)
		return false;

	/* Second check that it has a valid mapping :
		1 - get current thread page directory --> call pagedir:active_pd (void)
		2 - get virtual address --> call pagedir:pagedir_get_page (uint32_t *pd, const void *uaddr)
		3 - check that the returned address is not null
	*/

	// Get page directory of current thread
	uint32_t * pd = thread_current()->pagedir;

	/* Second check that user pointer points below PHYS_BASE --> user virtual address */
	if (!is_user_vaddr(esp))
		return false;

	// Get virtual address
	void* potential_address = pagedir_get_page (pd, esp);

	// Check if it's mapped to null
	if (potential_address == NULL)
		return false;
	
	/* All true */
	return true;
}

/*get the needed arguments given the number of the 
arguments needed */
static void
get_Args(void* esp , int args_count , char* arg_0, char * arg_1 , char * arg_2 ){

	if(args_count>0)
	{
		arg_0 =  (char*) esp;
		esp += 4;
	}

	if(args_count>1)
	{
		arg_1 = (char*) esp ;
		esp += 4;
	}

	if(args_count>2)
	{
		arg_2 = (char*) esp ;
		esp += 4;
	}

}//end function.
