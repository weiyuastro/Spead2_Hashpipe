#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include "hashpipe.h"
#include "Xengine_databuf.h"








static void *run(hashpipe_thread_args_t * args)
{

    Xengine_input_databuf_t *db_in = (Xengine_input_databuf_t *)args->ibuf;
    Xengine_output_databuf_t *db_out = (Xengine_output_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char * status_key = args->thread_desc->skey;

    int rv,a,b,c;
    uint64_t mcnt=0;
    int curblock_in=0;
    int curblock_out=0;





    int i,j,i0,i1,min;
    Recv_packet  tmp_my_recv_packet;
    uint64_t timestamp_tmp;
    uint64_t fengid_tmp;
    int buffer_index, Max_buffer_index;
    uint64_t previous_timestamp_tmp;

    //std::deque<boost::shared_ptr<BufferPacket> > buffer;
    //CorrelatorTest myCorrelatorTest;

    int all_heap_cnt;
    int8_t output_real;
    int8_t output_imag;




    while (run_threads()) 
    {

        hashpipe_status_lock_safe(&st);
        hputi4(st.buf, "REORDERBLKIN", curblock_in);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "REORDERBKOUT", curblock_out);
	    hputi8(st.buf,"REORDERMCNT",mcnt);
        hashpipe_status_unlock_safe(&st);

        // Wait for new input block to be filled
        while ((rv=Xengine_input_databuf_wait_filled(db_in, curblock_in)) != HASHPIPE_OK) {
            if (rv==HASHPIPE_TIMEOUT) {
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked");
                hashpipe_status_unlock_safe(&st);
                continue;
            } else {
                hashpipe_error(__FUNCTION__, "error waiting for filled databuf");
                pthread_exit(NULL);
                break;
            }
        }


	// Wait for new output block to be free
	 while ((rv=Xengine_output_databuf_wait_free(db_out, curblock_out)) != HASHPIPE_OK) {
            if (rv==HASHPIPE_TIMEOUT) {
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "block_out");
                hashpipe_status_unlock_safe(&st);
                continue;
            } else {
                hashpipe_error(__FUNCTION__, "error waiting for free databuf");
                pthread_exit(NULL);
                break;
            }
        }


        // Note processing status
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "reorder processing");
        hashpipe_status_unlock_safe(&st);


     fengid_tmp=db_in->block[curblock_in].my_recv_packet_pool[0].fengid;
     db_out->block[curblock_out].feng_out=fengid_tmp;


    // Mark output block as full and advance
        Xengine_output_databuf_set_filled(db_out, curblock_out);
        curblock_out = (curblock_out + 1) % db_out->header.n_block;

    // Mark input block as free and advance
        Xengine_input_databuf_set_free(db_in, curblock_in);
        curblock_in = (curblock_in + 1) % db_in->header.n_block;

    mcnt++;

    //display sum in status
	hashpipe_status_lock_safe(&st);
	hputi4(st.buf,"Reorder",c);
	hashpipe_status_unlock_safe(&st);
    /* Check for cancel */
    pthread_testcancel();

    } 

    return THREAD_OK;   
}




static hashpipe_thread_desc_t Xengine_gpu_thread = {
    name: "Xengine_gpu_thread",
    skey: "REORDERSTAT",
    init: NULL,
    run:  run,
    ibuf_desc: {Xengine_input_databuf_create},
    obuf_desc: {Xengine_output_databuf_create}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&Xengine_gpu_thread);
}

