# Cache Coherence

The program tries to simulate a symmentric multiprocessor system. To maintain coherence between the caches we employ one of the following three protocols, 

- MSI 
- MESI
- Dragon

The program simulate an SMP system, which consists on 'n' processors and each processors have the their own L1 cache.In this memory heirarchy, we have L1 cache which is private to a processor and a common memory pool. 

The base code was given. It simulates the workings of a L1 cache. The code was then modified according to the specs mentioned above. The program prints the metrics (like read misses, write misses, memory transactions, interventions, cache to cache transaction) that were calculated when the memory heirarchy was simulated. 

## Usage 

Run the make file on a linux machine using the make command. Execute the following command from the teriminal to run the program. 

                  ./smp_cache <cache_size>  <assoc> <block_size> <num_processors> <protocol> <trace_file>

Where: 
* `smp_cache`: Executable of the SMP simulator generated after making. 
* `cache_size`: Size of each cache in the system (all caches are of the same size)
* `assoc`: Associativity of each cache (all caches are of the same associativity)
* `block_size`:  Block size of each cache line (all caches are of the same block size) 
* `num_processors`: Number of processors in the system (represents how many caches should be instantiated) 
* `protocol`: Coherence protocol to be used (0: MSI, 1:MESI, 2:Dragon) 
* `trace_file`: The input file that has the multi threaded workload trace. 
