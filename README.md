# Modified xv6-riscv

# Introduction

xv6-riscv is a teaching operating system developed in the summer of 2006 for MIT's operating systems course by the professors of MIT. The following addition features have been added to it:

- [strace system call](#strace)
- [FCFS scheduling](#fcfs)
- [PBS scheduling](#pbs)
- [setpriority system call](#setpriority)
- [modified procdump system call](#procdump )

----

# Requirements

1. GCC compiler
2. GNU make

----

# Set-Up Instructions

1. Clone the repository to your local system by executing the following command in your shell:

```sh
https://github.com/siddharth-mavani/Modified-xv6-riscv
```
2. cd into the cloned repository.

```sh
cd xv6-rsicv
```

----

# Running the OS

- This OS has 3 different types of schedulers:
    1. Round Robin (Default)
    2. First Come First Serve
    3. Priority Based Scheduling

- To use Round Robin, execute `make qemu` or `make qemu SCHEDULER=RR` in the terminal.

- To use First Come First Serve, execute `make qemu SCHEDULER=FCFS` in the terminal.

- To use Priority Based Scheduling, execute `make qemu SCHEDULER=PBS` in the terminal.

**NOTE:** Before switching to a different scheduler, run `make clean` in the terminal

----

# strace

It intercepts and records the system calls which are called by a process during its execution.

It should take one argument, an integer mask, whose bits specify which system calls to trace.

syntax: 

```
strace <mask> <command> [args]
```

```
- mask: a bit mask of the system calls to trace.
- command: the command to execute.
- args: the arguments to the command.
```

1. Added `$U/_strace` to the `UPROGS` in the Makefile.

2. Added a variable `mask` in `kernel/proc.h` and initialized it in the `fork()` function in `kernel/proc.c` to make a copy of the trace mask from the parent to the child process.

```c
np->mask = p->mask;
```

3. Added a function `sys_trace()` in `kernel/sysproc.c`. This function retrieves the mask that is passed as an argument from user space to kernel space.

```c
uint64
sys_trace()
{
  int mask;
  int arg_num = 0;

  if(argint(arg_num, &mask) >= 0)
  {
    myproc() -> mask = mask;
    return 0;
  }
  else
  {
    return -1;
  }  
}
```

4. Created the `strace.c` file in the `user` directory to generate the user-space stubs for the system call. This file allows user to access the strace systme call.

```c
#include "../kernel/types.h"
#include "../kernel/param.h"
#include "../kernel/stat.h"
#include "./user.h"

int is_num(char s[])
{
    for(int i = 0; s[i] != '\0' ; i++)
    {
        if(s[i] < '0' || s[i] > '9')
        {
            return 0;
        }
    }

    return 1;
}


int main(int argc, char*argv[])
{
    char *nargv[MAXARG];

    int check;
    check = is_num(argv[1]);
    
    // checking for incorrect input
    if(argc < 3 || check == 0)
    {
        fprintf(2, "Incorrect Input !\n");
        fprintf(2, "Correct usage: strace <mask> <command>\n");
        exit(1);
    }

    int temp, temp_num;
    temp_num = atoi(argv[1]);
    temp = trace(temp_num);

    if(temp < 0)
    {
        fprintf(2, "strace: trace failed\n");
    }

    // copying the command
    for(int i = 2; i < argc && i < MAXARG; i++)
    {
        nargv[i - 2] = argv[i];
    }

    exec(nargv[0], nargv);   // executing command

    return 0;
}
```

5. Modified the `syscall()` function in `kernel/syscall.c` to show details about each system call. Also added `syscall_names` and `syscall_num` arrays in the same file to help with this.

```c
void
syscall(void)
{
  int num, num_args;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  num_args = syscall_num[num];
  
  int arr[num_args];
  for(int i = 0; i < num_args ; i++){
    arr[i] = argraw(i);
  }

  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();
    if((p -> mask >> num) & 1)
    {
      printf("%d: syscall %s (", p -> pid, syscall_names[num]);

      for(int i = 0; i < syscall_num[num]; i++)
      {
        printf("%d ", arr[i]);
      }

      printf("\b");
      printf(") -> %d", argraw(0));  
      printf("\n");
    }
  } else {
    printf("%d %s: unknown sys call %d\n",p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
```

6. Added a new stub to `user/usys.pl`, which makes the Makefile invoke the script `user/usys.pl` and produce `user/usys.S`, and a syscall number to `kernel/syscall.h.`

----

# FCFS

First Come First Serve (FCFS) is a scheduling algorithm that automatically executes queued requests and processes `in order of their arrival`. It is the easiest and simplest CPU scheduling algorithm. In this type of algorithm, processes which requests the CPU first get the CPU allocation first. This is a `non-preemptive` algorithm.

1. Modified the `Makefile` to support `SCHEDULER` macro for the compilation of the specified scheduling algorithm.

```
ifndef SCHEDULER
SCHEDULER := RR
endif
```

```
CFLAGS += -D $(SCHEDULER)
```

2. Added `create_time` as a parameter in `struct proc` which stores when a process was spawned. This is present in `kernel/proc.h`

3. Initialised `create_time` to `ticks` in `allocproc()` function in `kernel/proc.c`

```c
p->create_time = ticks;
```

4. Added the implementation of `FCFS` in the `scheduler()` function present in `kernel/proc.c`

```c
#ifdef FCFS

  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){

    intr_on();

    struct proc* first_proc;
    first_proc = 0;
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        if(!first_proc || p->create_time < first_proc->create_time){
          
          if(first_proc != 0){
            release(&first_proc->lock);
          }

          first_proc = p;
          continue;
        }
      }
      release(&p->lock);
    }

    if(first_proc != 0){
      
      first_proc->state = RUNNING;
      c->proc = first_proc;
      swtch(&c->context, &first_proc->context);

      c->proc = 0;
      release(&first_proc->lock);
    }
  }

#endif
```

5. Disabled `yield()` function in `kernel/trap.c` to prevent preemption using macros.

```c
// FCFS & PBS are non-preemptive
#if defined RR || defined MLFQ
  if(which_dev == 2)
    yield();  
#endif
```

```c
// FCFS & PBS are non-preemptive
#if defined RR || defined MLFQ
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();
#endif
```

----

# PBS

Priority Based Scheduling (PBS) is a scheduling algorithm that executes queued requests and processes `based on priority`. This is a `non-preemptive` algorithm.

Priority depends on `niceness` and `Static Priority(SP)`.

`Static Priority(SP)` can be in the range `[0,100]`. The `defualt value` is `60`. Lower the value, higher the priority. This can be modified using `setpriority` system call.

`niceness` can be in the range `[0,10]`. This measuers the percentage of time the process was sleeping. This is calculated in the following manner:

```
niceness = Int( ticks spent in sleeping state * 10/ticks spent in (sleeping + running) state)
```

The final priority, also called `Dynamic Priority(DP)` is calculated by:

```
DP = max(0, min(SP - niceness + 5, 0))
```

1. Added variables `run_time`, `start_time`, `sleep_time`, `n_runs`, and `priority` to `struct proc` in `kernel/proc.h`

```c
uint64 run_time;             // Run-Time
uint64 start_time;           // Start-Time
uint64 sleep_time;           // Sleep-Time
uint64 n_runs;               // Number of times the process ran
uint64 priority;             // Priority of the Process;
```

2. Initialised the above variables in `allocproc() ` function in `kernel/proc.c`

```c
p->run_time = 0;
p->start_time = 0;
p->sleep_time = 0;
p->n_runs = 0;
p->priority = 60;         // Default Priority is 60
```

3. Added the implementation of `PBS` in the `scheduler()` function present in `kernel/proc.c`

```c
#ifdef PBS

  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    struct proc* high_priority_proc;
    high_priority_proc = 0;
    int dynamic_priority = 101;     // Lower dynamic_priority value => higher preference in scheduling

    for(p = proc; p < &proc[NPROC]; p++) {

      acquire(&p->lock);

      int nice;

      if(p->run_time + p->sleep_time > 0){
        nice = p->sleep_time * 10;
        nice = nice / (p->sleep_time + p->run_time);
      }
      else{
        nice = 5;               // Defualt value of nice;
      }

      int curr_dynamic_priority;
      curr_dynamic_priority = max(0, min(p->priority - nice + 5, 100));

      if(p->state == RUNNABLE){

        int dp_check = 0;
        int check_1 = 0, check_2 = 0;
        
        if(dynamic_priority == curr_dynamic_priority){
          dp_check = 1;
        }

        // If 2 processes have same dynamic priority, we check for number of times the process has been scheduled
        if(dp_check && p->n_runs < high_priority_proc->n_runs){
          check_1 = 1;
        }

        // If 2 processes have same dynamic priority and number of runs
        // we check for creation time
        if(dp_check && high_priority_proc->n_runs == p->n_runs && p->create_time < high_priority_proc->create_time){
          check_2 = 1;
        }
        
        if(high_priority_proc == 0 || curr_dynamic_priority > dynamic_priority || (dp_check && check_1) || check_2){
          
          if(high_priority_proc != 0){
            release(&high_priority_proc->lock);
          }

          dynamic_priority = curr_dynamic_priority;
          high_priority_proc = p;
          continue;

        }
      }
      
      release(&p->lock);

    }

    if(high_priority_proc != 0){

      high_priority_proc->state = RUNNING;
      high_priority_proc->start_time = ticks;
      high_priority_proc->run_time = 0;
      high_priority_proc->sleep_time = 0;
      high_priority_proc->n_runs += 1;

      c->proc = high_priority_proc;
      swtch(&c->context, &high_priority_proc->context);

      c->proc = 0;
      release(&high_priority_proc->lock);

    }
  }

#endif
```

----

# setpriority

This sytem call `updates` the `Static Priority` of a system call based on its `PID`

syntax:

```c
setpriority <priority> <pid>
```

This system call `returns` the `old Static Priority` of the process.

1. Added `$U/_setpriority` to the `UPROGS` in the Makefile.

2. Added a function `sys_setpriority()` in `kernel/sysproc.c`. This function retrieves the pid and priority that is passed as an argument from user space to kernel space.

```c
uint64
sys_setpriority()
{
  int pid, priority;
  int arg_num[2] = {0, 1};

  if(argint(arg_num[0], &priority) < 0)
  {
    return -1;
  }
  if(argint(arg_num[1], &pid) < 0)
  {
    return -1;
  }
   
  return setpriority(priority, pid);
}
```

3. Created the `setpriority.c` file in the `user` directory to generate the user-space stubs for the system call. This file allows user to access the setpriority system call.

```c
#include "../kernel/types.h"
#include "../kernel/param.h"
#include "../kernel/stat.h"
#include "./user.h"

int check_priority_range(int priority){

    if(priority >= 0 || priority <= 100){
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]){
    
    if(argc < 3){
        fprintf(2, "Incorrect Input !\n");
        fprintf(2, "Correct usage: setpriority <priority> <pid>\n");
        exit(1);
    }

    int priority = atoi(argv[1]);
    int pid = atoi(argv[2]);

    if(check_priority_range(priority) == 0){
        fprintf(2, "Incorrect Range !\n");
        fprintf(2, "Correct Range: [0,100]\n");
        exit(1);
    }

    setpriority(priority, pid);
    exit(1);
}
```

4. Added a new function `setpriority()` in `kernel/proc.c`

```c
int
setpriority(int new_priority, int pid)
{
  int prev_priority;
  prev_priority = 0;

  struct proc* p;
  for(p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);

    if(p->pid == pid)
    {
      prev_priority = p->priority;
      p->priority = new_priority;

      p->sleep_time = 0;
      p->run_time = 0;

      int reschedule = 0;

      if(new_priority < prev_priority){
        reschedule = 1;
      }

      release(&p->lock);
      if(reschedule){
        yield();
      }

      break;
    }
    release(&p->lock);
  }
  return prev_priority;
}
```

5. Added a new stub to `user/usys.pl`, which makes the Makefile invoke the script `user/usys.pl` and produce `user/usys.S`, and a syscall number to `kernel/syscall.h.`

----

# procdump 

1. Added new attributes `total_run_time`, `wait_time` and `exit_time` in `struct proc` in `kernel/proc.h`.

```c
uint64 total_run_time;       // Total Run-Time
uint64 wait_time;            // Wait-Time
uint64 exit_time;            // Exit-Time
```

2.  Initialised the above variables in `allocproc() ` function in `kernel/proc.c`

```c
p->total_run_time = 0;
p->exit_time = 0;
```

3. `exit_time` is set to `ticks` in `exit()` function in `kernel/proc.c` once it becomes a `zombie process`

```c
p->exit_time = ticks;
```

4. Modified `procdump()` in `kernel/proc.c` to print more information about a process.

```c
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runnable",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

#if defined RR || defined FCFS 

    int wait_time;

    if(p->exit_time > 0){
      wait_time = p->exit_time - p->create_time - p->total_run_time;
    }
    else{
       wait_time = ticks - p->create_time - p->total_run_time;
    }
    printf("%d %s %d %d %d", p->pid, state, p->total_run_time, wait_time, p->n_runs);

#endif

#ifdef PBS

    int wait_time;

    if(p->exit_time > 0){
      wait_time = p->exit_time - p->create_time - p->total_run_time;
    }
    else{
       wait_time = ticks - p->create_time - p->total_run_time;
    }

    printf("%d %d %s %d %d %d", p->pid, p->priority, state, p->total_run_time, wait_time, p->n_runs);
#endif

    printf("\n");
  }
}
```

----

# Analysis

- used the `schedulertest` system call to find the average run time and wait time of a process in different scheduling algorithms.

| Scheduling Algorithm | Run Time | Wait Time |
|:-----------:|:------------:|:------:|
| Round Robin (RR) | 57 | 137 |
| First Come First Serve (FCFS) | 96 | 139 |
| Priority Based Scheduling (PBS) | 59 | 123 |

----

# Original README


xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6).  xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.828/, which
provides pointers to on-line resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Takahiro Aoyagi, Silas Boyd-Wickizer, Anton Burtsev, Ian Chen, Dan
Cross, Cody Cutler, Mike CAT, Tej Chajed, Asami Doi, eyalz800, Nelson
Elhage, Saar Ettinger, Alice Ferrazzi, Nathaniel Filardo, flespark,
Peter Froehlich, Yakir Goaron,Shivam Handa, Matt Harvey, Bryan Henry,
jaichenhengjie, Jim Huang, Matúš Jókay, Alexander Kapshuk, Anders
Kaseorg, kehao95, Wolfgang Keller, Jungwoo Kim, Jonathan Kimmitt,
Eddie Kohler, Vadim Kolontsov , Austin Liew, l0stman, Pavan
Maddamsetti, Imbar Marinescu, Yandong Mao, , Matan Shabtay, Hitoshi
Mitake, Carmi Merimovich, Mark Morrissey, mtasm, Joel Nider,
OptimisticSide, Greg Price, Jude Rich, Ayan Shafqat, Eldar Sehayek,
Yongming Shen, Fumiya Shigemitsu, Cam Tenny, tyfkda, Warren Toomey,
Stephen Tu, Rafael Ubal, Amane Uehara, Pablo Ventura, Xi Wang, Keiichi
Watanabe, Nicolas Wolovick, wxdao, Grant Wu, Jindong Zhang, Icenowy
Zheng, ZhUyU1997, and Zou Chang Wei.

The code in the files that constitute xv6 is
Copyright 2006-2020 Frans Kaashoek, Robert Morris, and Russ Cox.

ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
operating system for MIT's 6.S081, so we are more interested in
simplifications and clarifications than new features.

BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu. Once they are installed, and in your shell
search path, you can run "make qemu".
