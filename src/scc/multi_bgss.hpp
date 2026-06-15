#ifndef MULTI_BGSS_HPP
#define MULTI_BGSS_HPP

#include "graph/graph.hpp"
#include "benchmark/metrics.hpp"
#include "hashbag.hpp"
#include <vector>

namespace scc {

/**
 * @brief Realiza una búsqueda multi-pivote paralela optimizada.
 */
void multi_search(const graph::Graph& g, const std::vector<char>& handled, 
                  const std::vector<int>& centers, 
                  ConcurrentHashTable<int, int>& table);

/**
 * @brief Implementación del algoritmo Multi-Search BGSS inspirado en GBBS.
 * Permite procesar múltiples pivotes simultáneamente usando tablas de hash.
 */
class MultiBGSS {
public:
    static std::vector<int> find_sccs(const graph::Graph& g, double beta = 1.1, benchmark::Breakdown* breakdown = nullptr);
};

} // namespace scc

#endif // MULTI_BGSS_HPP
