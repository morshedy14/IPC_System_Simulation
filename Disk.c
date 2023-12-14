#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

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


// Function to get request status
void getRequestStatus(Disk *disk, Message *responseMessage, int success) {
    if (success) {
        responseMessage->mtype = 3;  // Success
    } else {
        responseMessage->mtype = 4;  // Failure
    }
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
        }
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
    }
}


// Function to handle Disk process
void diskProcessHandler(int upQueue, int downQueue) {
    Disk disk;
    initializeDisk(&disk);

    while (1) {
        // The disk receives data addition/deletion request from kernel on the DOWN queue
        Message receivedMessage;
        receiveMessage(downQueue, receivedMessage.mtype, &receivedMessage, sizeof(receivedMessage.data));

        if (receivedMessage.mtype == 1) { // Disk received ADD request
            addDataToDisk(&disk, &receivedMessage);
        } else if (receivedMessage.mtype == 2) { // Disk received DEL request
            deleteDataFromDisk(&disk, &receivedMessage);
        }

        // Respond to the kernel with the request status on the DOWN queue
        Message responseMessage;
        getRequestStatus(&disk, &responseMessage, 3); // 3-> success condition
        sendMessage(downQueue, responseMessage.mtype, &responseMessage, sizeof(Message));

        // The disk sends the number of available slots on UP queue to the kernel
        Message availableSlotsMessage;
        getRequestStatus(&disk, &availableSlotsMessage, 1);
        sendMessage(upQueue, availableSlotsMessage.mtype, &availableSlotsMessage, sizeof(Message));

        sleep(1);
    }
}

// Signal handler for SIGUSR1 (Disk status request)
void sigusr1Handler(int signal, Disk *disk) {
    // Respond to the Kernel's request for Disk status
    // Send a message on the UP queue containing the number of available slots
    Message availableSlotsMessage;
    getRequestStatus(disk, &availableSlotsMessage, 1); // Appropriate success condition
    sendMessage(UP_QUEUE_KEY, availableSlotsMessage.mtype, &availableSlotsMessage, sizeof(Message));
}

// Signal handler for SIGUSR2 (Clock synchronization)
void sigusr2Handler(int signal, Disk *disk) {
    // Increment the local clk variable to synchronize with the Kernel
    disk->clk++;
}

int main() {
    // Initialize message queues and other data structures
    int upQueue = initMessageQueue(UP_QUEUE_KEY);
    int downQueue = initMessageQueue(DOWN_QUEUE_KEY);

    // Initialize Disk
    Disk disk;
    initializeDisk(&disk);

    // Register signal handlers
    signal(SIGUSR1, sigusr1Handler);
    signal(SIGUSR2, sigusr2Handler);

    while (1) {
        // The disk receives data addition/deletion request from kernel on the DOWN queue
        Message receivedMessage;
        receiveMessage(downQueue, receivedMessage.mtype, &receivedMessage, sizeof(receivedMessage.data));

        if (receivedMessage.mtype == 1) { // Disk received ADD request
            addDataToDisk(&disk, &receivedMessage);
        } else if (receivedMessage.mtype == 2) { // Disk received DEL request
            deleteDataFromDisk(&disk, &receivedMessage);
        }

        // Respond to the kernel with the request status on the DOWN queue
        Message responseMessage;
        getRequestStatus(&disk, &responseMessage, 3); // 3-> success condition
        sendMessage(downQueue, responseMessage.mtype, &responseMessage, sizeof(Message));

        // The disk sends the number of available slots on UP queue to the kernel
        sigusr1Handler(SIGUSR1, &disk);

        // Increment the clock for synchronization
        sigusr2Handler(SIGUSR2, &disk);

    }

    return 0;
}