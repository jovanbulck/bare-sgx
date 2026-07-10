#ifndef ARCH_H_INC
#define ARCH_H_INC

/*
 * Architectural definitions shared between C and assembly code.
 */

#define PAGE_SIZE                       4096
#define GPRSGX_SIZE                     184
#define SSA_FRAME_BEFORE_GPR_SIZE       (PAGE_SIZE-GPRSGX_SIZE)

#endif
