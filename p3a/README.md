# CS 537 Project 3A: Parallel Zip

Project description can be found [here](https://pages.cs.wisc.edu/~remzi/Classes/537/Fall2021/Projects/p3a.html)

## Building and Testing

- Run `make` to build the project.
- Results from running the testsuite are not made available in the repo since the test input files are very large.

## Intro

Goal is to parallelize the compression process. Need to identify what can be done in parallel, what needs to be done serially and how to be as fast as possible.

- The main thread M divides the input file into fixed-size chunks, and dispatch those chunks sequentially to the **Compressor** threads (denoted by C), which run in parallel. The compressed data are stored in reusable buffers.

- When a C thread finishes compressing its chunks, it signals the **Writer** thread (denoted by W).

- The W thread writes the buffered data (if any) to stdout. When a write completes, the W thread notifies the M thread that a buffer is now free so that M can dispatch the next chunk to a new C thread. Asuuming that the W thread is always slower than the C threads, the writer will only be idle when the very first chunk is being processed by a C thread.

- Determining number of threads to create. On Linux, this means using interfaces like `get_nprocs()`. So, we create threads to match the number of CPU resources available.

- Since the bottleneck is writing out compressed data, we create 1 W thread and N-1 C threads, where N is the number of CPU cores (assuming all cores are available). While parallelization will yield speed up, each thread’s efficiency in performing the compression is also of critical importance. Thus, making the core compression loop as CPU efficient as possible is needed for high performance.

- We accessing the input file efficiently by using `mmap()` system call. By mapping the input file into the address space, we can then access bytes of the input file via pointers and do so quite efficiently.

- The M thread calls `mmap()` to memory-map the entire file to some memory location (with parameters PROT_READ and MAP_SHARED). When it dispatches a chunk to a C thread, it tells C what the starting address and the chunk size are. The C thread calls madvis (MADV_SEQUENTIAL) to tell the OS that subsequent reads will be strictly sequential. Then it reads the input file as if it accesses the memory.

## Threads description

**Main Thread (denoted by M)** 
- Creates a W thread, waits for W thread to complete (i.e. all data are compressed and all writes are complete).
- Allocate N arrays of (char, count) pairs of chunck size (only once).
- Dispatches chuncks (and an array) to C threads together with allocated buffers, and wait for C threads to complete,
- When signaled of a completion by a C, dispatches the next chunck to a new C threads (C thread creation) until there are no free CPU cores, and wait for W threads to free up buffers
- Free the arrays at the end

**Compressor threads (# of CPUs, denoted by C)**
- Compresses data,
- Store the compressed data (int, char) pair in the designated array (could be struct)
- Send a signal to M and W when done.

**W Writer thread (only 1, denoted by W)**
- Wait for (any) C thread completion signal, and checks if the next chunck is compressed.
- If not, goes back to sleep (hopefully this will never happen, or happen only at the very beginning);
- Else, coaleces compressed data created by C threads, and write them out
- Goes back to sleep if the next chunck is not compressed yet (hopefully this will never happen).
- If everything has been written out, sends a signal to M and returns.

## Chunk description

A chunk can be in 3 possible states: Assigned, Compressed, Freed.
- **Assigned** 
    * case1: chunk is waiting for its mapped slot buffer to become free.
    * case2: chunk is being compressed by a C thread to a mapped slot buffer.
- **Compressed** 
    case1: chunk has been compressed into a slot buffer and is waiting for the W thread to become available to write it to output
    case2: chunk is being written by the W thread from its mapped slot buffer to output.
- **Freed**
    chunk has been compressed and written (attached to previous compressor by W thread from its slot buffer. The corresponding slot 
    buffer is freed up and W thread signals for the C thread to use the freed slot buffer.

Note: Follow the comments in `pzip.c` for further details.