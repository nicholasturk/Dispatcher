/*
dispatcher.c

Student Name : Nicholas Sturk
Student ID # : 1058650

Dispatch Algorithm : Shortest remaining time
*/

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "LinkedListAPI.h"

#define MAX_LINE_LENGTH 100
#define MAX_BURST 2147483647

typedef struct process{
  int start_time;
  int process_id;
  int run_time;
  int time_remaining;
  int time_ready;
  int time_blocked;
  List *exchanges;
} Process;

//New Process function
Process *newProcess();

//LinkedListAPI necessary functions for queues
int proComp(const void *process1,const void *process2);
void proDel(void *process);
char *proToStr(void *process);

//LinkedListAPI necessary function for exchange lists
int exComp(const void *ex1, const void *ex2);
void exDel(void *ex);
char *exToStr(void *ex);

//Logic functions
void refreshProcesser();
void refreshHardDrive();
void refreshReady();
void refreshBlocked();

//Queues
List *incoming;
List *ready;
List *blocked;
List *completed;

//Current processes occupying harddrive or cpu
Process *cpu_user;
Process *harddrive_user;

//Global counters
int exchange_time_remaining;
int exchange_time_length;
int counter = 0;

void dispatcher(FILE *fd, int harddrive){

  exchange_time_length = harddrive;

  int num_processes = 0;
  char line_buffer[MAX_LINE_LENGTH];
  char *token;
  int *exchange_time;

  //Initialize four queues
  incoming = initializeList(proToStr,proDel,proComp);
  ready = initializeList(proToStr,proDel,proComp);
  blocked = initializeList(proToStr,proDel,proComp);
  completed = initializeList(proToStr,proDel,proComp);

  //Process_0 initialization
  Process *initial = newProcess();
  initial->time_remaining = MAX_BURST;

  //Initial starts as the current cpu_user always
  cpu_user = initial;
  
  while (fgets(line_buffer, MAX_LINE_LENGTH, fd) != NULL && line_buffer[0] != '\n'){
      
    //Create structure for each process
    Process *ptr = newProcess();
    
    token = strtok(line_buffer, " ");
    sscanf(token, "%d",&(ptr->start_time));
   
    token = strtok(NULL, " ");
    sscanf(token, "%d",&(ptr->process_id));
    
    token = strtok(NULL, " ");
    sscanf(token, "%d",&(ptr->time_remaining));
    
    token = strtok(NULL, " ");
    while ( token != NULL ){
        exchange_time = malloc(sizeof(int));
        sscanf(token, "%d", exchange_time);
        insertBack(ptr->exchanges, exchange_time);
        token = strtok(NULL, " ");
    }
    //Insert process into the incoming queue
    insertBack(incoming,ptr);
    num_processes++;

  }

  //Main loop
  while (getLength(completed) < num_processes){

    //Apply logic
    refreshReady();
    refreshProcesser();
    refreshHardDrive();
    refreshBlocked();

    if(harddrive_user){
      harddrive_user->time_blocked++;
      exchange_time_remaining--;
    } 
    
    //Update time if its not p0
    if(cpu_user->process_id != 0)
    {
      cpu_user->time_remaining--;
    }

    cpu_user->run_time++;

    Process *ptr_ready;
    Process *ptr_checker;

    ListIterator ready_checker = createIterator(ready);
    ListIterator blocked_checker = createIterator(blocked);

    //Increase ready times that are in queue
    while((ptr_ready = (Process*)nextElement(&ready_checker)) != NULL){
      ptr_ready->time_ready++;
    }

    //Increase blocked times that are in queue
    while((ptr_checker = (Process*)nextElement(&blocked_checker)) != NULL){
      ptr_checker->time_blocked++;
    }

    //Increase global time counter;
    counter++;
  }

  Process *ptr;
  ListIterator printer = createIterator(completed);

  printf("%d %d\n",cpu_user->process_id,cpu_user->run_time - 1);

  char *temp;

  while((ptr = (Process*)nextElement(&printer)) != NULL){
    temp = proToStr(ptr);
    printf("%s", temp);
    free(temp);
  }

  freeList(incoming);
  freeList(ready);
  freeList(blocked);
  freeList(completed);
  if (cpu_user) proDel(cpu_user);
}

Process *newProcess(){
  Process *ptr = malloc(sizeof(Process));
  ptr->start_time = 0;
  ptr->process_id = 0;
  ptr->run_time = 0;
  ptr->time_remaining = 0;
  ptr->time_ready = 0;
  ptr->time_blocked = 0;
  ptr->exchanges = initializeList(exToStr,exDel,exComp);
  return ptr;
}

void refreshReady(){

  //Refresh the harddrive_user again if it meets its time

  if(getLength(cpu_user->exchanges) > 0 &&
     cpu_user->run_time == *((int*)getFromFront(cpu_user->exchanges)))
  {
    deleteDataFromList(cpu_user->exchanges, getFromFront(cpu_user->exchanges));
    if(!harddrive_user){
      exchange_time_remaining = exchange_time_length;
      harddrive_user = cpu_user;
    } else{
      insertBack(blocked,cpu_user);
    }
    cpu_user = deleteDataFromList(ready, getFromFront(ready));
  }

  if(harddrive_user != NULL && exchange_time_remaining == 0 && 
     harddrive_user->time_remaining != 0)
  {
    exchange_time_remaining = exchange_time_length;
    insertSorted(ready,harddrive_user);
    harddrive_user = NULL;
  }
 
  if(getLength(ready) > 1 && 
          cpu_user->time_remaining > ((Process*)getFromFront(ready))->time_remaining)
  {
    insertSorted(ready,cpu_user);
    cpu_user = deleteDataFromList(ready, getFromFront(ready));
  }

  if(getLength(incoming) > 0 && 
     counter == ((Process*)getFromFront(incoming))->start_time)
  {
    insertSorted(ready, deleteDataFromList(incoming,getFromFront(incoming)));
  }
  
    //If ready queue has a smaller processes left switch it out

  if(getLength(ready) > 0 && 
     cpu_user->time_remaining > ((Process*)getFromFront(ready))->time_remaining)
  {
    insertSorted(ready,cpu_user);
    cpu_user = deleteDataFromList(ready, getFromFront(ready));
  }
}

void refreshProcesser(){

  //Update ready queue to get rid of p0

  if(cpu_user->process_id == 0 && getLength(ready) > 0)
  {
    insertBack(ready, cpu_user);
    cpu_user = deleteDataFromList(ready, getFromFront(ready));
  }

  //Put into blocked queue if it meets its exchange time 

  if(cpu_user->process_id != 0 && 
     getLength(cpu_user->exchanges) > 0 && 
     cpu_user->run_time == *((int*)getFromFront(cpu_user->exchanges)))
  {
    insertBack(blocked,cpu_user);
    cpu_user = deleteDataFromList(ready, getFromFront(ready));
  }

  //Put into complete queue if it's finished

  if(cpu_user->time_remaining == 0)
  {
    insertBack(completed,cpu_user);
    cpu_user = deleteDataFromList(ready, getFromFront(ready));
  }

  //If ready queue has a smaller processes left switch it out

  if(getLength(ready) > 0 && 
     cpu_user->time_remaining > ((Process*)getFromFront(ready))->time_remaining)
  {
    insertSorted(ready,cpu_user);
    cpu_user = deleteDataFromList(ready, getFromFront(ready));
  }
}

void refreshHardDrive(){

  //If exchange is over get new harddrive from blocked queue

  if(exchange_time_remaining == 0 && getLength(blocked) > 0 && harddrive_user != NULL){
    if(harddrive_user->time_remaining == 0){
      insertBack(completed,harddrive_user);
    } else{
      insertSorted(ready,harddrive_user);
    }
    exchange_time_remaining = exchange_time_length;
    harddrive_user = deleteDataFromList(blocked, getFromFront(blocked));
  }

  //If exchange is over but nothing in blocked queue set to NULL

  else if(exchange_time_remaining == 0 && getLength(blocked) == 0)
  {
    exchange_time_remaining = exchange_time_length;
    if(harddrive_user != NULL && harddrive_user->time_remaining == 0){
      insertBack(completed,harddrive_user);
    } else{
      insertSorted(ready,harddrive_user);
    }
    harddrive_user = NULL;
  }

  //If no exchanges are happening then always check to see if its time for an exchange
  
  else if(getLength(cpu_user->exchanges) > 0 &&
          cpu_user->run_time == *((int*)getFromFront(cpu_user->exchanges)))
  {
    deleteDataFromList(cpu_user->exchanges, getFromFront(cpu_user->exchanges));
    harddrive_user = cpu_user;
  }

  //Harddrive user blocked time increase if it exists
}

void refreshBlocked(){

  if(getLength(cpu_user->exchanges) > 0 && 
     cpu_user->run_time == *((int*)getFromFront(cpu_user->exchanges)) &&
     harddrive_user != NULL)
  {
    insertBack(blocked,cpu_user);
  }
  if(harddrive_user == NULL && getLength(blocked) > 0){
    harddrive_user = deleteDataFromList(blocked, getFromFront(blocked));
  }
}

char *proToStr(void *voidPtr){

  if(voidPtr == NULL){
    return NULL;
  }

  char *str = malloc(256);
  Process *ptr = (Process*)voidPtr;

  snprintf(str,255,"%d %d %d %d\n", 
           ptr->process_id,ptr->run_time,
           ptr->time_ready,ptr->time_blocked);

  return str;
}

void proDel(void *voidPtr){

  if(voidPtr == NULL){
    return;
  }

  Process *ptr = (Process*)voidPtr;

  if (ptr->exchanges)
    freeList(ptr->exchanges);
  free(ptr);
}

int proComp(const void *process1,const void *process2){

  return ((Process*) process1)->time_remaining -
         ((Process*) process2)->time_remaining;
}

char *exToStr(void *ex){

  if(ex == NULL){
    return NULL;  
  }

  char *str = malloc(256);
  snprintf(str, 256, "Exchange time %d", *((int*)ex));
  return str;
}

void exDel(void *ex){
  if(ex){
    free(ex);
  }
}

int exComp(const void *ex1, const void *ex2){
  return 0;
}




