#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "filemon.h"

uint64 sys_get_log_entry(void) {
  //  Security: Only root (uid 0) can retrieve audit logs
  if(myproc()->uid != 0)
    return -1;

  int idx, size;
  uint64 buf;

  //  Fetch arguments (void-returning in your xv6 version)
  argint(0, &idx);
  argaddr(1, &buf);
  argint(2, &size);

  //  Manual validation (since argint/argaddr don't return error codes)
  if(idx < 0 || idx >= MAX_LOG_ENTRIES)
    return -1;
  if(size < (int)sizeof(struct FileLogEntry))
    return -1;

  acquire(&file_log.lock);
  struct FileLogEntry *e = &file_log.entries[idx];
  if(!e->valid) {
    release(&file_log.lock);
    return 0; // No log entry at this index
  }

  //  Safe kernel → user memory transfer (Ch 2.22, Ch 1.39)
  if(copyout(myproc()->pagetable, buf, (char *)e, sizeof(*e)) < 0) {
    release(&file_log.lock);
    return -1;
  }
  release(&file_log.lock);
  return sizeof(*e);
}
