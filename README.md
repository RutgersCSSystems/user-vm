Installation
---------------
Implementation of 2-level virtual memory manager with support for different page sizes and hashtable based TLB.
To test, we have created a simple Matrix multiplication example, which uses a_malloc (our custome page-granularity allocator) to allocate pages. 
Please see vm_documentation.pdf for code and other details.
We are in the process of cleaning up and making simple changes to support 4-level table, and possibly other fixes.

Authors: Manav Patel, Kirollos Basta, Sudarsun Kannan

```
sudo apt-get install gcc-multilib
make clean
make
cd benchmark
make
```

Running
```
./test
```
