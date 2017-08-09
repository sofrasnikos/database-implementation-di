#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BF.h"
#include "column_store.h"

int main(void) {
	CS_Init();
	int i;
	HeaderInfo arrayFDS[6];
	for (i = 0; i < 6; i++) {
		arrayFDS[i].column_fds = malloc(sizeof(ColumnFds));
		arrayFDS[i].column_fds->columnName = malloc(sizeof(char));
		arrayFDS[i].column_fds->fd = -1;
	}
	char *fieldNames[6] = { "id", "name", "surname", "status", "dateOfBirth",
			"salary" };
	char dbDirectory[10] = "./db/";

	if (CS_CreateFiles(fieldNames, dbDirectory) < 0) {
		printf("Error in create\n");
		return -1;
	}
	if (CS_OpenFile(arrayFDS, dbDirectory) < 0) {
		printf("Error in open\n");
		return -1;
	}
	insertFromTXT("records.dat", arrayFDS);
	char* arguments2[3] = { "dateOfBirth", "name", "id" };
	char* arguments3[3] = { "name", "surname", "salary" };
	char* arguments4[1] = { "surname" };
	char* arguments5[6] = { "id", "name", "surname", "status", "dateOfBirth",
			"salary" };
	char* arguments6[4] = { "name", "surname", "salary", "dateOfBirth" };

	printf("Printing all records\n");
	CS_GetAllEntries(arrayFDS, NULL, 23, arguments2, 0);
	int value = 2000;
	printf("Printing all records with salary = 2000\n");
	CS_GetAllEntries(arrayFDS, "salary", &value, arguments2, 3);

	printf("Printing all records with surname = \"Dimopoulos\"\n");
	CS_GetAllEntries(arrayFDS, "surname", "Dimopoulos", arguments3, 3);

	printf("Printing all records with dateOfBirth = \"15-06-1990\"\n");
	CS_GetAllEntries(arrayFDS, "dateOfBirth", "15-06-1990", arguments3, 3);

	printf("Printing all records with dateOfBirth = \"21-12-1977\"\n");
	CS_GetAllEntries(arrayFDS, "dateOfBirth", "21-12-1977", arguments6, 4);

	printf("Printing all records with dateOfBirth = \"15-06-1990\"\n");
	CS_GetAllEntries(arrayFDS, "dateOfBirth", "15-06-1990", arguments5, 6);

	printf("Printing all records with status = \"S\"\n");
	CS_GetAllEntries(arrayFDS, "status", "S", arguments4, 1);

	// This shouldn't print any records
	printf("Printing all records with surname = \"Iwannidis\"\n");
	CS_GetAllEntries(arrayFDS, "surname", "Iwannidis", arguments6, 4);

	// This should print "Unrecognized field name"
	printf("Printing all records with ------------------ = \"Dimopoulos\"\n");
	CS_GetAllEntries(arrayFDS, "------------------", "Dimopoulos", arguments6,
			4);

	if (CS_CloseFile(arrayFDS) < 0) {
		printf("Error in close\n");
		return -1;
	}
	for (i = 0; i < 6; i++) {
		free(arrayFDS[i].column_fds->columnName);
		free(arrayFDS[i].column_fds);
	}
	return 0;
}
