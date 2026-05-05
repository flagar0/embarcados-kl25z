#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS_GREEN_RED 2000
#define SLEEP_TIME_MS_BLUE 500

// Define o LED usando Device Tree
//#define LED0_NODE DT_ALIAS(led0)
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

void main(void)
{
    int ret;

    // Verifica se o device está pronto
    if (!gpio_is_ready_dt(&ledgreen)) {
        printk("Error: LED device %s is not ready\n", ledgreen.port->name);
        return;
    }

	if (!gpio_is_ready_dt(&ledred)) {
        printk("Error: LED device %s is not ready\n", ledred.port->name);
        return;
    }

	if (!gpio_is_ready_dt(&ledblue)) {
        printk("Error: LED device %s is not ready\n", ledblue.port->name);
        return;
    }

    // Configura o pino como saída
    ret = gpio_pin_configure_dt(&ledred, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }

	ret = gpio_pin_configure_dt(&ledblue, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }

	ret = gpio_pin_configure_dt(&ledgreen, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }


    while (1) {
        // verde
        corVerde();
        k_msleep(SLEEP_TIME_MS_GREEN_RED);
        //amarelo
        corAmarela();
        k_msleep(SLEEP_TIME_MS_BLUE);
        // vermelho
        corVermelho();
        k_msleep(SLEEP_TIME_MS_GREEN_RED);
    }
}

void corVerde(void){
    gpio_pin_set_dt(&ledblue,0);
    gpio_pin_set_dt(&ledred,0);
    gpio_pin_set_dt(&ledgreen,1);
}

void corAmarela(void){
    gpio_pin_set_dt(&ledred,1);
    gpio_pin_set_dt(&ledgreen,1);
    gpio_pin_set_dt(&ledblue,0);
}

void corVermelho(void){
    gpio_pin_set_dt(&ledgreen,0);
    gpio_pin_set_dt(&ledblue,0);
    gpio_pin_set_dt(&ledred,1);
}