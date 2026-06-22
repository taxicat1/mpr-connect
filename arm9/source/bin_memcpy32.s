.cpu arm946e-s
.syntax unified

.section ".itcm"
.arm
.balign 4

.global Bin_Memcpy32
.type Bin_Memcpy32, %function
Bin_Memcpy32:
	// r0 = dst, r1 = src, r2 = len
	push  {r0-r1, r3-r11, lr}
	
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
	ldmlo  r1!, {r2}
	stmlo  r0!, {r2}
	blo    copy_loop_4
	
	// r0 = dst_start 32-aligned
	// r1 = dummy stack alignment
	pop   {r0-r1}
	bic   r0, #0x1F
	mov   r3, #0
	
	// Flush data cache, invalidate instruction cache
cache_loop:
	mcr   p15, 0, r3, c7, c10, 4  // Wait write buffer empty
	mcr   p15, 0, r0, c7, c14, 1  // DC flush
	mcr   p15, 0, r0, c7, c5,  1  // IC invalidate
	add   r0, #0x20
	cmp   r0, r10
	blo   cache_loop
	
	pop   {r3-r11, pc}

.pool
.end
