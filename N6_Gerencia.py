# FEE247 - Desenvolvimento de Soluções IoT com LoRa e LoRaWAN
#======= Nível 6 - Gerência ============
# Gráficos de RSSI e PSR
import tkinter as tk
from tkinter import messagebox, filedialog
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.style as style

style.use("ggplot")

MAX_PONTOS = 60
cor_rssi_down = "#1f77b4"
cor_rssi_up = "#ff7f0e"
cor_psr = "#2ca02c"

# ===================== ATUALIZA GRÁFICOS =====================
def atualizar_grafico(ax1, ax2, canvas1, canvas2, raiz, label_down, label_up, label_psr):
    rssi_down = []
    rssi_up = []
    psr = []

    # ===================== RSSI =====================
    try:
        with open("N4/Dados_Processados/rssi.tmp", 'r') as f:
            for linha in f:
                linha = linha.strip()
                if linha:
                    try:
                        partes = linha.split()
                        down = float(partes[0].replace(",", "."))
                        up = float(partes[1].replace(",", "."))
                        rssi_down.append(down)
                        rssi_up.append(up)
                    except:
                        pass
    except FileNotFoundError:
        pass

    # ===================== PSR =====================
    try:
        with open("N4/Dados_Processados/psr.tmp", 'r') as f:
            for linha in f:
                linha = linha.strip()
                if linha:
                    try:
                        psr.append(float(linha))
                    except ValueError:
                        pass
    except FileNotFoundError:
        pass

    # ===================== JANELA DESLIZANTE =====================
    rssi_down = rssi_down[-MAX_PONTOS:]
    rssi_up = rssi_up[-MAX_PONTOS:]
    psr = psr[-MAX_PONTOS:]

    # ===================== LABELS =====================
    if rssi_down:
        label_down.config(text="RSSI Downlink atual: " + str(round(rssi_down[-1],2)) + " dBm")
    else:
        label_down.config(text="RSSI Downlink atual: --")

    if rssi_up:
        label_up.config(text="RSSI Uplink atual: " + str(round(rssi_up[-1],2)) + " dBm")
    else:
        label_up.config(text="RSSI Uplink atual: --")

    if psr:
        label_psr.config(text="PSR atual: " + str(round(psr[-1],2)) + " %")
    else:
        label_psr.config(text="PSR atual: --")

    # ===================== GRÁFICO RSSI =====================
    ax1.clear()
    if rssi_down:
        ax1.plot(
            rssi_down,
            label="RSSI Downlink (dBm)",
            linewidth=2,
            marker='o',
            markersize=4,
            color=cor_rssi_down
        )
    if rssi_up:
        ax1.plot(
            rssi_up,
            label="RSSI Uplink (dBm)",
            linewidth=2,
            marker='s',
            markersize=4,
            color=cor_rssi_up
        )
    if rssi_down or rssi_up:
        ax1.legend(loc='upper right')
        todos_rssi = rssi_down + rssi_up
        val_min = min(todos_rssi)
        val_max = max(todos_rssi)
        margem = (val_max - val_min) * 0.10
        if margem == 0:
            margem = 5
        ax1.set_ylim(val_min - margem, val_max + margem)

    ax1.set_title("RSSI LoRa (Downlink / Uplink)")
    ax1.set_ylabel("RSSI (dBm)")
    ax1.set_xlabel("Últimas " + str(MAX_PONTOS) + " medidas")

    # ===================== GRÁFICO PSR =====================
    ax2.clear()
    if psr:
        ax2.plot(
            psr,
            label="PSR (%)",
            linewidth=2,
            marker='o',
            markersize=4,
            color=cor_psr
        )
        ax2.legend(loc='upper right')
        val_min = min(psr)
        val_max = max(psr)
        margem = (val_max - val_min) * 0.10
        if margem == 0:
            margem = 5
        ax2.set_ylim(max(0, val_min - margem), min(105, val_max + margem))

    ax2.set_title("Packet Success Rate - PSR")
    ax2.set_ylabel("PSR (%)")
    ax2.set_xlabel("Últimas " + str(MAX_PONTOS) + " medidas")

    # ===================== ATUALIZA =====================
    canvas1.draw()
    canvas2.draw()
    raiz.after(1000,atualizar_grafico,ax1,ax2,canvas1,canvas2,raiz,label_down,label_up,label_psr)

# ===================== BOTÕES =====================
def fechar():
    if messagebox.askokcancel("Sair","Deseja fechar o monitor?"):
        raiz.destroy()

def salvar(fig1, fig2):
    arquivo = filedialog.asksaveasfilename(defaultextension=".png")
    if arquivo:
        fig1.savefig(arquivo)
        fig2.savefig(arquivo.replace(".png","_psr.png"))

# ===================== INTERFACE =====================
raiz = tk.Tk()
raiz.title("FEE247 - NÍVEL 6 - GERÊNCIA")
raiz.geometry("1000x800")

# ===================== LABELS DAS RSSIS =====================
frame_labels_rssi = tk.Frame(raiz)
frame_labels_rssi.pack(fill="x", padx=10, pady=(10,5))

label_down = tk.Label(frame_labels_rssi,text="RSSI Downlink atual: --",font=("Arial",12,"bold"),bg=cor_rssi_down,fg="white",relief="ridge",bd=3,width=30,pady=8)
label_down.pack(side="left",padx=10)

label_up = tk.Label(frame_labels_rssi,text="RSSI Uplink atual: --",font=("Arial",12,"bold"),bg=cor_rssi_up,fg="white",relief="ridge",bd=3,width=30,pady=8)
label_up.pack(side="left",padx=10)

# ===================== GRÁFICO DAS RSSIS =====================
frame_rssi = tk.Frame(raiz)
frame_rssi.pack(fill="both", expand=True, padx=10, pady=5)

fig1 = Figure(figsize=(10, 3))
ax1 = fig1.add_subplot(111)
canvas1 = FigureCanvasTkAgg(fig1,master=frame_rssi)
canvas1.get_tk_widget().pack(fill="both",expand=True)

# ===================== LABEL DA PSR =====================
frame_label_psr = tk.Frame(raiz)
frame_label_psr.pack(fill="x", padx=10, pady=5)

label_psr = tk.Label(frame_label_psr,text="PSR atual: --",font=("Arial",12,"bold"),bg=cor_psr,fg="white",relief="ridge",bd=3,width=25,pady=8)
label_psr.pack(side="left",padx=10)

# ===================== GRÁFICO DA PSR =====================
frame_psr = tk.Frame(raiz)
frame_psr.pack(fill="both", expand=True, padx=10, pady=5)

fig2 = Figure(figsize=(10, 3))
ax2 = fig2.add_subplot(111)
canvas2 = FigureCanvasTkAgg(fig2,master=frame_psr)
canvas2.get_tk_widget().pack(fill="both",expand=True)

btn = tk.Button(raiz,text="Salvar Gráficos",command=lambda: salvar(fig1,fig2))
btn.pack(pady=5)

atualizar_grafico(ax1,ax2,canvas1,canvas2,raiz,label_down,label_up,label_psr)
raiz.protocol("WM_DELETE_WINDOW",fechar)

raiz.mainloop()
