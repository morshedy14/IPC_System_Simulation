#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Constants  
#define UP_QUEUE_KEY 1111
#define DOWN_QUEUE_KEY 2222
#define LOG_FILE "kernel_log.txt"
#define NUM_SLOTS 10
#define SLOT_SIZE 64

// Data structure for Disk  
typedef struct {
    char slots[NUM_SLOTS][SLOT_SIZE];
    int availableSlots;
    int clk;
} Disk;

// Define message structure  
typedef struct {
    long mtype;  // Message type
    int slotIdentifier;  // Slot identifier for DELETE request
    char data[100];  // Data for ADD request
} Message;

// Data structure for Kernel (updated with logging functionality)
typedef struct {
    int clk;
    FILE *logFile;  // File pointer for the log file
} Kernel;

// Function to initialize message queue  
int initMessageQueue(key_t key) {
    int msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    return msqid;
}

// Function to send a message to a queue  
void sendMessage(int msqid, long mtype, void *msg, size_t msgSize) {
    Message message;
    message.mtype = mtype;
    memcpy(&message, msg, msgSize);

    if (msgsnd(msqid, &message, msgSize, 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
}

// Function to receive a message from a queue  
void receiveMessage(int msqid, long mtype, void *msg, size_t msgSize) {
    Message message;

    if (msgrcv(msqid, &message, msgSize, mtype, 0) == -1) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }

    memcpy(msg, &message, msgSize);
}

// Function to initialize Disk  
void initializeDisk(Disk *disk) {
    // Initialize disk data
    for (int i = 0; i < NUM_SLOTS; ++i) {
        for (int j = 0; j < SLOT_SIZE; ++j) {
            disk->slots[i][j] = '\0';  // Initialize each character to null
        }
    }
    disk->availableSlots = NUM_SLOTS;  // All slots are initially available
    disk->clk = 0;  // Initialize the clock
}

// Function to handle data addition to Disk
void addDataToDisk(Disk *disk, Message *msg) {
    // Check if there are available slots
    if (disk->availableSlots > 0) {
        // Find an available slot
        int slotIndex = -1;
        for (int i = 0; i < NUM_SLOTS; ++i) {
            if (disk->slots[i][0] == '\0') {
                slotIndex = i;
                break;
            }
        }

        if (slotIndex != -1) {
            // Simulate latency (3 CLK cycles for addition)
            sleep(3);

            // Add data to the found slot
            strncpy(disk->slots[slotIndex], msg->data, SLOT_SIZE);
            disk->slots[slotIndex][SLOT_SIZE - 1] = '\0';  // Ensure null-termination

            // Update available slots and increment clock
            disk->availableSlots--;
            disk->clk++;
        } else {
            // Send a response indicating failure   
        }
    } else {
        // Send a response indicating failure   
    }
}

// Function to handle data deletion from Disk
void deleteDataFromDisk(Disk *disk, Message *msg) {
    // Extract slot identifier from the message
    int slotIdentifierToDelete = msg->slotIdentifier;

    // Find the corresponding slot index
    int slotIndex = -1;
    for (int i = 0; i < NUM_SLOTS; ++i) {
        if (strncmp(disk->slots[i], msg->data, SLOT_SIZE) == 0) {
            slotIndex = i;
            break;
        }
    }

    // Check if the slot index is valid
    if (slotIndex != -1) {
        // Simulate latency (1 CLK cycle for deletion)
        sleep(1);

        // Delete data from the specified slot
        memset(disk->slots[slotIndex], 0, SLOT_SIZE);

        // Update available slots and increment clock
        disk->availableSlots++;
        disk->clk++;
    } else {
        // Send a response indicating failure   
    }
}

// Function to handle data addition to Disk (updated without slot identifiers for data addition)
void addDataToDisk(Disk *disk, Message *msg) {
    // Check if there are available slots
    if (disk->availableSlots > 0) {
        // Find an available slot
        int slotIndex = findAvailableSlot(disk);

        if (slotIndex != -1) {
            // Simulate latency (3 CLK cycles for addition)
            sleep(3);

            // Add data to the found slot
            strncpy(disk->slots[slotIndex], msg->data, SLOT_SIZE);
            disk->slots[slotIndex][SLOT_SIZE - 1] = '\0';  // Ensure null-termination

            // Update available slots and increment clock
            disk->availableSlots--;
            disk->clk++;

            // Send a response to the kernel or user process  
            printf("Data added to slot %d\n", slotIndex);
        } else {
            // Send a response indicating failure   
            printf("Data addition failed: No available slots\n");
        }
    } else {
        // Send a response indicating failure   
        printf("Data addition failed: No available slots\n");
    }
}

// Function to handle data deletion from Disk (updated with slot identifiers)
void deleteDataFromDisk(Disk *disk, Message *msg) {
    // Extract slot identifier from the message
    int slotIdentifierToDelete = msg->slotIdentifier;

    // Find an available slot for compatibility with findAvailableSlot function
    int slotIndex = findAvailableSlot(disk);

    // Check if the slot index is valid
    if (slotIndex != -1) {
        // Simulate latency (1 CLK cycle for deletion)
        sleep(1);

        // Delete data from the specified slot
        memset(disk->slots[slotIndex], 0, SLOT_SIZE);

        // Update available slots and increment clock
        disk->availableSlots++;
        disk->clk++;

        // Send a response to the kernel or user process   
        printf("Data deleted from slot %d with identifier %d\n", slotIndex, slotIdentifierToDelete);
    } else {

        // Send a response indicating failure   
        printf("Data deletion failed: No available slots\n");
    }
}

// Function to handle Disk process
void diskProcess(int upQueue, int downQueue) {

    Disk disk;
    Message receivedMessage;
    Message responseMessage;

    // The disk receives data addition/deletion request on the DOWN queue 
    // ADD request -> mtype = 1
    // DEL request -> mtype = 2
    receiveMessage(upQueue, receivedMessage.mtype, &receivedMessage, sizeof(receivedMessage.data));
    if (receivedMessage.mtype == 1) { 
        addDataToDisk(&disk, &receivedMessage);
    } else if (receivedMessage.mtype == 2) { 
        deleteDataFromDisk(&disk, &receivedMessage);    // DEL FUNC SHOULD TAKE ONLY SLOT INDEX
    }
    // The disk responds to the kernel data addition/deletion requests with request status (success/faliure) on the DOWN queue 
    if(1){ //request status == success
    sendMessage(downQueue, responseMessage.mtype, &responseMessage, sizeof(responseMessage.data));
    }
    // The disk sends number of avaiable slots on UP queue to the kernel
    sendMessage(downQueue, responseMessage.mtype, &responseMessage, sizeof(responseMessage.data));
}

int main() {
    // Initialize message queues and other data structures
    int upQueue = initMessageQueue(UP_QUEUE_KEY);
    int downQueue = initMessageQueue(DOWN_QUEUE_KEY);

    // Initialize data structures
    Disk disk;
    Kernel kernel;
    initializeDisk(&disk);
    initializeKernel(&kernel);

    while (1) {
        diskProcess(upQueue, downQueue);
        // initializeKernel(&kernel);
        sleep(1);
    }

    return 0;
}
