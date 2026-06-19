from scipy.signal import firwin, freqz
import numpy as np
import matplotlib.pyplot as plt

# ============================================
# CONFIGURAÇÃO
# ============================================

FS = 800.0
FC = 30.0

NUM_TAPS = 51

# ============================================
# FIR PASSA-BAIXA
# ============================================

coef = firwin(
    NUM_TAPS,
    FC,
    fs=FS,
    pass_zero=True,
    window='hamming'
)

# ============================================
# RESPOSTA EM FREQUÊNCIA
# ============================================

w, h = freqz(
    coef,
    worN=4096,
    fs=FS
)

plt.figure(figsize=(12,6))

plt.plot(
    w,
    20*np.log10(
        np.maximum(np.abs(h),1e-10)
    )
)

plt.axvline(
    FC,
    linestyle='--'
)

plt.title(
    f'FIR Passa-Baixa {FC} Hz'
)

plt.xlabel('Frequência (Hz)')
plt.ylabel('Ganho (dB)')
plt.grid(True)

plt.show()

# ============================================
# EXPORTAÇÃO C
# ============================================

print("\n")
print("#define FIR_ORDER", NUM_TAPS)
print("")

print(
    "const float fir_bp_40_60[FIR_ORDER] ="
)
print("{")

for i, c in enumerate(coef):

    if i != NUM_TAPS - 1:
        print(
            f"    {c:.9f}f,"
        )
    else:
        print(
            f"    {c:.9f}f"
        )

print("};")

print(
    f"Atraso = {(NUM_TAPS-1)/2/FS*1000:.2f} ms"
)