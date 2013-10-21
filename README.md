Lmn-concurrent-hashmap
===========

C/C++ library of concurrent hashmaps with high performance for parallel model checking

## Requirements
    
    autoconf 2.69
    automake 1.11.3
    gcc 4.6.3
    g++ 4.6.3 

## How to build
    
    $ autoreconf -i
    $ ./configure
    $ make

## Contents

1. Fine-Grained Lock ChainHash
2. Cliff Click HashMap for Model Checking
3. Split Orderd Linked List for Model Checking

## How to use
     
     $ ./benchmark [-a algorithm_name] [-n number_of_thread] [-t time]

## Thanks for the URL
- http://www.stanford.edu/class/ee380/Abstracts/070221_LockFreeHash.pdf 
- https://code.google.com/p/nbds/
- http://www.cs.ucf.edu/~dcm/Teaching/COT4810-Spring2011/Literature/SplitOrderedLists.pdf
- https://github.com/kumpera/Lock-free-hastable
