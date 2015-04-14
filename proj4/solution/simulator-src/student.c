/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 4
 * Fall 2014
 *
 * This file contains the CPU scheduler for the simulation.
 * Name:
 * GTID:
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"

#define DEBUG 0

#if DEBUG
 #define DEBUG_PRINTF(o) printf(o)
 #define DEBUG_PRINTFVAR(o, v) printf(o, v)
#else
 #define DEBUG_PRINTF(o) printf("")
 #define DEBUG_PRINTFVAR(o, v) printf("")
#endif

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pcb_t* head;
static pcb_t* tail;

static pthread_mutex_t current_mutex;
static pthread_mutex_t ready_mutex;

static pthread_cond_t quit_idle;

static int round_robin;
static int static_priority;
static int time_slice;
static int cpu_count; 


/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */

static void add_to_ready_queue(pcb_t* process);
static void add_with_priority(pcb_t* process);

static void schedule(unsigned int cpu_id)
{
    DEBUG_PRINTF("schedule() called\n");

    pthread_mutex_lock(&ready_mutex);
    DEBUG_PRINTF("schedule() mutex obtained\n");
    if(head == NULL) {
        DEBUG_PRINTF("schedule() head is null\n");
        pthread_mutex_unlock(&ready_mutex);
        context_switch(cpu_id, NULL, time_slice);
    } else {
        DEBUG_PRINTF("schedule() head not null\n");
        head->state = PROCESS_RUNNING;
        pcb_t* process = head;

        if (head == tail) {
            head = NULL;
            tail = NULL;
        } else {
            head = head->next;
        }

        pthread_mutex_unlock(&ready_mutex);

        pthread_mutex_lock(&current_mutex);
        DEBUG_PRINTF("schedule() current mutex obtained \n");
        current[cpu_id] = process;
        pthread_mutex_unlock(&current_mutex);

        
        context_switch(cpu_id, process, time_slice);
    }
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    DEBUG_PRINTF("idle() called\n");
    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */ 

     pthread_mutex_lock(&current_mutex);
     current[cpu_id] = NULL;
     pthread_mutex_unlock(&current_mutex);

     pthread_mutex_lock(&ready_mutex);
     DEBUG_PRINTF("idle() mutex obtained\n");
     while (head == NULL) {
        pthread_cond_wait(&quit_idle, &ready_mutex);
     }
     pthread_mutex_unlock(&ready_mutex);

     DEBUG_PRINTF("idle() mutex released");
     schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    DEBUG_PRINTF("preempt() called\n");
    pcb_t* process = NULL;

    pthread_mutex_lock(&current_mutex);
    DEBUG_PRINTF("preempt() current mutex obtained \n");

    process = current[cpu_id];
    current[cpu_id] = NULL;

    pthread_mutex_unlock(&current_mutex);

    if (process != NULL) {
        process->state = PROCESS_READY;
        if (static_priority) {
            add_with_priority(process);
        } else {
            add_to_ready_queue(process);
        }
    }

    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    DEBUG_PRINTF("yield() called\n");
    pthread_mutex_lock(&current_mutex);
    DEBUG_PRINTF("yield() mutex obtained\n");

    if (current[cpu_id]) {
        current[cpu_id]->state = PROCESS_WAITING;
    }

    current[cpu_id] = NULL;

    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    DEBUG_PRINTF("terminate() called\n");
    pthread_mutex_lock(&current_mutex);
    DEBUG_PRINTF("terminate() mutex obtained\n");

    pcb_t* process = current[cpu_id];
    current[cpu_id] = NULL;
    process->state = PROCESS_TERMINATED;

    pthread_mutex_unlock(&current_mutex);

    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{   
    DEBUG_PRINTF("wake_up() called\n");

    process->state = PROCESS_READY;

    int processFree = 0; //bool
    int cpuToUse = 0;
    int lowestPriority = 11;

    if (static_priority) {
        pthread_mutex_lock(&current_mutex);
        DEBUG_PRINTF("wake_up() current mutex obtained\n");

        int i;
        for (i = 0; i < cpu_count; i++) {
            if (current[i] == NULL) {
                DEBUG_PRINTF("wake_up() Found a free cpu\n");
                processFree = 1;
                cpuToUse = i;
                break;
            }
        }

        DEBUG_PRINTF("wake_up() blank space\n");

        if (!processFree) {
            DEBUG_PRINTF("wake_up() no CPU free, finding best one\n");
            for (i = 0; i < cpu_count; i++) {
                DEBUG_PRINTFVAR("wake_up() CPU count: %d\n", i);
                if (current[i] != NULL && current[i]->static_priority < lowestPriority) {
                    DEBUG_PRINTF("wake_up() found valid replacement \n");
                    lowestPriority = current[i]->static_priority;
                    cpuToUse = i;
                }
            }
        }

        DEBUG_PRINTFVAR("wake_up() best CPU: %d\n", cpuToUse);

        pthread_mutex_unlock(&current_mutex);
    } else {
        processFree = 1; // easy break
    }


    if (static_priority) {
        add_with_priority(process);
    } else {
        add_to_ready_queue(process);
    }

    

    if(!processFree && process->static_priority > lowestPriority ) {
        DEBUG_PRINTFVAR("add_with_priority() forcing preempt for %d\n", cpuToUse);
        force_preempt(cpuToUse);
    }    
}

static void add_to_ready_queue(pcb_t* process) {
    DEBUG_PRINTF("add_to_ready_queue() called\n");
    pthread_mutex_lock(&ready_mutex);
    DEBUG_PRINTF("add_to_ready_queue() mutex entered\n");
    process->state = PROCESS_READY;
    if (head == NULL) {
        head = process;
        tail = process;
        process->next = NULL;
        pthread_cond_broadcast(&quit_idle);
    } else {
        tail->next = process;
        tail = process;
        process->next = NULL;
    }

    pthread_mutex_unlock(&ready_mutex);
}

static void add_with_priority(pcb_t* process) {
    DEBUG_PRINTF("add_with_priority() called\n");
    pthread_mutex_lock(&ready_mutex);
    DEBUG_PRINTF("add_with_priority() ready mutex obtianed \n");

    if (head == NULL) {
        head = process;
        tail = process;
        process->next = NULL;
        pthread_cond_broadcast(&quit_idle);
        DEBUG_PRINTF("add_with_priority() head is null\n");
    } else {
        DEBUG_PRINTF("add_with_priority() head is not null\n");
        
        if (process->static_priority > head->static_priority) {
            process->next = head;
            head = process;
        } else {
            pcb_t* curr = head;
            while (curr->next != NULL && curr->static_priority >= process->static_priority) {
                curr = curr->next;
            }
            process->next = curr->next;
            curr->next = process;

            if (curr == tail) {
                DEBUG_PRINTF("add_with_priority() updating tail \n");
                tail = process;
                tail->next = NULL;
            }
        }    
    }
    
    pthread_mutex_unlock(&ready_mutex); 
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{ 
    DEBUG_PRINTF("STARTING\n");

    current = NULL;
    head = NULL;
    tail = NULL;
    round_robin = 0;
    static_priority = 0;
    time_slice = -1;

    /* Parse command-line arguments */
    if (argc < 2)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }

    cpu_count = atoi(argv[1]);
    DEBUG_PRINTFVAR("NUM ARGS: %d\n", argc);
    DEBUG_PRINTFVAR("CPU COUNT: %d\n", cpu_count);


    if (argc == 4 && strcmp(argv[2],"-r")==0) {
        round_robin = 1;
        time_slice = atoi(argv[3]);
        DEBUG_PRINTFVAR("TIMESLICE: %d\n (in 100s of ms)", time_slice);
    } else if (argc == 3 && strcmp(argv[2], "-p") == 0) {
        static_priority = 1;
        DEBUG_PRINTF("Using static priority\n");
    } else if (argc != 2) {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
 

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&ready_mutex, NULL);
    pthread_cond_init(&quit_idle, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


