#include <cassert>
#include <sys/types.h>
typedef __uint64_t Reg;

/** http://www.freestandards.org/spec/ELF/ppc64/PPC-elf64abi.html#STACK
    High Address

              +-> Back chain
              |   Floating point register save area
              |   General register save area
              |   VRSAVE save word (32-bits)
              |   Alignment padding (4 or 12 bytes)
              |   Vector register save area (quadword aligned)
              |   Local variable space
              |   Parameter save area    (SP + 48)
              |   TOC save area          (SP + 40)
              |   link editor doubleword (SP + 32)
              |   compiler doubleword    (SP + 24)
              |   LR save area           (SP + 16)
              |   CR save area           (SP + 8)
    SP  --->  +-- Back chain             (SP + 0)

    Low Address
*/
struct ppc64_stack_frame {
    Reg back_chain;
    Reg save_cr;
    Reg save_lr;
    Reg reserved_compiler;
    Reg reserved_linker;
    Reg save_toc;
    Reg save_params[];
} __attribute__((aligned(16)));

struct ppc64_context {
    ppc64_stack_frame* r1;
    size_t stack_size;
    void* (*func)(void*);
    void* arg;
    void* rval;
    // return to the owner context when this context exits
    ppc64_context* owner;
} __attribute__((aligned(16)));

extern void ctx_start(ppc64_context* prev, ppc64_context* cur);
register ppc64_stack_frame* r1 asm("r1");

// http://www.freestandards.org/spec/ELF/ppc64/PPC-elf64abi.html
//
// WARNING!! THIS FILE MUST BE COMPILED W/ OPTIMIZATIONS!!!
// Otherwise gcc creates a stack frame, which messes everything up.
//
void ctx_swap(ppc64_context* from, ppc64_context* to) {
   
    //    bool jump = to->r1->save_lr == (Reg) &ctx_start;
        //    void (*jump)(ppc64_context*, ppc64_context*) = &ctx_start;
    // Conveniently, the ppc64 ABI allows you to go 288 bytes past the
    // stack pointer without getting into trouble. This is exactly
    // enough to hold all 36 callee-saved gp regs. In addition,
    // parameters get passed in registers. As a result we don't
    // actually need a stack frame!

    // this translates into
    // - 72 instructions to save/restore 18+18 regs
    // - 10 instructions to swap contexts
    // - 2 instructions for call/return
    // Total: 84 instructions or ~60ns assuming IPC=1 @1.5GHz
#if 0
    asm (""
         "# out with the old...\n\t"
         "mflr 10\n\t"
         "std 10, 16(1)\n\t"
         "mfcr 10\n\t"
         "std 10, 8(1)\n\t"
                  //         "std 2, 40(1)\n\t"
         "std 1, %0\n\t"
         "\n"
         "# ...and in with the new\n\t"
         "ld 1, %1\n\t"
                  //         "ld 2, 40(1)\n\t"
         "ld 10, 8(1)\n\t"
         "mtcr 10\n\t"
         "ld 10, 16(1)\n\t"
         "mtlr 10\n\t"
         : "=m"(from->r1)        // leaving
         : "m"(to->r1)           // arriving
         : "memory", "r10",
         // fool the compiler into saving the int and fp regs for
         // us. We sneak in a new stack between its save and restore
         // operations, saving a lot of error-prone typing.
         "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22",
         "r23", "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
         "fr14", "fr15", "fr16", "fr17", "fr18", "fr19", "fr20", "fr21", "fr22",
         "fr23", "fr24", "fr25", "fr26", "fr27", "fr28", "fr29", "fr30", "fr31"
         );
#elif 0 // slightly slower (???)
    // fool the compiler into saving the int and fp regs for
    // us. We sneak in a new stack between its save and restore
    // operations, saving a lot of error-prone typing.
    asm("":::
         "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22",
         "r23", "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
         "fr14", "fr15", "fr16", "fr17", "fr18", "fr19", "fr20", "fr21", "fr22",
         "fr23", "fr24", "fr25", "fr26", "fr27", "fr28", "fr29", "fr30", "fr31"
         );
    // save the special regs
    asm ("mflr %0\n\t" "mfcr %1" : "=r"(r1->save_lr), "=r"(r1->save_cr));

    // swap stacks
    from->r1 = r1;
    r1 = to->r1;

    // restore special regs
    asm ("mtcr %0\n\t" "mtlr %1" : : "r"(r1->save_cr), "r"(r1->save_lr));
#elif 1 // Winner!
    // fool the compiler into saving the int and fp regs for
    // us. We sneak in a new stack between its save and restore
    // operations, saving a lot of error-prone typing.
    asm("":::
        "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22",
        "r23", "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
        "fr14", "fr15", "fr16", "fr17", "fr18", "fr19", "fr20", "fr21", "fr22",
        "fr23", "fr24", "fr25", "fr26", "fr27", "fr28", "fr29", "fr30", "fr31");
    
    // save the special regs
    asm volatile("mflr %0" : "=r"(r1->save_lr));
    asm volatile("mfcr %0" : "=r"(r1->save_cr));

    // swap stacks
    from->r1 = r1;
    r1 = to->r1;

    // restore special regs
    asm volatile("mtlr %0" : : "r"(r1->save_lr));
    asm volatile("mtcr %0" : : "r"(r1->save_cr));
#else
    asm("":::
         // fool the compiler into saving the int and fp regs for
         // us. We sneak in a new stack between its save and restore
         // operations, saving a lot of error-prone typing.
         "r14", "r15", "r16", "r17", "r18", "r19", "r20", "r21", "r22",
         "r23", "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
         "fr14", "fr15", "fr16", "fr17", "fr18", "fr19", "fr20", "fr21", "fr22",
         "fr23", "fr24", "fr25", "fr26", "fr27", "fr28", "fr29", "fr30", "fr31"
         );
    asm volatile ("std 1, %0\n\t"
         "ld 1, %1\n\t"
         : "=m"(from->r1)
         : "m"(to->r1));
    asm ("mflr %0" : "=r"(from->r1->save_lr));
    asm ("mfcr %0" : "=r"(from->r1->save_cr));
    asm("mtcr %0" : : "r"(to->r1->save_cr));
    asm("mtlr %0" : : "r"(to->r1->save_lr));

#endif
    //    if(to->r1->save_lr == (Reg) jump)
    // hack! the address of a function is actually a pointer to it. Dereference before returning...
    //    if(jump)
    //        jump(from, to);
    //            asm( "ld 10, 0(10)\n\tmtctr 10\n\tbctr\n\t");
}
