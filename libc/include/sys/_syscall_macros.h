#ifndef LAMBOS_SYSCALL_MACROS_H
#define LAMBOS_SYSCALL_MACROS_H

#define SYSCALL_INTERRUPT_NUM "0x80"

#define DECL_SYSCALL0(fn) int sys_##fn();
#define DECL_SYSCALL1(fn,p1) int sys_##fn(p1);
#define DECL_SYSCALL2(fn,p1,p2) int sys_##fn(p1,p2);
#define DECL_SYSCALL3(fn,p1,p2,p3) int sys_##fn(p1,p2,p3);
#define DECL_SYSCALL4(fn,p1,p2,p3,p4) int sys_##fn(p1,p2,p3,p4);
#define DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) int sys_##fn(p1,p2,p3,p4,p5);

#define DEFN_SYSCALL0(fn, num) \
    int sys_##fn() { \
        int a; __asm__ __volatile__("int $" SYSCALL_INTERRUPT_NUM "" : "=a" (a) : "0" (num)); \
        return a; \
    }

#define DEFN_SYSCALL1(fn, num, P1) \
    int sys_##fn(P1 p1) { \
        int __res; __asm__ __volatile__("push %%ebx; movl %2,%%ebx; int $" SYSCALL_INTERRUPT_NUM "; pop %%ebx" \
                : "=a" (__res) \
                : "0" (num), "r" ((int)(p1))); \
        return __res; \
    }

#define DEFN_SYSCALL2(fn, num, P1, P2) \
    int sys_##fn(P1 p1, P2 p2) { \
        int __res; __asm__ __volatile__("push %%ebx; movl %2,%%ebx; int $" SYSCALL_INTERRUPT_NUM "; pop %%ebx" \
                : "=a" (__res) \
                : "0" (num), "r" ((int)(p1)), "c"((int)(p2))); \
        return __res; \
    }

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
    int sys_##fn(P1 p1, P2 p2, P3 p3) { \
        int __res; __asm__ __volatile__("push %%ebx; movl %2,%%ebx; int $" SYSCALL_INTERRUPT_NUM "; pop %%ebx" \
                : "=a" (__res) \
                : "0" (num), "r" ((int)(p1)), "c"((int)(p2)), "d"((int)(p3))); \
        return __res; \
    }

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
    int sys_##fn(P1 p1, P2 p2, P3 p3, P4 p4) { \
        int __res; __asm__ __volatile__("push %%ebx; movl %2,%%ebx; int $" SYSCALL_INTERRUPT_NUM "; pop %%ebx" \
                : "=a" (__res) \
                : "0" (num), "r" ((int)(p1)), "c"((int)(p2)), "d"((int)(p3)), "S"((int)(p4))); \
        return __res; \
    }

#define DEFN_SYSCALL5(fn, num, P1, P2, P3, P4, P5) \
    int sys_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { \
        int __res; __asm__ __volatile__("push %%ebx; movl %2,%%ebx; int $" SYSCALL_INTERRUPT_NUM "; pop %%ebx" \
                : "=a" (__res) \
                : "0" (num), "r" ((int)(p1)), "c"((int)(p2)), "d"((int)(p3)), "S"((int)(p4)), "D"((int)(p5))); \
        return __res; \
    }

#endif //LAMBOS_SYSCALL_MACROS_H
