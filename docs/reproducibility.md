# Reproducibilidad

Para reproducir los experimentos presentados en este proyecto, siga estos pasos:

## Requisitos
- Compilador C++17 (GCC o Clang)
- OpenMP
- CMake 3.10+
- Python 3 con pandas y matplotlib

## Pasos

1. **Generar Datasets Sintéticos**:
   ```bash
   ./build/gen_data
   ```

2. **Descargar Datasets Reales**:
   ```bash
   bash scripts/download_datasets.sh
   ```

3. **Configurar Parámetros**:
   Edite los archivos en `config/` (especialmente `benchmark.json`) para ajustar los hilos y valores de $\tau$.

4. **Ejecutar Benchmark**:
   ```bash
   ./build/parallel_scc
   ```

5. **Generar Gráficos**:
   ```bash
   python3 scripts/generate_plots.py
   ```

Los resultados estarán disponibles en `results/csv/` y los gráficos en `results/plots/`.
