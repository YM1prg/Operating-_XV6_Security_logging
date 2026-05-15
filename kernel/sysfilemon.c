// kernel/sysfilemon.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "filemon.h"

uint64 sys_get_log_entry(void) {
  // Lecture Ch1.50 / Ch2.7: Privilege enforcement
  if(myproc()->uid != 0)
    return -1;

  int idx;
  uint64 buf;
  int size;
  if(argint(0, &idx) < 0 || argaddr(1, &buf) < 0 || argint(2, &size) < 0)
    return -1;

  if(idx < 0 || idx >= MAX_LOG_ENTRIES)
    return -1;

  acquire(&file_log.lock);
  struct FileLogEntry *e = &file_log.entries[idx];
  if(!e->valid) {
    release(&file_log.lock);
    return 0;
  }
  if(copyout(myproc()->pagetable, buf, (char *)e, sizeof(*e)) < 0) {
    release(&file_log.lock);
    return -1;
  }
  release(&file_log.lock);
  return sizeof(*e);
}
