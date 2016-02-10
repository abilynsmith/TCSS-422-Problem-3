//defines
#define TIMER_INTERRUPT 1
#define TERMINATE_INTERRUPT 2
#define IO_INTERRUPT 3
#define TIMER_QUANTUM 300
#define NEW_PROCS_LOWER 3
#define NEW_PROCS_UPPER 3
#define PRIORITY_LEVELS 16
#define ROUNDS_TO_PRINT 4 // the number of rounds to wait before printing simulation data
#define SIMULATION_END 5000 //the number of instructions to execute before the simulation may end
#define MIN_PC_INCREMENT 3000
#define PC_INCREMENT_RANGE 1000


//Global variables
int currPID; //The number of processes created so far. The latest process has this as its ID.
//int timerCount; //the counter for timer interrupts
int io1Count; //the counter for I/O 1
int io2Count; //the counter for I/O 2
int timerCount;
unsigned int sysStackPC;
FifoQueue* newProcesses;
FifoQueue* readyProcesses;
FifoQueue* terminatedProcesses;
FifoQueue* waitQueue1;
FifoQueue* waitQueue2;
PcbPtr currProcess;
FILE* outFilePtr;

//Scheduler
//switch/case for TERMINATE_INTERRUPT
/*
case TERMINATE_INTERRUPT :
		fifoQueueEnqueue(terminatedProcesses, currProcess);
		PCBSetState(currProcess, terminated);
		currProcess->TERMINATION = (double)clock();
		dispatcher();
		break;
*/

void terminateIsr() {
	//save cpu to pcb??
	PCBSetState(currProcess, interrupted);
	scheduler(TERMINATE_INTERRUPT, -1);
}



/* I was assigned a IO_ISR Method. I assume we need it to
 * do something, but I dont know how relevant it is now.
 * The interrupt service routine for a IO interrupt
 * Does it still have to save Cpu state to Pcb? */
void IO_ISR(int numIO) {
	saveCpuToPcb();
	PCBSetState(currProcess, interrupted);
	//Get process from waiting queue
	PcbPtr pcb;
	if (numIO == 1) {
		pcb = fifoQueueDequeue(wait_queue1);
	}
	else if (numIO == 2) {
		pcb = fifoQueueDequeue(wait_queue2);
	}
	//put current process into ready queue
	fifoQueueEnqueue(readyProcesses, currProcess);
	PCBSetState(currProcess, ready);
	//set new io waiting queue process to running
	currProcess = pcb;
}

/*TODO:PUT THIS IN SHEDULER - Sean
 * PCBSetState(currProcess, running);
 */

/**Makes a new request to device*/
void IOTrapHandler(struct device* d) {
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
 
int main(void) {
	srand(time(NULL));
	outFilePtr = fopen("scheduleTrace.txt", "w");
	currPID = 0;
	sysStackPC = 0;
	io1Count = -1;
	io2Count = -1;
	timerCount = TIMER_QUANTUM;
	//unsigned int PCRegister;
	newProcesses = fifoQueueConstructor();
	readyProcesses = fifoQueueConstructor();
	terminatedProcesses = fifoQueueConstructor();
	waitQueue1 = fifoQueueConstructor();
	waitQueue2 = fifoQueueConstructor();

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
		//fprintf(outFilePtr, "%s\r\n", PCBToString(pcb));
	}
	dispatcher();

	printf("Begin Simulation:\n");
	printf("Currently running process: %s\n\n", PCBToString(currProcess));

	int simCounter = 0;

	while (simCounter <= SIMULATION_END) {

		//check for timer interrupt, if so, call timerISR()
		if (timerCheck() == 1) {
			printf("A timer interrupt has occurred.\n");

			timerIsr();

			printf("readyProcesses current state: %s\n", fifoQueueToString(readyProcesses));
			printf("Currently running process: %s\n\n", PCBToString(currProcess));
		}

		//check if there has been an IO interrupt, if so call appropriate ioISR
		if (io1Check() == 1) {
			printf("I/O 1 device has thrown an interrupt.\n");

			//call the IO service routine
			IO_ISR(1);

			printf("waitQueue1 current state: %s\n\n", fifoQueueToString(waitQueue1));
		}
		if (io2Check() == 1) {
			printf("I/O 2 device has thrown an interrupt.\n");

			//call the IO service routine
			IO_ISR(2);

			printf("waitQueue2 current state: %s\n\n", fifoQueueToString(waitQueue2));
		}

		//check the current process's PC, if it is MAX_PC, set to 0 and increment TERM_COUNT
		if (currProcess->PC == currProcess->MAX_PC) {
			currProcess->TERM_COUNT++;

			//if TERM_COUNT = TERMINATE, then call terminateISR to put this process in the terminated list
			if (currProcess->TERM_COUNT == currProcess->TERMINATE) {
				printf("process being terminated\n");

				terminateIsr();

				printf("Terminated process: %s\n", PCBToString(currProcess));
				printf("terminatedProcesses current state: %s\n", fifoQueueToString(terminatedProcesses));
				printf("readyProcesses current state: %s\n", fifoQueueToString(readyProcesses));
				printf("Currently running process: %s\n\n", PCBToString(currProcess));

				continue;	//currProcess has been terminated, we don't want to execute the rest of the loop, instead jump to next iteration
			}
			currProcess->PC = 0;
		}

		//increment the current process's PC
		currProcess->PC++;

		//check the current process's PC to see if it's an I/O, if so, call ioISR on appropriate I/O device
		if (currProcess->PC == currProcess->IO_1_TRAPS[0] ||
				currProcess->PC == currProcess->IO_1_TRAPS[1] ||
				currProcess->PC == currProcess->IO_1_TRAPS[2] ||
				currProcess->PC == currProcess->IO_1_TRAPS[3]) {

			printf("I/O 1 request\n");

			trapServiceHandler(1);

			printf("waitQueue1 current state: %s\n", fifoQueueToString(waitQueue1));
			printf("Currently running process: %s\n\n", PCBToString(currProcess));
		} else if (currProcess->PC == currProcess->IO_2_TRAPS[0] ||
				currProcess->PC == currProcess->IO_2_TRAPS[1] ||
				currProcess->PC == currProcess->IO_2_TRAPS[2] ||
				currProcess->PC == currProcess->IO_2_TRAPS[3]) {

			printf("I/O 2 request\n");

			trapServiceHandler(2);

			printf("waitQueue2 current state: %s\n", fifoQueueToString(waitQueue2));
			printf("Currently running process: %s\n\n", PCBToString(currProcess));
		}

		//at end
		simCounter++;
	}

	printf("End of simulation\n");
	return 0;
}
