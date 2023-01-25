
#include "arm.h"

.section ".text.startup"

/////////////////////////////////////////////////////////////////////////////
//// There are three global functions - start, halt and take_flags. Linker 
//// links start function at starting address, where the execution begins
//// (0x8000) and other two are used in kernel.
/////////////////////////////////////////////////////////////////////////////

.global _start
.global _halt
.global _take_flags

/////////////////////////////////////////////////////////////////////////////
//// First 8 instructions are interrupt handler calls. Each one of them loads
//// handlers starting address into PC.
/////////////////////////////////////////////////////////////////////////////

_start:
    ldr 		pc, _reset_h
    ldr 		pc, _undefined_instruction_vector_h
    ldr 		pc, _software_interrupt_vector_h
    ldr 		pc, _prefetch_abort_vector_h
    ldr 		pc, _data_abort_vector_h
    ldr 		pc, _unused_handler_h
    ldr 		pc, _interrupt_vector_h
    ldr 		pc, _fast_interrupt_vector_h

_reset_h:                           .word   _reset_
_undefined_instruction_vector_h:    .word   _reset_
_software_interrupt_vector_h:       .word   _svc_vector
_prefetch_abort_vector_h:           .word   _reset_
_data_abort_vector_h:               .word   _reset_
_unused_handler_h:                  .word   _reset_
_interrupt_vector_h:                .word   _irq_vector
_fast_interrupt_vector_h:           .word   _reset_

/////////////////////////////////////////////////////////////////////////////
//// When execution starts, reset handler is executed. You have to get out of
//// hypervisor mode, so the first few instructions are for that. After that 
//// is done, you have to copy interrupt handler calls at the beginning of
//// address spaces. You also have to initialize three stack areas: IRQ, SVC
//// and SYS.
/////////////////////////////////////////////////////////////////////////////

_reset_:
	// Disable interrupts
  	cpsid 	if

    // Check if HYP mode 
    mrs 	r0, cpsr_all
    // Apply mode mask
    and 	r0, r0, #ARM_MODE_MASK		
    mov 	r8, #ARM_MODE_HYP
    
    cmp 	r0, r8
    beq 	_out_of_hyp
    b 		_set_interrupts

_out_of_hyp: 
	// Get out of HYP mode 
    ldr 	r1, =_set_interrupts
    msr 	ELR_hyp, r1
    mrs 	r1, cpsr_all
    and 	r1, r1, #ARM_MODE_MASK   
    orr 	r1, r1, #ARM_MODE_SVC   
    msr 	SPSR_hyp, r1
    eret

_set_interrupts:
	
	// Copy exception table at 0x0000
    mov     r0, #0x8000
    mov     r1, #0x0000
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}

	// Set up IRQ stack at 0x7000
    cpsid 	if, #ARM_MODE_IRQ
	ldr 	sp, =0x7000
    
	// Set up SVC stack at 0x6000
	cpsid 	if, #ARM_MODE_SVC
    ldr		sp, =0x6000
    
    // Set up SYS stack at 0x8000
    cpsid 	if, #ARM_MODE_SYS
    ldr		sp, =0x8000
    
    // Start c startup and after that kernel
    bl      _cstartup
		
	b 		_halt

/////////////////////////////////////////////////////////////////////////////
//// Handling SVC and IRQ are basically the same, except they use different
//// stacks. Current task's context will be saved on SVC or IRQ stack and
//// that context will be passed to higher interrupt handler to perform task 
//// switching if needed. After higher interrupt handler has finished and 
//// context switched, it will be loaded back into the processor.
/////////////////////////////////////////////////////////////////////////////

_svc_vector:
	// Prepare context on SVC stack

    // Send SPSR and LR (CPSR from task and return address)
    /////////////////////////////
    // stack:
    // SPSR     (CPSR from task)
    // LR       (PC - return address)
    /////////////////////////////

	srsdb	sp!, #ARM_MODE_SVC

    // Send r0-r10, fp, ip to stack
    /////////////////////////////
    // stack:
    // SPSR     (CPSR from task)
    // LR       (PC - return address)
    // IP
    // FP
    // R10
    // R9
    // R8
    // R7
    // R6
    // R5
    // R4
    // R3
    // R2
    // R1
    // R0
    /////////////////////////////
	push	{r0-r10,fp,ip}

    // Change mode to SYS, to get task's SP and LR
	cpsid	if, #ARM_MODE_SYS
	mov 	r0, sp
	mov		r1, lr
    // Change back to SVC, to save them
	cpsid	if, #ARM_MODE_SVC
    /////////////////////////////
    // stack:
    // SPSR     (CPSR from task)
    // LR       (PC - return address)
    // IP
    // FP
    // R10
    // R9
    // R8
    // R7
    // R6
    // R5
    // R4
    // R3
    // R2
    // R1
    // R0
    // LR       (task's LR)
    // SP
    /////////////////////////////
	push 	{r0,r1}
	
	// Send context by address r0 = sp
	mov		r0, sp
	
	// Call svc_handler
	bl		svc_handler

	// Return context form SVC stack

    // Get SP and LR
	pop		{r0,r1}
	cpsid	if, #ARM_MODE_SYS
	mov		lr, r1
	mov		sp, r0
	cpsid	if, #ARM_MODE_SVC

    // Get r0-r10, fp and ip
	pop 	{r0-r10,fp,ip}
	
    // Return from exception
    // Load CPSR and PC from stack
	rfeia	sp!

/////////////////////////////////////////////////////////////////////////////
//// Same as svc_vector but on IRQ stack.
/////////////////////////////////////////////////////////////////////////////

_irq_vector:
    // When interrupt occurs, LR will be 1 instruction ahead.
	sub		lr, lr, #4
		
	// Prepare context on IRQ stack
    // Send SPSR and LR (CPSR from task and return address)
    /////////////////////////////
    // stack:
    // SPSR     (CPSR from task)
    // LR       (PC - return address)
    /////////////////////////////

	srsdb	sp!, #ARM_MODE_IRQ
	
    // Send r0-r10, fp, ip to stack
    /////////////////////////////
    // stack:
    // SPSR     (CPSR from task)
    // LR       (PC - return address)
    // IP
    // FP
    // R10
    // R9
    // R8
    // R7
    // R6
    // R5
    // R4
    // R3
    // R2
    // R1
    // R0
    /////////////////////////////
	push	{r0-r10,fp,ip}

    // Change mode to SYS, to get task's SP and LR
	cpsid	if, #ARM_MODE_SYS
	mov 	r0, sp
	mov		r1, lr
    // Change back to IRQ, to save them
	cpsid	if, #ARM_MODE_IRQ
    /////////////////////////////
    // stack:
    // SPSR     (CPSR from task)
    // LR       (PC - return address)
    // IP
    // FP
    // R10
    // R9
    // R8
    // R7
    // R6
    // R5
    // R4
    // R3
    // R2
    // R1
    // R0
    // LR       (task's LR)
    // SP
    /////////////////////////////
	push 	{r0,r1}

	// Send context by address r0 = sp
	mov		r0, sp
		
	// Call irq_handler
	bl		irq_handler

    // Return context form IRQ stack
	
    // Get SP and LR
	pop		{r0,r1}
	cpsid	if, #ARM_MODE_SYS
	mov		lr, r1
	mov		sp, r0
	cpsid	if, #ARM_MODE_IRQ

    // Get r0-r10, fp and ip
	pop 	{r0-r10,fp,ip}
	
    // Return from exception
    // Load CPSR and PC from stack
	rfeia	sp!

// Return CPSR
_take_flags:	
	mrs		r0, cpsr
	bx		lr
    
// Infinite loop
_halt:
	wfe
	b 		_halt
		
