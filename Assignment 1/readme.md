# L2-L3 Cache Simulator

## Problem 
A program to simulate a two-level cache hierarchy which counts the number
of misses and hits when L1 cache miss trace is passed through them. Let
us call the two levels of the hierarchy L2 and L3. The L2 cache should be
512 KB in size, 8-way set associative, and have 64 bytes block size. The
L3 cache should be 2 MB in size, 16-way set associative, and have 64 bytes
block size. The aim is to report the number of L2 and L3 cache misses
for each of the six applications for the following three cases. i.e Inclusive,
Exclusive & Non-Inclusive Non-Exclusive (NINE).

## Execution

1. To run code individually:  Here we need to pass the name of trace file and number of trace  files as argument. Similarly we can simulate for all the applications. <br/> 
  g++ cache sim incl.cpp -o inclusive  <br/>
  ./inclusive <l1_misstrace_filename> <no_of_trace_files>  <br/>
  g++ cache sim exlcl.cpp -o exclusive  <br/>
  ./exclusive <l1_misstrace_filename> <no_of_trace_files>  <br/>
  g++ cache sim nine.cpp -o nine  <br/>
  ./nine <l1_misstrace_filename> <no_of_trace_files>  <br/>
  
2. To run entire code:  <br/>
  g++ driver.cpp -o driver  <br/>
  ./driver > report.txt  <br/>
 
Reads the traces, compiles and runs the cache simulator for this problem. L2, L3 cache hits and misses are printed in report.txt
