CONTRIBUTION:
=======================================
Development by Laxmi Naarayanan - "HELPER FUNCTIONS, TABLE AND RECORD MANAGER FUNCTIONS, RECORD FUNCTIONS"
Development by Lakshey - "SCAN FUNCTIONS, SCHEMA FUNCTIONS, DEALING WITH RECORDS AND ATTRIBUTE VALUES"

RUNNING THE SCRIPT
=======================================

1) Go to Project root

2) run "make clean" to delete all old compiled .o files.

3) run "make" to compile all project files including "test_assign3_1.c" file 

4) run "make run" to run "test_assign3_1.c" file.


SOLUTION DESCRIPTION
=======================================

Referenced http://mrbook.org/blog/tutorials/make/ - For make file

1. TABLE AND RECORD MANAGER FUNCTIONS
=======================================

The record manager related functions deals with initializing and to shutdown the record manager using BufferPool and StorageManager. 

initRecordManager:
--> initializes the record manager.

shutdownRecordManager:
--> shutsdown the record manager and de-allocates all the allocated resources

createTable:
--> Creates a table with name as specified in the parameter 'name' in the schema specified in the parameter 'schema'.

openTable:
--> Opens the table having name specified by the paramater 'name', with attributes name, datatype and size.

closeTable:
--> Closes the table as pointed by the parameter 'rel'.

deleteTable:
--> Deletes the table with name specified by the parameter 'name'.

getNumTuples:
--> Returns the number of tuples in the table

2. RECORD FUNCTIONS
=======================================

These functions are used to perform operation on a record

insertRecord:
--> Inserts a record in the table

deleteRecord:
--> Deletes a record having Record ID 'id'

updateRecord:
--> Updates a record in the table referenced by "rel".

getRecord(....)
--> Retrieves a record having Record ID "id"

3. SCAN FUNCTIONS
=======================================

Scan related functions are used to retreieve all tuples from a table that fulfill a certain condition

startScan:
--> Starts a scan

next:
--> Returns the next tuple which satisfies the given condition

closeScan: 
--> Closes the scan operation.


4. SCHEMA FUNCTIONS
=========================================

Schema functions are used to return the size in bytes of records for a given schema and create a new schema. 

getRecordSize:
--> Returns the size of a record in the specified schema.

freeSchema:
--> Removes the schema from the memory.

createSchema:
--> Create a new schema in memory.

5. ATTRIBUTE FUNCTIONS
=========================================

access the attributes of a record

createRecord:
--> Creates a new record in the schema

attrOffset:
--> Sets the offset

freeRecord:
--> Deallocates the memory space allocated to the 'record'

getAttr:
--> Retrieves an attribute from the given record in the specified schema.

setAttr:
--> Sets the attribute value in the record in the specified schema.