#include "MKL25Z4.h"
#include <stdio.h> // ou <sys/printk.h> dependendo do seu ambiente

void delayMs(int n) {
    volatile int i;
    volatile int j;
    for (i = 0; i < n; i++)
        for (j = 0; j < 21000; j++) {}
}

void main(void)
{
    // 1. Habilita o clock na Porta B
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    // porta D para led azul
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
    // 2. Configura o PTB1 como Entrada Analógica (MUX = 0)
    PORTB->PCR[1] &= ~PORT_PCR_MUX_MASK; 

    // 3. Habilita o clock do módulo ADC0
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    // habilita o gpio para leds
    PORTB->PCR[19] = PORT_PCR_MUX(1); // LED Verde (PTB19)
    PORTD->PCR[1]  = PORT_PCR_MUX(1); // LED Azul (PTD1)

    // leds como saida
    PTB->PDDR |= (1 << 19);
    PTD->PDDR |= (1 << 1);
    
    // desliga leds
    PTB->PSOR = (1 << 19);
    PTD->PSOR = (1 << 1);
    // 4. Garante que o gatilho será via Software (ADTRG = 0)
    ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;

    // 5. Configura fonte de clock e resolução
    // Zerando a máscara MODE garantimos resolução de 8 bits (MODE = 0)
    ADC0->CFG1 &= ~ADC_CFG1_MODE_MASK; 
    ADC0->CFG1 |= ADC_CFG1_ADICLK(3); // Clock Assíncrono (ADACK)

    uint16_t saida;
    printk("Iniciando leituras...\n"); // Use printk se for seu padrão

    while (1) {
        // Dispara a conversão analógica selecionando o canal 9 (PTB1)
        // Isso deve ficar dentro do loop para disparar uma nova medição a cada ciclo
        ADC0->SC1[0] = ADC_SC1_ADCH(9);

        // Fica travado aguardando a flag COCO (Conversion Complete) ir para 1
        while (NXP_FLD2VAL(ADC_SC1_COCO, ADC0->SC1[0]) == 0) {
            // Aguardando o ADC terminar...
        }

        // Lê o resultado. Atenção: Ao ler o registrador R[0], a flag COCO volta para 0 automaticamente!
        saida = ADC0->R[0];
        
        // Imprime usando %u pois saida é uint16_t (unsigned)
        printk("SAIDA: %u\n", saida); 
        if (saida <= 20){
            // led verde
            PTB->PCOR = (1 << 19);
            PTD->PSOR = (1 << 1);
        } else if (saida >= 200){
            // led azul
            PTB->PSOR = (1 << 19);
            PTD->PCOR = (1 << 1);
        } else {
            // desliga leds
            PTB->PSOR = (1 << 19);
            PTD->PSOR = (1 << 1);
        }
        // Pequeno atraso para não encher sua tela de números absurdamente rápido
        delayMs(200); 
    }
}