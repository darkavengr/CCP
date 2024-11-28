char *cdhelp[] = { "Displays the name of or changes the current directory.\n",\
		   "CHDIR [drive:][path]\n",\
		   "Type CD without parameters to display the current drive and directory.\n\n" };
char *copyhelp[] = { "Copies one or more files to another location.\n",\
		     "COPY [options] [source] [destination\n",\
		     "[source]       Specifies the file or files to be copied.\n",\
		     "[destination]  Specifies the directory and/or filename for the new file(s).",\
		     "/A             Indicates an ASCII text file.\n",\
		     "/B             Indicates a binary file.\n",\
		     "/V           Verifies that new files are written correctly.\n",\
		     "/Y           Suppresses prompting to confirm you want to overwrite an\n",\
	             "		   existing destination file.\n",\
		     "/-Y          Causes prompting to confirm you want to overwrite an",\
               	     "existing destination file.\n\n",\
		     "The switch /Y may be preset in the COPYCMD environment variable.",\
		     "This may be overridden with /-Y on the command line\n\n",\
		     "To append files, specify a single file for destination, but multiple files",\
		     "for source (using wildcards or file1+file2+file3 format)." };

char *sethelp[]= {   "Displays, sets, or removes environment variables.\n",\
		     "SET [variable=[string]]\n",\
		     "\nvariable  Specifies the environment-variable name.\n",\
		     "string    Specifies a series of characters to assign to the variable.\n\n",\
		     "If you include a '=', but no string, then the variable is removed\n",\
		     "from the environment.\n\n",\
		     "Type SET without parameters to display the current environment variables.\n" };

char *delhelp[] = {  "Deletes one or more files.\n\n",\
		     "DEL [drive:][path]filename [/P]\n\n",\
		     "[drive:][path]filename  Specifies the file(s) to delete.  Specify multiple",\
                     "files by using wildcards.",\
		     "/P Prompts for confirmation before deleting each file.\n\n" };

char *dirhelp[] = { "Displays a list of files and subdirectories in a directory.\n\n",\
		    "DIR [drive:][path][filename] [/P] [/W] [/A[[:]attributes]]\n",\
		    "	 [/O[[:]sortorder]] [/S] [/B] [/L] [/V]\n\n",\
		    "	 [drive:][path][filename] Specifies drive, directory, and/or files to list.\n",\
		    "	 /P         Pauses after each screenful of information.\n",\
 		    "	 /W         Uses wide list format.\n",\
 		    "	 /A         Displays files with specified attributes.\n",\
		    "		    attributes  D  Directories                R  Read-only files\n",\		
		    "		                H  Hidden files               A  Files ready for archiving\n",\
		    "		                S  System files               -  Prefix meaning not\n",\
 		    "	 /O         List by files in sorted order.\n",\
 		    "		    sortorder   N  By name (alphabetic)       S  By size (smallest first)\n",\
		    "				E  By extension (alphabetic)  D  By date & time (earliest first)\n",\
		                 G  Group directories first    -  Prefix to reverse order
             A  By Last Access Date (earliest first)
 /S         Displays files in specified directory and all subdirectories.
 /B         Uses bare format (no heading information or summary).
 /L         Uses lowercase.
 /V         Verbose mode.
 /Y         Display 4-digit year.

Switches may be preset in the DIRCMD environment variable.  Override
preset switches by prefixing any switch with - (hyphen)--for example, /-W.
