#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <MKL25Z4.h>

/*
 * HC-SR04
 *
 * TRIG -> PTB2
 * ECHO -> PTB0 (TPM1_CH0)
 *
 * IMPORTANTE:
 * O pino ECHO do HC-SR04 é 5V.
 * Use divisor resistivo para entrar no PTB0.
 */

#define TRIG_PIN 2

volatile uint32_t rise_capture = 0;
volatile uint32_t fall_capture = 0;
volatile uint32_t pulse_ticks = 0;

volatile uint8_t capture_state = 0;

/*
 * TPM1 clock:
 * 48 MHz / 8 = 6 MHz
 *
 * 1 tick = 1/6 us
 */

void TPM1_IRQHandler(void)
{
    /*
     * Limpa flag da interrupção
     */
    TPM1->CONTROLS[0].CnSC |= TPM_CnSC_CHF_MASK;

    /*
     * Borda de subida
     */
    if (capture_state == 0)
    {
        rise_capture = TPM1->CONTROLS[0].CnV;

        /*
         * Agora captura borda de descida
         */
        TPM1->CONTROLS[0].CnSC &=
            ~(TPM_CnSC_ELSA_MASK |
              TPM_CnSC_ELSB_MASK);

        TPM1->CONTROLS[0].CnSC |= TPM_CnSC_ELSA_MASK;

        capture_state = 1;
    }
    else
    {
        /*
         * Borda de descida
         */
        fall_capture = TPM1->CONTROLS[0].CnV;

        /*
         * Trata overflow
         */
        if (fall_capture >= rise_capture)
        {
            pulse_ticks = fall_capture - rise_capture;
        }
        else
        {
            pulse_ticks =
                (65535 - rise_capture) + fall_capture;
        }

        /*
         * Volta para subida
         */
        TPM1->CONTROLS[0].CnSC &=
            ~(TPM_CnSC_ELSA_MASK |
              TPM_CnSC_ELSB_MASK);

        TPM1->CONTROLS[0].CnSC |= TPM_CnSC_ELSB_MASK;

        capture_state = 0;
    }
}

void main(void)
{
    float pulse_us;
    float distance_cm;

    printk("Inicializando HC-SR04...\n");

    /*
     * Habilita clock PORTB
     */
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    /*
     * Habilita TPM1
     */
    SIM->SCGC6 |= SIM_SCGC6_TPM1_MASK;

    /*
     * Clock TPM = PLL/FLL
     */
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1);

    /*
     * PTB0 = TPM1_CH0
     * ALT3
     */
    PORTB->PCR[0] = PORT_PCR_MUX(3);

    /*
     * PTB2 = GPIO
     */
    PORTB->PCR[2] = PORT_PCR_MUX(1);

    /*
     * PTB2 saída
     */
    GPIOB->PDDR |= (1 << TRIG_PIN);

    /*
     * Trigger inicia LOW
     */
    GPIOB->PCOR = (1 << TRIG_PIN);

    /*
     * TPM1 configuração
     */
    TPM1->SC = 0;

    /*
     * MOD máximo
     */
    TPM1->MOD = 65535;

    /*
     * Prescaler = 8
     */
    TPM1->SC |= TPM_SC_PS(3);

    /*
     * Input Capture:
     * borda de subida + interrupção
     */
    TPM1->CONTROLS[0].CnSC =
        TPM_CnSC_CHIE_MASK |
        TPM_CnSC_ELSB_MASK;

    /*
     * Habilita IRQ
     */
    NVIC_ClearPendingIRQ(TPM1_IRQn);
    NVIC_EnableIRQ(TPM1_IRQn);

    /*
     * Inicia TPM
     */
    TPM1->SC |= TPM_SC_CMOD(1);

    printk("Sistema iniciado\n");

    while (1)
    {
        /*
         * Zera medida anterior
         */
        pulse_ticks = 0;

        /*
         * Trigger HC-SR04
         */

        GPIOB->PCOR = (1 << TRIG_PIN);
        k_usleep(2);

        GPIOB->PSOR = (1 << TRIG_PIN);
        k_usleep(10);

        GPIOB->PCOR = (1 << TRIG_PIN);

        /*
         * Espera eco
         */
        k_msleep(60);

        /*
         * ticks -> us
         *
         * 6 MHz:
         * 6 ticks = 1 us
         */
        pulse_us = pulse_ticks / 6.0f;

        /*
         * HC-SR04:
         * distancia(cm) = tempo_us / 58
         */
        distance_cm = pulse_us / 58.0f;

        printk("Ticks: %u\n", pulse_ticks);
        printk("Pulso: %d us\n", (int)pulse_us);
        printk("Distancia: %d cm\n\n",
               (int)distance_cm);

        k_msleep(500);
    }
}