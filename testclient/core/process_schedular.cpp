#include<stdio.h>
#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include<pthread.h>
#include "OCPlatform.h"
#include "OCApi.h"
using namespace OC;

#define STOP 0
#define START 1
#define RUNNING 2
#define MAX_PROCESS_COUNT 1

static int process_start_all_flag = 0;

struct process_table
{
  pthread_t threadid;
  int (*funcptr)(void *);
  void *arg;
  bool is_running;
};


struct process_table process[MAX_PROCESS_COUNT];
void *process_schedule(void *arg)
{
  struct process_table *prtable = (struct process_table *)arg;
  int ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (ret != 0)
  {
    prtable->is_running = STOP;
    return NULL;
  }
  while(1)
  {
    if(prtable->funcptr)
    {
      std::cout <<"it is getting called------------------\n";
      ret = (prtable->funcptr)(prtable->arg);
      if(ret)
        break;
      if(prtable->is_running == STOP)
        break;
    }
  }
  prtable->is_running = STOP;
  pthread_exit(NULL);
}

void process_start_all(void)
{
  int i=0;
  int ret = -1;
  process_start_all_flag =1;
  for(i=0 ;i<MAX_PROCESS_COUNT; i++)
  {
    if (process[i].is_running == START)
    {
      ret = pthread_create(&process[i].threadid, NULL, &process_schedule, &process[i]);
      if(!ret)
        process[i].is_running = RUNNING; 
    }
    i++;
    if(i == MAX_PROCESS_COUNT)
      i=0;

    if(process_start_all_flag == 0) break;

    sleep(1);
  }
}

void process_stop_all(void)
{
  int ret =-1;
  int i =0;

  process_start_all_flag = 0 ;
  for(i=0;i<MAX_PROCESS_COUNT;i++)
  {
    if(process[i].is_running == RUNNING)
    {  
      ret = pthread_cancel(process[i].threadid);
      if (ret != 0)
        std::cout <<"pthread_cancel error";
      /* Join with thread to see what its exit status was */
      ret = pthread_join(process[i].threadid, NULL);
      if (ret != 0)
        std::cout <<"pthread_join error"<< std::endl;
      else
      {
        std::cout << "main(): thread was canceled\n";
        process[i].threadid = 0;
        process[i].is_running = STOP;
        process[i].funcptr=NULL;
        process[i].arg = NULL;
      }
    }
    else
      std::cout <<"main(): thread wasn't canceled (shouldn't happen!)\n";

  }
}

void process_schedular_init(void)
{
  int i =0;
  for(i=0;i<MAX_PROCESS_COUNT;i++)
  {
    process[i].threadid = 0;
    process[i].funcptr=NULL;
    process[i].arg = NULL;
    process[i].is_running = STOP;
  }
}

int add_in_process_queue(int (*funcptr)(void *), void *arg)
{
  static int i = 0;
  if (process[i].is_running == STOP)
  {
    process[i].funcptr=funcptr;
    process[i].arg = (void *)arg;
    process[i].is_running = START;
  }
  i++;
  return 0;
}

int remove_from_process_queue(int (*funcptr)(void *))
{
  int ret;
  int i =0;

  for(i=0;i<MAX_PROCESS_COUNT;i++)
  {
    if(process[i].funcptr== funcptr) break;
  }

  if(process[i].is_running == RUNNING)
  {  
    ret = pthread_cancel(process[i].threadid);
    if (ret != 0)
      std::cout <<"pthread_cancel error";
    /* Join with thread to see what its exit status was */
    ret = pthread_join(process[i].threadid, NULL);
    if (ret != 0)
      std::cout <<"pthread_join error"<< std::endl;
    else
    {
      std::cout << "main(): thread was canceled\n";
      process[i].threadid = 0;
      process[i].is_running = STOP;
      process[i].funcptr=NULL;
      process[i].arg = NULL;
    }
  }
  else
    std::cout <<"main(): thread wasn't canceled (shouldn't happen!)\n";
  return 0;  
}
