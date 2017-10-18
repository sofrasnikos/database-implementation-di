/*
 * AM.c
 *
 *  Created on: Dec 15, 2015
 *      Author: vangelis
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AM.h"
#include "BF.h"
#include "index.h"

static OpenFileNames openFileArray[MAXOPENFILES];

static OpenScans openScanArray[MAXSCANS];

static int numberOfOpenFiles;
static int numberOfOpenScans;

int AM_errno;

void AM_Init(void) {
	int i;
	BF_Init();
	AM_errno = 0;
	numberOfOpenFiles = 0;
	numberOfOpenScans = 0;
	for (i = 0; i < MAXOPENFILES; i++) {
		openFileArray[i].id = -1;
		strcpy(openFileArray[i].filename, "\0");
		openScanArray[i].fileDesc = -1;
	}
	return;
}

int AM_CreateIndex(char *fileName, char atrType1, int attrLength1,
		char attrType2, int attrLength2) {
	SpecialInfo si = { "This is a b+ tree", -1, atrType1, attrLength1,
			attrType2, attrLength2 };
	int fileDesc;
	SpecialInfo* block;
	int blkCnt;
	// Create a file
	if (BF_CreateFile(fileName) < 0) {
		BF_PrintError("Error creating file");
		AM_errno = AM_ERRORINCREATEFILE;
		return AM_ERRORINCREATEFILE;
	}
	// Open it
	if ((fileDesc = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Error opening file");
		AM_errno = AM_ERRORINOPENFILE;
		return AM_ERRORINOPENFILE;
	}
	// Allocate a block to store information about the file
	if (BF_AllocateBlock(fileDesc) < 0) {
		BF_PrintError("Error allocating block");
		AM_errno = AM_ERRORINALLOCATEBLOCK;
		return AM_ERRORINALLOCATEBLOCK;
	}
	blkCnt = BF_GetBlockCounter(fileDesc);
	if (BF_ReadBlock(fileDesc, blkCnt - 1, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Copy Identifier there
	block[0] = si;
	if (BF_WriteBlock(fileDesc, 0) < 0) {
		BF_PrintError("Error writing block back");
		AM_errno = AM_ERRORINWRITEBLOCK;
		return AM_ERRORINWRITEBLOCK;
	}
	// Close the file
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Error closing file");
		AM_errno = AM_ERRORINCLOSEFILE;
		return AM_ERRORINCLOSEFILE;
	}
	return AME_OK;
}

int AM_DestroyIndex(char *fileName) {

	if (fileIsOpen(fileName) == 1) {
		AM_errno = AM_FILEOPEN;
		return AM_FILEOPEN;
	}
	if (remove(fileName) != 0) {
		AM_errno = AM_RMVFAIL;
		return AM_RMVFAIL;
	}
	return AME_OK;
}

int AM_OpenIndex(char *fileName) {
	int i;
	int fileDesc;
	SpecialInfo* block;
	SpecialInfo si;
	// Open file
	if ((fileDesc = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Error opening file");
		AM_errno = AM_ERRORINOPENFILE;
		return AM_ERRORINOPENFILE;
	}
	// Read its first block
	if (BF_ReadBlock(fileDesc, 0, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Read special information
	memmove(&si, block, sizeof(SpecialInfo));

	if (numberOfOpenFiles >= MAXOPENFILES) {
		AM_errno = AM_MAXFILESREACHED;
		return AM_MAXFILESREACHED;
	}
	// Check if it is a b+ tree file
	if (strcmp(si.identifier, "This is a b+ tree") != 0) {
		AM_errno = AM_NOTBPLUSTREE;
		return AM_NOTBPLUSTREE;
	}
	// Find the first empty position
	for (i = 0; i < MAXOPENFILES; i++) {
		if (openFileArray[i].id == -1) {
			break;
		}
	}
	// Insert filename to the array
	strcpy(openFileArray[i].filename, fileName);
	openFileArray[i].id = fileDesc;
	numberOfOpenFiles++;
	return fileDesc;
}

int AM_CloseIndex(int fileDesc) {
	int i;
	if (scanIsOpen(fileDesc) == 1) {
		AM_errno = AM_SCANISOPEN;
		return AM_SCANISOPEN;
	}
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Error closing file");
		AM_errno = AM_ERRORINCLOSEFILE;
		return AM_ERRORINCLOSEFILE;
	}
	// Remove entry from openFileArray
	for (i = 0; i < MAXOPENFILES; i++) {
		if (openFileArray[i].id == fileDesc) {
			openFileArray[i].id = -1;
			strcpy(openFileArray[i].filename, "\0");
			numberOfOpenFiles--;
		}
	}
	return AME_OK;
}

int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
	SpecialInfo si;
	void* block;
	// Read special information
	if (BF_ReadBlock(fileDesc, 0, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	memmove(&si, block, sizeof(SpecialInfo));
	// If there is no root (only one leaf block)
	if (si.root == -1) {
		OverheadL overheadL = { 'l', sizeof(OverheadL), 0, -1, -1, -1 };
		// Create one
		if (BF_AllocateBlock(fileDesc) < 0) {
			BF_PrintError("Error allocating block");
			AM_errno = AM_ERRORINALLOCATEBLOCK;
			return AM_ERRORINALLOCATEBLOCK;
		}

		int blkCnt = BF_GetBlockCounter(fileDesc);

		if (BF_ReadBlock(fileDesc, blkCnt - 1, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			AM_errno = AM_ERRORINREADBLOCK;
			return AM_ERRORINREADBLOCK;
		}
		// Write overhead to block
		memmove(block, &overheadL, sizeof(OverheadL));

		if (BF_WriteBlock(fileDesc, blkCnt - 1) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
		// Update special information
		si.root = blkCnt - 1;
		if (BF_ReadBlock(fileDesc, 0, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			AM_errno = AM_ERRORINREADBLOCK;
			return AM_ERRORINREADBLOCK;
		}

		memmove(block, &si, sizeof(SpecialInfo));

		if (BF_WriteBlock(fileDesc, 0) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
	}
	int f;
	// Find where the new record should be placed
	f = findBlock(fileDesc, si.root, value1, si.size1, si.type1);
	// Insert it there
	if (insertInLeafBlock(fileDesc, f, value1, value2, si.size1, si.size2,
			si.type1) < 0) {
		return AM_errno;
	}
	return AME_OK;
}

int AM_OpenIndexScan(int fileDesc, int op, void* value) {
	SpecialInfo si;
	void* block;
	int target;
	int position;
	if (numberOfOpenScans >= MAXSCANS) {
		AM_errno = AM_MAXSCANSREACHED;
		return AM_MAXSCANSREACHED;
	}
	// Find the first empty position
	for (position = 0; position < MAXSCANS; position++) {
		if (openScanArray[position].fileDesc == -1) {
			break;
		}
	}
	numberOfOpenScans++;
	// Read special information
	if (BF_ReadBlock(fileDesc, 0, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	memmove(&si, block, sizeof(SpecialInfo));

	int* intvalue = value;
	float* floatvalue = value;
	char* charvalue = value;

	int intIterator;
	float floatIterator;
	char* charIterator = malloc(si.size1);
	if (charIterator == NULL) {
		AM_errno = AM_MALLOCERROR;
		return AM_MALLOCERROR;
	}
	OverheadL overheadL;

	int offset = sizeof(OverheadL);
	// If op == EQUAL, GREATER_THAN or GREATER_THAN_OR_EQUAL
	if (op == 1 || op == 4 || op == 6) {
		// Find the first record with given value
		target = findBlock(fileDesc, si.root, value, si.size1, si.type1);
		// Read that block
		if (BF_ReadBlock(fileDesc, target, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			AM_errno = AM_ERRORINREADBLOCK;
			return AM_ERRORINREADBLOCK;
		}
		memmove(&overheadL, block, sizeof(OverheadL));
		int i = 0;
		switch (si.type1) {
		case 'i':
			// Iterator = the first element of that block
			memmove(&intIterator, block + sizeof(OverheadL), si.size1);
			while (*intvalue > intIterator && i < overheadL.numberOfRecords) {
				offset += si.size1 + si.size2;
				// Iterator = next element
				memmove(&intIterator, block + offset, si.size1);
				i++;
			}
			break;
		case 'f':
			// Iterator = the first element of that block
			memmove(&floatIterator, block + sizeof(OverheadL), si.size1);
			while (*floatvalue > floatIterator && i < overheadL.numberOfRecords) {
				offset += si.size1 + si.size2;
				// Iterator = next element
				memmove(&floatIterator, block + offset, si.size1);
				i++;
			}
			break;
		case 'c':
			// Iterator = the first element of that block
			memmove(charIterator, block + sizeof(OverheadL), si.size1);
			while (strcmp(charvalue, charIterator) > 0
					&& i < overheadL.numberOfRecords) {
				offset += si.size1 + si.size2;
				// Iterator = next element
				memmove(charIterator, block + offset, si.size1);
				i++;
			}
			break;
		default:
			AM_errno = AM_UNKNOWNTYPE;
			return AM_UNKNOWNTYPE;
		}
		openScanArray[position].positionToBlock = offset;
		// positionToBlock now points to the record that has greater or equal value
	} else if (op == 2 || op == 3 || op == 5) {
		target = findFirst(fileDesc, si.root);
		if (BF_ReadBlock(fileDesc, target, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			AM_errno = AM_ERRORINREADBLOCK;
			return AM_ERRORINREADBLOCK;
		}
		memmove(&overheadL, block, sizeof(OverheadL));
		// positionToBlock now points to the first record of the "left" leaf block
		openScanArray[position].positionToBlock = sizeof(OverheadL);
	} else {
		AM_errno = AM_UNKNOWNOP;
		return AM_UNKNOWNOP;
	}
	free(charIterator);
	// Create new entry in openScanArray
	openScanArray[position].fileDesc = fileDesc;
	openScanArray[position].op = op;
	openScanArray[position].value1 = malloc(si.size1);
	memcpy(openScanArray[position].value1, value, si.size1);
	openScanArray[position].value2 = malloc(si.size2);
	openScanArray[position].blockNumber = target;
	return position;
}

void *AM_FindNextEntry(int scanDesc) {

	int fileDesc = openScanArray[scanDesc].fileDesc;
	int blockNumber = openScanArray[scanDesc].blockNumber;
	void* value1 = openScanArray[scanDesc].value1;
	void* value2 = openScanArray[scanDesc].value2;
	int offset = openScanArray[scanDesc].positionToBlock;
	int op = openScanArray[scanDesc].op;
	SpecialInfo si;
	void* block;
	if (blockNumber == -1) {
		AM_errno = AME_EOF;
		return NULL;
	}
	// Read special information
	if (BF_ReadBlock(fileDesc, 0, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return NULL;
	}
	memmove(&si, block, sizeof(SpecialInfo));
	// Read the overhead of the block
	if (BF_ReadBlock(fileDesc, blockNumber, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINWRITEBLOCK;
		return NULL;
	}
	OverheadL overheadL;
	memcpy(&overheadL, block, sizeof(OverheadL));
	int intValue;
	float floatValue;
	char* charValue = malloc(si.size1);
	if (charValue == NULL) {
		AM_errno = AM_MALLOCERROR;
		return NULL;
	}
	switch (si.type1) {
	case 'i':
		// Read the record from this position of the block
		memcpy(&intValue, block + offset, si.size1);
		switch (op) {
		case EQUAL:
			if (intValue == *(int*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case NOT_EQUAL:
			if (intValue != *(int*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				// In case when intValue == *(int*) value1
				while (intValue == *(int*) value1) {
					offset += si.size1 + si.size2;
					// In case we need to change block
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(&intValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		case LESS_THAN:
			if (intValue < *(int*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case GREATER_THAN:
			if (intValue > *(int*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				// In case when intValue <= *(int*) value1
				while (intValue <= *(int*) value1) {
					offset += si.size1 + si.size2;
					// In case we need to change block
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(&intValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		case LESS_THAN_OR_EQUAL:
			if (intValue <= *(int*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case GREATER_THAN_OR_EQUAL:
			if (intValue >= *(int*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				// In case when intValue < *(int*) value1
				while (intValue < *(int*) value1) {
					offset += si.size1 + si.size2;
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(&intValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		default:
			AM_errno = AM_UNKNOWNOP;
			return NULL;
		}
		break;
	case 'f':
		memcpy(&floatValue, block + offset, si.size1);
		switch (op) {
		case EQUAL:
			if (floatValue == *(float*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case NOT_EQUAL:
			if (floatValue != *(float*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				// In case when floatValue == *(float*) value1
				while (floatValue == *(float*) value1) {
					offset += si.size1 + si.size2;
					// In case we need to change block
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(&floatValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		case LESS_THAN:
			if (floatValue < *(float*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case GREATER_THAN:
			if (floatValue > *(float*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				// In case when floatValue <= *(float*) value1
				while (floatValue <= *(float*) value1) {
					offset += si.size1 + si.size2;
					// In case we need to change block
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(&floatValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		case LESS_THAN_OR_EQUAL:
			if (floatValue <= *(float*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case GREATER_THAN_OR_EQUAL:
			if (floatValue >= *(float*) value1) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				// In case when floatValue < *(float*) value1
				while (floatValue < *(float*) value1) {
					offset += si.size1 + si.size2;
					// In case we need to change block
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(&floatValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		default:
			AM_errno = AM_UNKNOWNOP;
			return NULL;
		}
		break;
	case 'c':
		memcpy(charValue, block + offset, si.size1);
		switch (op) {
		case EQUAL:
			if (strcmp(charValue, (char*) value1) == 0) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case NOT_EQUAL:
			if (strcmp(charValue, (char*) value1) != 0) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				// In case when charValue == (char*) value1
				while (strcmp(charValue, (char*) value1) == 0) {
					offset += si.size1 + si.size2;
					// In case we need to change block
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(charValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		case LESS_THAN:
			if (strcmp(charValue, (char*) value1) < 0) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case GREATER_THAN:
			if (strcmp(charValue, (char*) value1) > 0) {
				memcpy(value2, block + offset + si.size1, si.size2);
			} else {
				// In case when charValue <= (char*) value1
				while (strcmp(charValue, (char*) value1) <= 0) {
					offset += si.size1 + si.size2;
					// In case we need to change block
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(charValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		case LESS_THAN_OR_EQUAL:
			if (strcmp(charValue, (char*) value1) <= 0) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				AM_errno = AME_EOF;
				return NULL;
			}
			break;
		case GREATER_THAN_OR_EQUAL:
			if (strcmp(charValue, (char*) value1) >= 0) {
				memcpy(value2, block + offset + si.size1, si.size2);
				offset += si.size1 + si.size2;
			} else {
				// In case when charValue >= (char*) value1
				while (strcmp(charValue, (char*) value1) < 0) {
					offset += si.size1 + si.size2;
					// In case we need to change block
					if (offset >= overheadL.sizeInBytes) {
						openScanArray[scanDesc].blockNumber = overheadL.next;
						openScanArray[scanDesc].positionToBlock =
								sizeof(OverheadL);
						value2 = AM_FindNextEntry(scanDesc);
						return value2;
					}
					// Read the next element
					memcpy(charValue, block + offset, si.size1);
					memcpy(value2, block + offset + si.size1, si.size2);
					offset += si.size1 + si.size2;
				}
			}
			break;
		default:
			AM_errno = AM_UNKNOWNOP;
			return NULL;
		}
		break;
	}
	// In case we need to change block
	if (offset >= overheadL.sizeInBytes) {
		offset = sizeof(OverheadL);
		openScanArray[scanDesc].blockNumber = overheadL.next;
	}
	openScanArray[scanDesc].positionToBlock = offset;
	return value2;
}

int AM_CloseIndexScan(int scanDesc) {
	if (openScanArray[scanDesc].fileDesc == -1) {
		AM_errno = AM_SCANISOPEN;
		return AM_SCANISOPEN;
	}
	openScanArray[scanDesc].fileDesc = -1;
	free(openScanArray[scanDesc].value1);
	free(openScanArray[scanDesc].value2);
	numberOfOpenScans--;
	return AME_OK;
}

void AM_PrintError(char *errString) {
	if (AM_errno != 0) {
		printf("%s. ", errString);
	}
	switch (AM_errno) {
	case AM_MAXFILESREACHED:
		printf("Max number of files has been reached\n");
		break;
	case AM_FILEOPEN:
		printf("File is open\n");
		break;
	case AM_RMVFAIL:
		printf("File cannot be removed\n");
		break;
	case AM_FILENOTFOUND:
		printf("File not found\n");
		break;
	case AM_ERRORINCREATEFILE:
		printf("File cannot be created\n");
		break;
	case AM_ERRORINOPENFILE:
		printf("File cannot be opened\n");
		break;
	case AM_ERRORINALLOCATEBLOCK:
		printf("Error in allocate block\n");
		break;
	case AM_ERRORINREADBLOCK:
		printf("Error in read block\n");
		break;
	case AM_ERRORINWRITEBLOCK:
		printf("Error in write block\n");
		break;
	case AM_ERRORINCLOSEFILE:
		printf("Error in close file\n");
		break;
	case AM_NOTBPLUSTREE:
		printf("File not recognized as a b+ tree file\n");
		break;
	case AM_SCANISOPEN:
		printf("Scan is open\n");
		break;
	case AM_MAXSCANSREACHED:
		printf("Max scans reached\n");
		break;
	case AM_MALLOCERROR:
		printf("Malloc error\n");
		break;
	case AM_UNKNOWNOP:
		printf("Unknown operation\n");
		break;
	case AM_UNKNOWNTYPE:
		printf("Unknown data type\n");
		break;
	case AME_EOF:
		printf("\n");
		break;
	case 0:
		printf("\n");
		break;
	default:
		printf("Unknown error code\n");
		break;
	}
}

int fileIsOpen(char *fileName) {
	int i;
	for (i = 0; i < MAXOPENFILES; i++) {
		if (strcmp(openFileArray[i].filename, fileName) == 0) {
			return 1;
		}
	}
	return 0;
}

int scanIsOpen(int fileDesc) {
	int i;
	for (i = 0; i < MAXSCANS; i++) {
		if (openScanArray[i].fileDesc == fileDesc) {
			return 1;
		}
	}
	return 0;
}

int insertInLeafBlock(int fileDesc, int blockNumber, void *value1, void *value2,
		int size1, int size2, char type1) {

	int* intvalue = value1;
	float* floatvalue = value1;
	char* charvalue = value1;

	int intIterator;
	float floatIterator;
	char* charIterator = malloc(size1);
	if (charIterator == NULL) {
		AM_errno = AM_MALLOCERROR;
		return AM_MALLOCERROR;
	}
	OverheadL overheadL;
	void* block;
	if (BF_ReadBlock(fileDesc, blockNumber, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Read overhead
	memmove(&overheadL, block, sizeof(OverheadL));
	// If record can fit inside this block
	if ((BLOCK_SIZE - (overheadL.sizeInBytes)) >= (size1 + size2)) {
		int offset = sizeof(OverheadL);
		int i = 0;
		// If block is empty
		if (overheadL.numberOfRecords == 0) {
			// Insert first element
			memmove(block + sizeof(OverheadL), value1, size1);
			memmove(block + sizeof(OverheadL) + size1, value2, size2);
			overheadL.sizeInBytes += size1 + size2;
			overheadL.numberOfRecords++;
			memmove(block, &overheadL, sizeof(OverheadL));
			if (BF_WriteBlock(fileDesc, 1) < 0) {
				BF_PrintError("Error writing block back");
				AM_errno = AM_ERRORINWRITEBLOCK;
				return AM_ERRORINWRITEBLOCK;
			}
			free(charIterator);
			return AME_OK;
		}
		switch (type1) {
		case 'i':
			// Read the first element of the block
			memmove(&intIterator, block + sizeof(OverheadL), size1);
			while (*intvalue > intIterator && i < overheadL.numberOfRecords) {
				// Iterator = next element
				offset += size1 + size2;
				// Read the next element
				memmove(&intIterator, block + offset, size1);
				i++;
			}
			break;
		case 'f':
			// Read the first element of the block
			memmove(&floatIterator, block + sizeof(OverheadL), size1);
			while (*floatvalue > floatIterator && i < overheadL.numberOfRecords) {
				// Iterator = next element
				offset += size1 + size2;
				// Read the next element
				memmove(&floatIterator, block + offset, size1);
				i++;
			}
			break;
		case 'c':
			// Read the first element of the block
			memmove(charIterator, block + sizeof(OverheadL), size1);
			while (strcmp(charvalue, charIterator) > 0
					&& i < overheadL.numberOfRecords) {
				// Iterator = next element
				offset += size1 + size2;
				// Read the next element
				memmove(charIterator, block + offset, size1);
				i++;
			}
			break;
		default:
			AM_errno = AM_UNKNOWNTYPE;
			return AM_UNKNOWNTYPE;
		}

		// Offset now points to the position where the new record should be placed
		// If this is not the last entry
		if (i < overheadL.numberOfRecords) {
			// Shift right the records that have greater key value
			memmove(block + offset + size1 + size2, block + offset,
					overheadL.sizeInBytes - offset);
		}
		// Insert the record
		memmove(block + offset, value1, size1);
		overheadL.sizeInBytes += size1;
		memmove(block + offset + size1, value2, size2);
		overheadL.sizeInBytes += size2;
		overheadL.numberOfRecords++;
		memmove(block, &overheadL, sizeof(OverheadL));
		if (BF_WriteBlock(fileDesc, blockNumber) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
	} else { // If record cannot fit inside this block
		int brotherBlock = splitLeafBlock(fileDesc, blockNumber, size1, size2,
				type1);
		if (BF_ReadBlock(fileDesc, brotherBlock, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			AM_errno = AM_ERRORINREADBLOCK;
			return AM_ERRORINREADBLOCK;
		}
		int intFirstValueOfBrotherBlock;
		float floatFirstValueOfBrotherBlock;
		char* charFirstValueOfBrotherBlock;
		charFirstValueOfBrotherBlock = malloc(size1);
		if (charFirstValueOfBrotherBlock == NULL) {
			AM_errno = AM_MALLOCERROR;
			return AM_MALLOCERROR;
		}
		// Fetch the first element of the first record of the newly created block
		switch (type1) {
		case 'i':
			memcpy(&intFirstValueOfBrotherBlock, block + sizeof(OverheadL),
					size1);
			if (*intvalue < intFirstValueOfBrotherBlock) {
				// Insert it to the current block
				if (insertInLeafBlock(fileDesc, blockNumber, value1, value2,
						size1, size2, type1) < 0) {
					return AM_errno;
				}
			} else { // Insert it to the newly created block
				if (insertInLeafBlock(fileDesc, brotherBlock, value1, value2,
						size1, size2, type1) < 0) {
					return AM_errno;
				}
			}
			break;
		case 'f':
			memcpy(&floatFirstValueOfBrotherBlock, block + sizeof(OverheadL),
					size1);
			if (*floatvalue < floatFirstValueOfBrotherBlock) {
				// Insert it to the current block
				if (insertInLeafBlock(fileDesc, blockNumber, value1, value2,
						size1, size2, type1) < 0) {
					return AM_errno;
				}
			} else { // Insert it to the newly created block
				if (insertInLeafBlock(fileDesc, brotherBlock, value1, value2,
						size1, size2, type1) < 0) {
					return AM_errno;
				}
			}
			break;
		case 'c':
			memcpy(charFirstValueOfBrotherBlock, block + sizeof(OverheadL),
					size1);
			if (strcmp(charvalue, charFirstValueOfBrotherBlock) < 0) {
				// Insert it to the current block
				if (insertInLeafBlock(fileDesc, blockNumber, value1, value2,
						size1, size2, type1) < 0) {
					return AM_errno;
				}
			} else { // Insert it to the newly created block
				if (insertInLeafBlock(fileDesc, brotherBlock, value1, value2,
						size1, size2, type1) < 0) {
					return AM_errno;
				}
			}

			break;
		default:
			AM_errno = AM_UNKNOWNTYPE;
			return AM_UNKNOWNTYPE;
		}
		free(charFirstValueOfBrotherBlock);
	}
	free(charIterator);
	return AME_OK;
}

int insertInIndexBlock(int fileDesc, int blockNumber, void *value1, int size1,
		int pointer1, int pointer2, char type1) {
	OverheadI overheadI;
	void* block;
	int blkCnt = blockNumber;
	// If there is no allocated block, allocate a new one
	if (blockNumber == -1) {
		if (BF_AllocateBlock(fileDesc) < 0) {
			BF_PrintError("Error allocating block");
			AM_errno = AM_ERRORINALLOCATEBLOCK;
			return AM_ERRORINALLOCATEBLOCK;
		}
		// Initialize its overhead
		overheadI.type = 'i';
		overheadI.numberOfPointers = 0;
		overheadI.sizeInBytes = sizeof(OverheadI);
		overheadI.parent = -1;
		blkCnt = BF_GetBlockCounter(fileDesc);
		if (BF_ReadBlock(fileDesc, blkCnt - 1, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			AM_errno = AM_ERRORINREADBLOCK;
			return AM_ERRORINREADBLOCK;
		}
		memmove(block, &overheadI, sizeof(OverheadI));
		// Write back the overhead to the block
		if (BF_WriteBlock(fileDesc, blkCnt - 1) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
		blkCnt--;
	} else {
		if (BF_ReadBlock(fileDesc, blockNumber, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			AM_errno = AM_ERRORINREADBLOCK;
			return AM_ERRORINREADBLOCK;
		}
		memmove(&overheadI, block, sizeof(OverheadI));
	}
	if (overheadI.parent == -1) {
		SpecialInfo si;
		// Read special information
		if (BF_ReadBlock(fileDesc, 0, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			AM_errno = AM_ERRORINREADBLOCK;
			return AM_ERRORINREADBLOCK;
		}
		memmove(&si, block, sizeof(SpecialInfo));
		// Set new root
		si.root = blkCnt;
		// Update special information
		memmove(block, &si, sizeof(SpecialInfo));
		if (BF_WriteBlock(fileDesc, 0) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
	}
	if (BF_ReadBlock(fileDesc, blkCnt, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Read overhead
	memmove(&overheadI, block, sizeof(OverheadI));
	int offset = sizeof(overheadI);
	// If index block has just been created, insert both values
	if (overheadI.numberOfPointers == 0) {
		memmove(block + offset, &pointer1, sizeof(int));
		offset += sizeof(int);
		memmove(block + offset, value1, size1);
		offset += size1;
		memmove(block + offset, &pointer2, sizeof(int));
		offset += sizeof(int);
		overheadI.sizeInBytes = offset;
		overheadI.numberOfPointers = 2;
		memmove(block, &overheadI, sizeof(OverheadI));
		if (BF_WriteBlock(fileDesc, blkCnt) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
	} else {
		// Otherwise insert just the second value
		int i = 0;
		// Move offset so that it points to the first value
		offset += sizeof(int);
		int intIterator;
		float floatIterator;
		char* charIterator;
		switch (type1) {
		case 'i':
			// Read the first element of the block
			memmove(&intIterator, block + offset, size1);
			while (*(int*) value1 > intIterator
					&& i < overheadI.numberOfPointers - 1) {
				// Iterator = next element
				offset += size1 + sizeof(int);
				// Read the next element
				memmove(&intIterator, block + offset, size1);
				i++;
			}
			break;
		case 'f':
			// Read the first element of the block
			memmove(&floatIterator, block + offset, size1);
			while (*(float*) value1 > floatIterator
					&& i < overheadI.numberOfPointers - 1) {
				// Iterator = next element
				offset += size1 + sizeof(int);
				// Read the next element
				memmove(&floatIterator, block + offset, size1);
				i++;
			}
			break;
		case 'c':
			charIterator = malloc(size1);
			// Read the first element of the block
			memmove(charIterator, block + offset, size1);
			while (strcmp((char*) value1, charIterator) > 0
					&& i < overheadI.numberOfPointers - 1) {
				// Iterator = next element
				offset += size1 + sizeof(int);
				// Read the next element
				memmove(charIterator, block + offset, size1);
				i++;
			}
			break;
		default:
			AM_errno = AM_UNKNOWNTYPE;
			return AM_UNKNOWNTYPE;
		}
		// Offset now points to the position where the new value should be placed
		// If this is not the last entry
		if (i < overheadI.numberOfPointers - 1) {
			// Shift right the records that have greater key value
			memmove(block + offset + size1, block + offset - sizeof(int),
					overheadI.sizeInBytes - offset + sizeof(int));
		}
		memmove(block + offset, value1, size1);
		memmove(block + offset - sizeof(int), &pointer1, sizeof(int));
		memmove(block + offset + size1, &pointer2, sizeof(int));
		overheadI.numberOfPointers++;
		overheadI.sizeInBytes += size1 + sizeof(int);
		// Update overhead
		memmove(block, &overheadI, sizeof(overheadI));
		if (BF_WriteBlock(fileDesc, blkCnt) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
	}
	int freeSpace = BLOCK_SIZE - overheadI.sizeInBytes;
	// If the next element cannot fit
	if (freeSpace < (size1 + sizeof(int))) {
		splitIndexBlock(fileDesc, blockNumber, size1, type1);
	}
	return blkCnt;
}

int splitLeafBlock(int fileDesc, int blockNumber, int size1, int size2, char type1) {
	OverheadL overheadL;
	void* block;

	if (BF_ReadBlock(fileDesc, blockNumber, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Read overhead
	memmove(&overheadL, block, sizeof(OverheadL));

	int sizeOfRecordsToBeCopied;
	void* block2;
	// Create your brother
	// Allocate new block
	if (BF_AllocateBlock(fileDesc) < 0) {
		BF_PrintError("Error allocating block");
		AM_errno = AM_ERRORINALLOCATEBLOCK;
		return AM_ERRORINALLOCATEBLOCK;
	}
	int blkCnt = BF_GetBlockCounter(fileDesc);
	if (BF_ReadBlock(fileDesc, blkCnt - 1, (void**) &block2) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	OverheadL overheadL2 =
			{ 'l', sizeof(OverheadL), 0, -1, -1, overheadL.parent };
	int median = overheadL.numberOfRecords / 2;
	sizeOfRecordsToBeCopied = (size1 + size2)
			* (overheadL.numberOfRecords - median);
	memcpy(block2 + sizeof(OverheadL),
			block + overheadL.sizeInBytes - sizeOfRecordsToBeCopied,
			sizeOfRecordsToBeCopied);
	overheadL2.sizeInBytes += sizeOfRecordsToBeCopied;
	overheadL2.numberOfRecords = overheadL.numberOfRecords - median;
	overheadL.sizeInBytes -= sizeOfRecordsToBeCopied;
	overheadL.numberOfRecords -= overheadL2.numberOfRecords;
	overheadL2.prev = blockNumber;
	overheadL2.next = overheadL.next;
	if (overheadL.next != -1)
		modifyPrevInNextBlock(fileDesc, overheadL.next, blkCnt - 1);
	overheadL.next = blkCnt - 1;
	memcpy(block, &overheadL, sizeof(OverheadL));
	memcpy(block2, &overheadL2, sizeof(OverheadL));

	if (BF_WriteBlock(fileDesc, blockNumber) < 0) {
		BF_PrintError("Error writing block back");
		AM_errno = AM_ERRORINWRITEBLOCK;
		return AM_ERRORINWRITEBLOCK;
	}
	if (BF_WriteBlock(fileDesc, blkCnt - 1) < 0) {
		BF_PrintError("Error writing block back");
		AM_errno = AM_ERRORINWRITEBLOCK;
		return AM_ERRORINWRITEBLOCK;
	}
	void* valueOfFirstElement = malloc(size1);
	if (valueOfFirstElement == NULL) {
		AM_errno = AM_MALLOCERROR;
		return AM_MALLOCERROR;
	}
	memcpy(valueOfFirstElement, block2 + sizeof(OverheadL), size1);
	int par = insertInIndexBlock(fileDesc, overheadL.parent,
			valueOfFirstElement, size1, blockNumber, blkCnt - 1, type1);
	// If first time
	if (overheadL.parent == -1) {
		overheadL.parent = par;
		overheadL2.parent = par;
		memcpy(block, &overheadL, sizeof(OverheadL));
		memcpy(block2, &overheadL2, sizeof(OverheadL));

		if (BF_WriteBlock(fileDesc, blockNumber) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
		if (BF_WriteBlock(fileDesc, blkCnt - 1) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
	}
	free(valueOfFirstElement);
	overheadL2.parent = par;
	return blkCnt - 1;
}

int modifyPrevInNextBlock(int fileDesc, int blockNumber,
		int previousBlockNumber) {
	OverheadL overheadL;
	void* block;

	if (BF_ReadBlock(fileDesc, blockNumber, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Read overhead
	memmove(&overheadL, block, sizeof(OverheadL));
	overheadL.prev = previousBlockNumber;
	// Update overhead
	memcpy(block, &overheadL, sizeof(OverheadL));
	if (BF_WriteBlock(fileDesc, blockNumber) < 0) {
		BF_PrintError("Error writing block back");
		AM_errno = AM_ERRORINWRITEBLOCK;
		return AM_ERRORINWRITEBLOCK;
	}
	return AME_OK;
}

int splitIndexBlock(int fileDesc, int blockNumber, int size1, char type1) {
	OverheadI overheadI;
	void* block;
	if (BF_ReadBlock(fileDesc, blockNumber, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Read overhead
	memmove(&overheadI, block, sizeof(OverheadI));
	void* block2;
	// Create your brother
	// Allocate new block
	if (BF_AllocateBlock(fileDesc) < 0) {
		BF_PrintError("Error allocating block");
		AM_errno = AM_ERRORINALLOCATEBLOCK;
		return AM_ERRORINALLOCATEBLOCK;
	}
	int blkCnt = BF_GetBlockCounter(fileDesc);
	// Read new block
	if (BF_ReadBlock(fileDesc, blkCnt - 1, (void**) &block2) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Create its overhead
	OverheadI overheadI2 = { 'i', sizeof(OverheadI), 0, overheadI.parent };
	int median = overheadI.numberOfPointers / 2;
	int offset = sizeof(overheadI) + median * (size1 + sizeof(int));
	int sizeOfRecordsToBeCopied = overheadI.sizeInBytes - offset;
	memcpy(block2 + sizeof(overheadI), block + offset,
			overheadI.sizeInBytes - offset);
	overheadI2.sizeInBytes = sizeof(overheadI) + sizeOfRecordsToBeCopied;
	overheadI2.numberOfPointers = overheadI.numberOfPointers - median;
	overheadI.sizeInBytes -= sizeOfRecordsToBeCopied - (size1 + sizeof(int));
	overheadI.numberOfPointers -= overheadI2.numberOfPointers - 1;
	void* valueOfMedian = malloc(size1);
	if (valueOfMedian == NULL) {
		AM_errno = AM_MALLOCERROR;
		return AM_MALLOCERROR;
	}
	memmove(valueOfMedian, block + offset + sizeof(int), size1);
	memcpy(block, &overheadI, sizeof(OverheadI));
	memcpy(block2, &overheadI2, sizeof(OverheadI));
	// Write blocks back
	if (BF_WriteBlock(fileDesc, blockNumber) < 0) {
		BF_PrintError("Error writing block back");
		AM_errno = AM_ERRORINWRITEBLOCK;
		return AM_ERRORINWRITEBLOCK;
	}
	if (BF_WriteBlock(fileDesc, blkCnt - 1) < 0) {
		BF_PrintError("Error writing block back");
		AM_errno = AM_ERRORINWRITEBLOCK;
		return AM_ERRORINWRITEBLOCK;
	}
	int par = insertInIndexBlock(fileDesc, overheadI.parent, valueOfMedian,
			size1, blockNumber, blkCnt - 1, type1);
	// If first time
	if (overheadI.parent == -1) {
		overheadI.parent = par;
		overheadI2.parent = par;
		memcpy(block, &overheadI, sizeof(OverheadI));
		memcpy(block2, &overheadI2, sizeof(OverheadI));
		if (BF_WriteBlock(fileDesc, blockNumber) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
		if (BF_WriteBlock(fileDesc, blkCnt - 1) < 0) {
			BF_PrintError("Error writing block back");
			AM_errno = AM_ERRORINWRITEBLOCK;
			return AM_ERRORINWRITEBLOCK;
		}
	}
	setAllParents(fileDesc, blkCnt - 1, size1);
	free(valueOfMedian);
	return blkCnt - 1;
}

void setAllParents(int fileDesc, int blockNumber, int size1) {
	OverheadI overheadI;
	void* block;
	if (BF_ReadBlock(fileDesc, blockNumber, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return;
	}
	memmove(&overheadI, block, sizeof(OverheadI));
	int offset = sizeof(overheadI);
	int i, target;
	for (i = 0; i < overheadI.numberOfPointers; i++) {
		memcpy(&target, block + offset, sizeof(int));
		iAmYourFather(fileDesc, target, blockNumber);
		offset += size1 + sizeof(int);
	}
	return;
}

void iAmYourFather/*Luke*/(int fileDesc, int blockNumber, int father) {
	OverheadI overheadI;
	OverheadL overheadL;
	char type;
	void* block;
	if (BF_ReadBlock(fileDesc, blockNumber, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return;
	}
	memmove(&type, block, sizeof(char));
	if (type == 'i') {
		memmove(&overheadI, block, sizeof(OverheadI));
		overheadI.parent = father;
		memmove(block, &overheadI, sizeof(OverheadI));
	} else {  //type == 'l'
		memmove(&overheadL, block, sizeof(OverheadL));
		overheadL.parent = father;
		memmove(block, &overheadL, sizeof(OverheadL));
	}
	if (BF_WriteBlock(fileDesc, blockNumber) < 0) {
		BF_PrintError("Error writing block back");
		AM_errno = AM_ERRORINWRITEBLOCK;
		return;
	}
}

int findBlock(int fileDesc, int currentBlock, void *value, int size1,
		char type1) {
	OverheadI overheadI;
	void* block;
	if (BF_ReadBlock(fileDesc, currentBlock, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		return -1;
	}
	// Check if we are on a leaf or and index block
	char type;
	memmove(&type, block, sizeof(char));
	if (type == 'l') {
		return currentBlock;
	} else if (type == 'i') {
		int intIterator;
		float floatIterator;
		char* charIterator;
		memmove(&overheadI, block, sizeof(OverheadI));
		// offset now points to the first value
		int offset = sizeof(OverheadI) + sizeof(int);
		int i = 0;
		switch (type1) {
		case 'i':
			memmove(&intIterator, block + offset, size1);
			while (intIterator < *(int*) value && offset < overheadI.sizeInBytes) {
				offset += size1 + sizeof(int);
				// Read the next element
				memmove(&intIterator, block + offset, size1);
				i++;
			}
			break;
		case 'f':
			memmove(&floatIterator, block + offset, size1);
			while (floatIterator < *(float*) value
					&& offset < overheadI.sizeInBytes) {
				offset += size1 + sizeof(int);
				// Read the next element
				memmove(&floatIterator, block + offset, size1);
				i++;
			}
			break;
		case 'c':
			charIterator = malloc(size1);
			if (charIterator == NULL) {
				AM_errno = AM_MALLOCERROR;
				return AM_MALLOCERROR;
			}
			memmove(charIterator, block + offset, size1);
			while (strcmp(charIterator, (char*) value) < 0
					&& offset < overheadI.sizeInBytes) {
				offset += size1 + sizeof(int);
				// Read the next element
				memmove(charIterator, block + offset, size1);
				i++;
			}
			free(charIterator);
			break;
		default:
			AM_errno = AM_UNKNOWNTYPE;
			return AM_UNKNOWNTYPE;
		}
		// Offset now points to the position where the new record should be placed
		int target;
		memmove(&target, block + offset - sizeof(int), sizeof(int));
		int returnValue = findBlock(fileDesc, target, value, size1, type1);
		return returnValue;
	} else {
		AM_errno = AM_UNKNOWNBLOCKTYPE;
		return AM_UNKNOWNBLOCKTYPE;
	}
}

int findFirst(int fileDesc, int currentBlock) {
	void* block;
	// Special case when only one block in tree
	if (currentBlock == -1) {
		return currentBlock;
	}
	if (BF_ReadBlock(fileDesc, currentBlock, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		AM_errno = AM_ERRORINREADBLOCK;
		return AM_ERRORINREADBLOCK;
	}
	// Check if we are on a leaf or and index block
	char type;
	memmove(&type, block, sizeof(char));
	if (type == 'l') {
		return currentBlock;
	} else if (type == 'i') {
		int target;
		// target = pointer to the "left" block
		memmove(&target, block + sizeof(OverheadI), sizeof(int));
		return findFirst(fileDesc, target);
	} else {
		AM_errno = AM_UNKNOWNBLOCKTYPE;
		return AM_UNKNOWNBLOCKTYPE;
	}
}
