# Análisis Experimental: Conectividad Fuerte en Paralelo

Este documento presenta una interpretación detallada de los resultados obtenidos durante la experimentación con algoritmos de Componentes Fuertemente Conexas (SCC) en paralelo, enfocándose en las variantes **BGSS (Big SCC)** y **Multi-Search (estilo GBBS)**.

## 1. Metodología y Entorno
- **Hardware:** Sistema Linux con 12 hilos lógicos.
- **Algoritmos comparados:**
    - **Tarjan:** Algoritmo secuencial óptimo (Base de referencia).
    - **BGSS-BigSCC:** Algoritmo paralelo basado en FW-BW con optimización de componente gigante.
    - **BGSS-MultiSearch:** Implementación paralela de múltiples búsquedas simultáneas usando tablas de hash concurrentes.
- **Datasets:** Grafos sintéticos (10k a 1M de nodos) y un grafo tipo Lattice (malla) para evaluar el impacto del diámetro.

## 2. Resultados de Rendimiento Total
### 2.1. Comparativa vs Tarjan
En grafos de tamaño mediano-grande (`graph_1M.txt`), el algoritmo **BGSS-BigSCC** logra superar a Tarjan:
- **Tarjan:** ~240 ms
- **BGSS-BigSCC (8 hilos):** ~140 ms (**Speedup: 1.71x**)

Sin embargo, en grafos pequeños o con estructuras muy seriales (como el Lattice), Tarjan sigue siendo imbatible debido a su mínima sobrecarga y complejidad lineal.

### 2.2. BGSS-BigSCC vs MultiSearch
El algoritmo **Multi-Search** mostró ser competitivo en escalabilidad pero con un tiempo base mayor debido a la sobrecarga de la tabla de hash. Su rendimiento brilla en grafos donde se pueden lanzar miles de pivotes simultáneamente sin colisiones excesivas.

## 3. Análisis de Escalabilidad
Los gráficos de aceleración (**Speedup**) muestran un comportamiento sublineal pero positivo hasta los 8 hilos:
- La **Eficiencia** se mantiene por encima del 50% en la mayoría de los grafos reales hasta 4-8 hilos.
- Existe una caída de rendimiento al usar 16 hilos, lo cual es esperado dado que el sistema físico cuenta con 12 hilos; el sobre-suscripción del procesador introduce latencias de contexto.

## 4. Desglose del Tiempo (Execution Breakdown)
El análisis de las fases revela que:
- **Alcance (Reachability):** Es la fase más costosa, representando entre el 60% y 80% del tiempo total en BGSS.
- **Trimming:** Es extremadamente rápido pero crucial para eliminar "flecos" del grafo antes de las búsquedas costosas.
- **Big SCC:** La optimización de componente gigante en BGSS logra identificar rápidamente la masa principal del grafo, reduciendo drásticamente el trabajo restante.

## 5. Impacto de la Topología del Grafo
El grafo **Lattice** (rejilla) es el "peor caso" para los algoritmos basados en alcance:
- El gran diámetro del grafo obliga a que las búsquedas BFS/DFS paralelas tengan muchas iteraciones con fronteras pequeñas.
- Aquí, la sobrecarga de sincronización de hilos supera el beneficio del paralelismo, resultando en una eficiencia baja.

## 6. Conclusiones
1. El paralelismo en SCC es altamente dependiente de la topología: grafos de "mundo pequeño" (diámetro bajo) escalan significativamente mejor.
2. **BGSS con optimización de Big SCC** es la opción más robusta para propósitos generales.
3. El uso de **Tablas de Hash Concurrentes** en Multi-Search es prometedor pero requiere grafos lo suficientemente grandes para amortizar su costo de inicialización y acceso.

---
*Gráficos detallados disponibles en la carpeta `results/plots/`.*
