# new project for OS

# [[3- Sources/College/book-riscv-rev5.pdf|book-riscv-rev5]]

xv6 Secure File Access Monitor

To implement a **Secure File Access Monitor** in xv6 that tracks which processes access which files, you need to integrate system call tracing with process identification. This involves modifying the kernel to intercept file-related system calls (like `open`, `read`, `write`) and logging the associated process name and PID alongside the file path.

### 1. Modify System Calls to Log Access

You can add logging directly within the file system system calls (`sys_open`, `sys_read`, `sys_write`) in `syscall.c` or the relevant file descriptor handling code.

* **Identify the Process**: Inside the system call handler, use `myproc()` to get the current process structure.
* **Extract Details**: Retrieve the process name (`p->name`) and PID (`p->pid`).
* **Log the Event**: Use `cprintf` or a custom logging function to output the process ID, process name, and the file path being accessed.

**Example Implementation Snippet:**

```c
// In syscall.c or relevant file descriptor handler
void log_file_access(struct file *f, int type) {
    struct proc *p = myproc();
    if (p == 0) return;
    
    // Assuming f->name or similar exists to get the path
    // Note: In xv6, file paths are often resolved at open() time.
    // You may need to store the path in the file struct or inode.
    
    cprintf("[SECURE_MONITOR] PID: %d, Process: %s, Action: %s, File: %s\n", 
            p->pid, 
            p->name, 
            (type == FILE_ACCESS_READ) ? "READ" : "WRITE",
            f->name); // Adjust based on how file paths are stored in your xv6 version
}

// Hook into sys_open
int sys_open(void) {
    char *path;
    int flags, fd;
    
    if(argstr(0, &path, 0) < 0 || argint(1, &flags) < 0)
        return -1;
    
    fd = open(path, flags);
    if(fd >= 0) {
        struct file *f = fd2file(fd);
        if(f) {
            log_file_access(f, FILE_ACCESS_WRITE); // Assuming open for write
            // Or handle read/write separately in sys_read/sys_write
        }
    }
    return fd;
}
```

### 2. Track Open File Descriptors

To monitor all currently open files for all running processes (as referenced in Stack Overflow solutions), you can iterate through the process table.

* **Lock the Process Table**: Use `acquire(&ptable.lock)` to safely access process data.
* **Iterate Processes**: Loop through all processes in `ptable.proc`.
* **Check File Descriptors**: For each process, examine its `ofile` array to see which file descriptors are open.
* **Log Details**: Print the process name, PID, and file type (regular, pipe, socket) if accessible.

**Example Iteration Logic:**

```c
void monitor_all_open_files(void) {
    struct proc *p;
    sti();
    acquire(&ptable.lock);
    
    cprintf("PID\tName\t\tType\n");
    
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if(p->state == UNUSED) continue;
        
        // Access p->ofile[i] to check for open files
        // Note: Direct access to file details may require additional kernel internals
        cprintf("%d\t%s\t\t<check_ofile>\n", p->pid, p->name);
    }
    
    release(&ptable.lock);
}
```

### 3. Compile and Test

1. **Add Logging**: Ensure `cprintf` outputs are visible in the xv6 console.
2. **Recompile**: Run `make` to rebuild the xv6 kernel.
3. **Test**: Run programs like `cat`, `ls`, or custom scripts and observe the console output for entries matching the format:
    `[SECURE_MONITOR] PID: X, Process: Y, Action: Z, File: /path/to/file`

This approach provides a basic audit trail of file access events, linking each operation to the specific process responsible. For production-grade security, consider implementing more robust logging mechanisms and filtering out unnecessary noise.
