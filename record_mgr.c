#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

const int NUMBER_OF_PAGES_IN_BUFFER_POOL = 100;
const ReplacementStrategy DEFAULT_REPLACEMENT_STRATEGY = RS_LRU;
const int ATTRIBUTE_SIZE = 20; // Size of the name of the attribute
const int FIRST_PAGE_NUMBER = 0;
const int FIRSTPAGE_POS = 1;
const int FIRSTSLOT_POS = 0;

RecordManager *recordManager;
BM_BufferPool *bufferPool;
BM_PageHandle *pageHandle;

#pragma region HELPER FUNCTIONS

// This Function Increments the PageHandle Pointer with respect to the given offset
int *incrementPointer(char *pointer, int offset)
{
	// incrementing the pointer to the next position
	pointer = pointer + offset;
	return pointer;
}

// This function returns the index of a free slot
int getFreeSlotIndex(char *data, int recordSize)
{
	int index = 0;
	int offset;
	int totalSlots = PAGE_SIZE / recordSize;

	while (index < totalSlots)
	{
		offset = index * recordSize;

		// if slot has data, then # will be present AND if the slot is empty,then # will not be present.
		if (data[offset] != '#')
			return index;

		index++;
	}
	// if all the slots are processed and no free slot is available, code reaches here.
	return -1;
}

#pragma endregion

#pragma region TABLE AND RECORD MANAGER FUNCTIONS
// ******** TABLE AND RECORD MANAGER FUNCTIONS ******** //

// This function initializes the Record Manager
extern RC initRecordManager(void *mgmtData)
{
	// Initiliazing Storage Manager
	initStorageManager();
	printf(" Record Manager Initialized ");
	return RC_OK;
}

// This function shuts down the Record Manager
extern RC shutdownRecordManager()
{
	// Record Manager dereferenced
	recordManager = NULL;
	free(recordManager);
	printf(" Shutting down Record Manager ");
	return RC_OK;
}

// This function creates a Table
extern RC createTable(char *name, Schema *schema)
{
	int result;
	char data[PAGE_SIZE];
	char *pageHandle = data;

	int const NUMBER_OF_TUPLES = 0;
	SM_FileHandle fileHandle;

	// Allocating memory space to the record manager custom data structure
	recordManager = (RecordManager *)malloc(sizeof(RecordManager));
	bufferPool = &recordManager->bufferPool;

	// Initalizing the Buffer Pool using the default page replacement strategy
	initBufferPool(bufferPool, name, NUMBER_OF_PAGES_IN_BUFFER_POOL, DEFAULT_REPLACEMENT_STRATEGY, NULL);

	int tableCreationAttributes[] = {NUMBER_OF_TUPLES, FIRSTPAGE_POS, schema->numAttr, schema->keySize};
	int tableCreationAttributesSize = sizeof(tableCreationAttributes) / sizeof(tableCreationAttributes[0]);
	int iter = 0;
	while (iter < tableCreationAttributesSize)
	{
		*(int *)pageHandle = tableCreationAttributes[iter];
		pageHandle = incrementPointer(pageHandle, sizeof(int));
		iter++;
	}
	iter = 0;
	while (iter < schema->numAttr)
	{
		// Set Attribute Names
		strncpy(pageHandle, schema->attrNames[iter], ATTRIBUTE_SIZE);
		pageHandle = incrementPointer(pageHandle, ATTRIBUTE_SIZE);

		// Set datatype of the attribute
		*(int *)pageHandle = (int)schema->dataTypes[iter];
		pageHandle = incrementPointer(pageHandle, sizeof(int));

		// Set datatype length for the attribute
		*(int *)pageHandle = (int)schema->typeLength[iter];
		pageHandle = incrementPointer(pageHandle, sizeof(int));

		iter++;
	}

	// Exceptions Handling done
	bool isExceptionPresent = true;

	// Check page file creation with page name as table name
	if ((result = createPageFile(name)) != RC_OK)
		;

	// Check the success of opening the newly created page
	else if ((result = openPageFile(name, &fileHandle)) != RC_OK)
		;

	// Check the success of writing the schema to first location of the page file
	else if ((result = writeBlock(FIRST_PAGE_NUMBER, &fileHandle, data)) != RC_OK)
		;

	// Check the success of closing the file after writing
	else if ((result = closePageFile(&fileHandle)) != RC_OK)
		;

	else
		isExceptionPresent = false;

	// return the exception if there exists one
	if (isExceptionPresent == true)
	{
		return result;
	}

	return RC_OK;
}

// This function opens the table
extern RC openTable(RM_TableData *rel, char *name)
{
	int numberOfAttributes;

	bufferPool = &recordManager->bufferPool;
	pageHandle = &recordManager->pageHandle;

	// Pinning a page in Buffer Pool
	pinPage(bufferPool, pageHandle, FIRST_PAGE_NUMBER);

	// Setting the initial pointer (0th location) if the record manager's page data
	pageHandle = (char *)recordManager->pageHandle.data;

	// Getting the number of records count from the page file
	recordManager->totalRecordsInTable = *(int *)pageHandle;
	pageHandle = incrementPointer(pageHandle, sizeof(int));

	// get the first free page from the page file
	recordManager->firstFreePage.page = 1;
	recordManager->firstFreePage.slot = 0;

	pageHandle = incrementPointer(pageHandle, sizeof(int));

	// Getting the number of attributes from the page file
	numberOfAttributes = *(int *)pageHandle;
	pageHandle = incrementPointer(pageHandle, sizeof(int));

	Schema *schema;

	// memory space allocation to 'schema'
	schema = (Schema *)malloc(sizeof(Schema));

	// memory allocation for schema parameters
	schema->numAttr = numberOfAttributes;
	schema->attrNames = (char **)malloc(sizeof(char *) * numberOfAttributes);
	schema->dataTypes = (DataType *)malloc(sizeof(DataType) * numberOfAttributes);
	schema->typeLength = (int *)malloc(sizeof(int) * numberOfAttributes);

	int iter = 0;
	// Allocate memory space for storing attribute name for each attribute
	while (iter < numberOfAttributes)
	{
		schema->attrNames[iter] = (char *)malloc(ATTRIBUTE_SIZE);
		iter++;
	}

	iter = 0;
	while (iter < schema->numAttr)
	{
		// Setting attribute name
		strncpy(schema->attrNames[iter], pageHandle, ATTRIBUTE_SIZE);
		pageHandle = incrementPointer(pageHandle, ATTRIBUTE_SIZE);

		// Setting data type of attribute
		schema->dataTypes[iter] = *(int *)pageHandle;
		pageHandle = incrementPointer(pageHandle, sizeof(int));

		// Setting length of datatype (length of STRING) of the attribute
		schema->typeLength[iter] = *(int *)pageHandle;
		pageHandle = incrementPointer(pageHandle, sizeof(int));
		iter++;
	}

	// Setting the record manager meta
	rel->mgmtData = recordManager;

	// Setting table name
	rel->name = name;

	// Setting newly created schema to the table's schema
	rel->schema = schema;

	// Unpinning the page from Buffer Pool
	unpinPage(bufferPool, pageHandle);

	// Writing back the page to disk
	forcePage(bufferPool, pageHandle);

	return RC_OK;
}

// This function closes the table referenced by "rel"
extern RC closeTable(RM_TableData *rel)
{
	RecordManager *recordManager = rel->mgmtData;
	bufferPool = &recordManager->bufferPool;
	shutdownBufferPool(bufferPool);
	return RC_OK;
}

// This function deletes the table
extern RC deleteTable(char *name)
{
	destroyPageFile(name);
	return RC_OK;
}

// This function returns the number of records in the table
extern int getNumTuples(RM_TableData *rel)
{
	RecordManager *recordManager = rel->mgmtData;
	return recordManager->totalRecordsInTable;
}

#pragma endregion

#pragma region RECORD FUNCTIONS
// ******** RECORD FUNCTIONS ******** //

// This function inserts a new record into the table
extern RC insertRecord(RM_TableData *rel, Record *record)
{
	bufferPool = &recordManager->bufferPool;
	pageHandle = &recordManager->pageHandle;
	recordManager = rel->mgmtData;
	int recordSize = getRecordSize(rel->schema);

	// Setting the Record ID for this record
	RID *recordID = &record->id;

	char *data, *recordPointer;

	recordID->slot = -1;
	bool isSearchFirstTime = true;

	// We iterate through this loop untill we find a valid slot
	while (recordID->slot == -1)
	{
		if (isSearchFirstTime)
		{
			recordID->page = recordManager->firstFreePage.page;
			isSearchFirstTime = false;
		}
		else
		{
			// If the pinned page doesn't have a free slot then unpin that page
			unpinPage(bufferPool, pageHandle);

			// Incrementing page
			recordID->page++;
		}

		// Bring the new page into the Buffer Pool
		pinPage(bufferPool, pageHandle, recordID->page);

		// Setting the data to initial position of record's data
		data = recordManager->pageHandle.data;

		// Search again for a free slot
		recordID->slot = getFreeSlotIndex(data, recordSize);
	}

	recordPointer = data;

	// Mark page dirty to notify that this page was modified
	if (markDirty(bufferPool, pageHandle))
	{
		RC_message = "Page Mark Dirty Failed";
		return RC_MARK_DIRTY_FAILED;
	}

	int recordOffset = recordID->slot * recordSize;
	recordPointer = incrementPointer(recordPointer, recordOffset);

	// Character '#' is appended to indicate that this is a new record
	*recordPointer = '#';

	recordPointer++;
	// Copy the record's data to the memory location pointed by recordPointer
	memcpy(recordPointer, record->data + 1, recordSize - 1);

	// Unpinning a page from the Buffer Pool
	if (unpinPage(bufferPool, pageHandle))
	{
		RC_message = "Unpin Page failed Failed";
		return RC_UNPIN_PAGE_FAILED;
	}

	// Incrementing count of tuples
	recordManager->totalRecordsInTable++;

	// Pinback the page
	pinPage(bufferPool, pageHandle, 0);

	return RC_OK;
}

// This function deletes a record having Record ID "id" in the table referenced by "rel"
extern RC deleteRecord(RM_TableData *rel, RID id)
{
	bufferPool = &recordManager->bufferPool;
	pageHandle = &recordManager->pageHandle;
	// Retrieving our meta data stored in the table
	RecordManager *recordManager = rel->mgmtData;

	// Pinning the page which has the record which we want to update
	pinPage(bufferPool, pageHandle, id.page);

	// Update free page because this page
	recordManager->firstFreePage.page = id.page;

	char *pageData = recordManager->pageHandle.data;

	int recordSize = getRecordSize(rel->schema);
	int recordOffset = id.slot * recordSize;
	pageData = incrementPointer(pageData, recordOffset);

	// Mark page dirty to notify that this page was modified
	if (markDirty(bufferPool, pageHandle))
	{
		RC_message = "Page Mark Dirty Failed";
		return RC_MARK_DIRTY_FAILED;
	}

	// Unpin the page post the retrieving the record
	if (unpinPage(bufferPool, pageHandle))
	{
		RC_message = "Unpin Page failed Failed";
		return RC_UNPIN_PAGE_FAILED;
	}

	return RC_OK;
}

// This function updates a record referenced by "record" in the table referenced by "rel"
extern RC updateRecord(RM_TableData *rel, Record *record)
{
	bufferPool = &recordManager->bufferPool;
	pageHandle = &recordManager->pageHandle;

	recordManager = rel->mgmtData;

	RID id = record->id;

	// Pinning the page which has the record which we want to update
	pinPage(bufferPool, pageHandle, id.page);

	char *pageData = recordManager->pageHandle.data;

	int recordSize = getRecordSize(rel->schema);
	int recordOffset = id.slot * recordSize;
	pageData = incrementPointer(pageData, recordOffset);
	pageData++;

	// Copy the new record pageData to the exisitng record
	memcpy(pageData, record->data + 1, recordSize);

	// Mark page dirty to notify that this page was modified
	if (markDirty(bufferPool, pageHandle))
	{
		RC_message = "Page Mark Dirty Failed";
		return RC_MARK_DIRTY_FAILED;
	}

	// Unpin the page post the retrieving the record
	if (unpinPage(bufferPool, pageHandle))
	{
		RC_message = "Unpin Page failed Failed";
		return RC_UNPIN_PAGE_FAILED;
	}
	return RC_OK;
}

// This function retrieves a record from the table
extern RC getRecord(RM_TableData *rel, RID id, Record *record)
{
	bufferPool = &recordManager->bufferPool;
	pageHandle = &recordManager->pageHandle;
	recordManager = rel->mgmtData;
	record->id = id;
	int pageNumber = id.page;

	// Pinning the page which has the desired record
	if (pinPage(bufferPool, pageHandle, pageNumber) != RC_OK)
	{
		RC_message = "Pin page has failed: ";
		return RC_PIN_PAGE_FAILED;
	}

	// Getting the size of the record
	int recordSize = getRecordSize(rel->schema);
	int recordOffset = id.slot * recordSize;
	char *dataPointer = recordManager->pageHandle.data;
	dataPointer = incrementPointer(dataPointer, recordOffset);

	char *data = record->data;
	data++;
	memcpy(data, dataPointer + 1, recordSize);

	// Unpin the retrieved record
	if (unpinPage(bufferPool, pageHandle) != RC_OK)
	{
		RC_message = "Unpin Page has failed";
		return RC_UNPIN_PAGE_FAILED;
	}

	return RC_OK;
}

#pragma endregion

#pragma region SCAN FUNCTIONS
// ******** SCAN FUNCTIONS ******** //

// startScan function scans all the records which satisfies the condition
extern RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
	// Checking if there is no scan condition
	if (cond == NULL)
	{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}

	openTable(rel, "ScanTable");

	RecordManager *scanManager, *tableManager;
	tableManager = rel->mgmtData;
	tableManager->totalRecordsInTable = ATTRIBUTE_SIZE;

	// Memory is allotted to the scanManager.
	scanManager = (RecordManager *)malloc(sizeof(RecordManager));

	// Changing the meta data of the scan to our meta data
	scan->mgmtData = scanManager;

	RID scanManagerRecordID = scanManager->recordID;
	// start scanning from First Page
	scanManagerRecordID.page = FIRSTPAGE_POS;
	scanManagerRecordID.slot = FIRSTSLOT_POS;

	// scanCount set to 0 as no Record has been scanned yet.
	scanManager->scanCount = 0;

	// setting scan condition
	scanManager->condition = cond;

	// setting the table that needs to be scanned
	scan->rel = rel;

	return RC_OK;
}

// The record satisfying the criterion is stored in the place indicated by the'record' variable after this function reads each item in the table.
extern RC next(RM_ScanHandle *scan, Record *record)
{
	RecordManager *scanManager = scan->mgmtData;
	RecordManager *scanTableManager = scan->rel->mgmtData;
	Schema *schema = scan->rel->schema;

	// check if there is no scan condition
	if (scanManager->condition == NULL)
	{
		return RC_SCAN_CONDITION_NOT_FOUND;
	}

	Value *result = (Value *)malloc(sizeof(Value));

	char *data;

	// Getting the schema's record size
	int sizeOfRecord = getRecordSize(schema);

	//  Total number of slots to be calculated
	int totalSlotsCount = PAGE_SIZE / sizeOfRecord;

	// Scan count retrieval
	int scanCount = scanManager->scanCount;

	//  tuples count of the table to be retrieved
	int totalRecordsInTable = scanTableManager->totalRecordsInTable;

	// determining if tuples are present in the table.If there are no tuples in the tables, then give the appropriate message code.
	if (totalRecordsInTable == 0)
		return RC_RM_NO_MORE_TUPLES;

	RID scanManagerRecordID = scanManager->recordID;

	// iterating through the records
	while (scanCount <= totalRecordsInTable)
	{
		if (scanCount > 0)
		{
			scanManager->recordID.slot++;

			// If all the slots have been scanned execute this block
			if (scanManager->recordID.slot >= totalSlotsCount)
			{
				scanManager->recordID.slot = FIRSTSLOT_POS;
				scanManager->recordID.page++;
			}
		}
		else
		{
			// If no record has been scanned before, control comes here
			scanManager->recordID.page = FIRSTPAGE_POS;
			scanManager->recordID.slot = FIRSTSLOT_POS;
		}

		// Pinning the page i.e. putting the page in buffer pool
		pinPage(&scanTableManager->bufferPool, &scanManager->pageHandle, scanManager->recordID.page);

		data = scanManager->pageHandle.data;

		int recordOffset = scanManager->recordID.slot * sizeOfRecord;
		data = incrementPointer(data, recordOffset);

		// Setting record Id from Manager
		record->id.page = scanManager->recordID.page;
		record->id.slot = scanManager->recordID.slot;

		char *dataPointer = record->data;
		dataPointer++;

		memcpy(dataPointer, data + 1, sizeOfRecord);

		// Increment scan count by 1
		scanManager->scanCount++;
		scanCount++;

		// Test if the record satisfies the given condition
		evalExpr(record, schema, scanManager->condition, &result);

		// check if result of the scan record has satisfied the condition
		if (result->v.boolV == TRUE)
		{
			// Unpin the page from the buffer pool.
			unpinPage(&scanTableManager->bufferPool, &scanManager->pageHandle);
			// Return Seccess Status
			return RC_OK;
		}
	}

	// Unpin the page from the buffer pool.
	unpinPage(&scanTableManager->bufferPool, &scanManager->pageHandle);

	// Reset the Scan Manager's values
	scanManager->recordID.page = FIRSTPAGE_POS;
	scanManager->recordID.slot = FIRSTSLOT_POS;
	scanManager->scanCount = 0;

	// None of the tuples satisfied the condition
	return RC_RM_NO_MORE_TUPLES;
}

// This function closes the scan operation.
extern RC closeScan(RM_ScanHandle *scan)
{
	RecordManager *scanManager = scan->mgmtData;
	RecordManager *recordManager = scan->rel->mgmtData;
	RID scanManagerRecordID = scanManager->recordID;

	// Check if scan was incomplete
	if (scanManager->scanCount > 0)
	{
		// Unpinning the page from the buffer pool.
		unpinPage(&recordManager->bufferPool, &scanManager->pageHandle);

		// Reset the Scan Manager's values
		scanManagerRecordID.page = FIRSTPAGE_POS;
		scanManagerRecordID.slot = FIRSTSLOT_POS;
		scanManager->scanCount = 0;
	}

	// De-allocate all the memory space
	scanManager = NULL;
	free(scanManager);

	return RC_OK;
}

#pragma endregion

#pragma region SCHEMA FUNCTIONS
// ******** SCHEMA FUNCTIONS ******** //

// The "schema" function's return value is the schema's record size.
extern int getRecordSize(Schema *schema)
{
	int recordSize = 0, sizeOfAttribute = 0;
	int iter = 0;

	// iterating over each attribute in the schema one by one
	while (iter < schema->numAttr)
	{
		if (schema->dataTypes[iter] == DT_STRING)
		{
			sizeOfAttribute = schema->typeLength[iter];
		}
		else if (schema->dataTypes[iter] == DT_INT)
		{
			sizeOfAttribute = sizeof(int);
		}
		else if (schema->dataTypes[iter] == DT_FLOAT)
		{
			sizeOfAttribute = sizeof(float);
		}
		else if (schema->dataTypes[iter] == DT_BOOL)
		{
			sizeOfAttribute = sizeof(bool);
		}
		recordSize = recordSize + sizeOfAttribute;
		iter += 1;
	}

	return ++recordSize;
}

// A new schema is created via this function.
extern Schema *createSchema(int numberofAttribute, char **attributeNames, DataType *dataTypes, int *typeLength, int sizeOfKey, int *keys)
{
	// Allocate memory space to schema
	Schema *schema = (Schema *)malloc(sizeof(Schema));
	// Set the New Schema's Attribute Count to be .
	schema->numAttr = numberofAttribute;
	// Set  the new schema's Attribute Names
	schema->attrNames = attributeNames;
	// Set the new schema's  data types of attributes
	schema->dataTypes = dataTypes;
	// Set the new schema's Type Length of the Attributes i.e. STRING size
	schema->typeLength = typeLength;
	// Set the new schema's key size
	schema->keySize = sizeOfKey;
	// Set the new schema's Key Attributes
	schema->keyAttrs = keys;

	return schema;
}

// A schema is deleted from memory using this function, which also frees up all the memory that was allotted to it.
extern RC freeSchema(Schema *schema)
{
	// removing "schema" from the memory space it occupies
	free(schema);
	return RC_OK;
}

#pragma endregion

#pragma region DEALING WITH RECORDS AND ATTRIBUTE VALUES
// ******** DEALING WITH RECORDS AND ATTRIBUTE VALUES ******** //

// The "schema" referenced function adds a new record to the schema.
extern RC createRecord(Record **record, Schema *schema)
{
	// Set aside some memory for the new record.
	Record *newRecord = (Record *)malloc(sizeof(Record));

	// Get the record size.
	int sizeOfRecord = getRecordSize(schema);

	// Set aside some memory for the information from the new record.
	newRecord->data = (char *)malloc(sizeOfRecord);

	newRecord->id.page = newRecord->id.slot = -1;

	// preserving in memory the information for the record's beginning point
	char *dataPointer = newRecord->data;

	// denoting the record is empty
	*dataPointer = '-';
	++dataPointer;

	*dataPointer = '\0';

	*record = newRecord;

	return RC_OK;
}

// This function sets the attributtes offset to the record
RC attrOffset(Schema *schema, int attributeNumber, int *result)
{
	int iter = 0;
	int sizeOfAttribute = 0;
	*result = 1;

	// Unused
	int attributeList[3] = {DT_INT, DT_FLOAT, DT_BOOL};
	char datatypeList[3][10] = {"int", "float", "bool"};

	// Iterating through all the attributes in the schema
	while (iter < attributeNumber)
	{
		// calculating the size of Attribute
		if (schema->dataTypes[iter] == DT_STRING)
		{
			sizeOfAttribute = schema->typeLength[iter];
		}
		else if (schema->dataTypes[iter] == DT_INT)
		{
			sizeOfAttribute = sizeof(int);
		}
		else if (schema->dataTypes[iter] == DT_FLOAT)
		{
			sizeOfAttribute = sizeof(float);
		}
		else if (schema->dataTypes[iter] == DT_BOOL)
		{
			sizeOfAttribute = sizeof(bool);
		}
		*result = *result + sizeOfAttribute;
		iter++;
	}
	return RC_OK;
}

// This function discards the record from the memory.
extern RC freeRecord(Record *record)
{
	// releasing the memory space that was allotted for records and de-allocating it
	free(record);
	return RC_OK;
}

// With the help of this function, you can retrieve an attribute from a record in the specified schema.
extern RC getAttr(Record *record, Schema *schema, int attributeNumber, Value **value)
{
	int offset = 0;
	int attributeSize = 0;

	// Obtaining the attribute offset value based on the attribute number
	attrOffset(schema, attributeNumber, &offset);

	// Allocating memory for the attributes
	Value *attribute = (Value *)malloc(sizeof(Value));

	// Reference beginning address of record's data
	char *dataPointer = record->data;
	dataPointer = incrementPointer(dataPointer, offset);

	if (attributeNumber == 1)
	{
		schema->dataTypes[attributeNumber] = 1;
	}
	else
	{
		schema->dataTypes[attributeNumber] = schema->dataTypes[attributeNumber];
	}

	if (schema->dataTypes[attributeNumber] == DT_STRING)
	{
		int length = schema->typeLength[attributeNumber];
		// Allocate space for string attribute of size 'length'
		attribute->v.stringV = (char *)malloc(length + 1);

		// Copying string to position which is pointed by dataPointer
		strncpy(attribute->v.stringV, dataPointer, length);
		// Appending '\0' denotes end of string
		attribute->v.stringV[length] = '\0';
		attribute->dt = DT_STRING;
	}

	else if (schema->dataTypes[attributeNumber] == DT_INT)
	{
		int value = 0;
		attributeSize = sizeof(int);
		memcpy(&value, dataPointer, attributeSize);
		attribute->v.intV = value;
		attribute->dt = DT_INT;
	}
	else if (schema->dataTypes[attributeNumber] == DT_FLOAT)
	{
		float value;
		attributeSize = sizeof(float);
		memcpy(&value, dataPointer, attributeSize);
		attribute->v.floatV = value;
		attribute->dt = DT_FLOAT;
	}
	else if (schema->dataTypes[attributeNumber] == DT_BOOL)
	{
		bool value;
		attributeSize = sizeof(bool);
		memcpy(&value, dataPointer, attributeSize);
		attribute->v.boolV = value;
		attribute->dt = DT_BOOL;
	}
	else
	{
		printf("Serializer not defined for the given datatype. \n");
	}

	*value = attribute;
	return RC_OK;
}

// This function modifies the record's attribute value according to the specified schema
extern RC setAttr(Record *record, Schema *schema, int attributeNumber, Value *value)
{
	int offset = 0;

	// Obtaining the attribute offset value based on the attribute number
	attrOffset(schema, attributeNumber, &offset);

	// Reference beginning address of record's data
	char *dataPointer = record->data;
	dataPointer = incrementPointer(dataPointer, offset);

	if (schema->dataTypes[attributeNumber] == DT_STRING)
	{
		// Setting the value of a STRING-type attribute
		// obtaining the string's length as specified when the schema was created
		int length = schema->typeLength[attributeNumber];

		// copying the value of the attribute to the record Pointer
		strncpy(dataPointer, value->v.stringV, length);
		int offset = schema->typeLength[attributeNumber];
		dataPointer = incrementPointer(dataPointer, offset);
	}

	else if (schema->dataTypes[attributeNumber] == DT_INT)
	{
		// Setting the value of an INTEGER type attribute
		*(int *)dataPointer = value->v.intV;
		dataPointer = incrementPointer(dataPointer, sizeof(int));
	}

	else if (schema->dataTypes[attributeNumber] == DT_FLOAT)
	{
		// Setting the value of FLOAT type attribute
		*(float *)dataPointer = value->v.floatV;
		dataPointer = incrementPointer(dataPointer, sizeof(float));
	}

	else if (schema->dataTypes[attributeNumber] == DT_BOOL)
	{
		// Setting the value of a BOOL attribute
		*(bool *)dataPointer = value->v.boolV;
		dataPointer = incrementPointer(dataPointer, sizeof(bool));
	}

	else
	{
		printf("provided datatype does not have a Serializer \n");
	}
	return RC_OK;
}

#pragma endregion