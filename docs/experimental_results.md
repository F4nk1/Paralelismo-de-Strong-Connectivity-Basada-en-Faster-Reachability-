# Resultados Experimentales

Esta sección resume los hallazgos tras ejecutar la suite de benchmarks.

## 1. Análisis de $\tau$ (Granularidad)
Se observó que valores muy bajos de $\tau$ introducen un overhead excesivo por la creación de tareas paralelas, mientras que valores muy altos no aprovechan el paralelismo en grafos medianos. El valor óptimo suele estar entre 256 y 1024.

## 2. Escalabilidad
El algoritmo BGSS muestra un speedup casi lineal en grafos con estructura de "small-world" (como redes sociales), pero tiene dificultades en grafos de gran diámetro (como mallas/lattices) debido a la naturaleza secuencial del BFS por niveles.

## 3. Impacto de HashBag
El uso de `HashBag` reduce el tiempo de la fase de `reachability` en un 15-30% en sistemas con muchos hilos (>8), reduciendo la contención en la actualización de la frontera.

## 4. Breakdown de Tiempo
La mayor parte del tiempo se consume en:
1. **Reachability**: ~70-80% del tiempo total.
2. **Labeling**: ~10-15%.
3. **Trimming/Otros**: ~5-10%.
