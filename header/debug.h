#define DEBUG_PRINT_HEX(s) kprintf_direct(#s"=%X\n",s);
#define DEBUG_PRINT_DEC(s) kprintf_direct(#s"=%d\n",s);
#define DEBUG_PRINT_STRING(s) kprintf_direct(#s"=%s\n",s);

#define MAGIC_BREAKPOINT(a,b) \
if(a == b) { \
 kprintf_direct("MAGIC BREAKPOINT FOR CONDITION %X == %X\n",a,b); \
 asm("xchg %bx,%bx"); \
}


