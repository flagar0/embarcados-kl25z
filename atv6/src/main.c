#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>

/* LED via Device Tree (alias "led0") */
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(LED_NODE, gpios, {0});
// interrupcao botao pino PTA16
#define BUTTON_NODE DT_NODELABEL(user_button_0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

//ADC
#define ADC_RESOLUTION      8
#define ADC_GAIN            ADC_GAIN_1
#define ADC_REFERENCE       ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT
#define ADC_CHANNEL_ID      9  //Canal do ADC, veja a pinagem
#define ADC_VREF_MV         3300

static int16_t sample_buffer;

// Obter referência do dispositivo usando o nodelabel
static const struct device *const accel = DEVICE_DT_GET(DT_NODELABEL(mma8451q));

volatile bool botao_interrupt = false;


/* -------- THREAD A: leitura do ADC -------- */
void thread_a(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    //ADC
    const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));
    if (!device_is_ready(adc_dev)) {
        printk("ADC não está pronto\n");
        return;
    }

    struct adc_channel_cfg channel_cfg = {
        .gain = ADC_GAIN,
        .reference = ADC_REFERENCE,
        .acquisition_time = ADC_ACQUISITION_TIME,
        .channel_id = ADC_CHANNEL_ID,
        .differential = 0,
    };

    // if (adc_channel_setup(adc_dev, &channel_cfg) != 0) {
    //     printk("Erro ao configurar canal ADC\n");
    //     return;
    // }

    struct adc_sequence sequence = {
        .channels    = BIT(ADC_CHANNEL_ID),
        .buffer      = &sample_buffer,
        .buffer_size = sizeof(sample_buffer),
        .resolution  = ADC_RESOLUTION,
    };

    while (1) {
        //ADC
         int err = adc_read(adc_dev, &sequence);
        if (err != 0) {
            printk("Falha na leitura do ADC: %d\n", err);
        } else {
            int32_t mv = sample_buffer;
            adc_raw_to_millivolts(ADC_VREF_MV, ADC_GAIN, ADC_RESOLUTION, &mv);
            printk("ADC: %d (raw), %d mV\n", sample_buffer, mv);
        }
        //espera 500 ms
        k_msleep(500);
    }
}

/* -------- THREAD B: acelerometreo -------- */
void thread_b(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    struct sensor_value accel_x, accel_y, accel_z;
    int ret;
    uint32_t tempo_ms = 0;  // Contador de tempo em milissegundos

    printk("\n");
    printk("========================================\n");
    printk("  FRDM-KL25Z - Acelerometro MMA8451Q\n");
    printk("========================================\n");
    printk("I2C0: PTE24 (SCL), PTE25 (SDA)\n");
    printk("========================================\n\n");

    // Verificar se o dispositivo está pronto
    if (!device_is_ready(accel)) {
        printk("ERRO: Acelerometro nao esta pronto!\n");
        return;
    }

    printk("Acelerometro inicializado com sucesso!\n");
    printk("Iniciando leituras a cada 1s...\n\n");
    
    // Pequeno delay antes de começar a enviar dados
    k_msleep(1000);

    while (1) {
        // Solicitar leitura do sensor
        ret = sensor_sample_fetch(accel);
        if (ret) {
            printk("Erro ao ler sensor: %d\n", ret);
            k_msleep(500);
            tempo_ms += 500;
            continue;
        }

        // Obter valores dos eixos X, Y e Z
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &accel_x);
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &accel_y);
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &accel_z);

        // Formato: T: tempo_ms, X: valor, Y: valor, Z: valor
        printk("T: %u, X: %d.%06d, Y: %d.%06d, Z: %d.%06d\r\n", 
        tempo_ms,
        accel_x.val1, abs(accel_x.val2),
        accel_y.val1, abs(accel_y.val2),
        accel_z.val1, abs(accel_z.val2));

        // Aguardar 1000ms antes da próxima leitura
        k_msleep(1000);
        tempo_ms += 1000;
    }
}

/* Definição das threads (pilha, função, prioridades) */
K_THREAD_DEFINE(threadA_id, 512, thread_a, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(threadB_id, 512, thread_b, NULL, NULL, NULL, 5, 0, 0);

// funcao do butao
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    botao_interrupt = !botao_interrupt;
    if(botao_interrupt){
        k_thread_suspend(threadB_id);
    }else{
        k_thread_resume(threadB_id);
    }
    //so pegar uma vez
    //k_msleep(100);
}

/* -------- Função main -------- */
int main(void)
{
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);

    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    return 0;
}
