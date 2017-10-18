#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"

#define FILENAME "TEST_HEAP_FILE"

int main() {
	
	//einai testing main apla ektelei vasikes leitourgies etsi wste na ginei debugging 

	int i;

	FILE *fp;
	fp = fopen("100.csv", "r");
	char buff[100];
	const char s1[2] = ",\"";
   	char *token1;

   	Record rec;

   	char value[25];

  	BF_Init();

	if (HP_CreateFile(FILENAME) < 0)	
		printf("Error creating file\n"); 
	else
	{
		int fileDesc;
		fileDesc=HP_OpenFile(FILENAME);
		if (fileDesc == -1)
			printf("Error!\n");
		else
		{
			while(fgets (buff, 100, fp) != NULL)
 			{
   				int tokencounter = 0;
       			puts(buff);
       			sscanf(buff, "%d", &rec.id);
			   	token1 = strtok(buff, s1);
  			   	while( token1 != NULL ) 
   				{
      				
      				token1 = strtok(NULL, s1);
      				if (tokencounter == 0)
      				{
      					strcpy(rec.name, token1);
       				}
      				else if(tokencounter == 1)
      				{
      					strcpy(rec.surname, token1);
      				}
      				else if(tokencounter == 2)
      				{
      					strcpy(rec.city, token1);
      				}
      				tokencounter++;
   				}
   				if (HP_InsertEntry(fileDesc, rec) < 0)
   					printf("Error inserting record\n"); 
   			}
   			strcpy(value, "642014");
   			HP_GetAllEntries(fileDesc, "id", &value);

   			strcpy(value, "13758295");
   			HP_GetAllEntries(fileDesc, "id", &value);

   			strcpy(value, "Louanne");
   			HP_GetAllEntries(fileDesc, "name", &value);

   			strcpy(value, "Cordie");
   			HP_GetAllEntries(fileDesc, "name", &value);

   			strcpy(value, "Fredericksen");
   			HP_GetAllEntries(fileDesc, "surname", &value);

   			strcpy(value, "Stergios");
   			HP_GetAllEntries(fileDesc, "surname", &value);

   			strcpy(value, "Ioannina");
   			HP_GetAllEntries(fileDesc, "city", &value);

   			strcpy(value, "Egaleo");
   			HP_GetAllEntries(fileDesc, "city", &value);

   			HP_GetAllEntries(fileDesc, "all", &value);

			if (HP_CloseFile(fileDesc) < 0)
				printf("Error closing the file\n"); 
			else
				printf("File Closed!\n"); 	
		}
	}

  	return 0;
}