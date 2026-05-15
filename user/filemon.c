// user/filemon.c
#include "kernel/types.h"
#include "kernel/filemon.h"
#include "user/user.h"

int main() {
  struct FileLogEntry e;
  printf("Secure File Access Monitor\n");
  printf("PID\tUID\tNAME\t\tOP\t\tSTATE\t\tPATH\n");
  printf("===\t===\t====\t\t==\t\t=====\t\t====\n");

  for(int i = 0; i < 64; i++) {
    if(get_log_entry(i, &e, sizeof(e)) > 0 && e.valid) {

      const char *ops[] = {"OPEN", "READ", "WRITE", "CLOSE"};
      const char *states[] = {"UNUSED", "USED", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"};

      const char *op_str = (e.op < 4) ? ops[e.op] : "UNKNOWN";
      const char *state_str = (e.state < 6) ? states[e.state] : "UNKNOWN";

      printf("%d\t%d\t%s\t\t%s\t\t%s\t\t%s\n",e.pid, e.uid, e.comm, op_str, state_str, e.path);
    }
  }
  exit(0);
}
