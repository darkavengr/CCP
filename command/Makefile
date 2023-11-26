ASM=nasm
CC=i386-elf-gcc
LD=i386-elf-ld
CCFLAGS=-c -w -ffreestanding -g -lgcc -Wl,-Map -Wl,../command.map
LDFLAGS=-ffreestanding -nostdlib -T ld.script.command  -Wl,-Map -Wl,../command.map
OBJFILES=addvar.o command.o copyfile.o err.o getfullpath.o kprintf.o readline.o runcommand.o string.o tohex.o tokenize.o call.o
OUTFILE=../bin/i386/COMMAND.RUN 

all: 	$(OBJFILES)
	$(CC) -o $(OUTFILE) `find -iname \*.o` $(LDFLAGS)

addvar.o:
	$(CC) $(CCFLAGS) addvar.c

command.o:
	$(CC) $(CCFLAGS) command.c

copyfile.o:
	$(CC) $(CCFLAGS) copyfile.c

err.o:
	$(CC) $(CCFLAGS) err.c

getfullpath.o:
	$(CC) $(CCFLAGS) getfullpath.c

kprintf.o:
	$(CC) $(CCFLAGS) kprintf.c

readline.o:
	$(CC) $(CCFLAGS) readline.c

runcommand.o:
	$(CC) $(CCFLAGS) runcommand.c

string.o:
	$(CC) $(CCFLAGS) string.c

tohex.o:
	$(CC) $(CCFLAGS) tohex.c

tokenize.o:
	$(CC) $(CCFLAGS) tokenize.c

call.o:
	$(ASM) -felf32 i386/call.asm -o call.o

