#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 started; //if process ever started
  u32 remaining_time; //burst time minus how long it has run for
  u32 start_exec_time; //the 1st time the process runs
  u32 waiting_time; //increment as long as process is queued
  u32 response_time; //only increment when hasn't been run yet
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  u32 total_burst_time = 0;
  for(u32 i=0; i<size; i++){
    total_burst_time += data[i].burst_time;
    data[i].started = 0;
  }

  u32 current_time = 0;
  struct process *current_process = NULL;
  u32 q_remaining = quantum_length;
 
  while(total_burst_time!=0){
    for(u32 i=0; i<size; i++){
      struct process *p;
      p = &data[i];
      if(p->arrival_time == current_time){
	TAILQ_INSERT_TAIL(&list, p, pointers);
	p->remaining_time = p->burst_time;
      }
    }
    
    if(current_process==NULL && !TAILQ_EMPTY(&list)){
      struct process *head = TAILQ_FIRST(&list);
      TAILQ_REMOVE(&list,head,pointers);
      current_process=head;
      if(!current_process->started){
	current_process->started=1;
	current_process->start_exec_time=current_time;
      }
    }
    
    if(q_remaining == 0 || (current_process!=NULL && current_process->remaining_time==0)){
      //if current process is not over add to end of queue
      if(current_process!=NULL && current_process->remaining_time!=0){
	TAILQ_INSERT_TAIL(&list, current_process, pointers);
      }
      //if current process is over update waiting and response times
      else if(current_process!=NULL && current_process->remaining_time==0){
	current_process->waiting_time = current_time - current_process->arrival_time - current_process->burst_time;
	current_process->response_time = current_process->start_exec_time - current_process->arrival_time;
      }
      //remove from front of queue and set as current process
      if(!TAILQ_EMPTY(&list)){
	struct process *head = TAILQ_FIRST(&list);
	TAILQ_REMOVE(&list, head, pointers);
	current_process = head;
	if(!current_process->started){
	  current_process->started=1;
	  current_process->start_exec_time = current_time;
	}
	q_remaining = quantum_length;
      }
      else{
	current_process = NULL;
	q_remaining = quantum_length;
      }
    }

    current_time++;
    if(current_process!=NULL){
      current_process->remaining_time--;
      total_burst_time--;
      q_remaining--;
    }
    
  }
  //need to set wait and response time for last process
  if(current_process!=NULL){
    current_process->waiting_time = current_time - current_process->arrival_time - current_process->burst_time;
    current_process->response_time = current_process->start_exec_time - current_process->arrival_time;
  }
  
  for(int i=0; i<size; i++){
    struct process *p = &data[i];
    total_waiting_time += p->waiting_time;
    total_response_time += p->response_time;
  }
  if(size==0){
    total_waiting_time = 0;
    total_response_time = 0;
  }
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
