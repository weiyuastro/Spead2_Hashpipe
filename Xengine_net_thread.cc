/*
 * Xengine_net_thread.c
 *
 * This allows you to receive two numbers in two pakets from local ethernet, and then write these two received numbers into shared memory blocks. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include "hashpipe.h"
#include "Xengine_databuf.h"



#define PKTSIZE 10 //bytes of packet size

//defining a struct of type hashpipe_udp_params as defined in hashpipe_udp.h
static struct hashpipe_udp_params params;

static int init(hashpipe_thread_args_t * args)
{
       // hashpipe_status_t st = args->st;
       // strcpy(params.bindhost,"127.0.0.1");
        //selecting a port to listen to
       // params.bindport = 5009;
       // params.packet_size = 0;
       // hashpipe_udp_init(&params);
       // hashpipe_status_lock_safe(&st);
       // hputi8(st.buf, "NPACKETS", 0);
       // hputi8(st.buf, "NBYTES", 0);
       // hashpipe_status_unlock_safe(&st);
        return 0;

}

static void *run(hashpipe_thread_args_t * args)
{
    Xengine_input_databuf_t *db  = (Xengine_input_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char * status_key = args->thread_desc->skey;
    

    // Spead2 Related Codes
    uint64_t fengid;
    uint64_t timestamp;
    uint64_t frequency;
    uint8_t * payloadPtr_p;
    bool fengPacket = false;
    int buffer_heap_cnt=0;
    uint64_t expected_timestamp;


    spead2::thread_pool worker;
    std::shared_ptr<spead2::memory_pool> pool = std::make_shared<spead2::memory_pool>(16384, 26214400, 12, 8);

    spead2::recv::ring_stream<> stream(
        worker,
        spead2::recv::stream_config().set_memory_allocator(pool));

    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address_v4::any(), 7148);
    
    //stream.emplace_reader<spead2::recv::udp_reader>(
    //    endpoint, spead2::recv::udp_reader::default_max_size, 8 * 1024 * 1024);

    auto interface_address2 = boost::asio::ip::address::from_string("10.2.0.30");
    stream.emplace_reader<spead2::recv::udp_ibv_reader>(
        endpoint, interface_address2, spead2::recv::udp_ibv_config::default_max_size,
        spead2::recv::udp_ibv_config::default_buffer_size, 0, spead2::recv::udp_ibv_config::default_max_poll);

    std::cout << "after network initialization" <<std::endl;


    /* Main loop */
    int i, rv,input,n;
    uint64_t mcnt = 0;
    int block_idx = 0;
    int a, b; 
    char *str_rcv,*str_a,*str_b;
    str_rcv = (char *)malloc(PKTSIZE*sizeof(char));
    str_a = (char *)malloc(PKTSIZE*sizeof(char));
    str_b = (char *)malloc(PKTSIZE*sizeof(char));
    uint64_t npackets = 0; //number of received packets
    uint64_t nbytes = 0;  //number of received bytes
    char data[PKTSIZE]; //packet buffer
    int second_pkt; //second packet
    //Xengine_input_databuf_set_free(db, block_idx); 

    while (run_threads()) {

        hashpipe_status_lock_safe(&st);
        hputi4(st.buf,"A",a); 
        hputi4(st.buf,"B",b);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "NETBKOUT", block_idx);
	hputi8(st.buf,"NETMCNT",mcnt);
        hputi8(st.buf, "NPACKETS", npackets);
        hputi8(st.buf, "NBYTES", nbytes);
        hashpipe_status_unlock_safe(&st);
 
        // Wait for data
        /* Wait for new block to be free, then clear it
         * if necessary and fill its header with new values.
         */
        while ((rv=Xengine_input_databuf_wait_free(db, block_idx)) != HASHPIPE_OK) {
            if (rv==HASHPIPE_TIMEOUT) {
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked");
                hashpipe_status_unlock_safe(&st);
                continue;
            } else {
                hashpipe_error(__FUNCTION__, "error waiting for free databuf");
                pthread_exit(NULL);
                break;
            }
        }

        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "receiving");
        hashpipe_status_unlock_safe(&st);

        spead2::recv::heap fheap = stream.pop();
        const auto &items = fheap.get_items();

    for (const auto &item : items)
    {
        if(item.id == 0x1600){
          timestamp = item.immediate_value;
        }

        if(item.id == 0x4101){
          fengid = item.immediate_value;
        }

        if(item.id == 0x4103){
          frequency = item.immediate_value;
        }

        if(item.id == 0x4300){
          payloadPtr_p = item.ptr;
          fengPacket = true;
	  std::cout << "timestamp:" << timestamp <<std::endl;
        }

    }

        if(fengPacket && (timestamp-expected_timestamp>0) )
        {
          db->block[block_idx].my_recv_packet_pool[buffer_heap_cnt].fengid=fengid;
          db->block[block_idx].my_recv_packet_pool[buffer_heap_cnt].payloadPtr=payloadPtr_p;
          db->block[block_idx].my_recv_packet_pool[buffer_heap_cnt].timestamp=timestamp;

          buffer_heap_cnt++;
          
        }


          
          if(buffer_heap_cnt==ARMORTISER_SIZE/2)
          {
              // Mark block as full
        	   if(Xengine_input_databuf_set_filled(db, block_idx) != HASHPIPE_OK) {
                	hashpipe_error(__FUNCTION__, "error waiting for databuf filled call");
	                pthread_exit(NULL);
        	   }

             expected_timestamp=db->block[block_idx].my_recv_packet_pool[ARMORTISER_SIZE/2-1].timestamp;

             block_idx = (block_idx + 1) % db->header.n_block;
             buffer_heap_cnt=0;
          }





/*
  n = recvfrom(params.sock,str_rcv,PKTSIZE*sizeof(char),0,0,0);
	//if received packet has data,count packets and bytes,mark it as the first or second packet. 
     
	if (n>0) {
		if(npackets%2){
	                npackets++;
        	        nbytes += n;
			str_b=str_rcv;
		        //transform string to integer
        		b=atoi(str_b);
			second_pkt=1;
		}else{
		        npackets++;
		        nbytes += n;
	      		str_a=str_rcv;
		        //transform string to integer
        		a=atoi(str_a);
		        second_pkt = 0;
			}
	}else {	second_pkt = 0;}

        // if two packets received
        if (second_pkt) {
        	printf("received a is: %d\n",a);
	        printf("received b is: %d\n",b);
        	//sleep(0.1);

	        //move these two numbers to buffer
        	db->block[block_idx].header.mcnt = mcnt;
	        db->block[block_idx].number1=a;
        	db->block[block_idx].number2=b;

	        //display a and b 
        	hashpipe_status_lock_safe(&st);
	        hputi4(st.buf,"A",a);
        	hputi4(st.buf,"B",b);
	        hashpipe_status_unlock_safe(&st);

        	// Mark block as full
        	if(Xengine_input_databuf_set_filled(db, block_idx) != HASHPIPE_OK) {
                	hashpipe_error(__FUNCTION__, "error waiting for databuf filled call");
	                pthread_exit(NULL);
        	}
	}else{continue;}

        // Setup for next block
        block_idx = (block_idx + 1) % db->header.n_block;
        mcnt++;
*/


        /* Will exit if thread has been cancelled */
        pthread_testcancel();
    }

    // Thread success!
    return THREAD_OK;
}

/*
static hashpipe_thread_desc_t Xengine_net_thread = {
    name: "Xengine_net_thread",
    skey: "NETSTAT",
    init: init,
    run:  run,
    ibuf_desc: {NULL},
    obuf_desc: {Xengine_input_databuf_create}
};
*/


static hashpipe_thread_desc_t Xengine_net_thread = {
    name: "Xengine_net_thread",
    skey: "NETSTAT",
    init: NULL,
    run:  run,
    ibuf_desc: {NULL},
    obuf_desc: {Xengine_input_databuf_create}
};


static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&Xengine_net_thread);
}
