import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import firwin, freqz

# =====================================================
# CONFIGURAÇÕES
# =====================================================

FS = 800.0          # taxa de amostragem
F1 = 40.0           # início da banda
F2 = 60.0           # fim da banda

NUM_TAPS = 31       # número de coeficientes FIR

ARQUIVO = "dados.csv"

# =====================================================
# LEITURA DOS DADOS
# =====================================================

dados = np.loadtxt(
    ARQUIVO,
    delimiter=","
)

tempo = dados[:,0]
z = dados[:,1]

# =====================================================
# FFT
# =====================================================

z = z - np.mean(z)

N = len(z)

fft = np.fft.rfft(z)

freq = np.fft.rfftfreq(
    N,
    d=1/FS
)

mag = np.abs(fft)/N

# =====================================================
# PROJETO FIR
# =====================================================

coef = firwin(
    NUM_TAPS,
    [F1, F2],
    fs=FS,
    pass_zero=False,
    window='hamming'
)

# =====================================================
# RESPOSTA EM FREQUÊNCIA
# =====================================================

w, h = freqz(
    coef,
    worN=4096,
    fs=FS
)

# =====================================================
# PLOT FFT
# =====================================================

plt.figure(figsize=(12,6))

plt.plot(freq, mag)

plt.title("FFT do Sinal")

plt.xlabel("Frequência (Hz)")
plt.ylabel("Magnitude")

plt.grid(True)

# =====================================================
# PLOT FILTRO
# =====================================================

plt.figure(figsize=(12,6))

plt.plot(
    w,
    20*np.log10(
        np.maximum(
            np.abs(h),
            1e-10
        )
    )
)

plt.axvline(F1)
plt.axvline(F2)

plt.title("Resposta em Frequência do FIR")

plt.xlabel("Frequência (Hz)")
plt.ylabel("Ganho (dB)")

plt.grid(True)

# =====================================================
# EXPORTAÇÃO C
# =====================================================

print("\n")
print("#define FIR_ORDER", NUM_TAPS)
print("")

print(
    "const float fir_bp_40_60[FIR_ORDER] ="
)
print("{")

for i,c in enumerate(coef):

    if i != NUM_TAPS-1:
        print(
            f"    {c:.9ff},"
        )
    else:
        print(
            f"    {c:.9f}f"
        )

print("};")

# =====================================================
# INFO
# =====================================================

print("")
print(
    f"Atraso do filtro = {(NUM_TAPS-1)/2/FS*1000:.2f} ms"
)

print(
    f"Coeficientes = {NUM_TAPS}"
)

plt.show()