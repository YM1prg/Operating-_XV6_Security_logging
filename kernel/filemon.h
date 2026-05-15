// kernel/filemon.h (updated)

#ifndef FILEMON_H
#define FILEMON_H

#include "types.h"
#include "spinlock.h"



#define MAX_LOG_ENTRIES 64
#define MAX_PATH MAXPATH  // 128, matches xv6 exactly
#define MAX_COMM 16       // Matches proc.name[16]

enum FileOp { OP_OPEN, OP_READ, OP_WRITE, OP_CLOSE };

struct FileLogEntry {
  int pid;
  int uid;
  char comm[MAX_COMM];
  enum FileOp op;
  char path[MAX_PATH];
  int state;
  int valid;
};

struct FileLog {
  struct spinlock lock;
  struct FileLogEntry entries[MAX_LOG_ENTRIES];
  int next_index;
};

extern struct FileLog file_log;
void filemon_init(void);
void log_file_access(enum FileOp op, const char *path);

#endif
