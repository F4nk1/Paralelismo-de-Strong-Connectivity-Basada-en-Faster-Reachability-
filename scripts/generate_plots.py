import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np

def generate_plots():
    os.makedirs('results/plots', exist_ok=True)

    # 1. Tau Analysis
    tau_csv = 'results/csv/tau_analysis.csv'
    if os.path.exists(tau_csv):
        df = pd.read_csv(tau_csv)
        plt.figure(figsize=(10, 6))
        for graph in df['graph'].unique():
            subset = df[df['graph'] == graph]
            plt.plot(subset['tau'], subset['time_ms'], marker='o', label=graph)
        plt.xscale('log', base=2)
        plt.xlabel('Tau (Granularity Parameter)')
        plt.ylabel('Time (ms)')
        plt.title('VGC Analysis: Impact of Tau on Performance')
        plt.legend()
        plt.grid(True, which="both", ls="-")
        plt.savefig('results/plots/tau_analysis.png')

    # 2. Execution Breakdown
    breakdown_csv = 'results/csv/breakdown.csv'
    if os.path.exists(breakdown_csv):
        df = pd.read_csv(breakdown_csv)
        # Tomar un promedio o un caso representativo (ej: BGSS con 8 hilos en el grafo más grande)
        subset = df[df['algorithm'] == 'BGSS'].iloc[-1:] # Caso representativo
        phases = ['trimming', 'reachability', 'labeling', 'hash_bag', 'others']
        times = subset[phases].values.flatten()
        
        plt.figure(figsize=(10, 8))
        plt.pie(times, labels=phases, autopct='%1.1f%%', startangle=140)
        plt.title(f"Execution Breakdown - {subset['graph'].values[0]}")
        plt.savefig('results/plots/execution_breakdown.png')

    # 3. Scalability: Speedup and Efficiency
    scalability_csv = 'results/csv/scalability.csv'
    if os.path.exists(scalability_csv):
        df = pd.read_csv(scalability_csv)
        
        # Speedup
        plt.figure(figsize=(10, 6))
        for graph in df['graph'].unique():
            subset = df[(df['graph'] == graph) & (df['algorithm'] == 'BGSS')]
            plt.plot(subset['threads'], subset['speedup'], marker='o', label=f"BGSS - {graph}")
        plt.plot(df['threads'].unique(), df['threads'].unique(), 'k--', label='Ideal')
        plt.xlabel('Threads')
        plt.ylabel('Speedup')
        plt.title('Parallel Speedup')
        plt.legend()
        plt.grid(True)
        plt.savefig('results/plots/speedup.png')

        # Efficiency
        plt.figure(figsize=(10, 6))
        for graph in df['graph'].unique():
            subset = df[(df['graph'] == graph) & (df['algorithm'] == 'BGSS')]
            plt.plot(subset['threads'], subset['efficiency'], marker='s', label=f"BGSS - {graph}")
        plt.ylim(0, 1.1)
        plt.xlabel('Threads')
        plt.ylabel('Efficiency')
        plt.title('Parallel Efficiency')
        plt.legend()
        plt.grid(True)
        plt.savefig('results/plots/efficiency.png')

    # 4. Tarjan vs BGSS
    bench_csv = 'results/csv/benchmarks.csv'
    if os.path.exists(bench_csv):
        df = pd.read_csv(bench_csv)
        # Comparar Tarjan vs BGSS (con máx hilos)
        max_threads = df['threads'].max()
        comp_df = df[(df['algorithm'] == 'Tarjan') | ((df['algorithm'] == 'BGSS') & (df['threads'] == max_threads))]
        
        pivot_df = comp_df.pivot(index='graph', columns='algorithm', values='time_ms')
        pivot_df.plot(kind='bar', figsize=(12, 6))
        plt.ylabel('Time (ms)')
        plt.title(f'Performance Comparison: Tarjan vs BGSS ({max_threads} threads)')
        plt.xticks(rotation=45)
        plt.tight_layout()
        plt.savefig('results/plots/tarjan_vs_bgss.png')

    # 5. HashBag vs Vector
    hb_csv = 'results/csv/hashbag_vs_vector.csv'
    if os.path.exists(hb_csv):
        df = pd.read_csv(hb_csv)
        pivot_df = df.pivot(index='graph', columns='structure', values='time_ms')
        pivot_df.plot(kind='bar', figsize=(12, 6), color=['skyblue', 'salmon'])
        plt.ylabel('Time (ms)')
        plt.title('Data Structure Impact: std::vector vs HashBag')
        plt.xticks(rotation=45)
        plt.tight_layout()
        plt.savefig('results/plots/hashbag_vs_vector.png')

    print("[OK] All scientific plots generated in results/plots/")

if __name__ == "__main__":
    generate_plots()
