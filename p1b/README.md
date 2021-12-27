# CS 537 Project 1B: Warm-up Project (xv6)

Project description can be found [here](https://pages.cs.wisc.edu/~remzi/Classes/537/Fall2021/Projects/p1b.html)

## Building and Testing

- Run `make qemu` to build and launch xv6 operating system.
- Results from running the testsuite can be found in `xv6/tests-out` directory.

## Intro

- Adding system call `getiocount()` to the existing xv6 implementation.

## Summary of changes

- Added new system call number in `syscall.h`.
- Added function pointer to system call in `syscall.c`.
- Implemented system call function in `sysproc.c`.
- Specified system call in `usys.S` such that it can issue a trap instruction.
- Added a user program that calls the implemented system call for testing purpose.

## Notes

- `[SYS_getiocount] sys_getiocount` implies when system call occurs with number `SYS_getiocount`, function pointed to by the function pointer `sys_getiocount` will be called. Since this function is implemented elsewhere, it is declared as extern type.
- If the system call to be implemented requires or updates any info on the calling process, you need to create a wrapper function in `sysproc.c` and implement the exact function in `proc.c`.
- If the function was implemnted in `proc.c` by calling a wrapper function in `sysproc.c` then this function signature used in `proc.c` needs to be specified in `defs.h`.
- To pass arguments to a system call, `argint` function is used (for integers). This implies that system calls in `sysproc.c` are always void in arguments.
