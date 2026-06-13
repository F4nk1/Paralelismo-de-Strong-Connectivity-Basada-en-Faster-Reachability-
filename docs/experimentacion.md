# Informe de Experimentación SCC

## 1. Entorno Experimental
- **Hardware**: Multinúcleo con soporte para hilos POSIX/OpenMP.
- **Software**: Arch Linux, GCC 16.1.1, CMake, Python (Pandas/Matplotlib).

## 2. Metodología
Se implementaron tres variantes principales para la detección de SCC:
1. **Tarjan**: Algoritmo secuencial de referencia ($O(V+E)$).
2. **BGSS**: Algoritmo paralelo basado en reachability.
3. **VGC**: Algoritmo con control de granularidad vertical.

## 3. Datasets Utilizados
| Dataset | Nodos | Aristas | Tipo | Propiedad |
|---------|-------|---------|------|-----------|
| graph_1k | 1,000 | 5,000 | k-NN | Densidad media |
| graph_10k | 10,000 | 100,000 | k-NN | Densidad alta |
| lattice_10k | 10,000 | 19,800 | Lattice | Gran diámetro |

## 4. Análisis de Resultados
Los resultados se generan automáticamente en `results/csv/benchmarks.csv`. Se analizan métricas de:
- **Speedup**: $S_p = T_1 / T_p$.
- **Eficiencia de VGC**: Reducción de overhead en grafos con estructura de cadena o rejilla.

## 5. Conclusiones
La implementación paralela demuestra ser superior en grafos densos, mientras que VGC previene la degradación de rendimiento en grafos de gran diámetro al conmutar inteligentemente a lógica secuencial optimizada cuando el paralelismo de frontera no es rentable.
