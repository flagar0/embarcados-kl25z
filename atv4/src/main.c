#include "MKL25Z4.h"
// vou usata a porta PTB2 ou A2
void delayMs (int n) {
	volatile int i;
	volatile int j;
	for (i = 0; i < n; i++)
		for (j = 0; j < 21000; j++) {}
        
}

void main(void)
{
    //habilita o clock na porta B
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    //liga o mux para gpio da porta 2
    //PORTB->PCR[2] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[2] |= PORT_PCR_MUX(1);

    //habilita o clock do adc 
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    // tipo de trigger ADTRG
    ADC0->SC2 |= ADC_SC2_ADTRG_MASK;

    // fonte do clock e resolucao
    ADC0->CFG1 |= ADC_CFG1_ADICLK(3); //Asynchronous clock (ADACK)
    ADC0->CFG1 |= ~ADC_CFG1_MODE_MASK; // zera a resolucao -> 8 bit

    //define o canal 7 como de entrada
    ADC0->SC1[0] &= ~ADC_SC1_ADCH_MASK; //zera a mascara
    ADC0->SC1[0] |= ADC_SC1_ADCH(7); // canal 7

    
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

