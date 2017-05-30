#define irq_enable() asm volatile(	\
"mrs r0,cpsr\t\n"					\
"bic r0,r0,#0x80\t\n"				\
"msr cpsr_c,r0\t\n"					\
:									\
:									\
: "r0")
#define irq_disable() asm volatile(	\
"mrs r0,cpsr\t\n"					\
"orr r0,r0,#0x80\t\n"				\
"msr cpsr_c,r0\t\n"					\
:									\
:									\
: "r0")

int a;
int b = 1;
//static int c;
int c = 3;
char * s = "Salut";

unsigned int* ptr = (unsigned int*)0x0021C000;
unsigned int mask_gius = 0x80000000;
unsigned int mask_ocr2 = 0xC0000000;
unsigned int mask_dr =   0x80000000;
unsigned int mask_ddir = 0x80000000;
unsigned int mask_IRQEN = 0x00000011;
unsigned int mask_CLKSOURCE = 0x00000005;
int isOff = 1;

int main(void){
	/*
	// Main qui ne quitte pas
	while(1){
		c = 42;
		a = a + b + c + s[1] + f();
	}
	*/

	// Main qui quitte
	//c = 42;
	//a = a + b + c + s[1] + f();


	/*
	// Allume et éteind la LED à intervalle vaguement régulier
	configureLed();
	while(1){
		toggleLed();
		wait();
	}
	*/



	/*
	// Allume et éteint la LED à intervalle très régulier (version moche)
	configureTimer();
	configureLed();
	while(1){
		if(timeout()){
			resetUglyTimer();
			toggleLed();
		}
	}
	*/



	// Allume et éteint la LED à intervalle très régulier (version belle)
	configureIRQ();
	configureTimer();
	configureLed();


}


void wait(){
	for (b = 0; b<100000; b++){
		a *= 2;
		a /= 2;
	}
}

void configureIRQ(){
	// ARM9 core configuration
	irq_enable();

	// AITC routing interruption
	ptr = (unsigned int*)0x00223008;
	*ptr = 59;

	// Timer configuration
	ptr = (unsigned int*)0x00202000;
	*ptr = *ptr | mask_IRQEN;
}

void configureLed(){
	// Allumage de la LED via les registres dédiés GPIO port D sortie 31 (i = 31)

	/*
	 * Les adresses de registre GPIO se trouvent
	 * de 0x0021C000 à 0x0021CFFF (4KB)
	 */

	/*
	 * Opérateur :
	 * 	|  OU logique (pour passer des bits à 1)
	 * 	&	ET logique (pour passer des bits à 0)
	 * 	~	non binaire
	 */

	// Set bit i in the port D GPIO register in use register (GIUS_D = 0x0021C320) -> 1 (used for GPIO function)
	ptr = (unsigned int*)0x0021C320;
	*ptr = *ptr | mask_gius;


	// Set bits [2i-32+1] and [2i-32] in the port D Output conf. reg. 1 (OCR2_D = 0x0021C308) -> 1 et 1
	ptr = (unsigned int*)0x0021C308;
	*ptr = *ptr | mask_ocr2;

	// Set bit [i] in the port D data direction reg (DDIR_D = 0x0021C3000) -> 1 (= is an output, default 0)
	ptr = (unsigned int*)0x0021C300;
	*ptr = *ptr | mask_ddir;

}

void toggleLed(){
	// Write desired output value to bit [i] of the port D data reg. (DR_D = 0x0021C31C) -> 0 (LOW dans notre cas)
	ptr = (unsigned int*)0x0021C31C;
	if (isOff) {
		*ptr = *ptr & ~mask_dr;
	} else {
		*ptr = *ptr | mask_dr;
	}
	isOff = (isOff + 1) % 2;
}

void configureTimer() {
	/*
	 * TCTL (Timer ConTrol Register) choisi la clock à considérer (bits 3-1, manuel p. 700) :
	 * - OSC32
	 * - PERCLK1 (16 MHz)	001
	 * - PERCLK1/16 (1 MHz)	010
	 */
	ptr = (unsigned int*)0x00202000;
	*ptr = *ptr | mask_CLKSOURCE;

	// TPRER (Timer PRescaler Register) redivise la clock (0xFF = division par 256 = 3906 Hz)
	ptr = (unsigned int*)0x00202004;
	*ptr = 0x000000FF;

	// TCMP (Timer CoMpare Register) défini le nb de coups de clock avant de changer le status (1000 = 0x3E8)
	ptr = (unsigned int*)0x00202008;
	*ptr = 0x000003E8;

	// Note: on utilise pas le mode capture (sert à chopper le tick de clock courrant à l'arrivée d'un front montant d'un GPIO)
}

int timeout() {
	ptr = (unsigned int*)0x00202014;
	// Si le TSTAT (Timer STATus Register) est à 01, retourner que le timer a terminé (TSTAT = 0x00000003)
	if (*ptr == 0x00000001)
		return 1; // true
	else
		return 0; // false
}

void resetUglyTimer() {
	// Remet le TSTAT à 00
	ptr = (unsigned int*)0x00202014;
	*ptr = 0x00000000;
}

void __attribute__((interrupt("IRQ"))) irq_handler(void) {
/* Code du gestionnaire d’interruption IRQ */
	ptr = (unsigned int*)0x00202014;
	if (*ptr == 0x00000001) { // just read to be able to write
		resetUglyTimer();
		toggleLed();
	}
}



void __attribute__((interrupt("FIQ"))) fiq_handler(void) {
/* Code du gestionnaire d’interruption FIQ */
}







