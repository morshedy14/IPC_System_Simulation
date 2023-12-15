#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Constants (as before)
#define UP_QUEUE_KEY 1111
#define DOWN_QUEUE_KEY 2222
#define NUM_SLOTS 10
#define SLOT_SIZE 64
#define LOG_FILE "kernel_log.txt"

#define MAX_OPERATION_LEN 20
#define MAX_MESSAGE_LEN 64

// IORequest data structure to store I/O requests from input file
typedef struct {
    int step;
    char operation[MAX_OPERATION_LEN];
    char message[MAX_MESSAGE_LEN];
} IORequest;


// Data structure for User Process (as before)
typedef struct {
    int clk;
} UserProcess;

// Define message structure (as before)
typedef struct {
    long mtype;  // Message type
    int slotIdentifier;  // Slot identifier for DELETE request
    char data[100];  // Data for ADD request
} Message;



// Function to load I/O requests from file
IORequest* loadIORequests(const char *filename, int *numLines) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    int lineCount = 0;
    IORequest *ioRequests = malloc(sizeof(IORequest));
    if (ioRequests == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while (fscanf(file, "%d %s \"%99[^\"]\"\n",
                  &ioRequests[lineCount].step,
                  ioRequests[lineCount].operation,
                  ioRequests[lineCount].message) == 3) {
        lineCount++;

        IORequest *temp = realloc(ioRequests, (lineCount + 1) * sizeof(IORequest));
        if (temp == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }

        ioRequests = temp;
    }

    fclose(file);
    *numLines = lineCount;
    return ioRequests;
}


// Function to initialize message queue (as before)
int initMessageQueue(key_t key) {
    int msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    return msqid;
}

// Function to send a message to a queue (as before)
void sendMessage(int msqid, long mtype, void *msg, size_t msgSize) {
    Message message;
    message.mtype = mtype;
    memcpy(&message, msg, msgSize);

    if (msgsnd(msqid, &message, msgSize, 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
}

// Function to receive a message from a queue (as before)
void receiveMessage(int msqid, long mtype, void *msg, size_t msgSize) {
    Message message;

    if (msgrcv(msqid, &message, msgSize, mtype, 0) == -1) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }

    memcpy(msg, &message, msgSize);
}

// Function to initialize User Process (declaration and definition)
void initializeUserProcess(UserProcess *userProcess) {
    // Implementation of initialization logic for User Process
    // ...
}

// Function to handle User Process (as before)
void userProcess(UserProcess *userProcess, int upQueue, int downQueue, FILE *ioRequestsFile) {
    // Implementation of the User Process logic
    // ...
}

int main() {
    int numLines;
    IORequest *ioRequests = loadIORequests("io_requests.txt", &numLines);
    
    // Initialize message queues and other data structures
    int upQueue = initMessageQueue(UP_QUEUE_KEY);
    int downQueue = initMessageQueue(DOWN_QUEUE_KEY);

    // // Open the I/O requests file
    // FILE *ioRequestsFile = fopen("io_requests.txt", "r");
    // if (ioRequestsFile == NULL) {
    //     perror("fopen");
    //     exit(EXIT_FAILURE);
    // }

    // Initialize data structures
    UserProcess userProc;
    initializeUserProcess(&userProc);

    // // Execute simulation
    // while (1/* Some exit condition */) {
    //     // Simulate User Process I/O requests
    //     userProcess(&userProc, upQueue, downQueue, ioRequestsFile);

    //     // Add delay or synchronization mechanism
    //     sleep(1);
    // }

    

    // Close other resources and exit
    // ...
    free(ioRequests); // free the allocated memory for I/O requests
    return 0;
}



