/* *INDENT-OFF* */
#if (CPUS > 1)
#  if !defined(UNIX)
/*
 *******************************************************************************
 *                                                                             *
 *  this is a Microsoft windows-based operating system.                        *
 *                                                                             *
 *******************************************************************************
 */
#    include <windows.h>
#    define pthread_attr_t  HANDLE
#    define pthread_t       HANDLE
#    define thread_t        HANDLE
extern pthread_t NumaStartThread(void *func, void *args);

#    include <windows.h>
typedef volatile LONG lock_t[1];

#    define LockInit(v)      ((v)[0] = 0)
#    define LockFree(v)      ((v)[0] = 0)
#    define Unlock(v)        ((v)[0] = 0)
__forceinline void Lock(volatile LONG * hPtr) {
  int iValue;

  for (;;) {
    iValue = _InterlockedExchange((LPLONG) hPtr, 1);
    if (0 == iValue)
      return;
    while (*hPtr);
  }
}
void Pause() {
}
#  else
/*
 *******************************************************************************
 *                                                                             *
 *  this is a Unix-based operating system.  define the spinlock code as needed *
 *  for SMP synchronization.                                                   *
 *                                                                             *
 *******************************************************************************
 */
static void __inline__ LockX86(volatile int *lock) {
  int dummy;
  asm __volatile__(
      "1:          movl    $1, %0"          "\n\t"
      "            xchgl   (%1), %0"        "\n\t"
      "            testl   %0, %0"          "\n\t"
      "            jz      3f"              "\n\t"
      "2:          pause"                   "\n\t"
      "            movl    (%1), %0"        "\n\t"
      "            testl   %0, %0"          "\n\t"
      "            jnz     2b"              "\n\t"
      "            jmp     1b"              "\n\t"
      "3:"                                  "\n\t"
      :"=&q"(dummy)
      :"q"(lock)
      :"cc", "memory");
}
static void __inline__ Pause() {
  asm __volatile__(
      "            pause"                   "\n\t");
}
static void __inline__ UnlockX86(volatile int *lock) {
  asm __volatile__(
      "movl        $0, (%0)"
      :
      :"q"(lock)
      :"memory");
}

#    define LockInit(p)           (p=0)
#    define LockFree(p)           (p=0)
#    define Unlock(p)             (UnlockX86(&p))
#    define Lock(p)               (LockX86(&p))
#    define lock_t                volatile int
#  endif
#else
#  define LockInit(p)
#  define LockFree(p)
#  define Lock(p)
#  define Unlock(p)
#  define lock_t                volatile int
#endif /*  SMP code */
/* *INDENT-ON* */
