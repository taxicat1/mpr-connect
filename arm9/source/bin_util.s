.cpu arm946e-s
.syntax unified

.section ".itcm"
.arm
.balign 4


.global Bin_Memcpy32
.type Bin_Memcpy32, %function
Bin_Memcpy32:
	// r0 = dst, r1 = src, r2 = len
	push  {r3-r11, lr}
	
	// r10 = dst_end
	add   r10, r0, r2
	
	// r11 = dst_end_32
	bic   r11, r2, #0x1F
	add   r11, r0
	
	// 32-byte block copy
copy_loop_32:
	cmp    r0, r11
	ldmlo  r1!, {r2-r9}
	stmlo  r0!, {r2-r9}
	blo    copy_loop_32
	
	// 4-byte word copy
copy_loop_4:
	cmp    r0, r10
	ldrlo  r2, [r1], #4
	strlo  r2, [r0], #4
	blo    copy_loop_4
	
	pop   {r3-r11, pc}

.pool


.global Bin_PatchRSACheck
.type Bin_PatchRSACheck, %function
Bin_PatchRSACheck:
	// r0 = arm9_start, r1 = arm9_len
	push  {r3-r11, lr}
	
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
	
	mov   r0, #1
	pop   {r3-r11, pc}
	
no_match:
	mov   r0, #0
	pop   {r3-r11, pc}

rsaCheckSignature:
	sub   sp, sp, #0x120
	mov   r3, #0
	str   r3, [sp, #0x10]
	str   r3, [sp, #0x18]

rsaCheckPatch:
	mov   r0, #1
	pop   {r4, pc}

.pool


.global Bin_ClearCache
.type Bin_ClearCache, %function
Bin_ClearCache:
	mov   ip, #0
	mov   r0, ip
	
dc_loop:
	mcr   p15, 0, ip, c7, c10, 4  // Wait write buffer empty
	mcr   p15, 0, r0, c7, c14, 2  // DC flush
	
	add   r0, r0, #0x20
	tst   r0, #0x400
	beq   dc_loop
	
	bic   r1, r0, #0x400
	adds  r0, r1, #0x40000000
	bne   dc_loop
	
	mcr   p15, 0, r0, c7, c5,  0  // IC invalidate all
	mcr   p15, 0, ip, c7, c10, 4  // Wait write buffer empty
	
	bx    lr

.pool


.end
