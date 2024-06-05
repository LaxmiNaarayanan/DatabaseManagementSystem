#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

// Representation of a Page Frame in buffer pool (memory).
typedef struct Page
{
	SM_PageHandle data;
	PageNumber pageNum;
	bool isPageDirty;
	// number of clients accessing a given page at a given instance
	int clientCount;
	int hitNumber;
	int refNumber;
} PageFrame;

// bufferSize denotes the size of the buffer pool, denoting how much page frames can be stored in the buffer pool
int bufferSize = 0;

// "numPagesReadCount" basically stores the count of number of pages read from the disk, used to calculate current Index
int numPagesReadCount = 0;

// "totalDiskWriteCount" counts the number of I/O write to the disk i.e. number of pages writen to the disk
int totalDiskWriteCount = 0;

// "hit" is incremented whenever a page frame is added into the buffer pool, used in LRU strategy
int hit = 0;

// "clockPointer" tracks on to the last added page in the buffer pool.
int clockPointer = 0;

// ***** HELPER FUNCTIONS ***** //
#pragma region HELPER FUNCTIONS

// writeBlockToDisk method implemented
extern void writeBlockToDisk(BM_BufferPool *const bm, PageFrame *pageFrame, int pageFrameIndex)
{
	SM_FileHandle fh;
	openPageFile(bm->pageFile, &fh);
	// Writing pageFrame data to the page file on disk
	writeBlock(pageFrame[pageFrameIndex].pageNum, &fh, pageFrame[pageFrameIndex].data);

	// Increase the totalDiskWriteCount which records the number of writes done by the buffer manager.
	totalDiskWriteCount++;
}

// setNewPageToPageFrame method implemented
extern void setNewPageToPageFrame(PageFrame *pageFrame, PageFrame *page, int pageFrameIndex)
{
	// Setting page frame's content to new page's content
	pageFrame[pageFrameIndex].data = page->data;
	pageFrame[pageFrameIndex].pageNum = page->pageNum;
	pageFrame[pageFrameIndex].isPageDirty = page->isPageDirty;
	pageFrame[pageFrameIndex].clientCount = page->clientCount;
	pageFrame[pageFrameIndex].hitNumber = page->hitNumber;
}

// Unused Function
// creates an instance of a new page
extern PageFrame getNewPageInstance (BM_BufferPool *const bm, const PageNumber pageNum) {
	// Create a new page to store data read from the file.
	PageFrame *newPage = (PageFrame *)malloc(sizeof(PageFrame));

	// Reading page from disk and initializing page frame's content in the buffer pool
	SM_FileHandle fh;
	openPageFile(bm->pageFile, &fh);
	newPage->data = (SM_PageHandle)malloc(PAGE_SIZE);
	ensureCapacity(pageNum, &fh);
	readBlock(pageNum, &fh, newPage->data);
	newPage->pageNum = pageNum;
	newPage->isPageDirty = false;
	newPage->clientCount = 1;
	newPage->refNumber = 0;
	newPage->hitNumber = hit;
	return *newPage;
}

#pragma endregion

// ***** REPLACEMENT STRATEGY FUNCTIONS ***** //
#pragma region REPLACEMENT STRATEGY FUNCTIONS

// First In First Out Implementation
extern void FIFO(BM_BufferPool *const bm, PageFrame *page)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int iter = 0;
	int currentIndex = numPagesReadCount % bufferSize;

	// Interating through all the page frames in the buffer pool
	for (iter = 0; iter < bufferSize; iter++)
	{
		// Check if the current page frame is not being used by any client
		if (pageFrame[currentIndex].clientCount != 0)
		{
			currentIndex++;
			// For the last page in the buffer pool, Current Index equals bufferSize-1
			if (currentIndex % bufferSize == 0)
			{
				// Resetting index to 0, if last page has already been read.
				currentIndex = 0;
			}
			else
				currentIndex = currentIndex;
		}
		else
		{
			// If page in memory has been modified then write the page to the disk
			if (pageFrame[currentIndex].isPageDirty == true)
			{
				writeBlockToDisk(bm, pageFrame, currentIndex);
			}

			setNewPageToPageFrame(pageFrame, page, currentIndex);
			break;
		}
	}
}

// Implementing LRU (Least Recently Used) function
extern void LRU(BM_BufferPool *const bm, PageFrame *page)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	int iter = 0, leastHitIndex, leastHitNum;

	// set the least hit index to the first page frame
	leastHitIndex = 0;
	leastHitNum = pageFrame[0].hitNumber;

	// Finding the least recently used page frame by finding the page frame with minimum hitNumber
	for (iter = leastHitIndex + 1; iter < bufferSize; iter++)
	{
		if (pageFrame[iter].hitNumber < leastHitNum)
		{
			leastHitIndex = iter;
			leastHitNum = pageFrame[iter].hitNumber;
		}
	}

	// If page in memory has been modified then write the page to the disk
	if (pageFrame[leastHitIndex].isPageDirty == true)
	{
		writeBlockToDisk(bm, pageFrame, leastHitIndex);
	}

	// Setting page frame's content to new page's content
	setNewPageToPageFrame(pageFrame, page, leastHitIndex);
}

// Implementing CLOCK function
extern void CLOCK(BM_BufferPool *const bm, PageFrame *page)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	while (true)
	{
		// Resetting clock pointer
		if (clockPointer % bufferSize == 0)
			clockPointer = 0;

		// if hitnum is zero then set a new page into the page frame
		if (pageFrame[clockPointer].hitNumber == 0)
		{
			// If page in memory has been modified then write the page to the disk
			if (pageFrame[clockPointer].isPageDirty == true)
			{
				writeBlockToDisk(bm, pageFrame, clockPointer);
			}

			// Setting page frame's content to new page's content
			setNewPageToPageFrame(pageFrame, page, clockPointer);
			clockPointer++;
			break;
		}
		else
		{
			// setting hitNumber = 0 to terminate infinite looping
			pageFrame[clockPointer].hitNumber = 0;
			clockPointer++;
		}
	}
}

#pragma endregion

// ***** BUFFER POOL FUNCTIONS ***** //
#pragma region BUFFER POOL FUNCTIONS

/*
   This function creates and initializes the buffer pool
*/
extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
						 const int numberOfPages, ReplacementStrategy strategy,
						 void *stratData)
{
	// memory allocation for the pageFrame
	PageFrame *page = malloc(sizeof(PageFrame) * numberOfPages);

	bufferSize = numberOfPages;
	int iter = 0;

	// Intilalizing all pages in the buffer pool with default values.
	while (iter < bufferSize)
	{
		page[iter].data = NULL;
		page[iter].pageNum = -1;
		page[iter].isPageDirty = false;
		page[iter].clientCount = 0;
		page[iter].hitNumber = 0;
		page[iter].refNumber = 0;
		iter++;
	}

	bm->mgmtData = page;
	bm->pageFile = (char *)pageFileName;
	bm->numPages = numberOfPages;
	bm->strategy = strategy;

	totalDiskWriteCount = clockPointer = 0;
	return RC_OK;
}

// shutdownBufferPool implements closing of the buffer pool, i.e. removing all the pages from the memory and freeing up the unused memory space.
extern RC shutdownBufferPool(BM_BufferPool *const bm)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	// Write all dirty pages (modified pages) back to disk
	forceFlushPool(bm);

	int iter = 0;
	// check if any page in the buffer pool has an active user
	while (iter < bufferSize)
	{
		// The iterated page still has active users
		if (pageFrame[iter].clientCount != 0)
		{
			return RC_PINNED_PAGES_IN_BUFFER;
		}
		iter++;
	}

	// Releasing space occupied by the page Frame
	free(pageFrame);
	bm->mgmtData = NULL;
	return RC_OK;
}

// forceFlushPool function writes all the dirty pages back to the disk
extern RC forceFlushPool(BM_BufferPool *const bm)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int iter = 0;
	// Store all dirty pages (modified pages) in memory to page file on disk
	while (iter < bufferSize)
	{
		// Check if page in the buffer pool is currently not being used and also it is dirty
		if (pageFrame[iter].clientCount == 0 && pageFrame[iter].isPageDirty == true)
		{
			SM_FileHandle fh;
			openPageFile(bm->pageFile, &fh);
			// Writing pageFrame data to the page file on disk
			writeBlock(pageFrame[iter].pageNum, &fh, pageFrame[iter].data);
			// Mark the page not dirty.
			pageFrame[iter].isPageDirty = false;
			totalDiskWriteCount++;
		}
		iter++;
	}
	return RC_OK;
}

#pragma endregion

// ***** PAGE MANAGEMENT FUNCTIONS ***** //
#pragma region PAGE MANAGEMENT FUNCTIONS

// markDirty function marks modified page as dirty
extern RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int iter = 0;
	// Iterating through all the pages in the buffer pool
	while (iter < bufferSize)
	{
		// If the current page is the page to be marked dirty
		if (pageFrame[iter].pageNum == page->pageNum)
		{
			pageFrame[iter].isPageDirty = true;
			return RC_OK;
		}
		iter++;
	}
	// control reaches here only when the page is not found in the pageFrame
	return RC_ERROR;
}

// unpinPage function removes a page from the memory
extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int iter = 0;
	// Iterating through all the pages in the buffer pool
	while (iter < bufferSize)
	{
		if (pageFrame[iter].pageNum == page->pageNum)
		{
			// removes one client from the client count
			pageFrame[iter].clientCount--;
			break;
		}
		iter++;
	}
	return RC_OK;
}

// This function writes the contents of the modified pages back to the page file on disk
extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int iter = 0;
	// Iterating through all the pages in the buffer pool
	while (iter < bufferSize)
	{
		if (pageFrame[iter].pageNum == page->pageNum)
		{
			writeBlockToDisk(bm, pageFrame, iter);
			// Mark page as undirty because the modified page has been written to disk
			pageFrame[iter].isPageDirty = false;
		}
		iter++;
	}
	return RC_OK;
}

// pinPage function pins a page pageNum into the buffer pool
extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
				  const PageNumber pageNum)
{
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
	PageFrame *newPage = (PageFrame *)malloc(sizeof(PageFrame));
	const int firstPagePOS = 0;
	// Checking if buffer pool is empty and if its the first page to be pinned
	if (pageFrame[firstPagePOS].pageNum == -1)
	{
		numPagesReadCount = hit = 0;
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
		pageFrame[firstPagePOS].data = (SM_PageHandle)malloc(PAGE_SIZE);
		ensureCapacity(pageNum, &fh);
		readBlock(pageNum, &fh, pageFrame[firstPagePOS].data);
		pageFrame[firstPagePOS].pageNum = pageNum;
		pageFrame[firstPagePOS].clientCount++;
		pageFrame[firstPagePOS].hitNumber = hit;
		pageFrame[firstPagePOS].refNumber = 0;
		page->pageNum = pageNum;
		page->data = pageFrame[0].data;

		return RC_OK;
	}

	// Initially set the buffer pool as Full.
	bool isBufferPoolFull = true;

	int iter = 0;
	// If there is any empty frame in the pool or if the expected pageNum is available then set the buffer pool as not Full.
	while (iter < bufferSize)
	{
		// Ckeck if the page is empty
		if (pageFrame[iter].pageNum == -1) {
			SM_FileHandle fh;
			openPageFile(bm->pageFile, &fh);
			pageFrame[iter].data = (SM_PageHandle)malloc(PAGE_SIZE);
			readBlock(pageNum, &fh, pageFrame[iter].data);
			pageFrame[iter].pageNum = pageNum;
			pageFrame[iter].clientCount = 1;
			pageFrame[iter].refNumber = 0;
			numPagesReadCount++;
			hit++;

			if (bm->strategy == RS_LRU)
				pageFrame[iter].hitNumber = hit;
			else if (bm->strategy == RS_CLOCK)
				pageFrame[iter].hitNumber = 1;

			page->pageNum = pageNum;
			page->data = pageFrame[iter].data;

			isBufferPoolFull = false;
			break;
		}

		// Control reaches here only if the page frame is not empty
		// Now check if the page is in memory
		if (pageFrame[iter].pageNum == pageNum)
		{
			isBufferPoolFull = false;
			hit++;
			// One more client is accessing this page
			pageFrame[iter].clientCount++;

			if (bm->strategy == RS_LRU)
				pageFrame[iter].hitNumber = hit;
			else if (bm->strategy == RS_CLOCK)
				pageFrame[iter].hitNumber = 1;

			page->pageNum = pageNum;
			page->data = pageFrame[iter].data;

			clockPointer++;
			break;
		}
		iter++;
	}

	// Post iterating through the entire buffer pool, If isBufferPoolFull = true, then it means that the buffer is full and we must replace an existing page using page replacement strategy
	if (isBufferPoolFull == true)
	{
		// Create a new page to store data read from the file.
		PageFrame *newPage = (PageFrame *)malloc(sizeof(PageFrame));

		// Reading page from disk and initializing page frame's content in the buffer pool
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
		newPage->data = (SM_PageHandle)malloc(PAGE_SIZE);
		readBlock(pageNum, &fh, newPage->data);
		newPage->pageNum = pageNum;
		newPage->isPageDirty = false;
		newPage->clientCount = 1;
		newPage->refNumber = 0;
		numPagesReadCount++;
		hit++;

		if (bm->strategy == RS_LRU)
			newPage->hitNumber = hit;
		else if (bm->strategy == RS_CLOCK)
			newPage->hitNumber = 1;

		page->pageNum = pageNum;
		page->data = newPage->data;

		// Page Replacement Strategy Execution
		ReplacementStrategy strategy = bm->strategy;

		if(strategy == RS_FIFO)
			FIFO(bm, newPage);
		else if(strategy == RS_LRU)
			LRU(bm, newPage);
		else if(strategy == RS_CLOCK)
			CLOCK(bm, newPage);
		else if(strategy == RS_LFU)
			printf("\n LFU algorithm not implemented");
		else
			printf("\n Strategy not detected or Strategy not implemented\n");
	}
	return RC_OK;
}

#pragma endregion

// ***** STATISTICS FUNCTIONS ***** //
#pragma region STATISTICS FUNCTIONS

// getFrameContents function returns an array of page numbers.
extern PageNumber *getFrameContents(BM_BufferPool *const bm)
{
	PageNumber *frameContents = malloc(sizeof(PageNumber) * bufferSize);
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int iter = 0;
	// setting frameContents value for each page frame
	while (iter < bufferSize)
	{
		if (pageFrame[iter].pageNum != -1) {
			frameContents[iter] = pageFrame[iter].pageNum;
		}
		else {
			frameContents[iter] = NO_PAGE;
		}
		iter++;
	}
	return frameContents;
}

// getDirtyFlags function returns an array of isPageDirty falg for each page.
extern bool *getDirtyFlags(BM_BufferPool *const bm)
{
	bool *isPageDirtyFlags = malloc(sizeof(bool) * bufferSize);
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int iter;
	// setting isPageDirty flag for each page
	for (iter = 0; iter < bufferSize; iter++)
	{
		isPageDirtyFlags[iter] = false;
		if (pageFrame[iter].isPageDirty == true)
		{
			isPageDirtyFlags[iter] = true;
		}
	}
	return isPageDirtyFlags;
}

// getFixCounts function returns an array of the fix counts for each page frame.
extern int *getFixCounts(BM_BufferPool *const bm)
{
	int *fixCounts = malloc(sizeof(int) * bufferSize);
	PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

	int iter = 0;
	while (iter < bufferSize)
	{
		fixCounts[iter] = 0;
		if (pageFrame[iter].clientCount != -1)
		{
			fixCounts[iter] = pageFrame[iter].clientCount;
		}
		iter++;
	}
	return fixCounts;
}

// getNumReadIO function returns the number of pages that have been read from disk since a buffer pool has been initialized.
extern int getNumReadIO(BM_BufferPool *const bm)
{
	// numPagesReadCount starts with 0.
	return (numPagesReadCount + 1);
}

// getNumWriteIO function returns the number of pages written to the page file since the buffer pool has been initialized.
extern int getNumWriteIO(BM_BufferPool *const bm)
{
	return totalDiskWriteCount;
}

#pragma endregion