#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BF.h"
#include "heap.h"

static char* specialinform = "THIS IS A HEAP FILE!";		//eidikh plhroforia
static int records_per_block = 1024/sizeof(Record);			//megistos ari8mos eggrafwn ana block
static int record_counter = 0;								//metrhths eggrafwn ana block (xreiazetai sthn HP_InsertEntry)

int HP_CreateFile(char *fileName) {
	int bf, blkCnt;
	void* block;

	if(BF_CreateFile(fileName) < 0){				//dhmiourgia enos heap file
		BF_PrintError("Error creating file");
		return -1;
	}
	else
		printf("File Created!\n");
	if ((bf = BF_OpenFile(fileName)) < 0) {			//anoigma tou heap file kai allocation tou prwtou block 
		BF_PrintError("Error opening file");
		return -1;
	}
	if (BF_AllocateBlock(bf) < 0) {
		BF_PrintError("Error allocating block");
		return -1;
	}
	blkCnt = BF_GetBlockCounter(bf);
	if (BF_ReadBlock(bf, 0, &block) < 0) {			//diavasma kai sth sunexeia eggrafh ths eidikhs plhroforias sto prwto block 
		BF_PrintError("Error getting block");
		return -1;
	}
	memcpy(block,specialinform,strlen(specialinform)+1);
	if (BF_WriteBlock(bf, 0) < 0) {
		BF_PrintError("Error writing block back");
		return -1;
	}
	record_counter=0;

	return 0;
}

int HP_OpenFile(char *fileName) {
	int bf;
	void* block;
  	if ((bf = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Error opening file");
		return -1;
	}
	if (BF_ReadBlock(bf, 0, &block) < 0) {
		BF_PrintError("Error getting block");
		return -1;
	}
	if (memcmp(block, specialinform, strlen(specialinform)+1) == 0) {    //diavasma tou prwtou block tou heap file kai sth 
		printf("This is a Heap File with file number: %d\n" , bf);		 //sunexeia sugkrish tou periexomenou tou me th metablhth
		return bf;														 //special info.
	}
	else
	{
		printf("This is not a Heap File!\n");
		return -1;
	}
}

int HP_CloseFile(int fileDesc) {					//apla kleisimo tou heap file
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Error closing file");
		return -1;
	}
	return 0;
}

int HP_InsertEntry(int fileDesc, Record record) {
	Record* block;
	int blkCnt = BF_GetBlockCounter(fileDesc);
	
	if (blkCnt == 1 || record_counter == (records_per_block))		//ean uparxei mono 1 block sto heap file, to opoio profanws 8a einai to
	{																//block pou periexei thn eidikh plhroforia , H ean to teleutaio block tou		
		if (BF_AllocateBlock(fileDesc) < 0) {						//heap file einai plhres apo records tote na ginei allocate neou block 
			BF_PrintError("Error allocating block");				
			return -1;
		}
		record_counter = 0;											//mhdenismos tou record_counter ka8ws to block molis dhmiourgh8hke kai einai adeio
	}
	
	blkCnt = BF_GetBlockCounter(fileDesc);

	if (BF_ReadBlock(fileDesc, blkCnt-1, (void**) &block) < 0) {
		BF_PrintError("Error getting block");
		return -1;
	}

	block[record_counter] = record;									//antigrafh tou record ston deikth block

	//printf("new rec :%d %s %s %s\n", block[record_counter].id, block[record_counter].name, block[record_counter].surname, block[record_counter].city);

	if (BF_WriteBlock(fileDesc, blkCnt-1) < 0) {					//eggrafh tou eisax8entos record sto heap file
		BF_PrintError("Error writing block back");
		return -1;
	}
	record_counter++;												//au3hsh tou record_counter kata 1
	return 0;
}

void HP_GetAllEntries(int fileDesc, char* fieldName, void *value) {

  	Record* block;
  	int i, j;
  	int blkCnt = BF_GetBlockCounter(fileDesc);							//elegxos ean to fieldname einai iso me "id" H "name" H "surname" H "city" H "all"
  	if((strcmp(fieldName, "id") == 0) || (strcmp(fieldName, "name") == 0) || (strcmp(fieldName, "surname") == 0) || (strcmp(fieldName, "city") == 0) || (strcmp(fieldName, "all") == 0))
  	{
	  	for (i = 1; i < blkCnt; i++) 									//agnohsh prwtou block me thn eidikh plhroforia ara tote i=1
	  	{
			if (BF_ReadBlock(fileDesc, i, (void**) &block) < 0) {		//diabasma tou ka8e block
				BF_PrintError("Error getting block");
				exit(EXIT_FAILURE);
			}
			for (j = 0; j < records_per_block; j++)						//prospelash sta records tou ka8e block 3exwrista
			{															//epistrofh twn recs ta opoia anazhtoume se ka8e periptwsh
				if(strcmp(fieldName, "id") == 0)
				{
					if(block[j].id == atoi(value))
						printf("RECORD :%d %s %s %s\n", block[j].id, block[j].name, block[j].surname, block[j].city);
				}
				else if(strcmp(fieldName, "name") == 0)
				{
					if(strcmp(block[j].name, value) == 0)
						printf("RECORD :%d %s %s %s\n", block[j].id, block[j].name, block[j].surname, block[j].city);
				}
				else if(strcmp(fieldName, "surname") == 0)
				{
					if(strcmp(block[j].surname, value) == 0)
						printf("RECORD :%d %s %s %s\n", block[j].id, block[j].name, block[j].surname, block[j].city);
				}
				else if(strcmp(fieldName, "city") == 0)
				{
					if (strcmp(block[j].city, value) == 0)
						printf("RECORD :%d %s %s %s\n", block[j].id, block[j].name, block[j].surname, block[j].city);
				}
				else if(strcmp(fieldName, "all") == 0)
				{
					if(block[j].id != 0)
						printf("RECORD :%d %s %s %s\n", block[j].id, block[j].name, block[j].surname, block[j].city);
				}
				
			}
		}
		printf("Searched in %d blocks\n", (i-1));
	}
	else
	{
		printf("Wrong fieldName!\n");

	}
}
