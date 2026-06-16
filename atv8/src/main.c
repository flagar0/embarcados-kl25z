#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>


// === Endereço e registradores do MMA8451Q ===
#define MMA8451Q_I2C_ADDR    0x1D
#define MMA8451Q_CTRL_REG1   0x2A

// === Bits de configuração ===
#define MMA8451Q_ACTIVE_BIT  0x01
#define MMA8451Q_ODR   (0x0 << 0)  // 800 Hz conforme datasheet (DR=111b)

// DR2 DR1 DR0 ODR Period
// 0 0 0 800 Hz 1.25 ms

K_SEM_DEFINE(dado_coletado, 0, 1);  // Padeiro pode começar
K_MUTEX_DEFINE(valor_x);

volatile struct sensor_value accel_x, accel_y, accel_z;
static const struct device *const accel = DEVICE_DT_GET(DT_NODELABEL(mma8451q));
static const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

volatile float z_filtrado;

#define FIR_ORDER 31

const float fir_bp_40_60[FIR_ORDER] =
{
 -0.001315f,
 -0.003136f,
 -0.005294f,
 -0.006169f,
 -0.003126f,
  0.005883f,
  0.020015f,
  0.034313f,
  0.040344f,
  0.029000f,
 -0.005013f,
 -0.059875f,
 -0.125651f,
 -0.187042f,
 -0.228668f,
  0.755000f,
 -0.228668f,
 -0.187042f,
 -0.125651f,
 -0.059875f,
 -0.005013f,
  0.029000f,
  0.040344f,
  0.034313f,
  0.020015f,
  0.005883f,
 -0.003126f,
 -0.006169f,
 -0.005294f,
 -0.003136f,
 -0.001315f
};


static float z_buffer[FIR_ORDER] = {0};
static uint32_t fir_index = 0;

float fir_filter_z(float sample)
{
    z_buffer[fir_index] = sample;

    float y = 0.0f;

    int idx = fir_index;

    for(int i = 0; i < FIR_ORDER; i++)
    {
        y += fir_bp_40_60[i] * z_buffer[idx];

        idx--;

        if(idx < 0)
            idx = FIR_ORDER - 1;
    }

    fir_index++;

    if(fir_index >= FIR_ORDER)
        fir_index = 0;

    return y;
}

void mma8451q_configurar_odr(void)
{
    uint8_t buf[2];
    int ret;

    // 1️⃣ Colocar o sensor em standby (necessário antes de mudar ODR)
    buf[0] = MMA8451Q_CTRL_REG1;
    buf[1] = 0x00;
    ret = i2c_write(i2c_dev, buf, 2, MMA8451Q_I2C_ADDR);
    if (ret) {
        printk("ERRO ao colocar MMA8451Q em standby (%d)\n", ret);
        return;
    }

    // 2️⃣ Configurar ODR = 800 Hz (bits DR[5:3] = 111)
    buf[0] = MMA8451Q_CTRL_REG1;
    buf[1] = MMA8451Q_ODR;
    ret = i2c_write(i2c_dev, buf, 2, MMA8451Q_I2C_ADDR);
    if (ret) {
        printk("ERRO ao configurar ODR (%d)\n", ret);
        return;
    }

    // 3️⃣ Ativar o sensor novamente
    buf[0] = MMA8451Q_CTRL_REG1;
    buf[1] = MMA8451Q_ODR | MMA8451Q_ACTIVE_BIT;
    ret = i2c_write(i2c_dev, buf, 2, MMA8451Q_I2C_ADDR);
    if (ret) {
        printk("ERRO ao ativar MMA8451Q (%d)\n", ret);
        return;
    }

    printk("MMA8451Q configurado para 800 Hz via I2C.\n");
}


/* -------- THREAD A: acelerometro -------- */
void thread_a(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    int ret;
    // Verificar se o dispositivo está pronto
    // if (!device_is_ready(accel)) {
    //     printk("ERRO: Acelerometro nao esta pronto!\n");
    //     return;
    // }

    if (!device_is_ready(i2c_dev)) {
        printk("I2C não está pronto!\n");
        return;
    }

    printk("Configurando ODR via I2C...\n");
    mma8451q_configurar_odr();
    
    if (ret) {
        printk("Erro configurando ODR: %d\n", ret);
    }
        printk("Acelerometro inicializado com sucesso!\n"); 

    // Pequeno delay antes de começar a enviar dados
    k_msleep(1000);

    while (1) {
        // Solicitar leitura do sensor
        ret = sensor_sample_fetch(accel);
        if (ret) {
            printk("Erro ao ler sensor: %d\n", ret);
            k_msleep(500);
            continue;
        } else{
            //k_mutex_lock(&valor_x, K_FOREVER);
            sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &accel_x);
            sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &accel_y);
            sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &accel_z);

            float z =
                accel_z.val1 +
                accel_z.val2 / 1000000.0f;

            z_filtrado = fir_filter_z(z);
            //k_mutex_unlock(&valor_x);
            k_sem_give(&dado_coletado); // coletou e incrementa
        }
        // Obter valores dos eixos X, Y e Z
        //k_mutex_lock(&valor_x, K_FOREVER);

    }
}

/* -------- THREAD B: comunicacao print -------- */
void thread_b(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    printk("Comunicacao iniciada\n");
    int64_t timestamp_ms;
    while (1) {
        timestamp_ms = k_uptime_get();
        k_sem_take(&dado_coletado, K_FOREVER); // espera coletar algum dado
        //k_mutex_lock(&valor_x, K_FOREVER);
        printk("Time: %lli, X: %d.%06d, Y: %.6f, Z: %d.%06d\r\n", 
        timestamp_ms,
        accel_x.val1, abs(accel_x.val2),
        (double)z_filtrado,
        accel_z.val1, abs(accel_z.val2));

        //k_mutex_unlock(&valor_x);
    }
}

/* Definição das threads (pilha, função, prioridades) */
K_THREAD_DEFINE(threadAcce, 512, thread_a, NULL, NULL, NULL, 2, 0, 0);
K_THREAD_DEFINE(threadComu, 512, thread_b, NULL, NULL, NULL, 5, 0, 0);


/* -------- Função main -------- */
int main(void)
{

    
    return 0;
}