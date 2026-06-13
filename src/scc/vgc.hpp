#ifndef VGC_HPP
#define VGC_HPP

#include "graph/graph.hpp"
#include "benchmark/metrics.hpp"
#include <vector>

namespace scc {

/**
 * @brief Vertical Granularity Control (VGC).
 * Ayuda a controlar la profundidad de la búsqueda y la sincronización.
 */
class VGC {
public:
    static std::vector<int> run_parallel_scc(const graph::Graph& g, int tau = 1024, benchmark::Breakdown* breakdown = nullptr);
};

} // namespace scc

#endif // VGC_HPP
