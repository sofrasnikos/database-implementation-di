#ifndef AM_H_
#define AM_H_

/* Error codes */

extern int AM_errno;

#define AME_OK 0
#define AME_EOF -1

#define AM_MAXFILESREACHED -100
#define AM_FILEOPEN -101
#define AM_RMVFAIL -102
#define AM_FILENOTFOUND -103
#define AM_ERRORINCREATEFILE -104
#define AM_ERRORINOPENFILE -105
#define AM_ERRORINALLOCATEBLOCK -106
#define AM_ERRORINREADBLOCK -107
#define AM_ERRORINWRITEBLOCK -108
#define AM_ERRORINCLOSEFILE -109
#define AM_NOTBPLUSTREE -110
#define AM_SCANISOPEN -111
#define AM_MAXSCANSREACHED -112
#define AM_MALLOCERROR -113
#define AM_UNKNOWNOP -114
#define AM_UNKNOWNTYPE -115
#define AM_UNKNOWNBLOCKTYPE -116

#define EQUAL 1
#define NOT_EQUAL 2
#define LESS_THAN 3
#define GREATER_THAN 4
#define LESS_THAN_OR_EQUAL 5
#define GREATER_THAN_OR_EQUAL 6

void AM_Init(void);

int AM_CreateIndex(char *fileName, /* όνομα αρχείου */
char attrType1, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
int attrLength1, /* μήκος πρώτου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
char attrType2, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
int attrLength2 /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
);

int AM_DestroyIndex(char *fileName /* όνομα αρχείου */
);

int AM_OpenIndex(char *fileName /* όνομα αρχείου */
);

int AM_CloseIndex(int fileDesc /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
);

int AM_InsertEntry(int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
void *value1, /* τιμή του πεδίου-κλειδιού προς εισαγωγή */
void *value2 /* τιμή του δεύτερου πεδίου της εγγραφής προς εισαγωγή */
);

int AM_OpenIndexScan(int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
int op, /* τελεστής σύγκρισης */
void *value /* τιμή του πεδίου-κλειδιού προς σύγκριση */
);

void *AM_FindNextEntry(int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);

int AM_CloseIndexScan(int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);

void AM_PrintError(char *errString /* κείμενο για εκτύπωση */
);

int fileIsOpen(char *fileName);
/*
 * Return value: 0 when no files are open with this name
 *               1 otherwise
 */

int scanIsOpen(int fileDesc);
/*
 * Return value: 0 when no scans are open with this file descriptor
 *               1 otherwise
 */

int insertInLeafBlock(int fileDesc, int blockNumber, void *value1, void *value2,
		int size1, int size2, char type1);

int insertInIndexBlock(int fileDesc, int blockNumber, void *value1, int size1,
		int pointer1, int pointer2, char type1);

int splitLeafBlock(int fileDesc, int blockNumber, int size1, int size2, char type1);

int modifyPrevInNextBlock(int fileDesc, int blockNumber, int previousBlockNumber);
/*
 * This function is called when a block splits and assigns prev pointer
 * to its brother block
 */

int splitIndexBlock(int fileDesc, int blockNumber, int size1, char type1);

void setAllParents(int fileDesc, int blockNumber, int size1);
/*
 * This function is called when a block splits and assigns all pointers
 * to parent block of the newly allocated block to its new parents
 */

void iAmYourFather(int fileDesc, int blockNumber, int father);
/*
 * This function is called by the parent so that the child modifies its parent pointer
 */

int findBlock(int fileDesc, int currentBlock, void *value, int size1,
		char type1);
/*
 * This function searches for the block that the parameter value should be in
 */

int findFirst(int fileDesc, int currentBlock);
/*
 * This function returns the block number that has the minimum value
 */

#endif /* AM_H_ */
