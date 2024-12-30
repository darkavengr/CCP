FAT_DIRECTORY_FILENAME 			equ 0				; fat entry offsets
FAT_DIRECTORY_FILEATTR 			equ 11
FAT_DIRECTORY_BYTE_RESERVED		equ 12
FAT_DIRECTORY_CREATE_TIME_FINE_RES	equ 13 
FAT_DIRECTORY_CREATE_TIME 		equ 14
FAT_DIRECTORY_CREATE_DATE 		equ 16
FAT_DIRECTORY_LAST_ACCESS_DATE 		equ 18
FAT_DIRECTORY_BLOCK_HIGH_WORD 		equ 20
FAT_DIRECTORY_LAST_MODIFIED_TIME 	equ 22
FAT_DIRECTORY_LAST_MODIFIED_DATE 	equ 24
FAT_DIRECTORY_BLOCK_LOW_WORD		equ 26
FAT_DIRECTORY_FILE_SIZE			equ 28

FAT_DELETED	equ 0xE5
FAT_DIRECTORY_ENTRY_SIZE 		equ 32
MAX_CLUSTER_SIZE 	equ 32767			; maxiumum cluster size

BPB_JUMP 			equ 0					; bpb offsets											
BPB_FSMARKER 		equ 3
BPB_SECTORSIZE		equ 11
BPB_SECTORSPERBLOCK	equ 13
BPB_RESERVEDSECTORS	equ 14
BPB_NUMBEROFFATS	equ 16
BPB_ROOTDIRENTRIES	equ 17
BPB_NUMBEROFSECTORS	equ 19
BPB_MEDIADESCRIPTOR	equ 21
BPB_SECTORSPERFAT	equ 22
BPB_SECTORSPERTRACK	equ 24
BPB_NUMBEROFHEADS	equ 26
BPB_HIDDENSECTORS	equ 28
BPB_SECTORSDWORD	equ 32
BPB_PHYSICALDRIVE	equ 36
BPB_RESERVED		equ 37
BPB_EXBOOTSIG		equ 38
BPB_SERIALNO		equ 39
BPB_VOLUMELABEL		equ 43
BPB_FATNAME		equ 54

BPB_FAT32_SECTORS_PER_FAT	equ 36
BPB_FAT32_FLAGS		equ 30
BPB_FAT32_VERSION	equ 42
BPB_FAT32_ROOTDIR	equ 44
BPB_FAT32_FSINFO	equ 48
BPB_FAT32_BACKUP_BOOT	equ 50
BPB_FAT32_RESERVED	equ 52
BPB_FAT32_DRIVE		equ 64
BPB_FAT32_NT_FLAGS	equ 65
BPB_FAT32_SIG		equ 66
BPB_FAT32_VOLID		equ 67
BPB_FAT32_LABEL		equ 71
BPB_FAT32_ID		equ 82

