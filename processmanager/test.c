#include <stdio.h>
#include <stdint.h>

#define MAX_PATH 255
#include "process.h"

int main()
{
PROCESS test;

printf("regs 			%d\n",(unsigned int) &test.regs-(unsigned int) &test.regs);
printf("pid 			%d\n",(unsigned int) &test.pid-(unsigned int) &test.regs);
printf("filename 		%d\n",(unsigned int) &test.filename-(unsigned int) &test.regs);
printf("args 			%d\n",(unsigned int) &test.args-(unsigned int) &test.regs);
printf("ppid 			%d\n",(unsigned int) &test.parentprocess-(unsigned int) &test.regs);
printf("errorhandler 		%d\n",(unsigned int) &test.errorhandler-(unsigned int) &test.regs);
printf("signalhandler 		%d\n",(unsigned int) &test.signalhandler-(unsigned int) &test.regs);
printf("currentdir 		%d\n",(unsigned int) &test.currentdir-(unsigned int) &test.regs);
printf("rc 			%d\n",(unsigned int) &test.rc-(unsigned int) &test.regs);
printf("blockbuf 		%d\n",(unsigned int) &test.blockbuf-(unsigned int) &test.regs);
printf("flags 			%d\n",(unsigned int) &test.flags-(unsigned int) &test.regs);
printf("writeconsolehandle	%d\n",(unsigned int) &test.writeconsolehandle-(unsigned int) &test.regs);
printf("readconsolehandle	%d\n",(unsigned int) &test.readconsolehandle-(unsigned int) &test.regs);
printf("lasterror		%d\n",(unsigned int) &test.lasterror-(unsigned int) &test.regs);
printf("ticks			%d\n",(unsigned int) &test.ticks-(unsigned int) &test.regs);
printf("maxticks		%d\n",(unsigned int) &test.maxticks-(unsigned int) &test.regs);
printf("findptr			%d\n",(unsigned int) &test.findptr-(unsigned int) &test.regs);
printf("stackpointer		%d\n",(unsigned int) &test.stackpointer-(unsigned int) &test.regs);
printf("stackbase		%d\n",(unsigned int) &test.stackbase-(unsigned int) &test.regs);
printf("kernelstackbase		%d\n",(unsigned int) &test.kernelstackbase-(unsigned int) &test.regs);
printf("stackpointer		%d\n",(unsigned int) &test.kernelstackpointer-(unsigned int) &test.regs);
printf("next			%d\n",(unsigned int) &test.next-(unsigned int) &test.regs);
}

