#include <stdint.h>
#include <stdio.h>
#include "hashpipe.h"
#include "hashpipe_databuf.h"



#include <cstdlib>
#include <cstring>
#include <iostream>

#define GNU_SOURCE
#include <link.h>
#include <omp.h>



#include <spead2/common_thread_pool.h>
#include <spead2/recv_udp.h>
#include <spead2/recv_heap.h>
#include <spead2/recv_live_heap.h>
#include <spead2/recv_ring_stream.h>
#include <spead2/recv_udp_ibv.h>



#define ARMORTISER_SIZE 256



#define CACHE_ALIGNMENT         256
#define N_INPUT_BLOCKS          3 
#define N_OUTPUT_BLOCKS         3 //changed by weiyu




struct Recv_packet
{
   // spead2::recv::heap heap;
    //boost::shared_ptr<spead2::recv::heap> fheap;
    uint64_t timestamp;
    uint64_t fengid;
    uint8_t  * payloadPtr;
};

/* INPUT BUFFER STRUCTURES
  */
typedef struct Xengine_input_block_header {
   uint64_t mcnt;                    // mcount of first packet
} Xengine_input_block_header_t;

typedef uint8_t Xengine_input_header_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(Xengine_input_block_header_t)%CACHE_ALIGNMENT)
];

typedef struct Xengine_input_block {
   Xengine_input_block_header_t header;
   Xengine_input_header_cache_alignment padding; // Maintain cache alignment
   Recv_packet  my_recv_packet_pool[ARMORTISER_SIZE]; 
} Xengine_input_block_t;

typedef struct Xengine_input_databuf {
   hashpipe_databuf_t header;
   Xengine_input_header_cache_alignment padding;
   Xengine_input_block_t block[N_INPUT_BLOCKS];
} Xengine_input_databuf_t;


/*
  * OUTPUT BUFFER STRUCTURES
  */
typedef struct Xengine_output_block_header {
   uint64_t mcnt;
} Xengine_output_block_header_t;

typedef uint8_t Xengine_output_header_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(Xengine_output_block_header_t)%CACHE_ALIGNMENT)
];

typedef struct Xengine_output_block {
   Xengine_output_block_header_t header;
   Xengine_output_header_cache_alignment padding; // Maintain cache alignment
  // multi_array::array_ref<std::complex<int32_t>, 4> my_visibilities;
  uint64_t feng_out;
} Xengine_output_block_t;

typedef struct Xengine_output_databuf {
   hashpipe_databuf_t header;
   Xengine_output_header_cache_alignment padding;
   Xengine_output_block_t block[N_OUTPUT_BLOCKS];
} Xengine_output_databuf_t;


/*
  * Reorder OUTPUT BUFFER STRUCTURES
  */


/*
 * INPUT BUFFER FUNCTIONS
 */
hashpipe_databuf_t *Xengine_input_databuf_create(int instance_id, int databuf_id);

static inline Xengine_input_databuf_t *Xengine_input_databuf_attach(int instance_id, int databuf_id)
{
    return (Xengine_input_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int Xengine_input_databuf_detach(Xengine_input_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline void Xengine_input_databuf_clear(Xengine_input_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline int Xengine_input_databuf_block_status(Xengine_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_input_databuf_total_status(Xengine_input_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}

static inline int Xengine_input_databuf_wait_free(Xengine_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_input_databuf_busywait_free(Xengine_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_input_databuf_wait_filled(Xengine_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_input_databuf_busywait_filled(Xengine_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_input_databuf_set_free(Xengine_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_input_databuf_set_filled(Xengine_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}

/*
 * OUTPUT BUFFER FUNCTIONS
 */

hashpipe_databuf_t *Xengine_output_databuf_create(int instance_id, int databuf_id);


static inline void Xengine_output_databuf_clear(Xengine_output_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline Xengine_output_databuf_t *Xengine_output_databuf_attach(int instance_id, int databuf_id)
{
    return (Xengine_output_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int Xengine_output_databuf_detach(Xengine_output_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline int Xengine_output_databuf_block_status(Xengine_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_output_databuf_total_status(Xengine_output_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}

static inline int Xengine_output_databuf_wait_free(Xengine_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_output_databuf_busywait_free(Xengine_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
}
static inline int Xengine_output_databuf_wait_filled(Xengine_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_output_databuf_busywait_filled(Xengine_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_output_databuf_set_free(Xengine_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

static inline int Xengine_output_databuf_set_filled(Xengine_output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}



/*
 * Reorder OUTPUT BUFFER FUNCTIONS
 */

