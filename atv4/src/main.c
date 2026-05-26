#include "MKL25Z4.h"

void delayMs (int n) {
	volatile int i;
	volatile int j;
	for (i = 0; i < n; i++)
		for (j = 0; j < 21000; j++) {}
        
}

void main(void)
{
    //habilita o clock
    SIM_SCGC5 |= (1 << 10);

    PORTB_PCR19 &= ~(1 << 10);
    PORTB_PCR19 &= ~(1 << 9);
    PORTB_PCR19 |= (1 << 8);
    //habilita o led vermelho como saida
    GPIOB_PDDR |= (1 << 19);

    GPIOB_TSOR |= (1 << 19);
    while (1) {
        //espera 1 segundo
        delayMs(500);
        GPIOB_TSOR |= (1 << 19);
        delayMs(500);
        //desliga led vermelho
        GPIOB_TSOR |= (1 << 19);
        //liga led verde
        //GPIOB_PDOR |= (1 << 19);

    }
}

