# Resumen del Paper: Parallel Strong Connectivity Based on Faster Reachability

## Autores
Wang et al. (2023)

## Problema
Detección eficiente de Componentes Fuertemente Conexos (SCC) en grafos de gran escala y gran diámetro utilizando arquitecturas multinúcleo.

## Propuesta
1. **Vertical Granularity Control (VGC)**: Técnica para mitigar el problema de los grafos "estrechos y profundos" (gran diámetro) donde el paralelismo de nivel de borde es insuficiente.
2. **Parallel Hash Bag**: Estructura de datos concurrente para almacenar fronteras de búsqueda de manera eficiente.

## Resultados Clave
- Mejora significativa en grafos de gran diámetro comparado con GBBS e iSpan.
- Reducción de la sobrecarga de sincronización.
