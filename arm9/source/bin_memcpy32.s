.cpu arm946e-s
.syntax unified

.section ".itcm"
.arm
.balign 4

.global Bin_Memcpy32
.type Bin_Memcpy32, %function
Bin_Memcpy32:
	add   r3, r2, #3
	lsrs  r3, r3, #2
	bxeq  lr
	
copy_loop:
	subs  r3, #1
	ldr   ip, [r1], #4
	str   ip, [r0], #4
	bne   copy_loop
	
	sub   r1, r0, r2
	bic   r1, #0x1F
	
cache_loop:
	mcr   p15, 0, r3, c7, c10, 4  // Wait write buffer empty
	mcr   p15, 0, r1, c7, c14, 1  // DC flush
	mcr   p15, 0, r1, c7, c5,  1  // IC invalidate
	add   r1, #0x20
	cmp   r1, r0
	blo   cache_loop
	
	bx    lr

.pool
.end
