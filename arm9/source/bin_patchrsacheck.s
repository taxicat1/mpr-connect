.cpu arm946e-s
.syntax unified

.section ".itcm"
.arm
.balign 4

.global Bin_PatchRSACheck
.type Bin_PatchRSACheck, %function
Bin_PatchRSACheck:
	// r0 = arm9_start, r1 = arm9_len
	push  {r4-r11, lr}
	
	sub   r1, r1, #(0x10 - 4)
	add   r1, r1, r0
	
	// Load signature to r8-r11
	adr   r2, rsaCheckSignature
	ldm   r2, {r8-r11}
	
	// Check against signature (4 instructions / 32 bytes)
check_loop:
	cmp   r0, r1
	bhs   no_match
	ldr   r4, [r0], #4
	cmp   r4, r8
	bne   check_loop
	ldm   r0, {r5-r7}
	cmp   r5, r9
	bne   check_loop
	cmp   r6, r10
	bne   check_loop
	cmp   r7, r11
	bne   check_loop
	sub   r0, r0, #4
	
	// Apply patch (2 instructions / 8 bytes)
	adr   r2, rsaCheckPatch
	ldm   r2, {r4-r5}
	stm   r0, {r4-r5}
	
	// Clear cache (1 or 2 loops depending if patch location crosses cache line boundary)
	bic   r1, r0, #0x1F
	and   r2, r0, #0x1F
	mov   r3, #0
cache_loop:
	mcr    p15, 0, r3, c7, c10, 4  // Wait write buffer empty
	mcr    p15, 0, r1, c7, c14, 1  // DC flush
	mcr    p15, 0, r1, c7, c5,  1  // IC invalidate
	cmp    r2, #(0x20 - 0x8)
	movhi  r2, #0
	addhi  r1, r1, #0x20
	bhi    cache_loop
	
	mov   r0, #1
	pop   {r4-r11, pc}
	
no_match:
	mov   r0, #0
	pop   {r4-r11, pc}

rsaCheckSignature:
	sub   sp, sp, #0x120
	mov   r3, #0
	str   r3, [sp, #0x10]
	str   r3, [sp, #0x18]

rsaCheckPatch:
	mov   r0, #1
	pop   {r4, pc}

.pool
.end
