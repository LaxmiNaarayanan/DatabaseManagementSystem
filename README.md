CONTRIBUTIONS
=============

Development by Laxmi Naarayanan -> HELPER FUNCTIONS, REPLACEMENT STRATEGY FUNCTIONS and BUFFER POOL FUNCTIONS

Development by Lakshey -> PAGE MANAGEMENT FUNCTIONS and STATISTICS FUNCTIONS

RUNNING THE SCRIPT
=========================

1) Go to Project root (Assignment2_BufferManager) using Terminal.

2) Type "make clean" to delete old compiled .o files.

3) Type "make" to compile all project files 

4) Type "make run_test1" to run "test_assign2_1.c" file.


SOLUTION DESCRIPTION
===========================

We have implemented Buffer Management with implementing replacement strategies along with ensuring proper memory management.

1) HELPER FUNCTIONS
====================
writeBlockToDisk(...)
--> writes a page in the buffer pool to the disk

setNewPageToPageFrame(...)
--> sets the new page into the page frame

2) PAGE REPLACEMENT ALGORITHM FUNCTIONS
=========================================

The page replacement strategy functions implement FIFO, LRU, LFU, CLOCK algorithms which are used while pinning a page. If the buffer pool is full and a new page has to be pinned, then a page should be replaced from the buffer pool. These page replacement strategies determine which page has to be replaced from the buffer pool.

FIFO(...)
--> First In First Out (FIFO) is similar to a queue implementation
--> The First page into the Buffer Pool is the First to be replaced

LFU(...)
--> Least Frequently Used (LFU) removes the page frame which is used the least number of times
--> The field refNum keeps a count of the page frames being accessed by the client.
--> Set the least frequently used pointer to the page frame having the lowest value of refNum.

LRU(...)
--> Least Recently Used (LRU) removes the page frame which is least recent used amongst other page frames in the buffer pool.
--> The field hitNum keeps a count of of the page frames being pinned by the client.
--> Replace the page frame which has the lowest value of hitNum.

CLOCK(...)
--> Replaces the last added page frame in the buffer pool using hitnum and clockPointer

3) BUFFER POOL FUNCTIONS
===========================

The buffer pool related functions are used to create a buffer pool for an existing page file on disk. The buffer pool is created in memory while the page file is present on disk. We make the use of Storage Manager (Assignment 1) to perform operations on page file on disk.

initBufferPool(...)
--> Creates a new buffer pool in memory.
--> The parameter numPages defines the size of the buffer
--> pageFileName stores the name of the page file whose pages are being cached in memory.
--> strategy represents the page replacement strategy
--> stratData is used to pass parameters if any to the page replacement strategy. 

shutdownBufferPool(...)
--> This function destroys the buffer pool.
--> It free up all resources/memory space being used by the Buffer Manager for the buffer pool.
--> Before destroying the buffer pool, we call forceFlushPool(...) which writes all the dirty pages (modified pages) to the disk.
--> If any page is being used by any client, then it throws RC_PINNED_PAGES_IN_BUFFER error.

forceFlushPool(...)
--> This function writes all the dirty pages (modified pages whose isDirtyPage = 1) to the disk.
--> It checks all the page frames in buffer pool and checks if it's isDirtyPage = 1 (which indicates that content of the page frame has been modified by some client) and fixCount = 0 (which indicates no user is using that page Frame) and if both conditions are satisfied then it writes the page frame to the page file on disk.


4) PAGE MANAGEMENT FUNCTIONS
==========================

The page management related functions are used to load pages from disk into the buffer pool (pin pages), remove a page frame from buffer pool (unpin page), mark page as dirty and force a page fram to be written to the disk.

pinPage(...)
--> This function pins the page number pageNum i.e, it reads the page from the page file present on disk and stores it in the buffer pool.
--> Before pinning a page, it checks if the buffer pool ha an empty space. If it has an empty space, then the page frame can be stored in the buffer pool else a page replacement strategy has to be used in order to replace a page in the buffer pool.
--> We have implemented FIFO, LRU, LFU and CLOCK page replacement strategies which are used while pinning a page.
--> The page replacement algorithms determine which page has to be replaced. That respective page is checked if it is dirty. In case it's dirtyBit = 1, then the contents of the page frame is written to the page file on disk and the new page is placed at that location where the old page was.

unpinPage(...)
--> This function unpins the specified page. The page to be unpinned is decided using page's pageNum.
--> After locating the page using a loop, it decrements the fixCount of that page by 1 which means that the client is no longer using this page.

makeDirty(...)
--> This function set's the dirtyBit of the specified page frame to 1.
--> It locates the page frame through pageNum by iteratively checking each page in the buffer pool and when the page id founf it set's dirtyBit = 1 for that page.

forcePage(....)
--> This page writes the content of the specified page frame to the page file present on disk.
--> It locates the specified page using pageNum by checking all the pages in the buffer loop using a loop construct.
--> When the page is found, it uses the Storage Manager functions to write the content of the page frame to the page file on disk. After writing, it sets dirtyBit = 0 for that page.


5) STATISTICS FUNCTIONS
===========================

The statistics related functions are used to gather some information about the buffer pool. So it provides various statistical information about the buffer pool.

getFrameContents(...)
--> This function returns an array of PageNumbers. The array size = buffer size (numPages).
--> We iterate over all the page frames in the buffer pool to get the pageNum value of the page frames present in the buffer pool.
--> The "n"th element is the page number of the page stored in the "n"th page frame.

getDirtyFlags(...)
--> This function returns an array of bools. The array size = buffer size (numPages).
--> We iterate over all the page frames in the buffer pool to get the dirtyBit value of the page frames present in the buffer pool.
--> The "n"th element is the TRUE if the page stored in the "n"th page frame is dirty.

getFixCounts(...) 
--> This function returns an array of ints. The array size = buffer size (numPages).
--> We iterate over all the page frames in the buffer pool to get the fixCount value of the page frames present in the buffer pool.
--> The "n"th element is the fixCount of the page stored in the "n"th page frame.

getNumReadIO(...)
--> This function returns the count of total number of IO reads performed by the buffer pool i.e. number of pages read from the disk.
--> We maintain this data using the rearIndex variable.

getNumWriteIO(...)
--> This function returns the count of total number of IO writes performed by the buffer pool i.e. number of pages written to the disk.
--> We maintain this data using the writeCount variable. We initialize writeCount to 0 when buffer pool is initialized and increment it whenever a page frame is written to disk.