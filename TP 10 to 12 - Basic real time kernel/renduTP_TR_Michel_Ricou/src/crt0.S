/* Fichier de démarrage ASM */


/* Cette section contient les vecteurs d'exception placés à
   l'adresse 0x00000000.
*/
.section .vectors, "x"
.code 32
  ldr   pc, =_start       @ reset vector
  b     .                 @ Undefined instructions
  b     .                 @ software interrupt
  b     .                 @ prefetch abort
  b     .                 @ data abort
  b     .                 @ reserved
  ldr   pc, =scheduler    @ irq
  b     .                 @ fiq

/* Cette section contient le code de démarrage proprement dit */
.section .start, "x"
.code 32
.align 4

.global _start
.func   _start

_start:
  /* Initialiser le matériel : activation du cache I, initialisation PLL et
     contrôleur de SDRAM, ... */
  bl   _lowlevel_init

  /* Copier la section .data en ram si nécessaire :
     __data est la VMA de la section .data, __etext est la LMA de la section
     __data située à la suite de la section .text. Si les deux sont égales,
     le programme est configuré pour être chargé en RAM et il est inutile de
     copier la section .data puisqu'elle est déjà au bon endroit en mémoire. */
  ldr   r0, =__data
  ldr   r1, =__etext

  cmp   r0, r1
  beq   3f
  ldr   r2,=__edata
  b     2f
1:ldr   r3, [r1], #4
  str   r3, [r0], #4
2:cmp   r0, r2
  bne   1b

3:
  /* Initialiser bss à 0. __bss est la VMA du début de la section .bss,
     __ebss la VMA de la fin de cette même section. */
  ldr   r0,=__bss
  ldr   r1,=__ebss
  mov   r2, #0
  b     2f
1:str   r2, [r0], #4

2:cmp   r0,r1
  bne   1b

  /* Passer en mode système */
  mrs   r0, cpsr
  orr   r0, r0, #0x1f
  msr   cpsr_c, r0

  /* Initialiser la pile système */
  ldr   sp, =__sys_tos

  /* Mettre le frame pointer à 0, utile pour l'affichage de la pile d'appel
     dans gdb */
  mov   fp, #0

  /* On peut passer au code C, appeler main */
	bl    main

	/* Main ne devrait pas retourner, si c'est le cas, ne rien faire de plus */
	b     .
.endfunc

.end
