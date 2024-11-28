#include "../defs.h"
#include "command.h"

char *directories[26][MAX_PATH];

unsigned long lc; 
unsigned long errorlevel;
char *filename[MAX_PATH];
VARIABLES *vars;

char *errs[]= { "No error",\
              "Invalid function",\
              "File not found\n",\
	      "Path not found",\
              "Out of handles",\
              "Access denied",\
	        "Invalid handle","","Out of memory","","","Invalid drive specification","Acess error","Bad file","","Invalid drive",\
		"Directory contains files","Can't rename across drives","End of directory","Write protect error","Invalid device",\
	        "Drive not ready","Bad CRC","File already exists","Directory is full","Input past end of file","Device I/O error",\
		"Bad file","Invalid program","Device exists","Invalid process","Invalid device","Device in use","Invalid drive",\
		"Driver already loaded","Process not found","End of file","No drives","Seek past end of file","Can't close devices",\
		"Invalid block","File already open" };
