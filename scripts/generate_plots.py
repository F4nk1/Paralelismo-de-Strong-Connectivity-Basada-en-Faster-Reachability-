import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np
import seaborn as sns
import shutil

# Configuración de estilo
sns.set_theme(style="whitegrid")
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['font.sans-serif'] = ['Arial', 'DejaVu Sans']

def generate_plots():
    os.makedirs('results/plots', exist_ok=True)
    
    # 1. Escalabilidad: Speedup y Eficiencia
    scalability_csv = 'results/csv/scalability.csv'
    if os.path.exists(scalability_csv):
        print("[1/7] Generando gráficos de escalabilidad...")
        df = pd.read_csv(scalability_csv)
        df_p = df[df['algorithm'].isin(['BGSS-BigSCC', 'BGSS-BigSCC-HashBag', 'BGSS-MultiSearch', 'VGC'])]
        
        if not df_p.empty:
            # Speedup
            plt.figure(figsize=(12, 7))
            sns.lineplot(data=df_p, x='threads', y='speedup', hue='graph', style='algorithm', markers=True, dashes=False, markersize=8)
            threads = sorted(df['threads'].unique())
            plt.plot(threads, threads, 'k--', alpha=0.5, label='Ideal')
            plt.xlabel('Número de Hilos', fontsize=12)
            plt.ylabel('Aceleración (Speedup)', fontsize=12)
            plt.title('Análisis de Escalabilidad: Aceleración Paralela', fontsize=14, fontweight='bold')
            plt.legend(title='Grafo / Algoritmo', bbox_to_anchor=(1.05, 1), loc='upper left')
            plt.tight_layout()
            plt.savefig('results/plots/speedup.png', dpi=300)
            plt.close()

            # Eficiencia
            plt.figure(figsize=(12, 7))
            sns.lineplot(data=df_p, x='threads', y='efficiency', hue='graph', style='algorithm', markers=True, dashes=False, markersize=8)
            plt.axhline(y=1.0, color='r', linestyle='--', alpha=0.3, label='100% Eficiencia')
            plt.xlabel('Número de Hilos', fontsize=12)
            plt.ylabel('Eficiencia', fontsize=12)
            plt.title('Análisis de Escalabilidad: Eficiencia Paralela', fontsize=14, fontweight='bold')
            plt.legend(title='Grafo / Algoritmo', bbox_to_anchor=(1.05, 1), loc='upper left')
            plt.ylim(0, 1.2)
            plt.tight_layout()
            plt.savefig('results/plots/efficiency.png', dpi=300)
            plt.close()

    # 2. Desglose de Ejecución (Breakdown)
    breakdown_csv = 'results/csv/breakdown.csv'
    if os.path.exists(breakdown_csv):
        print("[2/7] Generando desglose de ejecución...")
        df = pd.read_csv(breakdown_csv)
        # Solo para algoritmos que realmente tienen breakdown (BGSS-*, MultiSearch, VGC)
        df_bd = df[df['algorithm'].isin(['BGSS-BigSCC', 'BGSS-BigSCC-HashBag', 'BGSS-MultiSearch', 'VGC'])]
        if not df_bd.empty:
            df_last = df_bd.groupby(['algorithm', 'graph']).last().reset_index()
            
            phases = ['trimming', 'forward_reach', 'backward_reach', 'reachability', 'labeling', 'hash_bag', 'big_scc', 'others']
            spanish_phases = ['Trimming', 'Alcance FW', 'Alcance BW', 'Alcance', 'Etiquetado', 'HashBag', 'Big SCC', 'Otros']
            
            for algo in df_last['algorithm'].unique():
                subset = df_last[df_last['algorithm'] == algo]
                plot_data = subset[['graph'] + phases].copy()
                plot_data.columns = ['Grafo'] + spanish_phases
                plot_data = plot_data.set_index('Grafo')
                
                # Filtrar solo columnas con datos
                plot_data = plot_data.loc[:, (plot_data != 0).any(axis=0)]
                
                if not plot_data.empty:
                    plot_data.plot(kind='barh', stacked=True, figsize=(12, 6), colormap='viridis')
                    plt.xlabel('Tiempo (ms)', fontsize=12)
                    plt.title(f'Desglose de Tiempo: {algo}', fontsize=14, fontweight='bold')
                    plt.legend(title='Fases', bbox_to_anchor=(1.05, 1), loc='upper left')
                    plt.tight_layout()
                    plt.savefig(f'results/plots/breakdown_{algo.lower().replace("-","_")}.png', dpi=300)
                    plt.close()
                else:
                    print(f"Aviso: Sin datos de desglose para {algo}")

    # 3. Comparativas generales
    bench_csv = 'results/csv/benchmarks.csv'
    if os.path.exists(bench_csv):
        print("[3/7] Generando comparativas de rendimiento...")
        df = pd.read_csv(bench_csv)
        max_threads = df['threads'].max()
        comp_df = df[(df['algorithm'] == 'Tarjan') | (df['threads'] == max_threads)]
        comp_df = comp_df[~comp_df['algorithm'].isin(['BGSS (Vector)', 'BGSS (HashBag)'])]

        # Comparison Total
        plt.figure(figsize=(14, 8))
        ax = sns.barplot(data=comp_df, x='graph', y='time_ms', hue='algorithm', palette='muted')
        if comp_df['time_ms'].max() / (comp_df['time_ms'].min() + 0.1) > 50:
            ax.set_yscale("log")
        plt.ylabel('Tiempo (ms)', fontsize=12)
        plt.title(f'Comparativa: Tarjan vs Paralelos ({max_threads} hilos)', fontsize=14, fontweight='bold')
        plt.xticks(rotation=45)
        plt.tight_layout()
        plt.savefig('results/plots/comparison_total.png', dpi=300)
        plt.close()

        # Throughput
        plt.figure(figsize=(14, 8))
        sns.barplot(data=df[df['threads'] == max_threads], x='graph', y='throughput', hue='algorithm', palette='magma')
        plt.ylabel('Rendimiento (M-edges/s)', fontsize=12)
        plt.title('Rendimiento (Throughput) por Algoritmo', fontsize=14, fontweight='bold')
        plt.xticks(rotation=45)
        plt.tight_layout()
        plt.savefig('results/plots/throughput.png', dpi=300)
        plt.close()

        # Runtime vs Threads
        plt.figure(figsize=(12, 7))
        df_lines = df[df['algorithm'].isin(['BGSS-BigSCC', 'BGSS-BigSCC-HashBag', 'BGSS-MultiSearch', 'VGC'])]
        sns.lineplot(data=df_lines, x='threads', y='time_ms', hue='graph', style='algorithm', markers=True)
        plt.yscale("log")
        plt.ylabel('Tiempo (ms) [Log]', fontsize=12)
        plt.title('Tiempo de Ejecución vs. Número de Hilos', fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.savefig('results/plots/runtime_vs_threads.png', dpi=300)
        plt.close()

    # 4. Análisis de Tau (VGC)
    tau_csv = 'results/csv/tau_analysis.csv'
    if os.path.exists(tau_csv):
        print("[4/7] Generando análisis de parámetro Tau...")
        df_tau = pd.read_csv(tau_csv)
        if not df_tau.empty:
            plt.figure(figsize=(12, 7))
            sns.lineplot(data=df_tau, x='tau', y='time_ms', hue='graph', markers=True, style='graph')
            plt.xscale('log', base=2)
            plt.xlabel('Parámetro Tau', fontsize=12)
            plt.ylabel('Tiempo (ms)', fontsize=12)
            plt.title('Impacto del Parámetro Tau en el Rendimiento (VGC)', fontsize=14, fontweight='bold')
            plt.tight_layout()
            plt.savefig('results/plots/tau_analysis.png', dpi=300)
            plt.close()

    # 5. HashBag vs Vector
    hb_csv = 'results/csv/hashbag_vs_vector.csv'
    if os.path.exists(hb_csv):
        print("[5/7] Generando comparativa HashBag vs Vector...")
        df_hb = pd.read_csv(hb_csv)
        if not df_hb.empty:
            plt.figure(figsize=(12, 7))
            sns.lineplot(data=df_hb, x='threads', y='time_ms', hue='structure', style='graph', markers=True, markersize=8)
            plt.yscale("log")
            plt.xlabel('Número de Hilos', fontsize=12)
            plt.ylabel('Tiempo de Ejecución (ms)', fontsize=12)
            plt.title('Comparativa de Estructuras: std::vector vs HashBag', fontsize=14, fontweight='bold')
            plt.legend(title='Estructura / Grafo', bbox_to_anchor=(1.05, 1), loc='upper left')
            plt.tight_layout()
            plt.savefig('results/plots/hashbag_vs_vector.png', dpi=300)
            plt.close()

    # 6. Análisis de AP-BGSS: Comparación de Estrategias y Reducción Progresiva
    ap_history_csv = 'results/csv/ap_bgss_pivot_history.csv'
    if os.path.exists(ap_history_csv):
        print("[6/7] Generando gráficos de análisis AP-BGSS (Historia de Pivotes)...")
        df_ap = pd.read_csv(ap_history_csv)
        if not df_ap.empty:
            # Filtrar por mayor número de hilos para ver la reducción
            max_threads_ap = df_ap['threads'].max()
            df_ap_subset = df_ap[df_ap['threads'] == max_threads_ap]

            # Graficar reducción progresiva de vértices
            plt.figure(figsize=(12, 7))
            sns.lineplot(data=df_ap_subset, x='pivot_index', y='remaining_vertices', hue='strategy', style='graph', markers=True, dashes=False)
            plt.xlabel('Índice de Pivote', fontsize=12)
            plt.ylabel('Vértices Restantes (No Procesados)', fontsize=12)
            plt.title(f'Reducción Progresiva del Grafo ({max_threads_ap} hilos)', fontsize=14, fontweight='bold')
            plt.grid(True, which="both", ls="--")
            plt.tight_layout()
            plt.savefig('results/plots/ap_bgss_reduction_progress.png', dpi=300)
            plt.close()

            # Graficar trabajo acumulado de reachability (nodos visitados)
            # Primero calculamos el trabajo acumulado por estrategia/grafo
            df_ap_subset = df_ap_subset.sort_values(by=['graph', 'strategy', 'pivot_index'])
            df_ap_subset['cumulative_work'] = df_ap_subset.groupby(['graph', 'strategy'])['reachability_work'].cumsum()

            plt.figure(figsize=(12, 7))
            sns.lineplot(data=df_ap_subset, x='pivot_index', y='cumulative_work', hue='strategy', style='graph', markers=True, dashes=False)
            plt.xlabel('Índice de Pivote', fontsize=12)
            plt.ylabel('Trabajo Acumulado (Vértices Visitados)', fontsize=12)
            plt.title(f'Trabajo Acumulado de Reachability ({max_threads_ap} hilos)', fontsize=14, fontweight='bold')
            plt.grid(True, which="both", ls="--")
            plt.tight_layout()
            plt.savefig('results/plots/ap_bgss_reachability_work.png', dpi=300)
            plt.close()

    if os.path.exists(bench_csv):
        print("[7/7] Generando gráficos de comparación AP-BGSS vs BGSS...")
        df_bench = pd.read_csv(bench_csv)
        # Comparar AP-BGSS-SUM, AP-BGSS-MAX, AP-BGSS-MUL vs BGSS original
        df_comp = df_bench[df_bench['algorithm'].isin(['BGSS-BigSCC', 'BGSS-MultiSearch', 'AP-BGSS-SUM', 'AP-BGSS-MAX', 'AP-BGSS-MUL'])]
        if not df_comp.empty:
            max_th = df_comp['threads'].max()
            df_comp_max = df_comp[df_comp['threads'] == max_th]
            
            plt.figure(figsize=(14, 8))
            sns.barplot(data=df_comp_max, x='graph', y='time_ms', hue='algorithm', palette='Set2')
            plt.ylabel('Tiempo (ms)', fontsize=12)
            plt.title(f'Comparativa de Rendimiento: AP-BGSS vs BGSS Original ({max_th} hilos)', fontsize=14, fontweight='bold')
            plt.xticks(rotation=45)
            plt.tight_layout()
            plt.savefig('results/plots/ap_bgss_vs_bgss.png', dpi=300)
            plt.close()

    print("[OK] Todos los gráficos han sido actualizados en results/plots/")

if __name__ == "__main__":
    generate_plots()
