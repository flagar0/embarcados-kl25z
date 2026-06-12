#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* LED via Device Tree (alias "led0") */
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(LED_NODE, gpios, {0});

volatile int saldo_vitrine = 0;

K_MUTEX_DEFINE(saldo);

K_SEM_DEFINE(pode_produzir, 0, 10);  // Padeiro pode começar
K_SEM_DEFINE(pao_comidos, 0, 10);  // Cliente espera

/* -------- THREAD A: padeiro -------- */
void thread_a(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    printk("Padeiro trabalhando\n");
    while (1) {
        k_msleep(1000); // 1seg
        //k_sem_take(&pode_produzir, K_FOREVER);
        if (saldo_vitrine < 10) { //maixmo de10 poes
        k_mutex_lock(&saldo, K_FOREVER);
        saldo_vitrine++;
        printk("Saldo vitrine: %i\n",saldo_vitrine);
        k_mutex_unlock(&saldo);
        k_sem_give(&pode_produzir);
        }
        //printk("smeaforo %u\n",k_sem_count_get(&pode_produzir));
    }
}

/* -------- THREAD B: cliente -------- */
void thread_b(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    printk("Cliente comprando\n");
    while (1) {
        k_msleep(1500); // 1.5seg
        k_sem_take(&pode_produzir, K_FOREVER); // espera ter algum pao
        k_mutex_lock(&saldo, K_FOREVER);
        saldo_vitrine--;
        printk("Saldo vitrine: %i\n",saldo_vitrine);
        printk("Pao comprado\n");
        k_mutex_unlock(&saldo);


    }
}

/* Definição das threads (pilha, função, prioridades) */
K_THREAD_DEFINE(threadPadeiro, 512, thread_a, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(threadCliente, 512, thread_b, NULL, NULL, NULL, 5, 0, 0);


/* -------- Função main -------- */
int main(void)
{
    while (1){
        if(saldo_vitrine == 0){
            printk("Sem poes no estoque\n");
        }
        k_msleep(500);
    }
}
