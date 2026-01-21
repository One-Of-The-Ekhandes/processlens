# ProcessLens

ProcessLens is a small Linux user-space tool built to better understand how running processes
are represented by the operating system.

Instead of relying on abstractions, it reads directly from the `/proc` filesystem and exposes
a few core signals about a process in a simple, inspectable way.

This project is intentionally minimal and focused.

---

## What it does

For a given process ID (PID), ProcessLens reads:

- the current process state  
- resident memory usage (VmRSS)  
- number of threads  

All data is obtained from kernel-exposed files under `/proc`.

It also supports listing the top *N* processes by memory usage to observe system-level behavior.

---

## Why this project exists

The goal of ProcessLens is not to replace existing tools like `top` or `ps`.

It exists to:
- understand how Linux exposes process information
- practice reading and interpreting `/proc` safely
- work with real system data instead of simulated inputs
- build something small that is easy to reason about and explain

---

## Build

```bash
make
