# Project Report: Secure File Access Monitor for xv6-riscv

## Abstract
This project implements a secure, kernel-level file access audit mechanism for the xv6-riscv operating system. The monitor intercepts file-management system calls (`open`, `read`, `write`, `close`), captures process metadata (PID, UID, execution state, target path/FD), and stores entries in a circular kernel buffer protected by a spinlock. A privileged retrieval syscall allows root-only user-space access to the audit trail. Throughout development, several low-level systems programming challenges were resolved: C preprocessor dependency cycles, GNU RISC-V assembler expression constraints, race-condition synchronization, and self-referential logging loops. The implementation aligns directly with core operating system principles covering syscall interfaces, concurrency control, memory protection, and audit isolation.

---


## 2. System Architecture & Design
The monitor follows a three-layer architecture:

| Layer | Component | Responsibility |
|-------|-----------|----------------|
| **Interception** | `kernel/sysfile.c` hooks | Captures syscall arguments & operation type |
| **Storage** | `kernel/filemon.c/h` | Circular buffer + spinlock-protected `FileLogEntry` struct |
| **Retrieval** | `kernel/sysfilemon.c` + `user/filemon.c` | Privileged `copyout()` + user-space formatter |

**Data Flow:**
User program → `ecall` trap → `syscall.c` dispatcher → `sys_open/read/write/close` → `log_file_access()` → `acquire()` → populate `FileLogEntry` → `release()` → return to user → `filemon` retrieves via `get_log_entry()` → prints formatted audit table.

---

## 3. Implementation Details

### 3.1 Syscall Interception Strategy
File syscalls were intercepted in `kernel/sysfile.c` rather than lower-level VFS or buffer-cache layers (`bio.c`, `buf.c`). At the `sysfile` boundary, the kernel still holds:
- User-supplied arguments (`argstr()`/`argfd()`)
- Process context via `myproc()`
- Clear operation semantics (`OP_OPEN`, `OP_READ`, etc.)

Modifications followed a consistent pattern:
```c
// Inside sys_open/sys_read/sys_write/sys_close
if(argstr/argfd validation fails) return -1;
log_file_access(OP_*, target_str); // ← Interception point
// Original file operation proceeds unchanged
```

### 3.2 Kernel Log Buffer & Spinlock Synchronization
A fixed-size circular buffer (`MAX_LOG_ENTRIES = 128`) stores `FileLogEntry` structs. Each entry contains `pid`, `uid`, `comm`, `op`, `path/fd`, `state`, and a `valid` flag.

Concurrency was handled using xv6's `spinlock`:
```c
acquire(&file_log.lock);
// Critical section: copy fields, increment index, set valid=1
release(&file_log.lock);
```
**Rationale:** The critical section spans <20 instructions. Spinlocks avoid scheduler context-switch overhead and guarantee atomicity under high contention (Ch 3.37, Ch 7.2–7.3). The `valid` flag prevents stale reads during buffer wrap-around.

### 3.3 Resolving Header Dependency Cycles
**Problem:** xv6's core headers (`spinlock.h`, `riscv.h`, `proc.h`) lack `#ifndef` include guards. Including `filemon.h` in multiple `.c` files triggered `redefinition of 'struct spinlock'` and `incomplete type` errors due to duplicate preprocessor pasting.

**Solution:**
1. Added standard guards to `spinlock.h` and `riscv.h`:
   ```c
   #ifndef SPINLOCK_H
   #define SPINLOCK_H
   // ... original content ...
   #endif
   ```
2. Enforced strict include ordering in `.c` files: `spinlock.h` → `proc.h` → `filemon.h`
3. Used `extern` declarations in `filemon.h` to avoid circular struct dependencies.

This resolved compilation failures while preserving xv6's monolithic kernel structure (Ch 2.32, Ch 2.36–2.38).

### 3.4 Assembler & Memory Layout Constraints
**Problem:** `trampoline.S` failed with:
```
illegal operands 'li a0, ((MAXVA-PGSIZE)-PGSIZE)'
```
GNU GAS (≥2.38) rejects nested parentheses or chained arithmetic in the `li` pseudo-instruction's immediate field, even though the expression is mathematically valid.

**Solution:** Precomputed the virtual address in `kernel/memlayout.h`:
```c
#define TRAPFRAME 0x3FFFFFE000L  // = MAXVA - 2*PGSIZE
```
This bypasses the assembler's rigid expression parser while preserving xv6's static memory layout. Page-table mapping, trap transitions, and mode-switch mechanics remain unchanged (Ch 2.40, Ch 1.43).

### 3.5 Self-Referential Logging & Process Filtering
**Problem:** Running `filemon` triggered `sys_write` for every `printf()` call, flooding the circular buffer with `WRITE fd:1` entries from the monitor's own PID. Older audit data was overwritten before retrieval.

**Solution:** Added a process-name guard at the top of `log_file_access()`:
```c
struct proc *p = myproc();
if (strncmp(p->name, "filemon", 8) == 0) return;
```
This isolates audit I/O from monitored syscalls, mirroring real-world logging daemons (e.g., `auditd`) that use dedicated channels or process exclusion (Ch 2.6, Ch 3.11).

---

## 4. Testing & Validation

### 4.1 Functional Testing
Executed standard file operations:
```sh
$ echo "test1" > a.txt
$ cat a.txt
$ filemon
```
**Result:** Logs correctly captured `OPEN a.txt`, `WRITE fd:1`, `CLOSE fd:3` with accurate PID, UID, and process state. Console noise (`fd:1`/`fd:2`) remained visible but non-corrupting.

### 4.2 Concurrency Stress Test
`stress_test.c` spawns 8 child processes, each performing 50 write operations to `stress_target.txt`. Total: 416 log events.

Validation Metrics:
- ✅ No garbled or interleaved `FileLogEntry` fields
- ✅ `valid` flags correctly indicate fresh vs. overwritten entries
- ✅ Circular buffer wraps predictably; newer entries replace oldest without corruption
- ✅ Spinlock serialization prevents `next_index` race conditions

### 4.3 Security Enforcement
- Non-root retrieval attempts return `-1`
- `copyout()` validates user-space destination pointers
- Kernel-mode execution prevents user-space bypass or log tampering

---

## 5. Security & Concurrency Analysis

| Aspect | Implementation | OS Principle Alignment |
|--------|----------------|------------------------|
| **Privilege Boundary** | `uid == 0` check in `sys_get_log_entry` | Ch 1.50: Protection controls access to system resources |
| **Memory Safety** | `copyout()` with pagetable validation | Ch 2.22: Syscall interface returns status safely |
| **Atomicity** | `acquire()/release()` around struct assignment | Ch 3.37: Prevents `counter++` interleaving corruption |
| **Audit Isolation** | `strncmp` process filter | Ch 2.6: Logging tracks resource usage without self-interference |
| **Toolchain Resilience** | Precomputed `TRAPFRAME`, include guards | Ch 2.32, 2.40: Separate compilation requires deterministic preprocessing |

**Limitations & Future Work:**
- FD-to-filename resolution requires traversing `myproc()->ofile[] → inode → directory entry` (not natively supported in xv6 VFS)
- Circular buffer lacks timestamping (`mycpu()->ticks` could be added)
- Log rotation/persistence to disk could be implemented via a background daemon
