#if defined(STM32G0xx)
    #include <stm32g0xx_hal.h>
#elif defined(STM32F0xx)
    #include <stm32f0xx_hal.h>
#else
    #error "Unsuported MCU family"
#endif

#include <system/dbg.hpp>
#include <unwind.h>

extern "C" void __attribute__((__used__)) NMI_Handler() {
    Dbg::error( "NMI occured" );
    while ( true );
}

extern "C" void SysTick_Handler() {
    HAL_IncTick();
}

// Disclaimer: huge hack follows. We want to print stack trace in the
// HardFaultHandler, however, unwinder cannot go though interrupt call frame.
// The solution is to manually restore frame context and call the unwinder there
// Inspiration: https://stackoverflow.com/questions/47331426/stack-backtrace-for-arm-core-using-gcc-compiler-when-there-is-a-msp-to-psp-swit

// This struct definition mimics the internal structures of libgcc in
// arm-none-eabi binary. It's not portable and might break in the future.
struct core_regs {
    unsigned r[16];
};

typedef struct {
    unsigned demand_save_flags;
    struct core_regs core;
} phase2_vrs;

// We store what we know about the external context at interrupt entry in this
// structure.
phase2_vrs mainContext;
// Saved value of the lr register at the exception entry.
unsigned saved_lr;

// Takes registers from the core state and the saved exception context and
// fills in the structure necessary for the LIBGCC unwinder.
// Interrupt frame:
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Babefdjc.html
void fillPhase2Vrs( volatile unsigned *stack ) {
    mainContext.demand_save_flags = 0;
    mainContext.core.r[0] = stack[0];
    mainContext.core.r[1] = stack[1];
    mainContext.core.r[2] = stack[2];
    mainContext.core.r[3] = stack[3];
    mainContext.core.r[12] = stack[4];
    mainContext.core.r[14] = stack[6];
    mainContext.core.r[15] = stack[6];
    saved_lr = stack[5];
    mainContext.core.r[13] = (unsigned)(stack + 8); // stack pointer
}

extern "C" {
_Unwind_Reason_Code __gnu_Unwind_Backtrace(
    _Unwind_Trace_Fn trace, void *trace_argument, phase2_vrs *entry_vrs);
}

_Unwind_Reason_Code trace( _Unwind_Context *ctx, void *d ) {
    int *depth = reinterpret_cast< int * >( d );
    Dbg::error( "  #%d: 0x%08x", *depth, _Unwind_GetIP( ctx ) );
    ( *depth )++;
    return _URC_NO_REASON;
}

void backtrace() {
    phase2_vrs firstContext = mainContext;
    int depth = 0;
    Dbg::error( "backtrace:");
    __gnu_Unwind_Backtrace( &trace, &depth, &firstContext );
}

extern "C" void __attribute__((__noinline__, __used__))
HardFault_HandlerDeref( volatile unsigned *stack) {
    Dbg::error( "\n\n\nHardFault ocurred" );
    fillPhase2Vrs(stack);
    backtrace();
    while ( true );
}

extern "C" void __attribute__((__naked__)) HardFault_Handler(void) {
    __asm volatile(
        "mov  r0, %0 \n"
        "str  r4, [r0, #4*4] \n"
        "str  r5, [r0, #5*4] \n"
        "str  r6, [r0, #6*4] \n"
        "str  r7, [r0, #7*4] \n"
        "mov r1, r8 \n"
        "str  r1, [r0, #8*4] \n"
        "mov r1, r9 \n"
        "str  r1, [r0, #9*4] \n"
        "mov r1, r10 \n"
        "str  r1, [r0, #10*4] \n"
        "mov r1, r11 \n"
        "str  r1, [r0, #11*4] \n"
        "mov r1, r12 \n"
        "str  r1, [r0, #12*4] \n"
        "mov r1, r13 \n"
        "str  r1, [r0, #13*4] \n"
        "mov r1, r14 \n"
        :
        : "r"( mainContext.core.r )
        : "r0" );
    __asm volatile(
        "mov r0, sp\n"
        "ldr r1,  =HardFault_HandlerDeref  \n"
        "bx  r1  \n"
        :
        :
        : "r0", "r1" );
}