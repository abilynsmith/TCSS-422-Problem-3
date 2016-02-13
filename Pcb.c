/***********************************************************************************************
* Pcb.c
*
* Programming Team:
* Sean Markus
* Wing-Sea Poon
* Abigail Smith
* Tabi Stein
*
* Date: 1/22/16
*
* Description:
* This C file implements the class and methods for the process control block.
*
************************************************************************************************/

#include "Pcb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

const char* stateNames[] = {"Created","Running","Ready","Interrupted","Blocked","Terminated"};

typedef struct PCB {
	int PID;
	int priority;
	State state;
	unsigned int PC;
	unsigned int maxPC;
	unsigned long int creation;
	unsigned long int termination;
	int terminate;
	unsigned int term_count;
	unsigned int IO_1_Traps[NUM_IO_TRAPS];
	unsigned int IO_2_Traps[NUM_IO_TRAPS];
} PcbStr;


unsigned int PCBGetIO1Trap(PcbStr* pcb, int index) {
	if (index < NUM_IO_TRAPS) {
		return pcb->IO_1_Traps[index];
	} else {
		return -1;
	}
}

unsigned int PCBGetIO2Trap(PcbStr* pcb, int index) {
	if (index < NUM_IO_TRAPS) {
		return pcb->IO_2_Traps[index];
	} else {
		return -1;
	}
}

char* StateToString(State state) {
	int len = strlen(stateNames[state]);
	char* string = malloc(sizeof(char) * len + 1);
	sprintf(string, "%s", stateNames[state]); //auto appends null at end
	return string;
}

void PCBSetPriority(PcbStr* pcb, int priority) {
	pcb->priority = priority;

}

void PCBSetID(PcbStr* pcb, int id) {
	pcb->PID = id;
}

void PCBSetState(PcbStr* pcb, State newState) {
	pcb->state = newState;
}

/**
 * Sets the PC for this PCB.
 */
void PCBSetPC(PcbStr* pcb, unsigned int newPC) {
	pcb->PC = newPC;
}

void PCBSetTermination(PcbStr* pcb, unsigned long newTermination) {
	pcb->termination = newTermination;
}

void PCBSetTerminate(PcbStr* pcb, int newTerminate) {
	pcb->terminate = newTerminate;
}

void PCBSetTermCount(PcbStr* pcb, unsigned int newTermCount) {
	pcb->term_count = newTermCount;
}

/**
 * Returns PC of this PCB.
 */
unsigned int PCBGetPC(PcbStr* pcb) {
	return pcb->PC;
}

int PCBGetPriority(PcbStr* pcb) {
	return pcb->priority;
}

int PCBGetID(PcbStr* pcb) {
	return pcb->PID;
}

State PCBGetState(PcbStr* pcb) {
	return pcb->state;
}

unsigned int PCBGetMaxPC(PcbStr* pcb) {
	return pcb->maxPC;
}

unsigned long PCBGetCreation(PcbStr* pcb) {
	return pcb->creation;
}

unsigned long PCBGetTermination(PcbStr* pcb) {
	return pcb->termination;
}

int PCBGetTerminate(PcbStr* pcb) {
	return pcb->terminate;
}

unsigned int PCBGetTermCount(PcbStr* pcb) {
	return pcb->term_count;
}


/**Generates a value greater or equal to min and less or equal to max*/
/*
int genLarger(int min, int max) {
	//srand(time(NULL));
	return (rand() % (max - min + 1)) + min;
}
*/

/*
 *How it works:
 *ex if we want 8 unique values in range from 1-20, inclusive, when we generate the first value,
 *it cannot be greater than 13 or we will not have enough unique values for the next 7 integers.
 *hence, the maximum value for the 1st integer is (20-8+1), for 2nd is (20-8+2), etc. Then to generalize,
 *the ith integer (starting at i=1) can be at most (maxVal-n+i). Hence if we start at i=0, we have
 *the formula (maxVal-n+i+1)*/
/*
void genNUniqueValsInRange(int n, unsigned int* storage, int minVal, int maxVal) {

	int i=0;
	storage[i++] = genLarger(minVal, maxVal - n + 1);
	for (; i <= n; i++) {
		storage[i] = genLarger(storage[i-1], maxVal-n+i+1);
	}

}
*/

PcbPtr PCBConstructor(unsigned int startPc){
	PcbStr* pcb = (PcbStr*) malloc(sizeof(PcbStr));
	pcb->PC = 0;
	pcb->PID = 1;
	pcb->priority = 1;
	pcb->state = created;
	pcb->creation = time(NULL);
	pcb->maxPC = 2000;
	pcb->terminate = rand()%10;	//ranges from 0-10
	pcb->term_count = 0;

	//genIOArrays(pcb);

	unsigned int* allTraps = malloc(sizeof(unsigned int) * NUM_IO_TRAPS * 2);
	//genNUniqueValsInRange(NUM_IO_TRAPS * 2, allTraps, 0, pcb->maxPC);
	genTraps(NUM_IO_TRAPS * 2, allTraps, 0, pcb->maxPC);

	int i;
	for (i = 0; i < NUM_IO_TRAPS; i++) {
		pcb->IO_1_Traps[i] = allTraps[i*2];		//grab even indices
		pcb->IO_2_Traps[i] = allTraps[(i*2) + 1]; //grab odd indices
	}

	free(allTraps);

	return pcb;
}


/*
 * Partitions (maxVal - minVal) into n non-overlapping partitions.
 * Sets storage[i] to a random number from the corresponding partition.
 *
 * Ex.: n = 8, minVal = 0, maxVal = 2000
 * Partition Size = 250
 * partition[0] = 0 to 249
 * partition[1] = 250 to 499
 * ...
 * partition[7] = 1750 to 1999
 */
void genTraps(int n, unsigned int* storage, int minVal, int maxVal) {
	int partitionSize = (maxVal - minVal) / n;	// truncate if the division results in a double
	int i;

	for(i = 0; i < n; i++) {
		storage[i] = (rand() % (partitionSize)) + (i * partitionSize);
	}
}

void genIOArrays(PcbPtr pcb) {
	int quarterMax = pcb->maxPC/4;


	pcb->IO_1_Traps[0] = rand()%quarterMax;//generate a random number that's in the first 1/4 of MAX_PC
	pcb->IO_1_Traps[1] = rand()%quarterMax + quarterMax;//random number between 1/4-2/4 of MAX_PC
	pcb->IO_1_Traps[2] = rand()%quarterMax + 2 * quarterMax;//random number between 2/4-3/4 of MAX_PC
	pcb->IO_1_Traps[3] = rand()%quarterMax + 3 * quarterMax;//random number between 3/4-end of MAX_PC

	pcb->IO_2_Traps[0] = rand()%quarterMax;
	while (pcb->IO_2_Traps[0] == pcb->IO_1_Traps[0]) {//the while loops are to ensure no duplicates between the two arrays
		pcb->IO_2_Traps[0] = rand()%quarterMax;
	}
	pcb->IO_2_Traps[1] = rand()%quarterMax + quarterMax;
	while (pcb->IO_2_Traps[1] == pcb->IO_1_Traps[1]) {
		pcb->IO_2_Traps[1] = rand()%quarterMax + quarterMax;
	}
	pcb->IO_2_Traps[2] = rand()%quarterMax + 2 * quarterMax;
	while (pcb->IO_2_Traps[2] == pcb->IO_1_Traps[2]) {
		pcb->IO_2_Traps[2] = rand()%quarterMax + 2 * quarterMax;
	}
	pcb->IO_2_Traps[3] = rand()%quarterMax + 3 * quarterMax;
	while (pcb->IO_2_Traps[3] == pcb->IO_1_Traps[3]) {
		pcb->IO_2_Traps[3] = rand()%quarterMax + 3 * quarterMax;
	}

 }

/**Need at most 5 chars for each 8 traps, plus 8 spaces before each, or 48*/
char* TrapsToString(PcbStr* pcb) {
	char* arrStr = (char*) malloc(sizeof(char) * (48 + 10 + 1));
	arrStr[0] = '\0';

	char dev1[sizeof(char) * (20 + 4 + 1)];
	dev1[0] = '\0';
	char dev2[sizeof(char) * (20 + 4 + 1)];
	dev2[0] = '\0';

	int i;
	for (i = 0; i < NUM_IO_TRAPS; i++) {
		char buffer[sizeof(char) * (5+2)]; //5 is max number of digits in unsigned int, + null + space
		sprintf(buffer, " %d", pcb->IO_1_Traps[i]);
		strncat(dev1,buffer,7);
		char buffer2[sizeof(char) * (5+2)];
		sprintf(buffer2, " %d", pcb->IO_2_Traps[i]);
		strncat(dev2,buffer2,7);
	}
	sprintf(arrStr, "d1[%s ] d2[%s ]", dev1, dev2);
	return arrStr;
}


char *PCBToString(PcbStr* pcb) {
	if (pcb == NULL)
		return NULL;

	char * emptyStr = (char*) malloc(sizeof(char) * 1000);
	emptyStr[199] = '\0';
	char* stateString = StateToString(pcb->state);
	char* trapString = TrapsToString(pcb);
	int lenNeeded = sprintf(emptyStr, "ID: %d, Priority: %d, State: %s, PC: %d"
			", MAX_PC %d"
			", CREATION %lu"
			", TERMINATE %d"
			", TERM_COUNT %d"
			", \n	Traps: (%s)"
							,pcb->PID, pcb->priority, stateString, pcb->PC
							,pcb->maxPC
							,pcb->creation
							,pcb->terminate
							, pcb->term_count
							, trapString
							);
	free(stateString);
	free(trapString);
	char * retString = (char *) malloc(sizeof(char) * lenNeeded);
	sprintf(retString, "%s", emptyStr);
	free(emptyStr);
	return retString;
}



void PCBDestructor(PcbPtr pcb) {
	free (pcb);
	pcb = NULL;	//Only locally sets the pointer to null
}


////test pcb
//int main(void) {
//	srand(time(NULL));
//	PcbStr* pcb = PCBConstructor(0);
//	printf("%s", PCBToString(pcb));
//	return 0;
//}
