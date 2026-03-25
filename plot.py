import pandas as pd
import matplotlib.pyplot as plt

def plot_cnc_analysis_fixed(file_path):
    # Carica il file come fa MATLAB con readtable
    # Gestisce automaticamente header e tipi di dati
    try:
        df = pd.read_csv(file_path)
    except Exception as e:
        print(f"Errore caricamento: {e}")
        return

    # Ordina per tempo totale (fondamentale per feedrate e traiettoria)
    df = df.sort_values('t_tot')

    # Creazione figura
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(18, 7))

    # --- PLOT 1: FEEDRATE ---
    # Usiamo i nomi delle colonne come in MATLAB
    ax1.plot(df['t_tot'], df['feedrate'], label='Feedrate (mm/s)', color='#1f77b4')
    
    # Linee cambio blocco (solo dove 'n' è presente e cambia)
    # Riempiamo i buchi di 'n' per identificare i segmenti
    df['n_filled'] = df['n'].ffill() 
    block_changes = df[df['n_filled'].diff() != 0]
    
    for _, row in block_changes.iterrows():
        ax1.axvline(x=row['t_tot'], color='red', linestyle='--', alpha=0.3)

    ax1.set_title('Profilo di Velocità (Corrected)')
    ax1.set_xlabel('Tempo (s)')
    ax1.set_ylabel('Velocità (mm/s)')
    ax1.grid(True, alpha=0.3)

    # --- PLOT 2: TRAIETTORIA XY ---
    ax2.plot(df['X'], df['Y'], label='Percorso', color='green', linewidth=1.5)
    ax2.set_title('Traiettoria XY (Archi inclusi)')
    ax2.set_xlabel('X (mm)')
    ax2.set_ylabel('Y (mm)')
    ax2.axis('equal')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig("cnc_plot_corrected.png")
    print("Plot generato con successo.")

if __name__ == "__main__":
    plot_cnc_analysis_fixed("log.csv")
