#define DEBUG_PRINT_HEX(s) kprintf_direct(#s"=%08X\n",s);
#define DEBUG_PRINT_HEX_LONG(s) kprintf_direct(#s"=%016lX\n",s);
#define DEBUG_PRINT_DEC(s) kprintf_direct(#s"=%d\n",s);
#define DEBUG_PRINT_STRING(s) kprintf_direct(#s"=%s\n",s);

#define MAGIC_BREAKPOINT(a,b) \
if(a == b) { \
 kprintf_direct("MAGIC BREAKPOINT FOR CONDITION " #a " == " #b "\n",a,b); \
 asm("xchg %bx,%bx"); \
}


