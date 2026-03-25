import pandas as pd
import matplotlib.pyplot as plt

def plot_cnc_analysis(file_path):
    try:
        # Caricamento robusto:
        # 1. 'sep=None' con 'engine=python' rileva automaticamente se usi virgole o spazi.
        # 2. 'on_bad_lines=skip' evita il crash se ci sono righe di testo/commenti a metà file.
        # 3. 'skip_blank_lines=True' ignora le righe vuote.
        df = pd.read_csv(
            file_path, 
            sep=None, 
            engine='python', 
            on_bad_lines='skip', 
            skip_blank_lines=True
        )

        # Pulizia: convertiamo in numeri e scartiamo ciò che non lo è
        # Questo risolve il problema delle righe 'n' mancanti negli archi
        cols = ['t_tot', 'feedrate', 'X', 'Y']
        for col in cols:
            if col in df.columns:
                df[col] = pd.to_numeric(df[col], errors='coerce')
        
        # Eliminiamo le righe dove mancano dati fondamentali
        df = df.dropna(subset=['X', 'Y', 't_tot'])

        # ORDINE CRONOLOGICO: Fondamentale per plottare gli archi correttamente
        # Senza questo, il feedrate e la traiettoria potrebbero fare dei salti strani
        df = df.sort_values('t_tot')

    except Exception as e:
        print(f"Errore caricamento: {e}")
        return

    # Creazione della figura
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

    # --- PLOT 1: FEEDRATE ---
    ax1.plot(df['t_tot'], df['feedrate'], label='Feedrate (mm/s)', color='#1f77b4', linewidth=1.5)
    ax1.set_title('Profilo di Velocità (Feedrate)')
    ax1.set_xlabel('Tempo (s)')
    ax1.set_ylabel('Velocità (mm/s)')
    ax1.grid(True, linestyle=':', alpha=0.6)
    ax1.legend()

    # --- PLOT 2: TRAIETTORIA XY ---
    # Plotting continuo: non saltando le righe senza 'n', gli archi vengono disegnati bene
    ax2.plot(df['X'], df['Y'], label='Percorso Ugello', color='green', linewidth=1.2)
    ax2.set_title('Traiettoria XY (Archi corretti)')
    ax2.set_xlabel('X (mm)')
    ax2.set_ylabel('Y (mm)')
    ax2.axis('equal') # Mantiene le proporzioni reali del CNC
    ax2.grid(True, linestyle=':', alpha=0.6)
    ax2.legend()

    plt.tight_layout()
    
    # --- GRAFICO FLOTTANTE ---
    # plt.show() apre la finestra interattiva sul tuo computer
    # senza salvare file nella cartella.
    plt.show()

if __name__ == "__main__":
    # Assicurati che il nome del file sia corretto (es. 'logtrc.csv')
    plot_cnc_analysis("log.csv")
