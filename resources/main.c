extern int printf(const char *fmt, ...);
extern void InitializePrint(void *fb, void *font);

void __main() {
    InitializePrint(*(void **) 0x1000000, *(void **) 0x1000008);
    printf("              PE + DLL test\n");
}