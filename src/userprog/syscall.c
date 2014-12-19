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

static void kill_process ();

static struct lock sync_lock;

static bool check(void *esp);
static void 
get_Args(void* esp , int args_count , void** arg_0, void ** arg_1 , void ** arg_2 );

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
  //printf("\n\n\nHereeeeeeeeeee in syscall_handler.\n");
	/* 1 - Fetch Stack ptr */
	void *esp = f->esp;

	/* 2 - Validate the stack ptr */
	bool ptr_valid = check(esp);

	if (!ptr_valid){
		// Call exit passing -1
		exit(-1);
	}

	/* 3 - Fetch system call number */
	int sys_call_number = (int) (*(int *)esp);
	//printf("syscall Num :  %d %x\n", sys_call_number, esp);

	//argument pointers.
	void *arg_0;
	void *arg_1;
	void *arg_2;

	esp += 4;
	ptr_valid = check(esp);
	if (!ptr_valid){
		// Call exit passing 0
		exit(-1);
	}

	/* 4 - Call the appropriate system call method */
	switch(sys_call_number){
		case SYS_HALT:

			get_Args(esp  , 0 ,&arg_0 ,&arg_1 ,&arg_2);
			halt();

			break;
		case SYS_EXIT:

			get_Args(esp  , 1 ,&arg_0 ,&arg_1 ,&arg_2);
			exit( (int) arg_0 );

			break;
		case SYS_EXEC:

			get_Args(esp  , 1 ,&arg_0 ,&arg_1 ,&arg_2);
			f->eax = (uint32_t) exec( (char *) arg_0);

			break;
		case SYS_WAIT:

			get_Args(esp  , 1 ,&arg_0 ,&arg_1 ,&arg_2);
			f->eax = (uint32_t) wait( (pid_t) arg_0);

			break;
		case SYS_CREATE:

			get_Args(esp  , 2 ,&arg_0 ,&arg_1 ,&arg_2);
			f->eax = (uint32_t) create( (char *) arg_0, (unsigned) arg_1);

			break;		
		case SYS_REMOVE:

			get_Args(esp  , 1 ,&arg_0 ,&arg_1 ,&arg_2);
			f->eax = (uint32_t) remove( (char *) arg_0);

			break;
		case SYS_OPEN:

			get_Args(esp  , 1 ,&arg_0 ,&arg_1 ,&arg_2);
			f->eax = (uint32_t) open( (char *) arg_0);

			break;
		case SYS_FILESIZE:

			get_Args(esp  , 1 ,&arg_0 ,&arg_1 ,&arg_2);
			f->eax = (uint32_t) filesize( (int) arg_0 );

			break;
		case SYS_READ:

			get_Args(esp  , 3 ,&arg_0 ,&arg_1 ,&arg_2);
			
			f->eax = (uint32_t) read ( (int)arg_0, (void *) arg_1, (unsigned) arg_2 );

			break;
		case SYS_WRITE:

			get_Args(esp , 3 ,&arg_0 ,&arg_1 ,&arg_2);

			//printf("\n Int: %d , const: %x , unsigned: %d \n", (int) arg_0 , 
			//	(const void *) arg_1 , (unsigned) arg_2 );

			f->eax = (uint32_t) write ( (int) arg_0, (const void *) arg_1, (unsigned) arg_2 );

			break;
		case SYS_SEEK:

			get_Args(esp  , 2 ,&arg_0 ,&arg_1 ,&arg_2);
			seek ( (int) arg_0, (unsigned)arg_1 );

			break;
		case SYS_TELL:

			get_Args(esp  , 1 ,&arg_0 ,&arg_1 ,&arg_2);
			f->eax = (uint32_t) tell ( (int) arg_0 ) ;

			break;
		case SYS_CLOSE:

			get_Args(esp  , 1 ,&arg_0 ,&arg_1 ,&arg_2);
			close ( (int) arg_0 );

			break;
		default:
			kill_process ();
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
	char* fileName = thread_current()->name;
	char temp[20];
	int i = 0;
	while( fileName[i] != ' ' && fileName[i] != NULL ){
		 temp[i] = fileName[i];
		i++;
	}//end while
	temp[i] = '\0';
	//printf("\n\n Hereeeeeeeeeee in exit fileName: %s. , TEMP: %s. \n", fileName , temp );
	printf("%s: exit(%d)\n",temp, status );
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
	// Aquire lock for acessing file system
	lock_acquire ( &sync_lock);

	// Call file system create
	bool created_succ = filesys_create (file, (off_t) initial_size);

	// Release lock
	lock_release ( &sync_lock );

	return created_succ;
	
}//end function.

static bool
remove (const char *file)
{
	// Aquire lock for acessing file system
	lock_acquire ( &sync_lock);

	// Call file system create
	bool deleted_succ = filesys_remove (file);

	// Release lock
	lock_release ( &sync_lock );

	return deleted_succ;
	
}//end function.

static int
open (const char *file){
	printf("\n\nana hena ya john!!\n\n" );
	//filesys_open(file);
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



/*--------------------------------------ADDED METHODS----------------------------------*/


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
		2 - get virtual address --> call pagedir:pagedir_get_page 
		(uint32_t *pd, const void *uaddr)
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
get_Args(void* esp , int args_count , void** arg_0, void ** arg_1 , void ** arg_2 ){

	if(args_count>0)
	{
		*arg_0 =   *(void **)esp;
			//printf("hiiiiiiiii %d %x\n", *arg_0 , esp);

		esp += 4;
		bool ptr_valid = check(esp);
		if (!ptr_valid){
			// Call exit passing 0
			exit(-1);
		}
	}

	if(args_count>1)
	{
		*arg_1 = *(void **)esp;
			//printf("%d %x\n", *arg_1, esp);

		esp += 4;
		bool ptr_valid = check(esp);
		if (!ptr_valid){
			// Call exit passing 0
			exit(-1);
		}
	}

	if(args_count>2)
	{
		*arg_2 = *(void **)esp;
			//printf("%d %x\n", *arg_2, esp);

	}

}//end function.

static void
kill_process(){
	exit(-1);
}//end function.

