#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <pwm_z42.h>
#include <zephyr/logging/log.h>

#define TPM_IRQ_LINE TPM1_IRQn
#define TPM_IRQ_PRIORITY 1
#define TPM_MODULE 10000

#define TPM_CLOCK_HZ 48000000.0f
#define TPM_PRESCALER 128.0f

volatile uint16_t captured_rise = 0;  // tick da borda de subida
volatile uint16_t captured_fall = 0;  // tick da borda de descida
volatile bool echo = false;
volatile bool waiting_fall = false;   // controla qual borda estamos esperando

//LOG_MODULE_REGISTER(ultraSom, LOG_LEVEL_DBG);

void tpm1_isr(void *arg)
{
    TPM1->STATUS |= TPM_STATUS_CH1F_MASK;

    if (!waiting_fall) {
        // Primeira interrupção: borda de SUBIDA
        captured_rise = TPM1->CONTROLS[1].CnV;
        waiting_fall = true;

    } else {
        // Segunda interrupção: borda de DESCIDA
        captured_fall = TPM1->CONTROLS[1].CnV;
        waiting_fall = false;
        echo = true;

    }
}

void main(void)
{
    // TPM2: Trigger em PTB2
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, 1, PS_8, EDGE_PWM);
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 2);

    // TPM1: Captura Echo em PTB1, começa na borda de SUBIDA
    pwm_tpm_Init(TPM1, TPM_PLLFLL, 65535, 1, PS_128, EDGE_PWM);
    pwm_tpm_Ch_Init(TPM1, 1, TPM_INPUT_CAPTURE_BOTH | TPM_CHANNEL_INTERRUPT, GPIOB, 1);

    IRQ_CONNECT(TPM_IRQ_LINE, TPM_IRQ_PRIORITY, tpm1_isr, NULL, 0);
    irq_enable(TPM_IRQ_LINE);

    //pwm_tpm_CnV(TPM2, 0, TPM_MODULE/1000);
    while (1) {
        echo = false;

        // Envia trigger de 10µs
        pwm_tpm_CnV(TPM2, 0, TPM_MODULE/1000);
        k_usleep(10);
        pwm_tpm_CnV(TPM2, 0, 0);

        // Aguarda o echo por até 50 ms
        for (int i = 0; i < 50; i++) {
            if (echo) {
                break;
            }
            k_msleep(1);
        }

        if (echo) {

            // Calcula duração do pulso em ticks
            uint16_t pulse_ticks;

            if (captured_fall >= captured_rise) {
                pulse_ticks = captured_fall - captured_rise;
            } 

            // Descarta medições inválidas
            printk("Largura de ticks: %u\n",pulse_ticks);
            if (pulse_ticks < 1000000) {

                float pulse_us = pulse_ticks *
                    (TPM_PRESCALER * 1000000.0f / TPM_CLOCK_HZ);

                float distance_cm = pulse_us * 0.0343f / 2.0f;
                //printk("Ticks: %u\n",pulse_ticks);
                printk("Distancia: %.0f cm\n", distance_cm);
            }
        }

        k_msleep(300);
    }
}