import pandas as pd
import json
import os

def generate_data():
    data = {}
    
    csv_files = {
        'benchmarks': 'results/csv/benchmarks.csv',
        'breakdown': 'results/csv/breakdown.csv',
        'scalability': 'results/csv/scalability.csv',
        'tau_analysis': 'results/csv/tau_analysis.csv',
        'hashbag_vs_vector': 'results/csv/hashbag_vs_vector.csv',
        'ap_bgss_pivot_history': 'results/csv/ap_bgss_pivot_history.csv'
    }
    
    for key, filepath in csv_files.items():
        if os.path.exists(filepath):
            try:
                df = pd.read_csv(filepath)
                # Convert NaN/Infinity to None/null for JSON safety
                df = df.where(pd.notnull(df), None)
                data[key] = df.to_dict(orient='records')
            except Exception as e:
                print(f"Error reading {filepath}: {e}")
                data[key] = []
        else:
            data[key] = []
            
    # Write to results/dashboard_data.js
    os.makedirs('results', exist_ok=True)
    with open('results/dashboard_data.js', 'w') as f:
        f.write("const dashboardData = ")
        f.write(json.dumps(data, indent=2))
        f.write(";\n")
    print("[OK] dashboard_data.js generated successfully.")

if __name__ == "__main__":
    generate_data()
