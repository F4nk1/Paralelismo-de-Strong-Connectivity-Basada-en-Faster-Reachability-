#!/bin/bash
# run_benchmarks.sh

# Asegurar que estamos en el directorio raíz
cd "$(dirname "$0")/.."

# 1. Compilar todo
echo "Compilando proyecto..."
mkdir -p build
cd build
cmake ..
make -j$(nproc)
cd ..

# 2. Generar datasets si no existen
if [ ! -f "datasets/synthetic/graph_10M.txt" ]; then
    echo "Generando datasets masivos..."
    ./build/gen_data
fi

# 3. Ejecutar el benchmark principal
echo "Ejecutando benchmarks..."
./build/parallel_scc

# 4. Generar gráficos (Usando venv si existe)
echo "Generando gráficos..."
if [ -d ".venv" ]; then
    echo "Usando entorno virtual (.venv)..."
    ./.venv/bin/python3 scripts/generate_plots.py
else
    echo "Aviso: No se detectó .venv, intentando con python3 del sistema..."
    python3 scripts/generate_plots.py
fi

echo "-------------------------------------------------------"
echo "PROCESO COMPLETADO EXITOSAMENTE"
echo "Resultados en results/csv/ y results/plots/"
echo "-------------------------------------------------------"
