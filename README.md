# Parallel Strong Connectivity Based on Faster Reachability

Este proyecto implementa y evalúa algoritmos paralelos para la detección de **Componentes Fuertemente Conexas (SCC)** en grafos dirigidos, basándose en el paper de Wang et al. (2023).

## Resultados y Análisis
El sistema genera automáticamente un análisis detallado tras ejecutar los experimentos:
- **Gráficos (Español):** Disponibles en `results/plots/` (Aceleración, Eficiencia, Desglose, etc.).
- **Reporte de Análisis:** Consulte `results/reports/analisis_experimental.md` para una interpretación científica de los datos.
- **CSV:** Datos crudos en `results/csv/`.

## Ejecución Ultra-Rápida (Automatizada)

Para simplificar el proceso, se ha incluido un script maestro que realiza la configuración del entorno, compilación, descarga de datos, validación y generación de gráficos con un solo comando:

```bash
chmod +x run_all.sh
./run_all.sh
```

---

## Detalles del Sistema

### Algoritmos y Características
- **Modelos**: Tarjan (Base), BGSS (Paralelo), VGC (Granularidad Vertical).
- **Estructuras**: CSR (Almacenamiento eficiente) y Parallel Hash Bag (Fronteras concurrentes).
- **Validación**: Todos los resultados paralelos se contrastan automáticamente contra Tarjan.
- **Métricas**: Speedup, Eficiencia, Breakdown de tiempo y Análisis de $\tau$.

### Requisitos Técnicos
- **C++**: Compilador con soporte C++17 y OpenMP.
- **Python 3**: Para generación de gráficos (`pandas`, `matplotlib`).
- **Sistema**: Linux (probado en Arch y Debian).

## Estructura del Proyecto
- `src/`: Implementación de algoritmos en C++.
- `config/`: Parámetros experimentales en formato JSON.
- `results/`: Salidas generadas (CSV y Gráficos PNG).
- `docs/`: Documentación técnica profunda sobre la implementación y resultados.
- `tests/`: Suite de pruebas de correctitud.

## Pasos Manuales (Opcional)
Si desea ejecutar fases específicas:
1. **Compilar**: `mkdir build && cd build && cmake .. && make`
2. **Entorno Python**: `python3 -m venv .venv && source .venv/bin/activate && pip install -r requirements.txt`
3. **Descargar Datos**: `bash scripts/download_datasets.sh`
4. **Gráficos**: `python3 scripts/generate_plots.py`

## Licencia
Este proyecto está bajo la Licencia MIT. Consulte el archivo `LICENSE` para más detalles.
