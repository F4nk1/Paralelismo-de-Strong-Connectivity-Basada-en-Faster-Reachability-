import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np
import seaborn as sns

# Configuración de estilo
sns.set_theme(style="whitegrid")
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['font.sans-serif'] = ['Arial', 'DejaVu Sans']

def generate_plots():
    os.makedirs('results/plots', exist_ok=True)
    print("[1/5] Generando gráficos de escalabilidad...")

    # 1. Escalabilidad: Speedup y Eficiencia
    scalability_csv = 'results/csv/scalability.csv'
    if os.path.exists(scalability_csv):
        df = pd.read_csv(scalability_csv)
        
        # Filtrar algoritmos paralelos
        df_p = df[df['algorithm'] != 'Tarjan']
        
        # Speedup
        plt.figure(figsize=(12, 7))
        sns.lineplot(data=df_p, x='threads', y='speedup', hue='graph', style='algorithm', markers=True, dashes=False, markersize=8)
        
        # Línea ideal
        threads = sorted(df['threads'].unique())
        plt.plot(threads, threads, 'k--', alpha=0.5, label='Ideal')
        
        plt.xlabel('Número de Hilos', fontsize=12)
        plt.ylabel('Aceleración (Speedup)', fontsize=12)
        plt.title('Análisis de Escalabilidad: Aceleración Paralela', fontsize=14, fontweight='bold')
        plt.legend(title='Grafo / Algoritmo', bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(True, which="both", ls="-", alpha=0.3)
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
        plt.ylim(0, 1.1)
        plt.grid(True, which="both", ls="-", alpha=0.3)
        plt.tight_layout()
        plt.savefig('results/plots/efficiency.png', dpi=300)
        plt.close()

    print("[2/5] Generando desglose de ejecución...")
    # 2. Desglose de Ejecución (Breakdown)
    breakdown_csv = 'results/csv/breakdown.csv'
    if os.path.exists(breakdown_csv):
        df = pd.read_csv(breakdown_csv)
        # Filtrar solo casos con hilos máximos para cada algoritmo/grafo
        # Por simplicidad, tomaremos la última entrada de cada combinación
        df_last = df.groupby(['algorithm', 'graph']).last().reset_index()
        
        # Columnas a graficar (traducidas para el plot interno si es necesario, pero usaremos las del CSV)
        phases = ['trimming', 'forward_reach', 'backward_reach', 'reachability', 'labeling', 'hash_bag', 'big_scc', 'others']
        spanish_phases = ['Trimming', 'Alcance FW', 'Alcance BW', 'Alcance', 'Etiquetado', 'HashBag', 'Big SCC', 'Otros']
        
        for algo in df_last['algorithm'].unique():
            if algo == 'Tarjan': continue
            subset = df_last[df_last['algorithm'] == algo]
            
            # Preparar datos para gráfico de barras apiladas
            plot_data = subset[['graph'] + phases].copy()
            plot_data.columns = ['Grafo'] + spanish_phases
            plot_data = plot_data.set_index('Grafo')
            
            # Eliminar columnas con todo ceros
            plot_data = plot_data.loc[:, (plot_data != 0).any(axis=0)]
            
            plot_data.plot(kind='barh', stacked=True, figsize=(12, 6), colormap='viridis')
            plt.xlabel('Tiempo (ms)', fontsize=12)
            plt.ylabel('Grafo', fontsize=12)
            plt.title(f'Desglose de Tiempo de Ejecución: {algo}', fontsize=14, fontweight='bold')
            plt.legend(title='Fases', bbox_to_anchor=(1.05, 1), loc='upper left')
            plt.tight_layout()
            plt.savefig(f'results/plots/breakdown_{algo.lower()}.png', dpi=300)
            plt.close()

    print("[3/5] Comparando Tarjan vs Algoritmos Paralelos...")
    # 3. Tarjan vs Paralelos
    bench_csv = 'results/csv/benchmarks.csv'
    if os.path.exists(bench_csv):
        df = pd.read_csv(bench_csv)
        # Comparar Tarjan vs BGSS y MultiSearch (con máx hilos)
        max_threads = df['threads'].max()
        comp_df = df[(df['algorithm'] == 'Tarjan') | (df['threads'] == max_threads)]
        
        plt.figure(figsize=(14, 8))
        ax = sns.barplot(data=comp_df, x='graph', y='time_ms', hue='algorithm', palette='muted')
        
        # Escala logarítmica si la diferencia es muy grande
        if comp_df['time_ms'].max() / comp_df['time_ms'].min() > 100:
            ax.set_yscale("log")
            plt.ylabel('Tiempo (ms) [Escala Log]', fontsize=12)
        else:
            plt.ylabel('Tiempo (ms)', fontsize=12)

        plt.xlabel('Grafo', fontsize=12)
        plt.title(f'Comparativa de Rendimiento: Tarjan vs Algoritmos Paralelos ({max_threads} hilos)', fontsize=14, fontweight='bold')
        plt.xticks(rotation=45)
        plt.legend(title='Algoritmo')
        plt.grid(True, axis='y', alpha=0.3)
        plt.tight_layout()
        plt.savefig('results/plots/comparison_total.png', dpi=300)
        plt.close()

    print("[4/5] Analizando Throughput...")
    # 4. Throughput
    if os.path.exists(bench_csv):
        plt.figure(figsize=(14, 8))
        sns.barplot(data=df[df['threads'] == max_threads], x='graph', y='throughput', hue='algorithm', palette='magma')
        plt.ylabel('Rendimiento (M-edges/s)', fontsize=12)
        plt.xlabel('Grafo', fontsize=12)
        plt.title('Rendimiento (Throughput) por Algoritmo', fontsize=14, fontweight='bold')
        plt.xticks(rotation=45)
        plt.grid(True, axis='y', alpha=0.3)
        plt.tight_layout()
        plt.savefig('results/plots/throughput.png', dpi=300)
        plt.close()

    print("[5/5] Analizando impacto de hilos en el tiempo...")
    # 5. Runtime vs Threads
    if os.path.exists(bench_csv):
        plt.figure(figsize=(12, 7))
        sns.lineplot(data=df[df['algorithm'] != 'Tarjan'], x='threads', y='time_ms', hue='graph', style='algorithm', markers=True)
        plt.yscale("log")
        plt.xlabel('Número de Hilos', fontsize=12)
        plt.ylabel('Tiempo de Ejecución (ms) [Log]', fontsize=12)
        plt.title('Tiempo de Ejecución vs. Número de Hilos', fontsize=14, fontweight='bold')
        plt.grid(True, which="both", ls="-", alpha=0.3)
        plt.tight_layout()
        plt.savefig('results/plots/runtime_vs_threads.png', dpi=300)
        plt.close()

    print("[OK] Todos los gráficos científicos han sido generados y mejorados en results/plots/")

if __name__ == "__main__":
    generate_plots()
