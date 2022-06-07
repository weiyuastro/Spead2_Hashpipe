/* Xengine_databuf.c
 *
 * Routines for creating and accessing main data transfer
 * buffer in shared memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>
#include "Xengine_databuf.h"


hashpipe_databuf_t *Xengine_input_databuf_create(int instance_id, int databuf_id)
{
    /* Calc databuf sizes */
    size_t header_size = sizeof(hashpipe_databuf_t)
                       + sizeof(Xengine_input_header_cache_alignment);
    size_t block_size  = sizeof(Xengine_input_block_t);
    int    n_block = N_INPUT_BLOCKS;
    return hashpipe_databuf_create(
        instance_id, databuf_id, header_size, block_size, n_block);
}

hashpipe_databuf_t *Xengine_output_databuf_create(int instance_id, int databuf_id)
{
    /* Calc databuf sizes */
    size_t header_size = sizeof(hashpipe_databuf_t)
                       + sizeof(Xengine_output_header_cache_alignment);
    size_t block_size  = sizeof(Xengine_output_block_t);
    int    n_block = N_OUTPUT_BLOCKS;
    return hashpipe_databuf_create(
        instance_id, databuf_id, header_size, block_size, n_block);
    
}



