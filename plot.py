import pandas as pd
import matplotlib.pyplot as plt

def plot_cnc_analysis(file_path):
    try:
        # Caricamento robusto
        df = pd.read_csv(
            file_path, 
            sep=None, 
            engine='python', 
            on_bad_lines='skip', 
            skip_blank_lines=True
        )

        # Aggiungiamo 'A' e 'C' alla lista delle colonne da convertire in numeri
        cols = ['t_tot', 'feedrate', 'X', 'Y', 'A', 'C']
        for col in cols:
            if col in df.columns:
                df[col] = pd.to_numeric(df[col], errors='coerce')
        
        # Pulizia dati fondamentali per la serie temporale
        df = df.dropna(subset=['t_tot'])
        df = df.sort_values('t_tot')

    except Exception as e:
        print(f"Errore caricamento: {e}")
        return

    # Creazione della figura con griglia 2x2 per includere gli assi A e C
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 12))

    # --- PLOT 1: FEEDRATE ---
    ax1.plot(df['t_tot'], df['feedrate'], label='Feedrate (mm/s)', 
             color='#1f77b4', linewidth=1.5, marker='o', markersize=2, alpha=0.7)
    ax1.set_title('Profilo di Velocità (Feedrate)')
    ax1.set_xlabel('Tempo (s)')
    ax1.set_ylabel('Velocità (mm/s)')
    ax1.grid(True, linestyle=':', alpha=0.6)
    ax1.legend()

    # --- PLOT 2: TRAIETTORIA XY ---
    ax2.plot(df['X'], df['Y'], label='Percorso XY', 
             color='green', linewidth=1.2, marker='.', markersize=2, alpha=0.5)
    ax2.set_title('Traiettoria XY')
    ax2.set_xlabel('X (mm)')
    ax2.set_ylabel('Y (mm)')
    ax2.axis('equal') 
    ax2.grid(True, linestyle=':', alpha=0.6)
    ax2.legend()

    # --- PLOT 3: ASSE A (Pitch) ---
    if 'A' in df.columns:
        ax3.plot(df['t_tot'], df['A'], label='Asse A (Pitch)', 
                 color='darkorange', linewidth=1.5)
        ax3.set_title('Serie Temporale Asse A')
        ax3.set_xlabel('Tempo (s)')
        ax3.set_ylabel('Angolo (deg)')
        ax3.grid(True, linestyle=':', alpha=0.6)
        ax3.legend()
    else:
        ax3.text(0.5, 0.5, 'Colonna A non trovata', ha='center')

    # --- PLOT 4: ASSE C (Yaw) ---
    if 'C' in df.columns:
        ax4.plot(df['t_tot'], df['C'], label='Asse C (Yaw)',
                 color='purple', linewidth=1.5)
        ax4.set_title('Serie Temporale Asse C')
        ax4.set_xlabel('Tempo (s)')
        ax4.set_ylabel('Angolo (deg)')
        ax4.grid(True, linestyle=':', alpha=0.6)
        ax4.legend()
    else:
        ax4.text(0.5, 0.5, 'Colonna C non trovata', ha='center')

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    plot_cnc_analysis("log.csv")