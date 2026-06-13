#!/bin/bash
# setup.sh

echo ">>> Configurando entorno de desarrollo..."

# 1. Crear entorno virtual si no existe
if [ ! -d ".venv" ]; then
    echo "Creando entorno virtual (.venv)..."
    python3 -m venv .venv
fi

# 2. Instalar dependencias
echo "Instalando dependencias de Python..."
./.venv/bin/pip install --upgrade pip
./.venv/bin/pip install -r requirements.txt

# 3. Preparar directorios
mkdir -p results/csv results/plots datasets/real

echo ">>> [OK] Entorno configurado correctamente."
echo ">>> Use 'source .venv/bin/activate' para activar el entorno."
