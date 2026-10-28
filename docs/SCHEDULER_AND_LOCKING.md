# Scheduler, allocator, and locking invariants

This document records failure boundaries that are easy to break during an otherwise local
refactor. Read it before changing scheduler state, allocator wrappers, exec/image publication,
or lock queues. (GUI sheet/compositor sections were removed with the GUI; see note at bottom.)

## Scheduler state has two different scopes

The scheduler has a global boot/fatal gate and a per-CPU runtime nesting counter. They solve
different problems:

- `scheduler_start()` opens scheduling after boot has initialized all CPUs and queues.
  `scheduler_stop()` closes scheduling for fatal/panic handling.
- `disable_scheduler()` increments only the current CPU's `scheduler_disable_depth`;
  `enable_scheduler()` decrements it without underflow.
- `scheduler_is_enabled()` is true only when the global gate is open and the current CPU's
  depth is zero.

Never use `enable_scheduler()` to start the system scheduler. Never replace a save/restore
pair with a naked enable: the caller may have entered with a nonzero depth. A function that
changes the depth must restore its entry value on every error path and before a non-returning
user-mode handoff.

`PROCESSOR_INFO.current_task`, `syscall_user_rsp`, and `syscall_user_rax` have fixed
offsets consumed by `kernel/intr/handler.S`. The compile-time assertions in
`kernel/task/scheduler.cpp` are intentional ABI checks. New per-CPU state belongs after those
assembly fields unless every assembly offset is reviewed and updated together.

## Lock and allocator rules

liballoc's internal lock does not mask interrupts. If an interrupt arrives while a CPU owns that
lock and the interrupt path allocates or frees memory, the same CPU can spin forever waiting for
the interrupted owner. Kernel allocation entry points therefore use IRQ-saving wrappers in
`kernel/memory/heap.cpp`; the kernel link must retain `--wrap=malloc`, `--wrap=calloc`,
`--wrap=free`, `--wrap=realloc`, and `--wrap=aligned_alloc`. `__real_*` calls in the
wrapper implementation are the deliberate raw-liballoc entry points. Do not add bypasses
elsewhere. `calloc` must continue to route the underlying allocation through wrapped
`malloc`. If changing that ABI, preserve overflow checking as well as IRQ safety.

Never call malloc/free, VFS, decoding, sleep/yield, or a blocking mutex while holding a spin
lock. Never do blocking work with scheduler preemption disabled. If a protected operation needs
a large copy or allocation, capture/pin the required state, release the spin lock, do the work,
then validate the generation/lifetime before publishing the result.

## Exec and IPC publication

`process_execve()` prepares a private ELF image while scheduling is available, then publishes
the complete image in a short `create_thread_lock` section. Keep filesystem reads, allocation,
and other blocking work out of that commit section.

The process IPC queue object remains stable across exec. Exec detaches its old message nodes
under the queue lock and frees detached nodes after unlocking. `ipc_send()` transfers message
ownership to the queue at publication; sender code must not read or write the message afterward.
Queue enqueue routines copy the node index before unlocking because a consumer may remove and
free the node immediately.

Known limitation: simultaneous `execve()` from multiple threads in one process and stale
threads still entering user mode after another thread commits exec need stronger page-directory
lifetime references/deferred reclamation. Do not claim this race is solved by the boot/readiness
or allocator fixes.

## GUI state (removed)

The sheet manager / compositor / task-dock sections from the OpenXJ380 version of this document
no longer apply: GUI was removed and the GUI XAPI syscalls are stubbed to `-ENOSYS`. If GUI
ever returns, re-add those invariants.

## Review and validation checklist

For changes touching these paths:

- Search for every `disable_scheduler()` and prove exact depth restoration on all returns and
  handoffs.
- Search for `malloc`, `free`, VFS, mutex, sleep/yield, and decoding calls inside spin-lock or
  scheduler-disabled regions.
- Confirm the Ninja link rule still lists all five allocator wrappers.
- Keep assembly offsets and their static assertions in sync.
- Build with `python3 tools/gen_ninja.py` and `ninja`.
- Run relevant unit tests and inspect `git diff --check`.

Build success alone does not validate scheduler liveness.
