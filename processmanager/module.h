#define DO_386_32(S, A)	((S) + (A))
#define DO_386_PC32(S, A, P)	((S) + (A) - (P))

typedef struct {
	char *name[MAX_PATH];
	size_t address;
	struct SYMBOL_TABLE_ENTRY *next;
} SYMBOL_TABLE_ENTRY;

