/*
 * index.h
 *
 *  Created on: Dec 15, 2015
 *      Author: vangelis
 */

#ifndef INDEX_H_
#define INDEX_H_


#define MAXOPENFILES 20
#define MAXSCANS 20

typedef struct OpenFileNames {
	char filename[100];
	int id;
} OpenFileNames;

typedef struct OpenScans {
	int fileDesc;
	int blockNumber;
	int positionToBlock;
	void* value1;
	void* value2;
	int op;
} OpenScans;

typedef struct SpecialInfo {
	char identifier[20];
	int root;
	char type1;
	int size1;
	char type2;
	int size2;
} SpecialInfo;

typedef struct OverheadL {
	char type;          // 'i' for index block, 'l' for leaf block
	int sizeInBytes;    // Number of bytes written in block
	int numberOfRecords;// Number of records inside the block
	int prev;           // Points to previous leaf block
	int next;           // Points to next leaf block
	int parent;         // Pointer to parent block;
} OverheadL;

typedef struct OverheadI {
	char type;           // 'i' for index block, 'l' for leaf block
	int sizeInBytes;     // Number of bytes written in block
	int numberOfPointers;// Number of pointers inside the block
	int parent;          // Pointer to parent block;
} OverheadI;

#endif /* INDEX_H_ */
