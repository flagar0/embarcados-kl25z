#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* LED via Device Tree (alias "led0") */
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(LED_NODE, gpios, {0});

/* Semáforo: valor inicial = 0 (bloqueado), máximo = 1 */
K_SEM_DEFINE(led_sem, 0, 1);

/* -------- THREAD A: espera o semáforo e acende o LED -------- */
void thread_a(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while (1) {
        printk("Thread A: esperando semáforo (count = %u)\n",
               k_sem_count_get(&led_sem));

        /* Bloqueia até que alguém libere o semáforo */
        k_sem_take(&led_sem, K_FOREVER);

        printk("Thread A: semáforo tomado! (count = %u)\n",
               k_sem_count_get(&led_sem));

        /* Acende LED por 500 ms */
        if (device_is_ready(led.port)) {
            gpio_pin_set_dt(&led, 1);
            k_msleep(500);
            gpio_pin_set_dt(&led, 0);
        } else {
            printk("Thread A: LED não disponível\n");
        }
    }
}

/* -------- THREAD B: libera o semáforo periodicamente -------- */
void thread_b(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while (1) {
        k_msleep(2000); /* espera 2 s */

        printk("Thread B: antes do give (count = %u)\n",
               k_sem_count_get(&led_sem));

        k_sem_give(&led_sem);

        printk("Thread B: depois do give (count = %u)\n",
               k_sem_count_get(&led_sem));
    }
}

/* Definição das threads (pilha, função, prioridades) */
K_THREAD_DEFINE(threadA_id, 512, thread_a, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(threadB_id, 512, thread_b, NULL, NULL, NULL, 5, 0, 0);

/* -------- Função main -------- */
int main(void)
{
    /* Verifica e configura LED (se existir) */
    if (!device_is_ready(led.port)) {
        printk("Atenção: LED não pronto (alias led0 ausente?). Continuando sem LED.\n");
    } else {
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    }

    printk("=== Exemplo de Semáforo: k_sem_take / k_sem_give ===\n");
    /* main pode retornar; threads continuam rodando */
    return 0;
}
