# FEE247 - Desenvolvimento de Soluções IoT com LoRa e LoRaWAN
#======= Nível 6 - Aplicação ============
# Gráfico da luminosidade e da média da luminosidade
import tkinter as tk
from tkinter import messagebox, filedialog
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.style as style

style.use("ggplot")

MAX_PONTOS = 60
cor_luminosidade = "#1f77b4"
cor_media = "#ff7f0e"

# ===================== ATUALIZA GRÁFICO =====================
def atualizar_grafico(ax1, canvas, raiz, label_lum, label_media):
    valores_lum = []
    valores_media = []

    # ===================== LUMINOSIDADE =====================
    try:
        with open("N4/Dados_Processados/luminosidade.tmp", 'r') as f:
            for linha in f:
                linha = linha.strip()
                if linha:
                    try:
                        valores_lum.append(float(linha))
                    except ValueError:
                        pass
    except FileNotFoundError:
        pass

    # ===================== MÉDIA DA LUMINOSIDADE =====================
    try:
        with open("N4/Dados_Processados/media_luminosidade.txt", 'r') as f:
            for linha in f:
                linha = linha.strip()
                if linha:
                    try:
                        valores_media.append(float(linha))
                    except ValueError:
                        pass
    except FileNotFoundError:
        valores_media = valores_lum.copy()

    # Ajusta tamanhos diferentes
    if len(valores_media) < len(valores_lum):
        if len(valores_media) > 0:
            ultimo = valores_media[-1]
            while len(valores_media) < len(valores_lum):
                valores_media.append(ultimo)
        else:
            valores_media = valores_lum.copy()

    # ===================== JANELA DESLIZANTE =====================
    valores_lum = valores_lum[-MAX_PONTOS:]
    valores_media = valores_media[-MAX_PONTOS:]

    # ===================== LABELS =====================
    if valores_lum:
        label_lum.config(text="Luminosidade atual: " + str(valores_lum[-1]))
    else:
        label_lum.config(text="Luminosidade atual: --")

    if valores_media:
        label_media.config(text="Média atual: " + str(round(valores_media[-1],2)))
    else:
        label_media.config(text="Média atual: --")

    # ===================== GRÁFICO =====================
    ax1.clear()
    if valores_lum:
        ax1.plot(
            valores_lum,
            label="Luminosidade",
            linewidth=2,
            marker='o',
            markersize=4,
            color=cor_luminosidade
        )
        ax1.plot(
            valores_media,
            label="Média",
            linewidth=3,
            color=cor_media
        )
        ax1.legend(loc='upper right')
        todos = valores_lum + valores_media
        val_min = min(todos)
        val_max = max(todos)
        margem = (val_max - val_min) * 0.10
        if margem == 0:
            margem = 100
        ax1.set_ylim(max(0, val_min - margem),min(4095, val_max + margem))

    ax1.set_title("Luminosidade e Média")
    ax1.set_ylabel("Intensidade (0-4095)")
    ax1.set_xlabel("Últimas " + str(MAX_PONTOS) + " medidas")

    # ===================== ATUALIZA =====================
    canvas.draw()
    raiz.after(1000,atualizar_grafico,ax1,canvas,raiz,label_lum,label_media)

# ===================== BOTÕES =====================
def fechar():
    if messagebox.askokcancel("Sair","Deseja fechar o monitor?"):
        raiz.destroy()

def salvar(fig):
    arquivo = filedialog.asksaveasfilename(defaultextension=".png")
    if arquivo:
        fig.savefig(arquivo)

# ===================== INTERFACE =====================
raiz = tk.Tk()
raiz.title("FEE247 - NÍVEL 6 - APLICAÇÃO")
raiz.geometry("1000x650")

frame_labels = tk.Frame(raiz)
frame_labels.pack(fill="x", padx=10, pady=10)

label_lum = tk.Label(frame_labels,text="Luminosidade atual: --",font=("Arial",12,"bold"),bg=cor_luminosidade,fg="white",relief="ridge",bd=3,width=28,pady=8)
label_lum.pack(side="left",padx=10)

label_media = tk.Label(frame_labels,text="Média atual: --",font=("Arial",12,"bold"),bg=cor_media,fg="white",relief="ridge",bd=3,width=28,pady=8)
label_media.pack(side="left",padx=10)

frame = tk.Frame(raiz)
frame.pack(fill="both", expand=True)

fig = Figure(figsize=(10, 5))
ax1 = fig.add_subplot(111)

canvas = FigureCanvasTkAgg(fig,master=frame)
canvas.get_tk_widget().pack(fill="both",expand=True)

btn = tk.Button(raiz,text="Salvar Gráfico",command=lambda: salvar(fig))
btn.pack(pady=5)

atualizar_grafico(ax1,canvas,raiz,label_lum,label_media)
raiz.protocol("WM_DELETE_WINDOW",fechar)

raiz.mainloop()
