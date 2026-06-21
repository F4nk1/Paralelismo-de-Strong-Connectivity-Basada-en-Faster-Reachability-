#!/bin/bash
# run_all.sh - Script maestro para automatizar todo el proceso

# Salir inmediatamente si un comando falla
set -e

echo "======================================================="
echo "  SISTEMA DE EXPERIMENTACION SCC: FLUJO AUTOMATIZADO   "
echo "======================================================="

# 1. Configuración del Entorno Python
echo -e "\n[1/6] Configurando entorno Python..."
if [ ! -d ".venv" ]; then
    python3 -m venv .venv
fi
source .venv/bin/activate
pip install --upgrade pip --quiet
pip install -r requirements.txt --quiet
echo "OK: Entorno Python listo."

# 2. Compilación del Proyecto C++
echo -e "\n[2/6] Compilando binarios C++..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..
echo "OK: Compilación finalizada."

# 3. Preparación de Datasets
echo -e "\n[3/6] Preparando datasets (Sintéticos y Reales)..."
mkdir -p datasets/real datasets/synthetic
./build/gen_data
bash scripts/download_datasets.sh
echo "OK: Datasets listos."

# 4. Validación de Correctitud
echo -e "\n[4/6] Ejecutando pruebas de correctitud..."
./build/test_correctness
echo "OK: Todos los algoritmos pasaron la validación."

# 5. Ejecución de Benchmarks
echo -e "\n[5/6] Ejecutando suite de experimentos..."
# Asegurar que existan carpetas de resultados
mkdir -p results/csv results/plots
./build/parallel_scc
echo "OK: Datos experimentales generados en results/csv/."

# 6. Generación de Gráficos y Data de Dashboard
echo -e "\n[6/6] Generando figuras y base de datos del dashboard..."
python3 scripts/generate_plots.py
python3 scripts/generate_dashboard_data.py
echo "OK: Gráficos y datos del dashboard guardados."

echo -e "\n======================================================="
echo "  ¡PROCESO COMPLETADO EXITOSAMENTE!"
echo "  Consulte los resultados en la carpeta 'results/'."
echo "======================================================="
