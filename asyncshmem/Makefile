

CFLAGS =  -O2 -I$(HCLIB_ROOT)/include -I$(HCLIB_HOME)/modules/openshmem/inc \
		  -I$(HCLIB_HOME)/modules/openshmem-am/inc -std=c++11
LDFLAGS =  -L$(HCLIB_ROOT)/lib -L$(HCLIB_HOME)/modules/openshmem/lib \
		   -L$(HCLIB_HOME)/modules/openshmem-am/lib \
		   $(HCLIB_HOME)/modules/openshmem/lib/libhclib_openshmem.a \
		   $(HCLIB_HOME)/modules/openshmem-am/lib/libhclib_openshmem-am.a \
		   -lhclib $(JSMN_HOME)/libjsmn.a
OSHCPP = oshc++ 

.PHONY: clean run build all default



build all default: list.cpp tsp.cpp tsp.h list.h master.cpp worker.cpp
	make clean;
	$(OSHCPP) $(CFLAGS) $^ ../support/rtc.o -o ./tsp_asyncshmem.out $(LDFLAGS);

run:
	srun -n 2 ./tsp*.out ../support/tspfile04.txt


clean:
	rm -rf ./tsp*.out
