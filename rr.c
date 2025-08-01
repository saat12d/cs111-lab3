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
  u32 remaining_time;
  bool started;
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

  /* Round Robin scheduling */
  u32 current_time = 0;
  u32 finished = 0;
  u32 next_idx = 0;

  /* Enqueue any processes arriving at time 0 */
  while (next_idx < size && data[next_idx].arrival_time <= current_time)
  {
    TAILQ_INSERT_TAIL(&ready_queue, &data[next_idx], pointers);
    next_idx++;
  }

  while (finished < size)
  {
    if (TAILQ_EMPTY(&ready_queue))
    {
      /* Jump forward to next arrival if no one is ready */
      current_time = data[next_idx].arrival_time;
      TAILQ_INSERT_TAIL(&ready_queue, &data[next_idx], pointers);
      next_idx++;
      continue;
    }

    /* Dequeue next process */
    struct process *p = TAILQ_FIRST(&ready_queue);
    TAILQ_REMOVE(&ready_queue, p, pointers);

    /* Record response time at first service */
    if (!p->started)
    {
      total_response_time += (current_time - p->arrival_time);
      p->started = true;
    }

    /* Execute up to one quantum */
    u32 slice = (p->remaining_time < quantum_length) ? p->remaining_time : quantum_length;
    p->remaining_time -= slice;
    current_time += slice;

    /* Enqueue newly arrived processes */
    while (next_idx < size && data[next_idx].arrival_time <= current_time)
    {
      TAILQ_INSERT_TAIL(&ready_queue, &data[next_idx], pointers);
      next_idx++;
    }

    if (p->remaining_time > 0)
    {
      /* Not finished yet, requeue */
      TAILQ_INSERT_TAIL(&ready_queue, p, pointers);
    }
    else
    {
      /* Process finished, accumulate waiting time */
      finished++;
      total_waiting_time += (current_time - p->arrival_time - p->burst_time);
    }
  }
  
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}

