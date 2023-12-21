#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdbool.h>

int msg_up, msg_down, msg_up_PK, msg_down_PK;
int clk;
pid_t disk_id;
pid_t process1_id;
pid_t process2_id;
pid_t process3_id;
FILE* log_file;

struct msg_buffer {
    long msg_type;
    char msg_text[64];
};

struct pid_msgdisk {
    long msg_type;
    pid_t disk_pid;
};
struct pid_msgprocess {
    long msg_type;
    pid_t process_pid;
};
typedef struct {
    long msg_type;
    bool status;
} status_msg;
struct ClkMessage {
    long msg_type;
    int clk;
};
void requestDiskStatus() {
    // Send SIGUSR1 signal to disk process to request status
    
    kill(disk_id, SIGUSR1);
    printf("kernel send request to disk\n");

}

// Signal handler for updating the clock
void sig_handler(int signum) {
    clk++;  // Increment the clock
}

void handleAddRequest(struct msg_buffer* request, FILE* log_file) {

    fprintf(log_file, "At time = %d, request to add \"%s\" from P%c\n", clk-1,request->msg_text+3,request->msg_text[1]); // Log the operation request in the log file
    struct msg_buffer availableslots;
    requestDiskStatus();         
        fprintf(log_file, "At time = %d, sent status request to Disk\n", clk-1); //log the sending status of the request to the disk
    int clkAdd=clk+2;
    // Receive available slots message from disk process
    int a = msgrcv(msg_up, &availableslots, sizeof(availableslots.msg_text), 42, 0);
    if (a == -1) {
        perror("Error receiving Disk status");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "At time = %d,  Disk status =  %s empty slots\n", clk-1, availableslots.msg_text); //log the number of available slots in the log file

    if (availableslots.msg_text[0] != '0') {  // Check if there are available slots
        // Send the data to the disk to be added
        request->msg_type=1;
        msgsnd(msg_down, request, sizeof(request->msg_text), 0);
        // Receive message from disk process indicating the result of addition
        struct msg_buffer DiskAdd; 
        // received ADD status from disk

         int messagerecive = msgrcv(msg_down, &DiskAdd, sizeof(DiskAdd.msg_text),330, 0) ; //test ipcnowait 
         printf("Message  index done is 17 %c \n",DiskAdd.msg_text[17]);
       
        // Send a message to the process indicating successful or unable to ADD
        DiskAdd.msg_type=502;
        msgsnd(msg_down_PK, &DiskAdd, sizeof(DiskAdd.msg_text), 0);  
        printf("send status to the process \n");
 //log the status of the request in the log file
       if(DiskAdd.msg_text[17]=='d')
        {
           fprintf(log_file, "At time = %d,send success to P%c\n", clkAdd,request->msg_text[1]);//log the success of the request in the log file
        }
        else
        {
            fprintf(log_file, "At time = %d,send failed to P%c\n", clkAdd,request->msg_text[1]); //log the failure of the request in the log file
        }
        fflush(log_file);

    } 
    else {
        // Send a message to the process indicating that the request can't be handled
        struct msg_buffer response;
        response.msg_type = 0;
        strcpy(response.msg_text, "The request can't be handled.");
        msgsnd(msg_down_PK, &response, sizeof(response.msg_text), 0);
    }
}

void handleDelRequest(struct msg_buffer* request, FILE* log_file) {
           //log the operation request in the log file
    fprintf(log_file, "At time = %d, request to delete slot\"%s\" from P%c\n", clk-1,request->msg_text+3,request->msg_text[1]);
    request->msg_type=2;
    int clkdel=clk+1;
    // Send the message to the Disk Process (down_queue)
    if (msgsnd(msg_down, request, sizeof(request->msg_text), 0) == -1) {
        perror("Error sending message to Disk Process");
    }

    // Receive message from disk process indicating the result of deletion
    struct msg_buffer DiskDelete;
    int a = msgrcv(msg_down, &DiskDelete, sizeof(DiskDelete.msg_text), 111, 0); //-ipc nowait 

    if (a == -1) {
        perror("Error receiving Disk status");
        //exit(EXIT_FAILURE);
    }

    // Send message to the process to indicate the result of deletion (msg_down_PK)
    DiskDelete.msg_type=502;
    msgsnd(msg_down_PK, &DiskDelete, sizeof(DiskDelete.msg_text), 0);
     // Log the status of the delete request in the log file
    if(DiskDelete.msg_text[19]=='d')
    {
       fprintf(log_file, "At time = %d,send success to P%c\n", clkdel,request->msg_text[1]);//log the success of the request in the log file
    }
    else
    {
        fprintf(log_file, "At time = %d,send failed to P%c\n", clk-1,request->msg_text[1]); //log the failure of the request in the log file
    }
    fflush(log_file);//flush the log file (clear the buffer)
}

int main() 
{

    //printf("recieved process id %d",process1id);
    clk=0;

    // Signal setup for clock updates
    signal(SIGUSR2, sig_handler);

    // Open the log file at the beginning of the program
    log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    //******* Disk Kernel Message Queues ***********//
    key_t up_key = 102;
    key_t down_key = 103;
    int up_queue = msgget(up_key, IPC_CREAT | 0666);
    int down_queue = msgget(down_key, IPC_CREAT | 0666);
    msg_up = up_queue;
    msg_down = down_queue;

   // ******* Process Kernel Message Queues ***********//
    key_t up_key_PK = 77;
    key_t down_key_PK = 55;
    int up_queue_PK = msgget(up_key_PK, IPC_CREAT | 0666);
    int down_queue_PK = msgget(down_key_PK, IPC_CREAT | 0666);
    msg_up_PK = up_queue_PK;
    msg_down_PK = down_queue_PK;

    if (up_queue == -1 || down_queue == -1 ) 
    {
        perror("Error in creating message queues");
        exit(EXIT_FAILURE);
    }

    // Compile the disk process
   
    // Receive the Disk PID from the Disk Kernel
    struct pid_msgdisk received_diskid;
    struct pid_msgprocess received_process1id;
    struct pid_msgprocess received_process2id;
    struct pid_msgprocess received_process3id;


  //**********************************get diskid *********************************//
    int diskid = msgrcv(up_queue, &received_diskid, sizeof(received_diskid.disk_pid), 4, 0);
   if (diskid== -1) {
        perror("Error receiving Disk PID");
        exit(EXIT_FAILURE);
    }
    disk_id = received_diskid.disk_pid;
    printf("Kernel Process: Received Disk PID: %d\n", disk_id);

    //**********************************get process 1 id *********************************//
     int processid = msgrcv(up_queue_PK, &received_process1id, sizeof(received_process1id.process_pid), 001, !IPC_NOWAIT);
    if (processid== -1) {
        perror("Error receiving process id");
        exit(EXIT_FAILURE);
    }
 
    process1_id=received_process1id.process_pid;

    printf("Kernel Process: Received process 1 PID: %d\n", process1_id);
  //**********************************get process 3 id *********************************//
     int process2id = msgrcv(up_queue_PK, &received_process2id, sizeof(received_process2id.process_pid), 002, !IPC_NOWAIT);
    if (process2id== -1) {
        perror("Error receiving process id");
        exit(EXIT_FAILURE);
    }
 
    process2_id=received_process2id.process_pid;

    printf("Kernel Process: Received process 2 PID: %d\n", process2_id);
      //**********************************get process 3 id *********************************//
     int process3id = msgrcv(up_queue_PK, &received_process3id, sizeof(received_process3id.process_pid), 003, !IPC_NOWAIT);
    if (process3id== -1) {
        perror("Error receiving process id");
        exit(EXIT_FAILURE);
    }
 
    process3_id=received_process3id.process_pid;

    printf("Kernel Process: Received process 3 PID: %d\n", process3_id);

    while (1)
    {
        sleep(1);

        kill(disk_id,SIGUSR2);
        kill(process1_id,SIGUSR2);
        kill(process2_id,SIGUSR2);
        kill(process3_id,SIGUSR2);
        clk++;

        printf("%d \n",clk); 

        // Receive message from process
        struct msg_buffer received_msg; // received message from process
        received_msg.msg_text[0]='N';

        if (msgrcv(up_queue_PK, &received_msg, sizeof(received_msg.msg_text), 0, IPC_NOWAIT) == -1) 
        {
            //printf("Error receiving message from process");
            //exit(1);
        }
        // Check the operation type
        if (received_msg.msg_text[0] == 'A') 
        {  // ADD
            printf("recieved add request from process");
            handleAddRequest(&received_msg, log_file);
        } else if (received_msg.msg_text[0] == 'D') 
        {  // DEL
            printf("recieved delete request from process\n");
            handleDelRequest(&received_msg, log_file);
        } else {
            // Send a message to the process indicating that the request can't be handled
            struct msg_buffer response;
            response.msg_type = 2;
            strcpy(response.msg_text, "The request can't be handled.");
            msgsnd(msg_down_PK, &response, sizeof(response.msg_text), 0);
            
        }
    }

    // Close the log file when done
    fclose(log_file);
    return 0;
}
