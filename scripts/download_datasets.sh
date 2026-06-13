#!/bin/bash
mkdir -p datasets/real
cd datasets/real

echo "Descargando grafos reales (versiones pequeñas para demo)..."

# Wiki-Vote
if [ ! -f wiki-Vote.txt ]; then
    curl -L https://snap.stanford.edu/data/wiki-Vote.txt.gz | gunzip > wiki-Vote.txt
    echo "wiki-Vote.txt descargado."
fi

# p2p-Gnutella08
if [ ! -f p2p-Gnutella08.txt ]; then
    curl -L https://snap.stanford.edu/data/p2p-Gnutella08.txt.gz | gunzip > p2p-Gnutella08.txt
    echo "p2p-Gnutella08.txt descargado."
fi

# Renombrar para que coincidan con config/datasets.json si es necesario
cp wiki-Vote.txt web_graph.txt
cp p2p-Gnutella08.txt social_graph.txt
# Generar uno de red de carreteras sintético si no hay uno pequeño a mano
# (O simplemente copiar uno de los otros para la demo)
cp wiki-Vote.txt road_network.txt

echo "[OK] Datasets reales listos en datasets/real/"
