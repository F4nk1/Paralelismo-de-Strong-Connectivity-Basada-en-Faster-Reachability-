#include "graph.hpp"
#include <sstream>

namespace graph {

Graph Graph::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filename);
    }

    std::vector<std::pair<int, int>> edges;
    int max_v = -1;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::stringstream ss(line);
        int u, v;
        if (ss >> u >> v) {
            edges.push_back({u, v});
            max_v = std::max({max_v, u, v});
        }
    }

    Graph g;
    g.num_vertices = max_v + 1;
    g.num_edges = edges.size();
    g.offsets.assign(g.num_vertices + 1, 0);
    g.neighbors.resize(g.num_edges);

    // Contar grados para construir offsets
    for (const auto& edge : edges) {
        g.offsets[edge.first + 1]++;
    }

    for (int i = 0; i < g.num_vertices; ++i) {
        g.offsets[i + 1] += g.offsets[i];
    }

    // Llenar vecinos
    std::vector<long long> current_offset = g.offsets;
    for (const auto& edge : edges) {
        g.neighbors[current_offset[edge.first]++] = edge.second;
    }

    return g;
}

Graph Graph::transpose() const {
    Graph rev;
    rev.num_vertices = num_vertices;
    rev.num_edges = num_edges;
    rev.offsets.assign(num_vertices + 1, 0);
    rev.neighbors.resize(num_edges);

    for (int v : neighbors) {
        rev.offsets[v + 1]++;
    }

    for (int i = 0; i < num_vertices; ++i) {
        rev.offsets[i + 1] += rev.offsets[i];
    }

    std::vector<long long> current_offset = rev.offsets;
    for (int u = 0; u < num_vertices; ++u) {
        for (long long i = offsets[u]; i < offsets[u + 1]; ++i) {
            int v = neighbors[i];
            rev.neighbors[current_offset[v]++] = u;
        }
    }

    return rev;
}

Graph Graph::generate_lattice(int rows, int cols) {
    Graph g;
    g.num_vertices = rows * cols;
    g.num_edges = 0;
    std::vector<std::pair<int, int>> edges;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int u = r * cols + c;
            // Conectar con el vecino a la derecha
            if (c + 1 < cols) {
                edges.push_back({u, r * cols + (c + 1)});
            }
            // Conectar con el vecino abajo
            if (r + 1 < rows) {
                edges.push_back({u, (r + 1) * cols + c});
            }
        }
    }

    g.num_edges = edges.size();
    g.offsets.assign(g.num_vertices + 1, 0);
    g.neighbors.resize(g.num_edges);

    for (const auto& edge : edges) g.offsets[edge.first + 1]++;
    for (int i = 0; i < g.num_vertices; ++i) g.offsets[i + 1] += g.offsets[i];
    std::vector<long long> current_offset = g.offsets;
    for (const auto& edge : edges) g.neighbors[current_offset[edge.first]++] = edge.second;

    return g;
}

Graph Graph::generate_knn(int n, int k) {
    Graph g;
    g.num_vertices = n;
    g.num_edges = (long long)n * k;
    g.offsets.assign(n + 1, 0);
    g.neighbors.resize(g.num_edges);

    for (int i = 0; i < n; ++i) g.offsets[i+1] = (i+1) * k;

    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        // Semilla local por hilo para evitar contención y asegurar variabilidad
        unsigned int seed = i;
        for (int j = 0; j < k; ++j) {
            int neighbor = rand_r(&seed) % n;
            g.neighbors[i * k + j] = neighbor;
        }
    }

    return g;
}

} // namespace graph
