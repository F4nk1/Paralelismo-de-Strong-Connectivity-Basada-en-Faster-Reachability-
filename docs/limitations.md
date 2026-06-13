# Limitaciones

## Algorítmicas
- **Selección de Pivote**: Actualmente se usa una selección secuencial. Una selección aleatoria o basada en grado podría mejorar el balanceo de carga.
- **BFS por niveles**: El paralelismo está limitado por el ancho de cada nivel del BFS. En grafos de gran diámetro, el speedup es limitado.

## Implementación
- **Consumo de Memoria**: La representación de ancestros y descendientes como `vector<char>` consume O(V) por paso (1 byte por nodo), lo cual es más seguro para hilos que `vector<bool>` pero ocupa más memoria (8x). Esto puede ser prohibitivo para grafos extremadamente grandes (>1B nodos).
- **NUMA**: No se han implementado optimizaciones específicas para arquitecturas NUMA.
