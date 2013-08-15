Lmn-concurrent-hashmap
===========

C library of concurrent hashmaps with high performance for parallel model checking

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

1. Lock Based ChainHash (Close Addressing)
2. Lock Free ChainHash (Close Addressing)
3. Lock Based ClosedHash (Open Addressing)
4. Lock Free ClosedHash (Open Addressing)

## How to use
     
     $ ./benchmark [-a algorithm_name] [-c insert_count] [-n number_of_thread] 