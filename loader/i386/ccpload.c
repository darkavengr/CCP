#include "bootinfo.h"
#include "loader.h"
#include "vfs.h"

int loadermain() {
BOOT_INFO *bootinfo=BOOT_INFO_ADDRESS;
FILERECORD findrecord;
void *currentend;

/* create boot information */

if(bootinfo->physicaldrive) >= 0x80) {		/* get logical drive from physical drive */
	bootinfo->drive=(bootinfo->physicaldrive-0x80);
}
else
{
	bootinfo->drive=bootinfo->physicaldrive;
}

bootinfo->cursor_row=get_cursor_row();		/* get cursor row */
bootinfo->cursor_col=get_cursor_col();		/* get cursor column */
bootinfo->kernel_start=LOAD_ADDRESS;		/* kernel start address */

if(findfile(ccpname,&findrecord) == -1) {	/* find kernel */
	outputstring(missing_ccp);		/* display error message */
	outputstring(press_any_key);
	
	getkey();				/* wait for keypress */
	reboot();				/* reboot */
}

bootinfo->kernel_size=findrecord.filesize);	/* kernel size */

outputstring(loading_ccp);			/* display loading message */

if(load_file(ccpname,bootinfo->kernel_start) == -1) {	/* load kernel */
loadfile(findrecord.filename,bootinfo->kernel_start);	/*
