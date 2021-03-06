#include <stdio.h>
#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <mutex>
#include <condition_variable>
#include "OCPlatform.h"
#include "OCApi.h"
using namespace OC;
#include "iot_api.h"
#include "cloud_iot_common.h"
#include "cloud_iot_api.h"
#include "resource_xml.h"
#include "UIManager.h"
#include "resource_param.h"
#include "resource_list.h"

/* I need to change the logic, the scheduler thread should start only after it is trigered.at present it starts and wait for user
   input to schedule the resourcce activity. This is waste of cpu resource so need to change the logic- i have changed now*/

/* Please do not lock in this portion of code , anywhere here*/

void *schedule_mytask(void *param)
{
  struct iot_resource *ior;
  time_t start_time;
  int old_state = -1;
  int old_power = -1;
  class ResourceParam *resparam = (ResourceParam*) param;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  start_time = time(NULL);
  while(resparam)
  {
    if((time(NULL) - start_time >=resparam->schedule_start_time) && (resparam->schedule_start_time))
    {
      if((resparam->resource_type >=1) && (resparam->resource_type <=3)) // for output device
      {
        if((resparam->state != resparam->schedule_state) || (resparam->power != resparam->schedule_power))
        {
          ior = (struct iot_resource *)malloc(sizeof(struct iot_resource));
          if(ior)
          {
            old_state  = resparam->state;
            old_power  = resparam->power;
            resparam->state =  resparam->schedule_state;
            resparam->power =  resparam->schedule_power;
            std::cout<<"setting state to : "<<resparam->state<< " and power to: "<<resparam->power <<std::endl;
            ior->state =  resparam->state;
            ior->power =  resparam->power;
            strcpy(ior->host,resparam->m_hostname.c_str());
            strcpy(ior->Uri,resparam->Uri.c_str());
            // set the value
            if(set_sensor(ior))
            {
            }
            free(ior);
          }                  
        }
        else
        {
          if((time(NULL) - start_time >=resparam->schedule_end_time) && (resparam->schedule_end_time))
          {
            if((old_state != -1) && (old_power != -1)) 
            {
              ior = (struct iot_resource *)malloc(sizeof(struct iot_resource));
              if(ior)
              {
                resparam->state =  old_state;
                resparam->power =  old_power;
                std::cout<<"setting previous state to : "<<resparam->state<< " and previous power to: "<<resparam->power <<std::endl;
                ior->state =  resparam->state;
                ior->power =  resparam->power;
                strcpy(ior->host,resparam->m_hostname.c_str());
                strcpy(ior->Uri,resparam->Uri.c_str());
                // set the value
                if(set_sensor(ior))
                {
                }
                free(ior);
              }  
            }
            if(resparam->is_repeat_next_day == 1)
              start_time = time(NULL);
            else
            {
              std::cout<<"coming out time over\n";
              break;
            }
          }
        }
      }
      else if(resparam->resource_type > 3) // 0 means wrong device and greater than 3 menas other than controllable
      {
        if((time(NULL) - start_time >=resparam->schedule_end_time) && (resparam->schedule_end_time))
        {
          if(resparam->power >= resparam->threshold_power)
          {
            // generate the alaram for 30 seconds 
          }
        }
        else
        {
          if(resparam->is_repeat_next_day == 1)
            start_time = time(NULL);
          else
          {
            std::cout<<"coming out time over\n";
            break;
          }
        }
      }
    }
  }
  if(resparam)
    resparam->is_schedule_task_running = false;
  pthread_exit(NULL);
  return NULL; 
}

void start_schedular(class ResourceParam *resparam)
{
  if(resparam != NULL)
  {
    if(resparam->schedule_staus == 1)
    {
      if(resparam->is_schedule_task_running == false)
      {
        if(!pthread_create(&resparam->schedular_thread, NULL, schedule_mytask,(void *)resparam))
          resparam->is_schedule_task_running = true;
      }  
    }
  }
}

void stop_schedular(class ResourceParam *resparam)
{
  void *res;
  if(resparam) {
    if(resparam->schedule_staus == 0) // stop scheduling
    {
      if(resparam->is_schedule_task_running == true) {
        if(!pthread_cancel(resparam->schedular_thread))
        {
          resparam->is_schedule_task_running = false; 
          printf("schedular thread is cancelaed\n");
          if(!pthread_join(resparam->schedular_thread, &res))
          {
            if (res == PTHREAD_CANCELED)
            {
              printf("schedular thread completely cancealed\n");
            }
          }          
        }    
      }
    }
  }
}

void stop_all_schedular_threads()
{
  void *res;
  struct resource_list *pointer = get_resource_head();
  while(pointer != NULL)
  {
    if(pointer->resparam->is_schedule_task_running == true) {
      if(!pthread_cancel(pointer->resparam->schedular_thread))
      {
        pointer->resparam->is_schedule_task_running = false;
        /* Join with thread to see what its exit status was */
        if(!pthread_join(pointer->resparam->schedular_thread, &res))
        {
          if (res == PTHREAD_CANCELED)
          {
            printf("schedular_thread thread cancealed\n");
          }
        }  
      }
    }
    pointer = pointer->next; 
  }
}

