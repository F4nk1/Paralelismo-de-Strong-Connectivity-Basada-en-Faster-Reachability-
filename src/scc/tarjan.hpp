#ifndef TARJAN_HPP
#define TARJAN_HPP

#include "graph/graph.hpp"
#include <vector>
#include <stack>

namespace scc {

/**
 * @brief Implementación secuencial del algoritmo de Tarjan para SCC.
 */
class Tarjan {
public:
    static std::vector<int> find_sccs(const graph::Graph& g);
};

} // namespace scc

#endif // TARJAN_HPP
