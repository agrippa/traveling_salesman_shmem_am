//////////////////////////////////////////////////////////////////////////
// Traveling Salesman Problem with OpenSHMEM Active Messages
// Note: this is a C++ program.
//
// This program follows a master - worker communication pattern
//   - Process 0 manages the queue of incomplte paths, and keeps
//   track of the best path.
//   - All other processes are workers that get and put jobs from and into 
//   the queue. Each time they get a path, they are also informed
//   about the best length so far.
//
// Note that, unlike previous examples, this one does not work with
// only one process.
//
// Starting city is assumed to be city 0.
//////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <shmem.h>

#include "list.h"
#include "tsp.h"
#include "master.h"

// Symmetric Vars
#define DIST_MAX_SIZE 500
int *waiting;
int Dist[DIST_MAX_SIZE]; 
int newshortestlen, isnewpath, isdone, NumProcs, mype, isshortest, NumCities;
int *best_path_list, *subscribe_list, *put_path_list;
Msg_t *worker_new_path;
volatile int nwait;
long pSync[_SHMEM_BCAST_SYNC_SIZE];
Msg_t msg_in, worker_shortest_path;
char* input_file;


///////////////////////////////////////////////////////////////////////////

Path::Path ()
{ 
  length=0; 
  visited=1;
  for (int i=0; i<NumCities; i++) city[i]=i;
}

void Path::Set (int len, int *cit, int vis)
{
  length = len;
  memcpy (city, cit, NumCities*sizeof(int));
  visited = vis;
}

void Path::Print()
{
  for (int i=0; i<visited; i++) 
     printf("  %d", city[i]);
  printf("; length = %d\n", length);
}

///////////////////////////////////////////////////////////////////////////


void Fill_Dist( void )
{

  FILE* inptr = fopen(input_file, "r");

  for( int i = 0 ; i<DIST_MAX_SIZE ; i++ ) 
        Dist[i]=0;
  

  if (mype == 0) { 
     fscanf(inptr,"%d", &NumCities);
     printf("Number of cities: %d\n", NumCities);
     for( int i = 0 ; i<NumCities ; i++ ) {
        for( int j = 0 ; j<NumCities ; j++ ) {
           fscanf(inptr,"%d", &Dist[i*NumCities + j]);
           printf("%5d", Dist[i*NumCities+j] );
	}
        printf("\n");
     }
  }

  
  // Defining pSnyc array for collective operations
  for (int i=0; i < _SHMEM_BCAST_SYNC_SIZE; i++) 
	  pSync[i] = _SHMEM_SYNC_VALUE;


  // global operation, all processes must call it
  shmem_barrier_all();

  shmem_broadcast32(&NumCities, &NumCities, 1, 0, 0, 0, NumProcs, pSync);
  assert(NumCities<=MAXCITIES);
  if(NumCities*NumCities > DIST_MAX_SIZE)
	  fprintf(stderr, "Increase size of Dist array\n");

  shmem_barrier_all();
  shmem_broadcast32(Dist, Dist, NumCities*NumCities, 0, 0, 0, NumProcs, pSync);
}



int main(int argc, char *argv[])
{
  shmem_init();
  input_file = argv[1];
  mype = shmem_my_pe();
  NumProcs = shmem_n_pes();

  if (NumProcs<2) {
    printf("At least 2 processes are required\n");
    exit(-1);
  }  

  
  // Initialize distance matrix. Ususally done by one process 
  // and bcast, or initialized from a file in a shared file system.
  Fill_Dist();  // process 0 read the data and broadcast it to the others

  best_path_list=(int*)shmem_malloc(sizeof(int)*NumProcs);
  subscribe_list=(int*)shmem_malloc(sizeof(int)*NumProcs);
  put_path_list=(int*)shmem_malloc(sizeof(int)*NumProcs);
  worker_new_path = (Msg_t*)shmem_malloc(sizeof(Msg_t)*NumCities);
  memset(best_path_list,0,sizeof(int)*NumProcs);
  memset(subscribe_list,0,sizeof(int)*NumProcs);
  memset(put_path_list,0,sizeof(int)*NumProcs);
  memset(worker_new_path,0,sizeof(Msg_t)*NumCities);

  if (mype==0) 
    Master();
  else
    Worker();
  
  shmem_barrier_all();

  shmem_free(best_path_list);
  shmem_free(subscribe_list);
  shmem_free(put_path_list);
  shmem_free(worker_new_path);

  shmem_finalize();
  return 0;
}

