#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "proghistory.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct
{
  struct spinlock lock;
  int progid;
  struct proghistory history[NHISTORY];
} runhistory;

void addpredictedtime(struct proc *);
void updateproghistory(struct proc *);

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}


void initSwapStructs(struct proc* p) {
  int i;
  for (i = 0; i < MAX_TOTAL_PAGES - MAX_PYSC_PAGES; i++)
    p->fileCtrlr[i].state = NOTUSED;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  acquire(&tickslock);
  p->time.start_time = ticks;
  release(&tickslock);
  p->time.run_time = 0;
  p->time.wait_time = 0;
  p->time.sleep_time = 0;
  p->time.n_context_switches = 0;

#if defined(SCHEDULER_MLFQ) && defined(MLFQ0)
  p->queue = 0;
  p->time.time_slice = MLFQ0;
#endif

#if defined(SCHEDULER_RR) && defined(RR0)
  p->time.time_slice = RR0;
#endif

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  p->loadOrderCounter = 0;
  p->time.faultCounter = 0;
  p->time.countOfPagedOut = 0;

  if (p->pid > 2)
    createSwapFile(p);

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

void updateproctime(void)
{

  acquire(&ptable.lock);

  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    switch (p->state)
    {
    case RUNNING:
      p->time.run_time++;
      break;

    case RUNNABLE:
      p->time.wait_time++;
      break;

    case SLEEPING:
      p->time.sleep_time++;
      break;

    default:
      break;
    }
  }

  release(&ptable.lock);
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  acquire(&runhistory.lock);
  runhistory.progid = 1;
  release(&runhistory.lock);

  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
#ifdef SCHEDULER_SJF
  addpredictedtime(p);
#endif
  p->cwd = namei("/");

  p->state = RUNNABLE;
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}


// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
    if (proc->pid > 2){
      copySwapFile(proc, np);
      np->loadOrderCounter = proc->loadOrderCounter;
      for (i = 0; i < MAX_PYSC_PAGES; i++){
        np->ramCtrlr[i] = proc->ramCtrlr[i]; //deep copies ramCtrlr list
        np->ramCtrlr[i].pgdir = np->pgdir;  //replace parent pgdir with child new pgdir
      }
      for (i = 0; i < MAX_TOTAL_PAGES-MAX_PYSC_PAGES; i++){
        np->fileCtrlr[i] = proc->fileCtrlr[i]; //deep copies fileCtrlr list
        np->fileCtrlr[i].pgdir = np->pgdir;   //replace parent pgdir with child new pgdir
      }
    }

  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

#ifdef SCHEDULER_SJF
  addpredictedtime(np);
#endif

  pid = np->pid;
  np->time.faultCounter = 0;
  np->time.countOfPagedOut = 0;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);
  return pid;
}

int forkandrename(char *name)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if ((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if ((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0)
  {
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  if (proc->pid > 2)
  {
    copySwapFile(proc, np);
    np->loadOrderCounter = proc->loadOrderCounter;
    for (i = 0; i < MAX_PYSC_PAGES; i++)
    {
      np->ramCtrlr[i] = proc->ramCtrlr[i]; // deep copies ramCtrlr list
      np->ramCtrlr[i].pgdir = np->pgdir;   // replace parent pgdir with child new pgdir
    }
    for (i = 0; i < MAX_TOTAL_PAGES - MAX_PYSC_PAGES; i++)
    {
      np->fileCtrlr[i] = proc->fileCtrlr[i]; // deep copies fileCtrlr list
      np->fileCtrlr[i].pgdir = np->pgdir;    // replace parent pgdir with child new pgdir
    }
  }

  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for (i = 0; i < NOFILE; i++)
    if (proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, name, sizeof(proc->name));

#ifdef SCHEDULER_SJF
  addpredictedtime(np);
#endif

  pid = np->pid;
  np->time.faultCounter = 0;
  np->time.countOfPagedOut = 0;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }
  if (proc->pid > 2) 
    removeSwapFile(proc);


  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;


  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  proc->state = ZOMBIE;
  acquire(&tickslock);
  proc->time.end_time = ticks;
  release(&tickslock);
#ifdef SCHEDULER_SJF
  updateproghistory(proc);
#endif

#if TRUE
  procdump();
#endif

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        int i;
        for (i = 0; i < MAX_PYSC_PAGES; i++)
          p->ramCtrlr[i].state = NOTUSED;
        for (i = 0; i < MAX_TOTAL_PAGES-MAX_PYSC_PAGES; i++)
          p->fileCtrlr[i].state = NOTUSED;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

int waitandgettime(struct proctime *time)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for (;;)
  {
    // Scan through table looking for zombie children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != proc)
        continue;
      havekids = 1;
      if (p->state == ZOMBIE)
      {
        // Found one.

        time->start_time = p->time.start_time;
        time->end_time = p->time.end_time;
        time->run_time = p->time.run_time;
        time->wait_time = p->time.wait_time;
        time->sleep_time = p->time.sleep_time;
        time->n_context_switches = p->time.n_context_switches;
        time->predicted_time = p->time.predicted_time;

        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        int i;
        for (i = 0; i < MAX_PYSC_PAGES; i++)
          p->ramCtrlr[i].state = NOTUSED;
        for (i = 0; i < MAX_TOTAL_PAGES - MAX_PYSC_PAGES; i++)
          p->fileCtrlr[i].state = NOTUSED;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || proc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock); // DOC: wait-sleep
  }
}

void swtch_process(struct proc *p, struct cpu *c)
{
  c->proc = p;

  switchuvm(p);
  p->state = RUNNING;
  swtch(&(c->scheduler), p->context);
  switchkvm();

  c->proc = 0;
}

void addpredictedtime(struct proc *np)
{
  acquire(&runhistory.lock);

#if defined(SJFDEF)
  struct proghistory *prog;

  int flag = 0;
  for (prog = runhistory.history; prog < &runhistory.history[NHISTORY]; prog++)
  {
    if (prog->state == EMPTY)
      continue;

    if (strncmp(prog->name, np->name, sizeof(prog->name)))
      continue;

    flag = 1;
    np->time.predicted_time = prog->predicted_time;
    np->progid = prog->progid;
    break;
  }

  if (!flag)
  {
    np->time.predicted_time = SJFDEF;
    np->progid = ++runhistory.progid;
  }
#endif

  release(&runhistory.lock);

  return;
}

float getalfa(void)
{
#ifdef ALFA

  int x = ALFA;
  if (x == 1)
    return 0;
  else if (x == 2)
    return 0.3;
  else if (x == 3)
    return 0.5;
  else if (x == 4)
    return 0.7;
  else if (x == 5)
    return 1;

#endif
  return 0.5;
}

void updateproghistory(struct proc *p)
{
  acquire(&runhistory.lock);
  struct proghistory *prog;
  int flag = 0;
  for (prog = runhistory.history; prog < &runhistory.history[NHISTORY]; prog++)
  {
    if (prog->state == EMPTY)
      continue;
    if (p->progid != prog->progid)
      continue;

    flag = 1;
    float a = getalfa();
    prog->last_predicted_time = prog->predicted_time;
    prog->predicted_time = (p->time.run_time * a) + ((1 - a) * prog->predicted_time);
    prog->last_run_time = p->time.run_time;
    break;
  }

  if (flag)
  {
    release(&runhistory.lock);
    return;
  }

  for (prog = runhistory.history; prog < &runhistory.history[NHISTORY]; prog++)
  {
    if (prog->state == USED)
      continue;

    prog->state = USED;
    safestrcpy(prog->name, p->name, sizeof(p->name));
    prog->last_predicted_time = prog->predicted_time;
    prog->predicted_time = p->time.run_time;
    prog->last_run_time = p->time.run_time;
    prog->progid = p->progid;
    break;
  }

  release(&runhistory.lock);
}

void SJF(void)
{
  struct proc *p;
  struct proc *shortest = 0;
  struct cpu *c = cpu;
  c->proc = 0;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state != RUNNABLE)
      continue;
    if (!shortest)
    {
      shortest = p;
      continue;
    }
    if (p->time.predicted_time < shortest->time.predicted_time)
      shortest = p;
  }
  if (shortest != 0)
    swtch_process(shortest, c);
  return;
}

void MLFQ(void)
{
  struct proc *p;
  struct proc *p2;
  struct cpu *c = cpu;
  c->proc = 0;
q0:
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state == RUNNABLE && p->queue == 0)
      swtch_process(p, c);
  }

q1:
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    for (p2 = ptable.proc; p2 < &ptable.proc[NPROC]; p2++)
    {
      if (p->state == RUNNABLE && p->queue < 1)
        goto q0;
    }

    if (p->state == RUNNABLE && p->queue == 1)
      swtch_process(p, c);
  }

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    int lowestqueue = 2;
    for (p2 = ptable.proc; p2 < &ptable.proc[NPROC]; p2++)
    {
      if (p->state == RUNNABLE && p->queue < lowestqueue)
        lowestqueue = p->queue;
    }

    if (lowestqueue == 0)
      goto q0;

    if (lowestqueue == 1)
      goto q1;

    if (p->state == RUNNABLE && p->queue == 2)
      swtch_process(p, c);
  }

  return;
}

void RR(void)
{
  struct proc *p;
  struct cpu *c = cpu;
  c->proc = 0;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state != RUNNABLE)
      continue;
    swtch_process(p, c);
  }

  return;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void)
{
  for (;;)
  {
    // Enable interrupts on this processor.
    sti();

    acquire(&ptable.lock);

#if defined(SCHEDULER_RR)
    RR();
#elif defined(SCHEDULER_SJF)
    SJF();
#elif defined(SCHEDULER_MLFQ)
    MLFQ();
#endif

    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  struct proc *p = proc;
  p->time.n_context_switches++;
  p->state = RUNNABLE;
#if defined(SCHEDULER_MLFQ) && defined(MLFQ0) && defined(MLFQ1) && defined(MLFQ2)
  if (p->queue < 2)
  {
    p->queue += 1;
  }

  if (p->queue == 1)
  {
    p->time.time_slice = MLFQ1;
  }
  else if (p->queue == 2)
  {
    p->time.time_slice = MLFQ2;
  }
#endif

#if defined(SCHEDULER_RR) && defined(RR0)
  p->time.time_slice = RR0;
#endif
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot 
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      acquire(&tickslock);
      p->time.end_time = ticks;
      release(&tickslock);
#ifdef SCHEDULER_SJF
      updateproghistory(p);
#endif
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int getPagedOutAmout(struct proc* p){
 
  int i;
  int amout = 0;

  for (i=0;i < MAX_PYSC_PAGES; i++){
    if (p->fileCtrlr[i].state == INUSE)
      amout++;
  }
  return amout;
}

void updateLap(){
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->pid > 2 && p->state > 1 && p->state < 5) //proc is either running, runnable or sleeping
      updateAccessCounters(p); //implemented in vm.c
  }
  release(&ptable.lock);
}

int printpagingstat(void)
{
  struct proc *p = proc;
  int allocatedPages;
  int pagedOutAmount;

  allocatedPages = PGROUNDUP(p->sz)/PGSIZE;
  pagedOutAmount = getPagedOutAmout(p);
  cprintf("\nname: %s, pid: %d, alocatedpages: %d, pagedout: %d, faltcounter: %d, coutofpagedout: %d\n",
  p->name,
  p->pid,
  allocatedPages,
  pagedOutAmount,
  p->time.faultCounter,
  p->time.countOfPagedOut);
  cprintf("%d/%d free pages in the system\n",getFreePages(),getTotalPages());
  return 1;
}


//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.

void procdump(void){
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  int allocatedPages;
  int pagedOutAmount;
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

    allocatedPages = PGROUNDUP(p->sz)/PGSIZE;
    pagedOutAmount = getPagedOutAmout(p);
    cprintf("\npid: %d, state: %s, alocatedpages: %d, pagedout: %d, faltcounter: %d, coutofpagedout: %d, name: %s\n", p->pid, state, allocatedPages,
            pagedOutAmount, p->time.faultCounter, p->time.countOfPagedOut, p->name);

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }

    cprintf("\n");
  }
  cprintf("%d/%d free pages in the system\n",getFreePages(),getTotalPages());


}
