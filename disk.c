#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#define MAX_SLOTS 10 
#define SLOT_SIZE 64
int msg_up, msg_down;
int clk = 0;
int available_slots = 10;
typedef struct {
    char data[MAX_SLOTS][SLOT_SIZE];
    int used_slots;
} Storage;
struct ClkMessage {
    long msg_type;
    int clk;
};

void waitopperation( int slice) 
{
    int time = clk ;
    while (1) 
    {
        if (clk>=time+slice) 
        {
            break; // Break the loop when the desired time has passed
        }
    }
}


void initializeStorage(Storage *storage) {
    for (int i = 0; i < MAX_SLOTS; ++i) {
        storage->data[i][0] = '\0'; // Initialize all slots as empty
    }
    storage->used_slots = 0;
}
bool addMessage(Storage *storage, const char *message) {
    if (storage->used_slots >= MAX_SLOTS) {
        return false; // Storage is full
    }

    for (int i = 0; i < MAX_SLOTS; ++i) {
        if (storage->data[i][0] == '\0') { // Check for an empty slot
            strncpy(storage->data[i], message, SLOT_SIZE - 1);
            storage->data[i][SLOT_SIZE - 1] = '\0'; // Ensure null termination
            storage->used_slots++;
            return true;
        }
    }
    return false;
}
bool deleteMessage(Storage *storage, const char *slotIndexchar) {

    char justslotIndex=slotIndexchar[3];

    int slotIndex = justslotIndex-'0' ; // Convert the whole string to an integer
    if (slotIndex < 0 || slotIndex >= MAX_SLOTS) {
        return false; // Slot index out of range
    }

    if (storage->data[slotIndex][0] != '\0') { // Check if the slot is not already empty
        // Clear the entire slot by setting each character to '\0'
        for (int i = 0; i < SLOT_SIZE; ++i) {
            storage->data[slotIndex][i] = '\0';
        }
        storage->used_slots--;
        return true;
    }
    return false; // Slot was already empty
}


int availableSlots(const Storage *storage) {
    return MAX_SLOTS - storage->used_slots;
}



typedef  struct 
{
    long msg_type; 
    char msg_text[64];
} msg_buffer ;

struct pid_msg {
    long msg_type;
    pid_t disk_pid;
};
  msg_buffer msg_handlersend;
  Storage storage;

void sigusr1_handler(int signum) 
{
    int availableslots = availableSlots(&storage); 
    msg_buffer msg_handlersend;

    // Set msg_type to 42 (long)
    msg_handlersend.msg_type = 42;

    // Convert availableslots integer to string and copy to msg_text
    snprintf(msg_handlersend.msg_text, sizeof(msg_handlersend.msg_text), "%d", availableslots);

    // Send the message to the UP queue
    msgsnd(msg_up, &msg_handlersend, sizeof(msg_handlersend.msg_text), 0) ;

 }
 void siguser2_handler(int signum)
 {
    clk++;
    printf("clk now is %d \n",clk);
    
 }

int main() 
{
     signal(SIGUSR1, sigusr1_handler);
     signal(SIGUSR2, siguser2_handler);


    initializeStorage(&storage);
    //----------------
    key_t up_key = 102;
    key_t down_key = 103;
    int up_queue = msgget(up_key, IPC_CREAT | 0666);
    int down_queue = msgget(down_key, IPC_CREAT | 0666);
    msg_up = up_queue;
    msg_down = down_queue;
    //--------------------
    pid_t disk_pid = getpid();

    struct pid_msg pid_message;
    pid_message.msg_type = 4; // Message type for PID communication
    pid_message.disk_pid = disk_pid;


    msg_buffer msg_recv; // message got from the kernel to down queue 
    msg_buffer msg_send; // message send for success or failure 


// Send the PID to the kernel process
    if (msgsnd(up_queue, &pid_message, sizeof(pid_message.disk_pid), !IPC_NOWAIT) == -1) {
        perror("Error sending Disk PID");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int operationfromthekernel = msgrcv(msg_down, &msg_recv, sizeof(msg_recv.msg_text), 0 , IPC_NOWAIT) == -1 ;
        
      
      
        if(msg_recv.msg_type==1) // received add from the kernel 
        {
            
              
            if(addMessage(&storage,msg_recv.msg_text)==true) 
            {
        
             msg_send.msg_type =330;
             printf("Adding operation done successfully\n");
             strcpy(msg_send.msg_text, "Adding operation done successfully");
             printf("message isn %s \n",msg_send.msg_text);
        
             msgsnd(msg_down, &msg_send, sizeof(msg_send.msg_text), 0);
      
            }
            else 
            {
            msg_send.msg_type =2;
            printf("Adding operation failure \n");

            strcpy(msg_send.msg_text, "Adding operation failure");
             msgsnd(msg_down, &msg_send, sizeof(msg_send.msg_text), 0);
            }

        }
        else if(msg_recv.msg_type==2) // recived deletion from the kernel 
        {
            
            printf("start no delete \n");
            if(deleteMessage(&storage,msg_recv.msg_text)==true) 
            {
             msg_send.msg_type =111;
             printf("inside delete \n");
             strcpy(msg_send.msg_text, "Deleting operation done successfully");
             //waitopperation(1);
             msgsnd(msg_down, &msg_send, sizeof(msg_send.msg_text), 0);
             printf("send action : to the disk %s \n",msg_send.msg_text);
            }
            else 
            {
            msg_send.msg_type =111;
            printf("no thing to delete \n");
             strcpy(msg_send.msg_text, "Deleting operation failure");
             msgsnd(msg_down, &msg_send, sizeof(msg_send.msg_text), 0);
            }
        }
        else
        {

        }
    }

    return 0;
}
