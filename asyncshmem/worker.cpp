#include <shmem.h>
#include "list.h"
#include "tsp.h"


#define MASTER_PE 0
extern Msg_t msg_in;
extern int Dist[];
extern int newshortestlen, isnewpath, isdone, NumProcs, mype, isshortest, NumCities;
extern shmemx_am_mutex lock_shortestlen, lock_queue, lock_workers_stack;
extern Path Shortest;
extern volatile int nwait;
extern int* waiting;     // to save peids
extern LIST queue;
int temp_buf;

void Worker ()
{ 
  const int worker_pe = hclib::shmem_my_pe();

  newshortestlen = INTMAX;
  isdone = 0; isnewpath = 0;
  Msg_t outbuf[NumCities];
  
  hclib::shmem_barrier_all();

  hclib::async_remote([worker_pe] {
    shmemx_am_mutex_lock(&lock_workers_stack);
    *(waiting+nwait) = worker_pe;
    nwait++;
    shmemx_am_mutex_unlock(&lock_workers_stack);
  }, MASTER_PE);

  // shmemx_am_request(MASTER_PE, hid_SUBSCRIBE, NULL, 0);
  // shmemx_am_quiet();

  while (1) {

    if (hclib::shmem_int_swap(&isnewpath,0,mype)) { 
	//printf("Worker %d just received new path\n",mype);
	//fflush(stdout);

    	msg_in.visited++;

    	if (msg_in.visited==NumCities) {
	  //printf("Worker %d checking for short distance\n",mype);
	  //fflush(stdout);
    	  int d1 = Dist[(msg_in.city[NumCities-2])*NumCities + msg_in.city[NumCities-1]];
    	  int d2 = Dist[(msg_in.city[NumCities-1]) * NumCities ];

    	  if (d1 * d2) { 
    	     // both edges exist 
    	     msg_in.length += d1 + d2;
    	  
    	     // if path is good, send it to master
    	     if (msg_in.length < newshortestlen) {
                shmemx_am_mutex *lock_shortestlen_addr = &lock_shortestlen;
                Path *Shortest_addr = &Shortest;

                Msg_t msg_to_send;
                memcpy(&msg_to_send, &msg_in, sizeof(msg_to_send));

                printf("Worker %d sending shortest path of %d to master\n", hclib::shmem_my_pe(), msg_in.length);
                hclib::async_remote([msg_to_send, lock_shortestlen_addr, Shortest_addr] {
                    static int bpath=0;
                    shmemx_am_mutex_lock(lock_shortestlen_addr);
                    if (msg_to_send.length < Shortest_addr->length) {
                        bpath ++;
                        printf("Master: Got best path %d, length = %d\n", bpath,
                            msg_to_send.length);
                        fflush(stdout);
                        Shortest_addr->Set(msg_to_send.length, (int *)msg_to_send.city, NumCities);
                        newshortestlen = msg_to_send.length;
                        hclib::shmem_int_swap(&isshortest,1,mype); // communication with self is allowed within AM handler
                    }
                    shmemx_am_mutex_unlock(lock_shortestlen_addr);
                }, MASTER_PE);

    	        // shmemx_am_request(MASTER_PE, hid_BESTPATH, &msg_in, sizeof(Msg_t));
             }
    	  }
    	  // not a valid path, ask for another partial path
    	}
	else {
        // For each city not yet visited, extend the path:
        // (use of symmetric buffer msg_in to compute every extended path)
        int length = msg_in.length;
        int pathcnt = 0;
        for (int i=msg_in.visited-1; i<NumCities; i++) {
            // swap city[i] and city[visted-1]
            if (i > msg_in.visited-1) {
                int tmp = msg_in.city[msg_in.visited-1];
                msg_in.city[msg_in.visited-1] = msg_in.city[i];
                msg_in.city[i] = tmp;
    	    }
    	  
            // visit city[visited-1]
    	    if (int d = Dist[(msg_in.city[msg_in.visited-2])*NumCities +
                    msg_in.city[msg_in.visited-1] ]) {
                msg_in.length = length + d;
                if (msg_in.length < newshortestlen) { 
                    memcpy(&outbuf[pathcnt],&msg_in,sizeof(Msg_t));
                    pathcnt++;
                }
            }
        }
        if(pathcnt) {
            hclib::async_remote([pathcnt] (void *user_data, size_t nbytes) {
                Msg_t *outbuf = (Msg_t *)user_data;

                for(int i=0;i<pathcnt;i++) {
                    Path* P = new Path();
                    P->Set (outbuf[i].length, (int *)outbuf[i].city, outbuf[i].visited);
                    shmemx_am_mutex_lock(&lock_queue);
                    queue.Insert(P, outbuf[i].length);
                    shmemx_am_mutex_unlock(&lock_queue);
                }
            }, MASTER_PE, outbuf, NumCities * sizeof(Msg_t));

            // shmemx_am_request(MASTER_PE, hid_PUTPATH, outbuf, pathcnt*sizeof(Msg_t));
        }
    }

    hclib::async_remote([worker_pe] {
        shmemx_am_mutex_lock(&lock_workers_stack);
        *(waiting+nwait) = worker_pe;
        nwait++;
        shmemx_am_mutex_unlock(&lock_workers_stack);
    }, MASTER_PE);

    // shmemx_am_request(MASTER_PE, hid_SUBSCRIBE, NULL, 0);
	// shmemx_am_quiet();

    } /* end of new path check */

    if (hclib::shmem_int_swap(&isdone,0,mype)) { 
        printf("Worker %d received DONE_TAG ..\n", mype); 
        break; 
    }

  } /* end of while(1) */
  hclib::shmem_barrier_all();

}
