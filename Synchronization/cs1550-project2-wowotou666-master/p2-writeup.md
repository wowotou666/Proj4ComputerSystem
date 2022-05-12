# Project2 Report
Wentao Wu

# Deadlock
Very unfortunately, deadlock exists in my assignment. Therefore, I failed to pass all tests on Gradescope . I've spent four or five days on it and I haven't been able to fix this logical error. I think I'm using mutex correctly to prevent deadlocks as much as possible. According to the information displayed by gdb, my program should have a problem when the tour guide left, and there are still visitors in it at that time, thus forming a block. In this case, I want to avoid deadlock conditions as much as possible.It's a pity that I didn't end up solving this problem.

# Starvation
There is no starvation in my assignment. Once tickets are successfully obtained and there is a corresponding number of satisfied guides, visitors will be admitted. My thread is not blocked or lost by itself. To prevent starvation, all my variables are based on global shared variables. I did not appear to obtain the required resources, resulting in a state that has been unable to execute, thus avoiding starvation. Unlike deadlock, starvation will eventually execute threads with low priority for a period of time. For example, threads with high priority will release resources after execution.
