
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"    /* For the message struct */
#include <iostream>
#include <fstream>


/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile";


/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory 
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	
	/*  1. Create a file called keyfile.txt containing string "Hello world" (you may do
 		    so manually or from the code).
	         2. Use ftok("keyfile.txt", 'a') in order to generate the key.
		 3. Use the key in the TODO's below. Use the same key for the queue
		    and the shared memory segment. This also serves to illustrate the difference
		    between the key and the id used in message queues and shared memory. The id
		    for any System V object (i.e. message queues, shared memory, and sempahores) 
		    is unique system-wide among all System V objects. Two objects, on the other hand,
		    may have the same key.
	 */



	key_t key = ftok("keyfile.txt", 'a');

	if (key == -1)  						/*error checking */
	{
		perror("ftok error from recv\n");
		exit(EXIT_FAILURE); 
	}
	/* Allocates a piece of shared memory. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. */
	/* Attach to the shared memory */
	/* Create a message queue */
	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */

	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, 0666);

	if(shmid == -1)								/*error checking */
	{
		perror("shmid error from recv\n");
		exit(EXIT_FAILURE); 
	}

	sharedMemPtr = shmat(shmid,(void*)0,0); 

	if(sharedMemPtr == (void*)(-1))
	{
		perror("sharedMemPtr error from recv\n");
		exit(EXIT_FAILURE); 
	}

	msqid = msgget(key, 0666 |IPC_CREAT);
	
	if(msqid == -1)
	{
		perror("msqid error from recv\n");
		exit(EXIT_FAILURE); 
	}
}
 

/**
 * The main loop
 */
void mainLoop()
{
	/* The size of the mesage */
	int msgSize = 0;
	
	/* Open the file for writing */
	FILE* fp = fopen(recvFileName, "w");
		
	/* Error checks */
	if(!fp)
	{
		perror("fopen");	
		exit(-1);
	}
		
    /* Receive the message and get the message size. The message will 
     * contain regular information. The message will be of SENDER_DATA_TYPE
     * (the macro SENDER_DATA_TYPE is defined in msg.h).  If the size field
     * of the message is not 0, then we copy that many bytes from the shared
     * memory region to the file. Otherwise, if 0, then we close the file and
     * exit.
     *
     * NOTE: the received file will always be saved into the file called
     * "recvfile"
     */

	message rcvMsg;       		//type defined in msg.h
	message sndMsg;

	sndMsg.mtype = RECV_DONE_TYPE;
	msgSize = rcvMsg.size; 
	
	/* Keep receiving until the sender set the size to 0, indicating that
 	 * there is no more data to send
 	 */	

	while(msgSize != 0)
	{	
		int val = msgrcv(msqid, &rcvMsg, sizeof(message) - sizeof(long), SENDER_DATA_TYPE, 0);

		if (val == -1)
		{
			perror("msgrcv erorr in recieve\n");
			exit(1);
		}
		else
		{
			printf("message recieved.\n");	
	
		}
		msgSize = rcvMsg.size; 
		/* If the sender is not telling us that we are done, then get to work */
		if(msgSize != 0)
		{
			/* Save the shared memory to file */
			if(fwrite(sharedMemPtr, sizeof(char), msgSize, fp) < 0)
			{
				perror("fwrite");
			}
			
			/* Tell the sender that we are ready for the next file chunk. 
 			 * I.e. send a message of type RECV_DONE_TYPE (the value of size field
 			 * does not matter in this case). 
 			 */

			int val2 = msgsnd(msqid, &sndMsg, sizeof(message) - sizeof(long), 0);

			if (val2 == -1)					/*error checking */
			{
				perror("msgsnd error in reciever\n");
				exit(EXIT_FAILURE);
			}
		}
		/* We are done */
		else
		{
			printf("Nothing else to read\nProgram is exiting\n");
			/* Close the file */
			fclose(fp);
		}
	}
}



/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* Detach from shared memory */	
	/*  Deallocate the shared memory chunk */	
	/*  Deallocate the message queue */

	shmdt(sharedMemPtr);
	shmctl(shmid, IPC_RMID, NULL);
	msgctl(msqid, IPC_RMID, NULL);
}

/**
 * Handles the exit signal
 * @param signal - the signal type
 */

void ctrlCSignal(int signal)
{
	/* Free system V resources */
	cleanUp(shmid, msqid, sharedMemPtr);

}

int main(int argc, char** argv)
{
	
	/* Installs a singnal handler (see signaldemo.cpp sample file).
 	 * In a case user presses Ctrl-c your program should delete message
 	 * queues and shared memory before exiting. You may add the cleaning functionality
 	 * in ctrlCSignal().
 	 */

	signal(SIGINT, ctrlCSignal);	

	/* Initialize */
	init(shmid, msqid, sharedMemPtr);
	
	/* Go to the main loop */
	mainLoop();

	/** Detach from shared memory segment, and deallocate shared memory and message queue (i.e. call cleanup) **/
	cleanUp(shmid, msqid, sharedMemPtr);	

	return 0;
}
