#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Pcb.h"
#include "Fifo.h"

/******defines*****/
#define TIMER_INTERRUPT 	1
#define TERMINATE_INTERRUPT 2
#define IO_INTERRUPT 		3
#define IO_REQUEST 			4

#define TIMER_QUANTUM 		300
#define NEW_PROCS_LOWER 	3
#define NEW_PROCS_UPPER 	3
#define PRIORITY_LEVELS 	16
#define ROUNDS_TO_PRINT 	4 // the number of rounds to wait before printing simulation data
#define SIMULATION_END 		5000 //the number of instructions to execute before the simulation may end
#define MIN_PC_INCREMENT 	3000
#define PC_INCREMENT_RANGE 	1000
#define NEW_PROCS			5

/*Defines a range for how much greater it takes for IO to complete compared with the time quantum.*/
#define UPPER_IO_TIME_RATIO 5
#define LOWER_IO_TIME_RATIO 3

/****Global variables*****/
int currPID; //The number of processes created so far. The latest process has this as its ID.
int timerCount;
unsigned int sysStackPC;
FifoQueue* newProcesses;
FifoQueue* readyProcesses;
FifoQueue* terminatedProcesses;

/*Groups data needed for a single device*/
typedef struct Device {
	FifoQueue* waitQ;
	int counter;
} Device;

Device device1;
Device device2;

PcbPtr currProcess;


/*Prepares the waiting process to be executed.*/
void dispatcher() {
	currProcess = fifoQueueDequeue(readyProcesses);
	PCBSetState(currProcess, running);
	sysStackPC = PCBGetPC(currProcess);
}

/*Based on the type of interrupt indicated,
  decides what to do with the current process.*/
void scheduler(int interruptType) {
	switch (interruptType)  {
	case TIMER_INTERRUPT:
		PCBSetState(currProcess, ready);
		//Get new processes into ready queue
		int i;
		for (i = 0; i < newProcesses->size; i++) {
			PcbPtr pcb = fifoQueueDequeue(newProcesses);
			PCBSetState(pcb, ready);
			fifoQueueEnqueue(readyProcesses, pcb);
			printf("%s\r\n", PCBToString(pcb));
		}
		fifoQueueEnqueue(readyProcesses, currProcess);
		dispatcher();
		break;
	case TERMINATE_INTERRUPT:
		fifoQueueEnqueue(terminatedProcesses, currProcess);
		PCBSetState(currProcess, terminated);
		PCBSetTermination(currProcess, (double)clock());
		dispatcher();
		break;
	}
}

/*Randomly generates between 0 and 5 new processes and enqueues them to the New Processes Queue.*/
void genProcesses() {
	PcbPtr newProc;
	int i;
	// rand() % NEW_PROCS will range from 0 to NEW_PROCS - 1, so we must use rand() % (NEW_PROCS + 1)
	for(i = 0; i < rand() % (NEW_PROCS + 1); i++)
	{
		newProc = PCBConstructor();
		if(newProc != NULL)	// Remember to call the destructor when finished using newProc
		{
			currPID++;
			PCBSetID(newProc, currPID);
			PCBSetPriority(newProc, rand() % PRIORITY_LEVELS);
			PCBSetState(newProc, created);
			fifoQueueEnqueue(newProcesses, newProc);
		}
	}
}

/*Saves the state of the CPU to the currently running PCB.*/
void saveCpuToPcb() {
	PCBSetPC(currProcess, sysStackPC);
}

void terminateIsr() {
	//save cpu to pcb??
	PCBSetState(currProcess, interrupted);
	scheduler(TERMINATE_INTERRUPT);
//	scheduler(TERMINATE_INTERRUPT, -1); //-Tabi
}

/**If there's a process waiting for IO from this device, and the counter
 * is at 0, then resets the timer.*/
void setIOTimer(Device* d) {
	if (fifoQueuePeek(d->waitQ) && d->counter == 0)
		d->counter = (rand()
				% ((UPPER_IO_TIME_RATIO - LOWER_IO_TIME_RATIO) * TIMER_QUANTUM))
				+ (LOWER_IO_TIME_RATIO * TIMER_QUANTUM);
}


/* I was assigned a IO_ISR Method. I assume we need it to
 * do something, but I dont know how relevant it is now.
 * The interrupt service routine for a IO interrupt
 * Does it still have to save Cpu state to Pcb? */
void IO_ISR(int numIO) { //IOCompletionHandler
    saveCpuToPcb();
    PCBSetState(currProcess, interrupted);
    //Get process from waiting queue
    PcbPtr pcb;
    if (numIO == 1) {
        pcb = fifoQueueDequeue(device1.waitQ);
    }
    else if (numIO == 2) {
        pcb = fifoQueueDequeue(device2.waitQ);
    }
    fifoQueueEnqueue(readyProcesses, pcb);
    PCBSetState(pcb, ready);
    scheduler(IO_REQUEST);
}

/*TODO:PUT THIS IN SHEDULER - Sean
 * PCBSetState(currProcess, running);
 */

/*Returns 0 if there is no interrupt; 1 otherwise.*/
int checkIOInterrupt(Device* d) {
	int interrupt = 0;
//TODO check if pc of head of d.waitQ equals sysstack pc
	return interrupt;
}


/**Makes a new request to device*/
void IOTrapHandler(Device* d) {
	saveCpuToPcb();
	PCBSetState(currProcess, blocked);
	fifoQueueEnqueue(d->waitQ, currProcess);
	setIOTimer(d);
	scheduler(IO_REQUEST);
}

//returns 0 if there's no io request, nonzero if request was made.
int checkIORequest(int devnum) {
	int requestMade = 0;
	int i;
	if (currProcess) {
		if (devnum == 1) { //look through array 1
			for (i=0; i < NUM_IO_TRAPS; i++) {
				requestMade = get_IO_1_Trap(currProcess, i) == sysStackPC? 1: requestMade;
			}
		}
		else if (devnum == 2) { //look through array 2
			for (i=0; i < NUM_IO_TRAPS; i++) {
				requestMade = get_IO_2_Trap(currProcess, i) == sysStackPC? 1: requestMade;
			}
		}
	}
	return requestMade;
} 

void timerIsr() {
//TODO
}

/*Returns 1 if there should be a timer interrupt; 0 otherwise.*/
int timerCheck() {
	int timerWentOff = 0;
//TODO determine if timer should go off
	return timerWentOff;
}

 
int main(void) {
	srand(time(NULL));
	currPID = 0;
	sysStackPC = 0;
	timerCount = TIMER_QUANTUM;
	//unsigned int PCRegister;
	newProcesses = fifoQueueConstructor();
	readyProcesses = fifoQueueConstructor();
	terminatedProcesses = fifoQueueConstructor();
	device1.waitQ = fifoQueueConstructor();
	device2.waitQ = fifoQueueConstructor();

	/*
	//An initial process to start with
	currProcess = PCBConstructor();
	PCBSetPC(currProcess, rand());
	PCBSetID(currProcess, currPID);
	PCBSetPriority(currProcess, rand() % PRIORITY_LEVELS);
	PCBSetState(currProcess, running);
	currPID++;
	//PCRegister = currProcess->PC;

	 */

	printf("Generating Processes:\n");
	genProcesses();
	int i;
	for (i = 0; i < newProcesses->size; i++) {
		PcbPtr pcb = fifoQueueDequeue(newProcesses);
		PCBSetState(pcb, ready);
		fifoQueueEnqueue(readyProcesses, pcb);
	}
	dispatcher();

	printf("Begin Simulation:\n");
	printf("Currently running process: %s\n\n", PCBToString(currProcess));

	int simCounter = 0;
	while (simCounter <= SIMULATION_END) {

		//check for timer interrupt, if so, call timerISR()
		if (timerCheck()) {
			printf("A timer interrupt has occurred.\n");

			timerIsr();

			printf("readyProcesses current state: %s\n", fifoQueueToString(readyProcesses));
			printf("Currently running process: %s\n\n", PCBToString(currProcess));
		}

		//check if there has been an IO interrupt, if so call appropriate ioISR
		if (checkIOInterrupt(&device1)) {
			printf("I/O 1 device has thrown an interrupt.\n");

			//call the IO service routine
			IO_ISR(1);

			printf("waitQueue1 current state: %s\n\n", fifoQueueToString(device1.waitQ));
		}
		if (checkIOInterrupt(&device2)) {
			printf("I/O 2 device has thrown an interrupt.\n");

			//call the IO service routine
			IO_ISR(2);

			printf("waitQueue2 current state: %s\n\n", fifoQueueToString(device2.waitQ));
		}

		//check the current process's PC, if it is MAX_PC, set to 0 and increment TERM_COUNT
		if (PCBGetPC(currProcess) == PCBGetMaxPC(currProcess)) {
			PCBSetTermCount(currProcess, PCBGetTermCount(currProcess) + 1);

			//if TERM_COUNT = TERMINATE, then call terminateISR to put this process in the terminated list
			if (PCBGetTermCount(currProcess) == PCBGetTerminate(currProcess)) {
				printf("process being terminated\n");

				terminateIsr();

				printf("Terminated process: %s\n", PCBToString(currProcess));
				printf("terminatedProcesses current state: %s\n", fifoQueueToString(terminatedProcesses));
				printf("readyProcesses current state: %s\n", fifoQueueToString(readyProcesses));
				printf("Currently running process: %s\n\n", PCBToString(currProcess));

				continue;	//currProcess has been terminated, we don't want to execute the rest of the loop, instead jump to next iteration
			}
			PCBSetPC(currProcess, 0);
		}

		//increment the current process's PC
		PCBSetPC(currProcess, PCBGetPC(currProcess)+1);

		if (checkIORequest(1)) {
			printf("I/O 1 request\n");
			IOTrapHandler(&device1);
			printf("waitQueue1 current state: %s\n", fifoQueueToString(device1.waitQ));
			printf("Currently running process: %s\n\n", PCBToString(currProcess));
		}

		if (checkIORequest(2)) {
			printf("I/O 2 request\n");
			IOTrapHandler(&device2);
			printf("waitQueue2 current state: %s\n", fifoQueueToString(device2.waitQ));
			printf("Currently running process: %s\n\n", PCBToString(currProcess));
		}

/*********The following is replaced by the above calls to checkIORequest -Tabi*************/
		//check the current process's PC to see if it's an I/O, if so, call ioISR on appropriate I/O device
//		if (currProcess->PC == currProcess->IO_1_TRAPS[0] ||
//				currProcess->PC == currProcess->IO_1_TRAPS[1] ||
//				currProcess->PC == currProcess->IO_1_TRAPS[2] ||
//				currProcess->PC == currProcess->IO_1_TRAPS[3]) {
//
//			printf("I/O 1 request\n");
//
//			trapServiceHandler(1);
//
//			printf("waitQueue1 current state: %s\n", fifoQueueToString(waitQueue1));
//			printf("Currently running process: %s\n\n", PCBToString(currProcess));
//		} else if (currProcess->PC == currProcess->IO_2_TRAPS[0] ||
//				currProcess->PC == currProcess->IO_2_TRAPS[1] ||
//				currProcess->PC == currProcess->IO_2_TRAPS[2] ||
//				currProcess->PC == currProcess->IO_2_TRAPS[3]) {
//
//			printf("I/O 2 request\n");
//
//			trapServiceHandler(2);
//
//			printf("waitQueue2 current state: %s\n", fifoQueueToString(waitQueue2));
//			printf("Currently running process: %s\n\n", PCBToString(currProcess));
//		}
/********-Tabi**********/

		//at end
		simCounter++;
	}

	printf("End of simulation\n");
	return 0;
}
