# CS 537 Project 2B: Scheduling and Virtual Memory in xv6

Project description can be found [here](https://pages.cs.wisc.edu/~remzi/Classes/537/Fall2021/Projects/p2b.html)

## Building and Testing

- Run `make qemu` to build and launch xv6 operating system.
- Results from running the testsuite can be found in `xv6/tests-out` directory.

## Part 1 - Scheduling

- Created `pstat` structure in `pstat.h`.
- Added system call `int settickets(int number)`. Follow P1B summary for details.
- Added system call `int getpinfo(struct pstat *)`. Follow P1B summary for details.
- Update `scheduler()` code to run a picked process for `tickets` number of times. Keep track of the process duration in `ticks`.

## Part 2 - Virtual Memory

- To implement null pointer dereferencing in xv6.
- Change xv6 code to load program into memory from the next page instead of 0 i.e. address 4096 or 0x1000.
- Page size described in `mmu.h`
- Change in `makefile`. Force program to load into memory from address 0x1000.
```
149     $(LD) $(LDFLAGS) -N -e main -Ttext 0x1000 -o $@ $^
```
- Change in `exec.c`. Force process to load into memory from address 0x1000. Update variable `sz` which indicates the beginning of the memory of the program to the first address of the next page i.e. `PGSIZE`.
- Apply same change as before to `copyuvm` which creates a copy of the parent for the child process.
