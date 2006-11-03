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

register ppc64_stack_frame* r1 asm("r1");

// http://www.freestandards.org/spec/ELF/ppc64/PPC-elf64abi.html
//
// WARNING!! THIS FILE MUST BE COMPILED W/ OPTIMIZATIONS!!!
// Otherwise gcc creates a stack frame, which messes everything up.
//
void ctx_swap(ppc64_stack_frame** from, ppc64_stack_frame* to) {
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
    *from = r1;
    r1 = to;

    // restore special regs
    asm volatile("mtlr %0" : : "r"(r1->save_lr));
    asm volatile("mtcr %0" : : "r"(r1->save_cr));
}
