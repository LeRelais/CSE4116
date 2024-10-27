#include <stdio.h> 
#include <sys/msg.h> 
 main() { 
 key_t key; 
 int msgid; 

   key = ftok("keyfile", 1); 
 msgid = msgget(key, IPC_CREAT|0644); 
 if(msgid == -1) { 
 perror("msgget"); 
 exit(1); 
 } 
 
 printf("Before IPC_RMIDn"); 
 system("ipcs -q"); 
 msgctl(msgid, IPC_RMID, (struct msqid_ds *)NULL); 
 printf("After IPC_RMIDn"); 
 system("ipcs -q"); 
 }
