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

# email-EuAll
if [ ! -f email-EuAll.txt ]; then
    curl -L https://snap.stanford.edu/data/email-EuAll.txt.gz | gunzip > email-EuAll.txt
    echo "email-EuAll.txt descargado."
fi

# soc-Epinions1
if [ ! -f soc-Epinions1.txt ]; then
    curl -L https://snap.stanford.edu/data/soc-Epinions1.txt.gz | gunzip > soc-Epinions1.txt
    echo "soc-Epinions1.txt descargado."
fi

echo "[OK] Datasets reales listos en datasets/real/"
