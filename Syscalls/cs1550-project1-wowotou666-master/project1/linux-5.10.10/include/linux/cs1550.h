#ifndef _LINUX_CS1550_H
#define _LINUX_CS1550_H

#include <linux/list.h>


struct cs1550_task
{
	struct list_head list;
	struct task_struct *task;
};

/**
 * A generic semaphore, providing serialized signaling and
 * waiting based upon a user-supplied integer value.
 * All code of this part is from README Guidance.
 */
struct cs1550_sem
{
	/* Current value. If nonpositive, wait() will block */
	long value;

	/* Sequential numeric ID of the semaphore */
	long sem_id;

	/* Per-semaphore lock, serializes access to value */
	spinlock_t lock;

	struct list_head waiting_tasks; //list of waiting processes

	struct list_head list; //pointers to next and previous semaphores

};


#endif
