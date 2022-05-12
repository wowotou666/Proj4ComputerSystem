# Pros & Cons of using a FIFO queue for the global semaphore list
Wentao Wu

# Pros
FIFO queues are simpler to implement and have less cost. There is no starvation because every process has a chance to run, which means no context switch. Semaphores allow flexible resource management.
In the worst case, the efficiency of FIFO queue is better than that of hash table.

# Cons
FIFO is nonpreemptive. Once a large task has a processor, it will cause many small processes to wait, filling up the ready queue. It will not be preempted even after a long time, which makes the little people have to wait for a long time, thus reducing the efficiency of the system. We must insert and search in the right way, otherwise using semaphores can easily lead to deadlocks.
In most cases, the time complexity of hash table is better than that of FIFO queue, which is O(1).
