# Overview

This is a C library with unit testing and logging used to verify the code.

Module `hl_blocks`:
* Manages a block of memory consisting of N fixed sized blocks, typical flash memory on an embedded system.
* Give the module a function to write block N and to get the address of block N.
* Specify the size and number of blocks (e.g. 64 blocks of 512-bytes each)
* Call `hl_blocks_r_write` to write a messages on N bytes.
* The message is appended by writing into a single block of heap memory
* When the head block is full, call the write function then start again from the beginning of the heap block.
* Reading returns the oldest unread message.
* Reading from underlying memory or heap if that's all that remain.
* It will rotate to the first block when reached the end of the buffer.
* Write fails when the buffer is full of unread messages.
* `hl_blocks_r_sync()` can be called at any type to writes any remaining data from heap to the underlying memory. However it is not required except when the data is crytical and may not be lost on a sudden power cut. It is automatically called each time heap is full.
* `hl_blocks_r_close()` syncs and releases local memory used to manage the block.
* `hl_blocks_r_open()` scans the memory to resume when last synced and setup the local memory to manage the block.
* See `test_hl_qspi_mem.c` for examples.

# Unit Testing

Run all unit tests:
```
./bin/ctest.sh
```

Run all tests containing a particular string:
```
./bin/ctest <string>
```

The unit test is done by finding all tests then writing a main function to execute them.

All unit tests are written with the `TEST(<name>)` macro.
