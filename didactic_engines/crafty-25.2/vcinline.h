#if _MSC_VER >= 1200
#  define FORCEINLINE __forceinline
#else
#  define FORCEINLINE __inline
#endif
FORCEINLINE int PopCnt(uint64_t a) {
/* Because Crafty bitboards are typically sparsely populated, we use a
   streamlined version of the boolean.c algorithm instead of the one in x86.s */
  __asm {
    mov ecx, dword ptr a xor eax, eax test ecx, ecx jz l1 l0:lea edx,[ecx - 1]
    inc eax and ecx, edx jnz l0 l1:mov ecx, dword ptr a + 4 test ecx,
        ecx jz l3 l2:lea edx,[ecx - 1]
inc eax and ecx, edx jnz l2 l3:}} FORCEINLINE int MSB(uint64_t a) {
  __asm {
bsr edx, dword ptr a + 4 mov eax, 31 jnz l1 bsr edx, dword ptr a mov eax,
        63 jnz l1 mov edx, -1 l1:sub eax,
        edx}} FORCEINLINE int LSB(uint64_t a) {
  __asm {
bsf edx, dword ptr a mov eax, 63 jnz l1 bsf edx, dword ptr a + 4 mov eax,
        31 jnz l1 mov edx, -33 l1:sub eax, edx}}
