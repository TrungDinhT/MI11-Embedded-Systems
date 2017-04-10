.section ".vector", "x", %progbits
vectors:
	b resetHandler 		@ Reset vector
	b .					@ undefined @ Undefined instruction
	b .					@ soft_interrupt @ Software interrupt
	b .					@ prefetch_abort @ Prefetch abort exception
	b .					@ data_abort @ Data abort exception
	b . 				@ Reserved vector, not used
	b irq_handler		@ irq_handler @ Normal interrupt
	b .					@ iq_handler @ Fast interrupt

.text
.arm
.global resetHandler
@ .func resetHandler
resetHandler:

@ Configure target
bl _lowlevel_init

@ Copy variables to their virtual address
ldr r0, =data_VMA_start
ldr r1, =data_start
ldr r2, =data_end


.global dataToVMA
.func dataToVMA
dataToVMA:
	ldr r3, [r1]
	str r3, [r0]
	add r0, #4
	add r1, #4

	cmp r1, r2

	bne dataToVMA
.endfunc


@ Set global variables (bss) to 0
ldr r0, =bss_start
ldr r2, =bss_end

mov r1, #0

.global bssToZeros
.func bssToZeros
bssToZeros:
	str r1, [r0], #4

	cmp r0, r2

	bne bssToZeros
.endfunc

@ Initialize the stack pointer
ldr sp , =0x08010000

@ IRQ mode
mrs r0, CPSR      @ Copie CPSR dans r0
bic r0, r0, #0x1f @ Met à 0 les 5 bits M
orr r0, r0, #0x12 @ et change vers le mode interrupt
msr CPSR_c, r0    @ Recharge les bits de contrôle
nop               @ de CPSR

@ change IRQ stack pointer
ldr sp , =0x08020000

@ SVC mode
mrs r0, CPSR      @ Copie CPSR dans r0
bic r0, r0, #0x1f @ Met à 0 les 5 bits M
orr r0, r0, #0x13 @ et change vers le mode superviseur
msr CPSR_c, r0    @ Recharge les bits de contrôle
nop               @ de CPSR

@ Call main()
bl main

@ If main returns, loop
b .

.align 4
.end
