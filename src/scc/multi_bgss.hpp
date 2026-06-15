#ifndef MULTI_BGSS_HPP
#define MULTI_BGSS_HPP

#include "graph/graph.hpp"
#include "benchmark/metrics.hpp"
#include <vector>

namespace scc {

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
