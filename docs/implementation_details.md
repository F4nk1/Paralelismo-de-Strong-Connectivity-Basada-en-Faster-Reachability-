# Detalles de Implementación

Este proyecto implementa algoritmos para encontrar Componentes Fuertemente Conexas (SCC) de forma paralela, basándose en el paper "Faster Reachability".

## Algoritmos Implementados

### 1. Tarjan (Secuencial)
Utilizado como línea base (baseline) y para validación de correctitud. Es un algoritmo basado en DFS con complejidad O(V+E).

### 2. BGSS (Paralelo)
Algoritmo basado en Forward-Backward (FB). 
- **Pivoteo**: Selecciona un pivote y encuentra sus descendientes (desc) y ancestros (anc).
- **Paralelismo**: La búsqueda de alcanzabilidad (BFS) está paralelaizada con OpenMP.
- **Intersección**: Los nodos en `desc ∩ anc` forman una SCC.

### 3. VGC (Vertical Granularity Control)
Optimiza la ejecución decidiendo cuándo cambiar entre algoritmos paralelos y secuenciales según el tamaño del subgrafo.
- El parámetro $\tau$ define el umbral de granularidad.

### 4. HashBag
Una estructura de datos diseñada para inserciones concurrentes de alta velocidad durante la fase de reachability, minimizando la contención en comparación con un `std::vector` protegido por mutex.

## Estructuras de Datos
- **Grafo**: Representado en formato CSR (Compressed Sparse Row) para un acceso a memoria eficiente.
- **HashBag**: Implementado como un buffer atómico para manejar la frontera del BFS de forma concurrente.
