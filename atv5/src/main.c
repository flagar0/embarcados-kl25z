#include <zephyr/kernel.h>             // Funções básicas do Zephyr (ex: k_msleep, k_thread, etc.)
#include <zephyr/device.h>             // API para obter e utilizar dispositivos do sistema
#include <zephyr/drivers/gpio.h>       // API para controle de pinos de entrada/saída (GPIO)
//#include <zephyr.h>
#include <pwm_z42.h>                // Biblioteca personalizada com funções de controle do TPM (Timer/PWM Module)

#define TPM_IRQ_LINE TPM1_IRQn  // relaciona a interrupção ao timer TPM1
#define TPM_IRQ_PRIORITY 1      // define a prioridade da interrupção


#define TPM_MODULE 120         // Define a frequência do PWM fpwm = (TPM_CLK / (TPM_MODULE * PS))
#define INPUT_PORT  "GPIO_1"   // Porta E = GPIO_4 no seu .dts
#define INPUT_PIN   0         // PTB0

volatile uint16_t captured= 0; 

void tpm1_isr(void *arg)
{
       TPM1->STATUS |= TPM_STATUS_CH0F_MASK; // zerra a flag que gerou a interrupção

       captured = TPM1->CONTROLS[0].CnV; // coloca o valor atual do timer na variável "captured"
}

void main(void)
{
    // Inicializa o módulo TPM2 com:
    // - base do TPMx
    // - fonte de clock PLL/FLL (TPM_CLK)
    // - valor do registrador MOD
    // - tipo de clock (TPM_CLK) 
    // - prescaler de 1 a 128 (PS)
    // - modo de operação EDGE_PWM
    pwm_tpm_Init(TPM2, TPM_PLLFLL, 65535, TPM_CLK, PS_128, EDGE_PWM);

    // Inicializa o canal 1 do TPM2 para gerar sinal PWM na porta GPIOB_2
    // - modo TPM_PWM_H (nível alto durante o pulso)
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 2);

    // trigger
    pwm_tpm_CnV(TPM2, 1, TPM_MODULE); // pulso de 10%

    const struct device *input_dev;
    int ret, val;

    // input_dev = device_get_binding(INPUT_PORT);
    // if (!input_dev) {
    //     printk("Erro ao acessar porta %s\n", INPUT_PORT);
    //     return;
    // }

    // ret = gpio_pin_configure(input_dev, INPUT_PIN, GPIO_INPUT);
    // if (ret != 0) {
    //     printk("Erro ao configurar pino %d\n", INPUT_PIN);
    //     return;
    // }

        // Conecta a interrupção via Zephyr
    IRQ_CONNECT(TPM_IRQ_LINE, TPM_IRQ_PRIORITY, tpm1_isr, NULL, 0);
    irq_enable(TPM_IRQ_LINE);

    pwm_tpm_Init(TPM1, TPM_PLLFLL, 65535, TPM_CLK, PS_128, EDGE_PWM);
    pwm_tpm_Ch_Init(TPM1, 0, TPM_INPUT_CAPTURE_FALLING| TPM_CHANNEL_INTERRUPT , GPIOB, 0);

    while (1) {
        printk("Valor do TPM1: %u\n", captured);
        k_msleep(500);
    }
}