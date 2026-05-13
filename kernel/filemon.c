// kernel/filemon.c
#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "filemon.h"

struct FileLog file_log;

void filemon_init(void) {
  initlock(&file_log.lock, "file_log");
  file_log.next_index = 0;
  for(int i = 0; i < MAX_LOG_ENTRIES; i++)
    file_log.entries[i].valid = 0;
}

void log_file_access(enum FileOp op, const char *path) {
  struct proc *p = myproc();
  acquire(&file_log.lock);
  
  int idx = file_log.next_index % MAX_LOG_ENTRIES;
  struct FileLogEntry *e = &file_log.entries[idx];
  
  e->pid = p->pid;
  e->uid = p->uid;
  safestrcpy(e->comm, p->name, MAX_COMM);
  e->op = op;
  safestrcpy(e->path, path, MAX_PATH);
  e->state = p->state;
  e->valid = 1;
  
  file_log.next_index++;
  release(&file_log.lock);
}
