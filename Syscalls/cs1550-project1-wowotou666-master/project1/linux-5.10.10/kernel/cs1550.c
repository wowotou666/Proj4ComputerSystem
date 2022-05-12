#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/stddef.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/cs1550.h>

/*Marco declration and initialization*/
static DEFINE_RWLOCK(sem_rwlock);
static LIST_HEAD(sem_list);
static long sem_counter = 0;  // global counter

/**
 * Creates a new semaphore. The long integer value is used to
 * initialize the semaphore's value.
 *
 * The initial `value` must be greater than or equal to zero.
 *
 * On success, returns the identifier of the created
 * semaphore, which can be used with up() and down().
 *
 * On failure, returns -EINVAL or -ENOMEM, depending on the
 * failure condition.
 */
SYSCALL_DEFINE1(cs1550_create, long, value)
{
	struct cs1550_sem *sem = NULL;  // some memory allocation and initialization routine
	sem = kmalloc(sizeof(struct cs1550_sem), GFP_ATOMIC);

	if (value < 0) {  // Check if value is >= 0
		return -EINVAL;	
	} else if (sem == NULL) {  // Check if malloc returned null for the semaphore
		return -ENOMEM;
	}

	sem->value = value;  // initialize semaphore value
	sem->sem_id = sem_counter;  // initialize semaphore id
	spin_lock_init(&sem->lock);  // initialize spinlock
	INIT_LIST_HEAD(&sem->list);  // initialize previous and next pointers
	INIT_LIST_HEAD(&sem->waiting_tasks);  // initialize head and tail

	write_lock(&sem_rwlock);  // Lock the RWLock for Writing
	list_add(&sem->list, &sem_list);  // Insert to the Semaphore List
	sem_counter++;
	write_unlock(&sem_rwlock);  // Unlock the RWLock
	return sem->sem_id;
}

/**
 * Performs the down() operation on an existing semaphore
 * using the semaphore identifier obtained from a previous call
 * to cs1550_create().
 *
 * This decrements the value of the semaphore, and *may cause* the
 * calling process to sleep (if the semaphore's value goes below 0)
 * until up() is called on the semaphore by another process.
 *
 * Returns 0 when successful, or -EINVAL or -ENOMEM if an error
 * occurred.
 */
SYSCALL_DEFINE1(cs1550_down, long, sem_id)
{
	struct cs1550_sem *sem = NULL;  // some memory allocation and initialization routine
	struct cs1550_task *task_node = NULL;  // some memory allocation for a task
	int ret_state = 0;  // create a varable for various return state

	read_lock(&sem_rwlock);  // Lock the RWLock for reading
	list_for_each_entry(sem, &sem_list, list) {  // Search through Semaphore List for the given Semaphore
		if (sem->sem_id == sem_id) {  // find one semaphore that matches id argument
			break;
		}
	}
	spin_lock(&sem->lock);  // Lock the Spinlock for the Semaphore
	sem->value--;
	if(sem->value < 0){
		task_node = kmalloc(sizeof(struct cs1550_task), GFP_KERNEL);  // Allocate a task entry to the queue of waiting tasks
		INIT_LIST_HEAD(&task_node->list);  // initialize previous and next pointers
		list_add_tail(&task_node->list, &sem->waiting_tasks);  // insert a task entry to the queue of waiting tasks
		task_node->task = current;  // initialize task's pointer to the current task
		set_current_state(TASK_INTERRUPTIBLE);  // Change the Process State to Sleep
		spin_unlock(&sem->lock);  // Unlock the Spinlock
		schedule();  // Call the scheduler
		ret_state = 0;
	} else{
		spin_unlock(&sem->lock);  // Unlock the Spinlock
		ret_state = 0;
	}

	read_unlock(&sem_rwlock);  // Unlock the RWLock
	return ret_state;
}

/**
 * Performs the up() operation on an existing semaphore
 * using the semaphore identifier obtained from a previous call
 * to cs1550_create().
 *
 * This increments the value of the semaphore, and *may cause* the
 * calling process to wake up a process waiting on the semaphore,
 * if such a process exists in the queue.
 *
 * Returns 0 when successful, or -EINVAL if the semaphore ID is
 * invalid.
 */
SYSCALL_DEFINE1(cs1550_up, long, sem_id)
{
	struct cs1550_sem *sem;  // some memory allocation and initialization routine
	struct cs1550_task *temp_task; // some memory allocation for a task
	int ret_state = 0;  // create a varable for various return state

	read_lock(&sem_rwlock);  // Lock the RWLock for reading
	list_for_each_entry(sem, &sem_list, list) {  // Search through Semaphore List for the given Semaphore
		if (sem->sem_id == sem_id) {  // find one semaphore that matches id argument
			break;
		}
	}
	spin_lock(&sem->lock);  // Lock the Spinlock for the Semaphore
	sem->value++;
	if(sem->value <= 0){
		temp_task = list_first_entry(&sem->waiting_tasks, struct cs1550_task, list);  // get the first entry in a list
		list_del(&temp_task->list);  // Remove the head of the list
		wake_up_process(temp_task->task);  // Wakeup the process
		spin_unlock(&sem->lock);  // Unlock the Spinlock
		kfree(temp_task);  // Free its memory
		ret_state = 0;
	} else{
		spin_unlock(&sem->lock);  // Unlock the Spinlock
		ret_state = 0; 

	}
	
	read_unlock(&sem_rwlock);  // Unlock the RWLock
	return ret_state;
}





/**
 * Removes an already-created semaphore from the system-wide
 * semaphore list using the identifier obtained from a previous
 * call to cs1550_create().
 *
 * Returns 0 when successful or -EINVAL if the semaphore ID is
 * invalid or the semaphore's process queue is not empty.
 */
SYSCALL_DEFINE1(cs1550_close, long, sem_id)
{
	struct cs1550_sem *sem = NULL;  // some memory allocation and initialization routine
	int ret_state = 0;   // create a varable for various return state

	write_lock(&sem_rwlock);  // Lock the RWLock for writing
	list_for_each_entry(sem, &sem_list, list){  // Search through Semaphore List for the given Semaphore
		if (sem->sem_id == sem_id){  // find one semaphore that matches id argument
			break;
		}
	}
	spin_lock(&sem->lock);  // Lock the Spinlock for the Semaphore
	if(!list_empty(&sem->waiting_tasks)){  // If the queue is not empty
		ret_state = -EINVAL;
	} else{
		list_del(&sem->list);  // Remove the semaphore from the Semaphore list
		kfree(sem);  // Free its memory
		ret_state = 0;
	}

	spin_unlock(&sem->lock);  // Unlock the Spinlock
    write_unlock(&sem_rwlock);  // Unlock the RWLock
    return ret_state;
}
