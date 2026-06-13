#include "graph/graph.hpp"
#include <iostream>
#include <fstream>
#include <string>

using namespace graph;

void save_graph(const Graph& g, const std::string& path) {
    std::ofstream file(path);
    for (int u = 0; u < g.num_vertices; ++u) {
        for (long long i = g.offsets[u]; i < g.offsets[u + 1]; ++i) {
            file << u << " " << g.neighbors[i] << "\n";
        }
    }
}

int main() {
    std::cout << "Generando datasets sintéticos..." << std::endl;
    
    // 1k
    save_graph(Graph::generate_knn(1000, 5), "datasets/synthetic/graph_1k.txt");
    // 10k
    save_graph(Graph::generate_knn(10000, 10), "datasets/synthetic/graph_10k.txt");
    // 100k
    save_graph(Graph::generate_knn(100000, 10), "datasets/synthetic/graph_100k.txt");
    
    // 1M
    save_graph(Graph::generate_knn(1000000, 5), "datasets/synthetic/graph_1M.txt");
    
    // Lattice (Gran diámetro)
    save_graph(Graph::generate_lattice(100, 100), "datasets/synthetic/lattice_10k.txt");

    std::cout << "[OK] Datasets generados en datasets/synthetic/" << std::endl;
    return 0;
}
