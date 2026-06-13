#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace graph {

/**
 * @brief Estructura de Grafo en formato Compressed Sparse Row (CSR).
 */
struct Graph {
    int num_vertices;
    long long num_edges;
    
    std::vector<long long> offsets; // Punteros a los inicios de cada lista de adyacencia
    std::vector<int> neighbors;     // Lista compacta de vecinos

    Graph() : num_vertices(0), num_edges(0) {}

    /**
     * @brief Carga un grafo desde un archivo de lista de aristas.
     * Formato esperado: "u v" por línea.
     */
    static Graph load_from_file(const std::string& filename);

    /**
     * @brief Crea el grafo reverso (importante para SCC).
     */
    Graph transpose() const;

    /**
     * @brief Genera un grafo de tipo Lattice (rejilla) para pruebas de gran diámetro.
     */
    static Graph generate_lattice(int rows, int cols);

    /**
     * @brief Genera un grafo k-NN aleatorio.
     */
    static Graph generate_knn(int n, int k);
};

} // namespace graph

#endif // GRAPH_HPP
