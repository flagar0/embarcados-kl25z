import serial
import threading
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque

# ==================================
# CONFIGURAÇÃO
# ==================================

PORTA = "COM17"
BAUDRATE = 115200

N_PONTOS = 8000

# ==================================
# SERIAL
# ==================================

ser = serial.Serial(
    PORTA,
    BAUDRATE,
    timeout=0.01
)

# ==================================
# BUFFERS
# ==================================

tempo = deque(maxlen=N_PONTOS)

ax = deque(maxlen=N_PONTOS)
ay = deque(maxlen=N_PONTOS)
az = deque(maxlen=N_PONTOS)

tempo_freq = deque(maxlen=N_PONTOS)
freq_inst = deque(maxlen=N_PONTOS)

ultimo_time = None

# ==================================
# THREAD DE LEITURA
# ==================================

def serial_thread():

    global ultimo_time

    while True:

        try:

            linha = ser.readline()

            if not linha:
                continue

            linha = linha.decode(
                errors="ignore"
            ).strip()

            if "Time:" not in linha:
                continue

            # Exemplo:
            # Time: 1234, X: 0.123456, Y: 0.654321, Z: 1.000000

            partes = linha.split(",")

            if len(partes) != 4:
                continue

            t_ms = int(
                partes[0].split(":")[1]
            )

            x = float(
                partes[1].split(":")[1]
            )

            y = float(
                partes[2].split(":")[1]
            )

            z = float(
                partes[3].split(":")[1]
            )

            t_s = t_ms / 1000.0

            tempo.append(t_s)

            ax.append(x)
            ay.append(y)
            az.append(z)

            if ultimo_time is not None:

                dt_ms = t_ms - ultimo_time

                if dt_ms > 0:

                    freq = 1000.0 / dt_ms

                    if 0 < freq < 5000:

                        freq_inst.append(freq)
                        tempo_freq.append(t_s)

            ultimo_time = t_ms

        except Exception:
            pass


threading.Thread(
    target=serial_thread,
    daemon=True
).start()

# ==================================
# FIGURA
# ==================================

fig, axs = plt.subplots(
    2,
    1,
    figsize=(12, 8)
)

linha_ax, = axs[0].plot([], [], label="X")
linha_ay, = axs[0].plot([], [], label="Y")
linha_az, = axs[0].plot([], [], label="Z")

linha_freq, = axs[1].plot([], [], label="Hz")

axs[0].set_title("Acelerometro")
axs[0].set_ylabel("g")
axs[0].legend()
axs[0].grid(True)

axs[1].set_title("Taxa de aquisicao")
axs[1].set_ylabel("Hz")
axs[1].set_xlabel("Tempo (s)")
axs[1].grid(True)

# ==================================
# UPDATE DO GRAFICO
# ==================================

def atualizar(frame):

    if len(tempo) < 2:
        return

    linha_ax.set_data(tempo, ax)
    linha_ay.set_data(tempo, ay)
    linha_az.set_data(tempo, az)

    if len(freq_inst) > 0:

        linha_freq.set_data(
            tempo_freq,
            freq_inst
        )

    # janela móvel
    axs[0].set_xlim(
        tempo[0],
        tempo[-1]
    )

    if len(tempo_freq) > 2:

        axs[1].set_xlim(
            tempo_freq[0],
            tempo_freq[-1]
        )

    # limites Y acelerômetro

    valores = (
        list(ax)
        + list(ay)
        + list(az)
    )

    ymin = min(valores)
    ymax = max(valores)

    margem = 0.1

    axs[0].set_ylim(
        ymin - margem,
        ymax + margem
    )

    # limites frequência

    if len(freq_inst) > 5:

        ymin = min(freq_inst)
        ymax = max(freq_inst)

        axs[1].set_ylim(
            ymin * 0.95,
            ymax * 1.05
        )

        freq_media = (
            len(tempo)
            /
            (tempo[-1] - tempo[0])
        )

        axs[1].set_title(
            f"Taxa de aquisicao | Media = {freq_media:.1f} Hz"
        )

# ==================================
# ANIMAÇÃO
# ==================================

ani = FuncAnimation(
    fig,
    atualizar,
    interval=200,
    cache_frame_data=False
)

plt.tight_layout()
plt.show()

dados = np.column_stack(
    (
        np.array(tempo),
        np.array(az)
    )
)

np.savetxt(
    "dados.csv",
    dados,
    delimiter=","
)

# ==================================
# JANELA FECHOU -> FFT
# ==================================

print("\nCalculando FFT...")

if len(az) > 100:

    sinal = np.array(az)

    # remove componente DC
    sinal = sinal - np.mean(sinal)

    # taxa média de aquisição
    tempo_np = np.array(tempo)

    fs = (len(tempo_np) - 1) / (tempo_np[-1] - tempo_np[0])

    print(f"Taxa média de aquisição: {fs:.2f} Hz")

    N = len(sinal)

    janela = np.hanning(N)

    fft = np.fft.rfft(
        sinal * janela
    )

    freq = np.fft.rfftfreq(
        N,
        d=1/fs
    )

    magnitude = np.abs(fft) / N

    indices = np.argsort(magnitude)[-10:]

    print("\n10 maiores componentes:")

    for i in reversed(indices):

        print(
            f"{freq[i]:8.2f} Hz  "
            f"{magnitude[i]:.6f}"
        )

    plt.figure(figsize=(12, 6))

    plt.plot(freq, magnitude)

    plt.title(
        f"FFT do eixo z (Fs = {fs:.2f} Hz)"
    )

    plt.xlabel("Frequência (Hz)")
    plt.ylabel("Magnitude")

    plt.grid(True)

    plt.tight_layout()

    plt.show()

else:

    print("Poucos dados para FFT.")

