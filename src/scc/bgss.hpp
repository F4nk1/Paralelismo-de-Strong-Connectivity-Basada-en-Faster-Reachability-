#ifndef BGSS_HPP
#define BGSS_HPP

#include "graph/graph.hpp"
#include "benchmark/metrics.hpp"
#include <vector>

namespace scc {

/**
 * @brief Implementación del algoritmo BGSS para SCC paralelo.
 */
class BGSS {
public:
    static std::vector<int> find_sccs(const graph::Graph& g, bool use_hashbag = false, benchmark::Breakdown* breakdown = nullptr);
};

} // namespace scc

#endif // BGSS_HPP
