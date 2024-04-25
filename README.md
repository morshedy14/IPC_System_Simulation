# Operating System I/O Simulation

This project simulates an operating system's process handling I/O requests and the kernel's management of these requests. The simulation emphasizes the importance of kernel privilege for I/O operations and showcases structured communication between different processes.

## Processes Overview

### Disk Process
The Disk Process simulates storage with 10 character arrays (each of size 64 chars). It communicates with the Kernel Process via two message queues, UP and DOWN, handling data addition and deletion requests. This process operates with latencies: 3 CLK cycles for data addition and 1 CLK cycle for deletion. Additionally, the Disk Process keeps a local CLK variable, incrementing it in response to a signal from the Kernel Process.

### Kernel Process
The Kernel Process acts as a mediator between User Processes and the Disk Process. It ensures synchronization by sending signals to all processes every second. Communication with other processes occurs through message queues, with the Kernel responsible for forwarding data addition/deletion requests and returning request statuses to user processes. The Kernel also keeps a log of all events during execution.

### User Process
The User Process simulates user-initiated I/O requests. It reads a file containing a list of I/O operations to perform, with timing information indicating when to send each request to the Kernel. The process communicates with the Kernel via message queues to carry out data addition or deletion operations on the simulated storage.

## Project Highlights
- Process communication via message queues
- Kernel-mode operations and synchronization
- Disk Process management of storage and status checks
- User Process interaction with the Kernel for data addition/deletion


This is a IPC system simulation
**instructions while runnig**
- compile all files
- run the kernel first to init the communication channels
- then run the disk
- for the users start with user_1 then user_2 finally user_3
- after simulation erase the log file
- write in the terminal `ipcrm -a`

