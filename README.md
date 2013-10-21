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

## The MIT License

Copyright (c) 2013 Taketo Yoshida

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
