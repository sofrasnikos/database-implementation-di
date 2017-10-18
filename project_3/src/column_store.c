#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BF.h"
#include "column_store.h"

// These includes are needed to create a directory
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void CS_Init() {
	BF_Init();
}

int CS_CreateFiles(char **fieldNames, char *dbDirectory) {
	char str[50];
	// Create the directory if it does not exist and create the Header_Info file
	if (createHeaderInfo(dbDirectory, fieldNames) < 0) {
		return -1;
	}
	// Initialize special information
	SpecialInfo si = { "This is a column-store file", sizeof(int), 0, 0, 0 };
	// Create CSFiles
	sprintf(str, "%sCSFile_%s", dbDirectory, fieldNames[0]);
	if (createFile(str, si) < 0) {
		return -1;
	}
	si.sizeOfField = sizeof(char) * 15;
	sprintf(str, "%sCSFile_%s", dbDirectory, fieldNames[1]);
	if (createFile(str, si) < 0) {
		return -1;
	}
	si.sizeOfField = sizeof(char) * 20;
	sprintf(str, "%sCSFile_%s", dbDirectory, fieldNames[2]);
	if (createFile(str, si) < 0) {
		return -1;
	}
	si.sizeOfField = sizeof(char) * 10;
	sprintf(str, "%sCSFile_%s", dbDirectory, fieldNames[3]);
	if (createFile(str, si) < 0) {
		return -1;
	}
	si.sizeOfField = sizeof(char) * 11;
	sprintf(str, "%sCSFile_%s", dbDirectory, fieldNames[4]);
	if (createFile(str, si) < 0) {
		return -1;
	}
	si.sizeOfField = sizeof(int);
	sprintf(str, "%sCSFile_%s", dbDirectory, fieldNames[5]);
	if (createFile(str, si) < 0) {
		return -1;
	}
	return 0;
}

int CS_OpenFile(HeaderInfo* header_info, char *dbDirectory) {
	FILE * hFile;
	char pathToHeader_Info[50];
	char pathToFiles[50];
	char fileName[50];
	// Go to the directory and read the names of the CSFiles from Header_Info
	strcpy(pathToHeader_Info, dbDirectory);
	strcat(pathToHeader_Info, "Header_Info");
	hFile = fopen(pathToHeader_Info, "r");
	if (hFile != NULL) {
		int i;
		for (i = 0; i < 6; i++) {
			fscanf(hFile, "%s", fileName);
			strcpy(pathToFiles, dbDirectory);
			strcat(pathToFiles, fileName);
			int fd = openFile(pathToFiles);
			if (fd < 0) {
				return -1;
			}
			// Read next CSFile name
			strcpy(header_info[i].column_fds->columnName, fileName + 7);
			header_info[i].column_fds->fd = fd;
		}
		fclose(hFile);
	}
	return 0;
}

int CS_CloseFile(HeaderInfo* header_info) {
	int i;
	for (i = 0; i < 6; i++) {
		if (header_info[i].column_fds->fd != -1) {
			if (BF_CloseFile(header_info[i].column_fds->fd) < 0) {
				BF_PrintError("Error closing file");
				return -1;
			}
			header_info[i].column_fds->fd = -1;
		} else {
			printf("File already closed!\n");
		}
	}
	return 0;
}

int CS_InsertEntry(HeaderInfo* header_info, Record record) {
	void* block;
	SpecialInfo si;
	int fd, i;
	for (i = 0; i < 6; i++) {
		fd = header_info[i].column_fds->fd;
		// Read first block
		if (BF_ReadBlock(fd, 0, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			return -1;
		}
		int blkCnt = BF_GetBlockCounter(fd);
		memcpy(&si, block, sizeof(SpecialInfo));
		// Check if we need to allocate a new one
		if (BLOCK_SIZE - si.sizeInBytesInLastBlock < si.sizeOfField
				|| blkCnt - 1 == 0) {
			// Allocate new block
			if (BF_AllocateBlock(fd) < 0) {
				BF_PrintError("Error allocating block");
				return -1;
			}
			si.sizeInBytesInLastBlock = 0;
			si.numberOfRecordsInLastBlock = 0;
		}
		blkCnt = BF_GetBlockCounter(fd);
		// Read last block
		if (BF_ReadBlock(fd, blkCnt - 1, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			return -1;
		}
		// i == field of record (0 for 'id', 1 for 'name' etc..)
		switch (i) {
		case 0:
			memcpy(block + si.sizeInBytesInLastBlock, &(record.id),
					si.sizeOfField);
			break;
		case 1:
			memcpy(block + si.sizeInBytesInLastBlock, &(record.name),
					si.sizeOfField);
			break;
		case 2:
			memcpy(block + si.sizeInBytesInLastBlock, &(record.surname),
					si.sizeOfField);
			break;
		case 3:
			memcpy(block + si.sizeInBytesInLastBlock, &(record.status),
					si.sizeOfField);
			break;
		case 4:
			memcpy(block + si.sizeInBytesInLastBlock, &(record.dateOfBirth),
					si.sizeOfField);
			break;
		case 5:
			memcpy(block + si.sizeInBytesInLastBlock, &(record.salary),
					si.sizeOfField);
			break;
		}
		// Write last block
		if (BF_WriteBlock(fd, blkCnt - 1) < 0) {
			BF_PrintError("Error writing block back");
			return -1;
		}
		// Read first block to update special information
		if (BF_ReadBlock(fd, 0, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			return -1;
		}
		si.sizeInBytesInLastBlock += si.sizeOfField;
		si.numberOfRecordsInLastBlock++;
		si.numberOfRecords++;
		memcpy(block, &si, sizeof(SpecialInfo));
		// Write the updated first block
		if (BF_WriteBlock(fd, 0) < 0) {
			BF_PrintError("Error writing block back");
			return -1;
		}
	}
	return 0;
}

void CS_GetAllEntries(HeaderInfo* header_info, char *fieldName, void *value,
		char **fieldNames, int n) {
	void* block;
	int fd;
	SpecialInfo si;
	int i;
	int blockCounter = 0;
	int found = 0;
	int atLeastOneRecFound = 0;
	int offset = 0;
	int recordsRead = 0;
	int blockNumber = 0;
	int lastFetchedBlock[6] = { -1, -1, -1, -1, -1, -1 };
	int flags[6];
	// Set flags, one flag for each field with value 1 if we should print it
	setFlags(header_info, value, fieldNames, n, flags);
	int sizes[6];
	// Set field sizes
	for (i = 0; i < 6; i++) {
		fd = header_info[i].column_fds->fd;
		if (BF_ReadBlock(fd, 0, (void**) &block) < 0) {
			BF_PrintError("Error getting block");
			return;
		}
		memcpy(&si, block, sizeof(SpecialInfo));
		sizes[i] = si.sizeOfField;
	}
	// Find the correct field descriptor for given fieldName
	if (fieldName != NULL) {
		for (i = 0; i < 6; i++) {
			if (strcmp(fieldName, header_info[i].column_fds->columnName) == 0) {
				fd = header_info[i].column_fds->fd;
				break;
			}
		}
		if (i == 6) {
			printf("Unrecognized field name \n");
			return;
		}
	}
	// fd now has the correct value
	// Now, read the first block of this file
	if (BF_ReadBlock(fd, 0, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		return;
	}
	memcpy(&si, block, sizeof(SpecialInfo));
	void* iterator = malloc(si.sizeOfField);
	if (iterator == NULL) {
		printf("Cannot allocate memory\n");
		return;
	}
	int howManyRecordsCanFit = BLOCK_SIZE / si.sizeOfField;
	for (i = 0; i < si.numberOfRecords; i++) {
		// If we need to change block or this is the first block
		if (howManyRecordsCanFit <= recordsRead || i == 0) {
			// Read next block
			blockNumber++;
			blockCounter++;
			if (BF_ReadBlock(fd, blockNumber, (void**) &block) < 0) {
				BF_PrintError("Error getting block");
				return;
			}
			recordsRead = 0;
			offset = 0;
		}
		memcpy(iterator, block + offset, si.sizeOfField);
		if (value == NULL || fieldName == NULL) {
			// Print all records
			atLeastOneRecFound = 1;
			int j;
			printf("  ");
			for (j = 0; j < 6; j++) {
				if (j == 0 || j == 5) {
					printField(header_info[j].column_fds->fd,
							(i / (BLOCK_SIZE / sizes[j])) + 1,
							i % (BLOCK_SIZE / sizes[j]), sizes[j], 'i');
				} else {
					printField(header_info[j].column_fds->fd,
							(i / (BLOCK_SIZE / sizes[j])) + 1,
							i % (BLOCK_SIZE / sizes[j]), sizes[j], 'c');
				}
			}
			printf("\n");
		} else {
			found = 0;
			// if id or salary
			if (strcmp(fieldName, header_info[0].column_fds->columnName) == 0
					|| strcmp(fieldName, header_info[5].column_fds->columnName)
							== 0) {
				if (*(int*) value == *(int*) iterator) {
					found = 1;
				}
			}
			// if name or surname or status or dateOfBirth
			else {
				if (strcmp((char*) value, (char*) iterator) == 0) {
					found = 1;
				}
			}
			if (found) {
				atLeastOneRecFound = 1;
				printf("  ");
				int j;
				for (j = 0; j < 6; j++) {
					// if id or salary
					if (j == 0 || j == 5) {
						// if we should print this field
						if (flags[j]) {
							// if this is the first time we read this block
							if (lastFetchedBlock[j]
									!= (i / (BLOCK_SIZE / sizes[j])) + 1) {
								blockCounter++;
							}
							// Print this field
							printField(header_info[j].column_fds->fd,
									(i / (BLOCK_SIZE / sizes[j])) + 1,
									i % (BLOCK_SIZE / sizes[j]), sizes[j], 'i');
							lastFetchedBlock[j] = (i / (BLOCK_SIZE / sizes[j]))
									+ 1;
						}
					}
					// if name or surname or status or dateOfBirth
					else {
						// if we should print this field
						if (flags[j]) {
							// if this is the first time we read this block
							if (lastFetchedBlock[j]
									!= (i / (BLOCK_SIZE / sizes[j])) + 1) {
								blockCounter++;
							}
							// Print this field
							printField(header_info[j].column_fds->fd,
									(i / (BLOCK_SIZE / sizes[j])) + 1,
									i % (BLOCK_SIZE / sizes[j]), sizes[j], 'c');
							lastFetchedBlock[j] = (i / (BLOCK_SIZE / sizes[j]))
									+ 1;
						}
					}
				}
				printf("\n");
				// Restore the block
				if (BF_ReadBlock(fd, blockNumber, (void**) &block) < 0) {
					BF_PrintError("Error getting block");
					return;
				}
			}
		}
		offset += si.sizeOfField;
		recordsRead++;
	}
	if (atLeastOneRecFound == 0) {
		printf("No records found!\n");
	}
	free(iterator);
	printf("Number of fetched blocks: %d\n", blockCounter);
}

int createHeaderInfo(char *dbDirectory, char** fieldNames) {
	FILE * hFile;
	char str[50] = "\0";
	strcpy(str, dbDirectory);
	strcat(str, "Header_Info");
	struct stat st = { 0 };
	// Check if directory exists
	if (stat(dbDirectory, &st) == -1) {
		// All users can read, write and execute
		int i = mkdir(dbDirectory, 0777);
		if (i < 0) {
			printf("Cannot create directory\n");
			return -1;
		}
	}
	// Write the names of the CSFiles in the Header_Info
	hFile = fopen(str, "w");
	if (hFile != NULL) {
		int i;
		for (i = 0; i < 6; i++) {
			fprintf(hFile, "CSFile_%s ", fieldNames[i]);
		}
		fclose(hFile);
	} else {
		printf("Cannot create Header_Info\n");
		return -1;
	}
	return 0;
}

int createFile(char* fileName, SpecialInfo si) {
	int fileDesc;
	SpecialInfo* block;
	int blkCnt;
	// Create a file
	if (BF_CreateFile(fileName) < 0) {
		BF_PrintError("Error creating file");
		return -1;
	}
	// Open it
	if ((fileDesc = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Error opening file");
		return -1;
	}
	// Allocate a block to store information about the file
	if (BF_AllocateBlock(fileDesc) < 0) {
		BF_PrintError("Error allocating block");
		return -1;
	}
	blkCnt = BF_GetBlockCounter(fileDesc);
	if (BF_ReadBlock(fileDesc, blkCnt - 1, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		return -1;
	}
	// Copy Identifier there
	block[0] = si;
	if (BF_WriteBlock(fileDesc, 0) < 0) {
		BF_PrintError("Error writing block back");
		return -1;
	}
	// Close the file
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Error closing file");
		return -1;
	}
	return 0;
}

int openFile(char* fileName) {
	int fileDesc;
	SpecialInfo* block;
	SpecialInfo si;
	// Open file
	if ((fileDesc = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Error opening file");
		return -1;
	}
	// Read its first block
	if (BF_ReadBlock(fileDesc, 0, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		return -1;
	}
	// Read special information
	memmove(&si, block, sizeof(SpecialInfo));
	// Check if it is a column-store file
	if (strcmp(si.identifier, "This is a column-store file") != 0) {
		printf("This is not a column-store file of this application\n");
		return -1;
	}
	return fileDesc;
}

int insertFromTXT(char* inputFile, HeaderInfo* header_info) {
	FILE* iFile;
	Record record;
	char line[200];
	char* p;
	iFile = fopen(inputFile, "r");
	if (iFile == NULL) {
		puts("Cannot open input file");
		return -1;
	}
	while (fgets(line, 200, iFile) != NULL) {
		// Cut string to words
		p = strtok(line, ",\".");
		record.id = atoi(p);
		p = strtok(NULL, ",\".");
		strcpy(record.name, p);
		p = strtok(NULL, ",\".");
		strcpy(record.surname, p);
		p = strtok(NULL, ",\".");
		strcpy(record.status, p);
		p = strtok(NULL, ",\".");
		strcpy(record.dateOfBirth, p);
		p = strtok(NULL, ",\".");
		record.salary = atoi(p);
		if (CS_InsertEntry(header_info, record) < 0) {
			printf("Error in open\n");
			return -1;
		}
	}
	return 0;
}

int setFlags(HeaderInfo* header_info, void *value, char ** fieldNames, int n,
		int* flags) {
	int i;
	for (i = 0; i < 6; i++) {
		flags[i] = 0;
	}
	if (value == NULL) {
		for (i = 0; i < 6; i++) {
			flags[i] = 1;
		}
		return 1;
	} else {
		for (i = 0; i < n; i++) {
			// fieldNames == "id"
			if (strcmp(fieldNames[i], header_info[0].column_fds->columnName)
					== 0) {
				flags[0] = 1;

			}
			// fieldNames == "name"
			if (strcmp(fieldNames[i], header_info[1].column_fds->columnName)
					== 0) {
				flags[1] = 1;
			}
			// fieldNames == "surname"
			if (strcmp(fieldNames[i], header_info[2].column_fds->columnName)
					== 0) {
				flags[2] = 1;
			}
			// fieldNames == "status"
			if (strcmp(fieldNames[i], header_info[3].column_fds->columnName)
					== 0) {
				flags[3] = 1;
			}
			// fieldNames == "dateOfBirth"
			if (strcmp(fieldNames[i], header_info[4].column_fds->columnName)
					== 0) {
				flags[4] = 1;
			}
			// fieldNames == "salary"
			if (strcmp(fieldNames[i], header_info[5].column_fds->columnName)
					== 0) {
				flags[5] = 1;
			}

		}
		return 0;
	}
}

void printField(int fileDesc, int targetBlock, int targetRecord, int size,
		char typeOfField) {
	void* block;
	void* value;
	if (BF_ReadBlock(fileDesc, targetBlock, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		return;
	}
	value = block + targetRecord * size;
	if (typeOfField == 'i') {
		printf("%d ", *(int*) value);
	}
	if (typeOfField == 'c') {
		printf("%s ", (char*) value);
	}
}
