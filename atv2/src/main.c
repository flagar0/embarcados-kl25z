#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <pwm_z42.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TPM_MODULE 1000
// Define o LED usando Device Tree
#define LEDRED DT_ALIAS(led2)
#define LEDGREEN DT_ALIAS(led0)
#define LEDBLUE DT_ALIAS(led1)


// Verifica se o LED está definido no Device Tree
#if DT_NODE_HAS_STATUS(LEDGREEN, okay)
static const struct gpio_dt_spec ledgreen = GPIO_DT_SPEC_GET(LEDGREEN, gpios);
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

// Verifica se o LED está definido no Device Tree
#if DT_NODE_HAS_STATUS(LEDRED, okay)
static const struct gpio_dt_spec ledred = GPIO_DT_SPEC_GET(LEDRED, gpios);
#else
#error "Unsupported board: led2 devicetree alias is not defined"
#endif

// Verifica se o LED está definido no Device Tree
#if DT_NODE_HAS_STATUS(LEDBLUE, okay)
static const struct gpio_dt_spec ledblue = GPIO_DT_SPEC_GET(LEDBLUE, gpios);
#else
#error "Unsupported board: led1 devicetree alias is not defined"
#endif
int main(void)
{
    const struct device *uart_dev =
        DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

    char rx_buf[16];
    int buf_ptr = 0;
    int canal =0;

    uint16_t duty_terminal = 0;

    // Verifica UART
    if (!device_is_ready(uart_dev)) {
        printk("UART nao pronta!\n");
        return 0;
    }

    printk("UART pronta!\n");

    // Inicializa o módulo TPM2 com:
    // - base do TPMx
    // - fonte de clock PLL/FLL (TPM_CLK)
    // - valor do registrador MOD
    // - tipo de clock (TPM_CLK)
    // - prescaler de 1 a 128 (PS)
    // - modo de operação EDGE_PWM
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);

    // Inicializa o canal 0 do TPM2 para gerar sinal PWM na porta GPIOB_18
    // - modo TPM_PWM_H (nível alto durante o pulso)
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 18); // 0 -> red
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 19); // 1 -> green
    //pwm_tpm_Ch_Init(TPM2, 2, TPM_PWM_H, GPIOB, 1); // 2 -> blue

    // Começa desligado
    pwm_tpm_CnV(TPM2, 0, 0);
    pwm_tpm_CnV(TPM2, 1, 0);
    //pwm_tpm_CnV(TPM2, 2, 0);

    printk("Controle PWM iniciado\n");
    printk("LED VERMELHO\n");
    printk("Digite um valor entre 0 e 100:\n");

while (1)
{
    uint8_t c;
    // Há caractere disponível?
    if (uart_poll_in(uart_dev, &c) == 0)
    {
        // Ignora '\r'
        if (c == '\r')
            continue;

        // Echo
        uart_poll_out(uart_dev, c);

        // Enter
        if (c == '\n' || c=='\r')
        {
            rx_buf[buf_ptr] = '\0';

            if (buf_ptr > 0)
            {
                int numero = atoi(rx_buf);

                // Limita 0-100
                if (numero < 0)
                    numero = 0;

                if (numero > 100)
                    numero = 100;

                uint16_t duty =
                    (numero * TPM_MODULE) / 100;

                pwm_tpm_CnV(TPM2, canal, duty);

                printk("\nPWM = %d%%\n", numero);
                canal++;
                if (canal ==2){
                    canal =0;
                }
            }

            // Limpa buffer
            buf_ptr = 0;
            memset(rx_buf, 0, sizeof(rx_buf));

            printk("Canal = %d\n", canal);
            printk("Digite outro valor:\n");
        }
        else
        {
            // Aceita apenas números
            if ((c >= '0') && (c <= '9'))
            {
                if (buf_ptr < sizeof(rx_buf) - 1)
                {
                    rx_buf[buf_ptr++] = c;
                }
            }
        }
    }
}

    return 0;
}