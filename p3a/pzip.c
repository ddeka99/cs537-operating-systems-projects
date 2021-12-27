#include <stdio.h>          // IO operations
#include <stdlib.h>         // portable int type
#include <stdint.h>         // malloc
#include <fcntl.h>          // file descriptor fields
#include <assert.h>         // assert
#include <math.h>           // ceil
#include <pthread.h>        // pthreads
#include <sys/mman.h>       // mmap
#include <sys/stat.h>       // file stats
#include <sys/sysinfo.h>    // get nprocs for CPU cores
#include <unistd.h>         // close
#include <semaphore.h>      // semaphores

/*
n_chunk chunks are mapped to n_slots slots
Brief Summary:
A chunk can be in 3 possible states: Assigned, Compressed, Freed
Assigned: 
    case1: chunk is waiting for its mapped slot buffer to become free
    case2: chunk is being compressed by a C thread to a mapped slot buffer
Compressed: 
    case1: chunk has been compressed into a slot buffer and is waiting for the W thread to become available to write it to output
    case2: chunk is being written by the W thread from its mapped slot buffer to output
Freed:
    chunk has been compressed and written (attached to previous compressor by W thread from its slot buffer. The corresponding slot 
    buffer is freed up and W thread signals for the C thread to use the freed slot buffer.
*/

// macro constants
//#define MAX_CHUNK_SIZE (500*1024*1024)      // 10MB chunks
#define SLOTS_PER_CPU 10                    // number of slots per CPU
#define UNIT_SIZE 5                         // size of each compressed unit is 5 bytes (4 bytes integer + 1 byte char) in binary

// macro functions
#define MIN(a,b) (((a)<(b))?(a):(b))    // return minimum of two numbers
#define MAX(a,b) (((a)>(b))?(a):(b))    // return maximum of two numbers

// arguments to compressor threads
typedef struct __C_arg_t {
    int id;                     // represent index or id of the chunk that is being compressed
} C_arg_t;

// circular buffer struct
typedef struct __buf_t {
    char **data;                // actual data buffer
    sem_t *compressed;          // make W wait for C to compress a chunk into a slot before writing to output
    sem_t *freed;               // make C wait for W to write slot content to output before C starts compressing new chunk to slot
    int n_slots;                // # of slots
} buf_t;

// global variables
char *g_map_begin;              // file start address
char *g_map_end;                // file end address
buf_t *g_buf;                   // slot buffer struct
int g_compressors;              // number of compressor(C) threads
uint64_t g_chunk_size;          // chunk size chosen for compression
uint64_t g_n_chunks;            // number of chunks
uint64_t g_bytes_per_slot;      // size of buffer chosen for each compression thread
FILE *fpout;                    // output pointer

// get size of file specified by file descriptor
uint64_t get_file_size (int fd)
{
	struct stat stat_buf;
    fstat(fd, &stat_buf); // populate stat_buf with file stats using fd as descriptor
    return stat_buf.st_size;
}

// writes to output from slot buffer
void write_out(uint32_t count, unsigned char ch, FILE* fpout)
{
    fwrite(&count, sizeof(uint32_t), 1, fpout);
    fwrite(&ch, sizeof(unsigned char), 1, fpout);
}

// writes the compressed unit to the mapped slot buffer
void write_to_slotbuf (uint32_t count, char ch, char* ptr)
{
    *(uint32_t *) ptr = count;
    ptr += sizeof(uint32_t);
    *ptr = ch;
}

// compressor routine
void *compressor_routine (void *arg)
{
    // unpack arguments
    C_arg_t compressor = *(C_arg_t *) arg;
    int id = compressor.id;

    int chunck_index = id;
    char* chunk_ptr = g_map_begin + id * g_chunk_size; // current pos in file chunk currently at the beginning of chunk
    char* chunk_end = MIN (chunk_ptr + g_chunk_size, g_map_end);
    while (chunk_ptr < g_map_end)
    {
        // corresponding mapped buffer slot
        int slot = chunck_index % (g_buf->n_slots); 
        
        // wait until this mapped buffer slot is free
        sem_wait (&g_buf->freed[slot]);

        // slot is free, begin compression
        char* slot_ptr = g_buf->data[slot]; // current pos in slot currently at the beginning
        char* slot_end = slot_ptr + g_bytes_per_slot;  // ending address of slot
       
        unsigned char prev_ch = 0;
        uint32_t count = 0;

        // perform compression of chunk into slot buffer
        while (chunk_ptr < chunk_end)
        {
            if(*chunk_ptr == '\0')
            {
                chunk_ptr++;
                continue;
            }

            // read char
            char curr_ch = *chunk_ptr;
            chunk_ptr++;

            // initialize
            if (count == 0) 
            {
                prev_ch = curr_ch;
                count = 1;
            }
            // count in range and char repeats
            else if (count < UINT32_MAX && curr_ch == prev_ch)
            {   
                count += 1;   
            }
            // counter full or a different char
            else 
            {   
                write_to_slotbuf (count, prev_ch, slot_ptr);
                slot_ptr += UNIT_SIZE; // 4 bytes for count, 1 byte for char
                prev_ch = curr_ch;
                count = 1; // reinitialize
            }
        }

        // last compressed unit hasn't been written to slotbuf
        if (count > 0) 
        {
            write_to_slotbuf (count, prev_ch, slot_ptr);
            slot_ptr += UNIT_SIZE;
        }
        /* if slot is not full, write a (0,0) pair signaling the EOF
        slot will be full only if there are no adjacent repeating character, worst case scenario */
        if (slot_ptr < slot_end)
            write_to_slotbuf (0, 0, slot_ptr);

        // signal W to start writing if it was waiting, definitely be the case in the first pass.
        sem_post (&g_buf->compressed[slot]);
        
        // go to the next chunk if the current C is still running
        chunk_ptr += (g_compressors - 1) * g_chunk_size;
        chunck_index += g_compressors; 
    }
    return 0;
}

// writer routine
void *writer_routine(void *arg)
{
    // open file, stdout by default, can be changed using shell redirection
    FILE *fpout = stdout;
    assert(fpout != NULL);

    // transfer data from slot buffer to output
    int chunk_index = 0;
    int prev_count = 0;
    unsigned char prev_ch;

    while (chunk_index < g_n_chunks)
    {
        // corresponding mapped buffer slot
        int slot = chunk_index % (g_buf->n_slots); 

        // wait until the compression to this slot is done
        sem_wait (&g_buf->compressed[slot]);

        // slot is free, begin writing
        char* slot_ptr = g_buf->data[slot]; // current pos in slot currently at the beginning
        char* slot_end = slot_ptr + g_bytes_per_slot;  // ending address of slot

        while (slot_ptr < slot_end)
        {
            uint32_t curr_count = *(uint32_t *) slot_ptr; // read count
            slot_ptr += sizeof (uint32_t);
            char curr_ch = *slot_ptr; // read char
            slot_ptr++;

            // initialize
            if (prev_count == 0)
            {
                prev_count = curr_count;
                prev_ch = curr_ch;
            }
            // early exit case, EOF
            else if (curr_count == 0)
                break;
            // write to output if no longer a combination of previous compression unit
            else if (curr_ch != prev_ch)
            {
                write_out (prev_count, prev_ch, fpout);
                prev_count = curr_count;
                prev_ch = curr_ch;
            }
            // still a combination of previous compression unit
            else
            {
                uint64_t temp_count = prev_count + curr_count;
                // overflow, cannot be represented in 32 bits
                if (temp_count > UINT32_MAX) 
                {
                    write_out (UINT32_MAX, prev_ch, fpout);
                    prev_count = temp_count - UINT32_MAX;
                }
                // still a combination but no overflow!
                else 
                {   
                    // update count
                    prev_count = prev_count + curr_count;
                }
            }
        }

        // signal C to start compressing if it was waiting, slot is free not
        sem_post (&g_buf->freed[slot]);  
        // move on to next chunk (writing the corresponding slot)
        chunk_index++;
    }

    // write out the very last unit
    if (prev_count != 0)
        write_out (prev_count, prev_ch, fpout);
    
    // close output
    int rc = fclose (fpout);
    assert (rc == 0);

    return 0;
}

// perform parallel compression
int main (int argc, const char* argv[])
{
    // used for various return codes
    int rc = -1;

    // must take atleast one input file as argument
    if (argc < 2)
    {
        printf("pzip: file1 [file2 ...]\n");
        exit(1);
    }

    uint64_t map_size = 0;
    // open the files
    for (int i = 1; i < argc; i++) 
    {
        // for each file get size
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0)
            continue;   
        // get total size
        map_size += get_file_size (fd);
        close(fd);
    }

    fpout = stdout; // by default outputs to stdout, can be changed using shell redirection
    assert (fpout != NULL);

    // create a memory map
    int fd = open("/dev/zero", O_RDWR);
    g_map_begin = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
	//g_map_begin = mmap (NULL, map_size, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);
    assert (g_map_begin != MAP_FAILED);
    // map the file contents into mmap
    g_map_end = g_map_begin;
    for (int i = 1; i < argc; i++) 
    {
        FILE *f = fopen (argv[i], "rb");
        if (f == NULL)
            continue; 
        fseek(f, 0, SEEK_END);
        uint64_t bytes = ftell(f);
        fseek(f, 0, SEEK_SET);

        rc = (int) fread(g_map_end, (uint64_t) bytes, 1, f);
        g_map_end += bytes;

        fclose(f);
    }
    
    uint64_t MAX_CHUNK_SIZE;

    if (map_size < 15 * 1024 * 1024)
        MAX_CHUNK_SIZE = 4 * 1024 * 1024;
    else if (map_size < 1.5 * 1024 * 1024 * 1024)
        MAX_CHUNK_SIZE = 400 * 1024 * 1024;
    else if (map_size < 2.5 * 1024 * 1024 * 1024)
        MAX_CHUNK_SIZE = 800 * 1024 * 1024;
    else
        MAX_CHUNK_SIZE = 1.9 * 1024 * 1024 * 1024;

    // partition input file into chunks
    g_chunk_size = MIN (map_size, MAX_CHUNK_SIZE); // handling case when file size can be smaller than chosen chunk size
    g_n_chunks = (uint64_t) ceil (map_size * 1.0 / g_chunk_size);
    g_bytes_per_slot = g_chunk_size * UNIT_SIZE;

    // create a circular buffer
    g_buf = malloc (sizeof(buf_t));
    assert (g_buf != NULL);
    
    // number of CPU cores
    int cpu_cores = get_nprocs(); // number of processors available in the system
    assert (cpu_cores > 0); // must have atleast 1 available processor

    // number of slots
    g_buf->n_slots = SLOTS_PER_CPU * (cpu_cores - 1); // 1 core reserved for writer(W)

    // reserving space for the internal data buffers, a buffer for each slot
    g_buf->data = malloc (g_buf->n_slots * sizeof(char*)); // array of pointers to slot memory
    for(int i = 0; i < g_buf->n_slots; i++)
    {   
        g_buf->data[i] = malloc(g_bytes_per_slot); // slot memory
        assert (g_buf->data[i] != NULL);
    }

    // reserving space for compressed semaphores array
    g_buf->compressed = malloc(g_buf->n_slots * sizeof(sem_t));
    assert (g_buf->compressed != NULL);
    // reserving space for freed semaphores array
    g_buf->freed = malloc(g_buf->n_slots * sizeof(sem_t));
    assert (g_buf->freed != NULL);

    // initialize semaphores
    for(int i = 0; i < g_buf->n_slots; i++)
    {
        rc = sem_init(&g_buf->compressed[i], 0, 0); // value = 0 implies fork-join
        rc |= sem_init(&g_buf->freed[i], 0, 1); // value = 1 implies mutex
        assert (rc == 0);
    }

    // create N compressor(C) threads
    g_compressors = cpu_cores > 1? cpu_cores - 1: 1; // guard against single-core edge case
    pthread_t c_threads[g_compressors];
    C_arg_t cargs[g_compressors];
    for (int i = 0; i < g_compressors; i++)
    {
        cargs[i].id = i;
        rc = pthread_create(&c_threads[i], NULL, compressor_routine, &cargs[i]);
        assert (rc == 0);
    }

    // create one writer(W) thread
    pthread_t w_thread;
    rc = pthread_create (&w_thread, NULL, writer_routine, NULL);
    assert (rc == 0);

    // main should wait for W to finish
    pthread_join (w_thread, (void **) &rc);

    // un-mmap the file
    rc = munmap (g_map_begin, map_size); 
    assert (rc == 0); 
    
    // free up slot data buffers
    for (int i = 0; i < g_buf->n_slots; i++)
        free (g_buf->data[i]);
    free (g_buf->data);

    // free up semaphores
    free (g_buf->compressed);
    free (g_buf->freed);

    // free up slot
    free (g_buf);

    return EXIT_SUCCESS;
}