#include "vgc.hpp"
#include "bgss.hpp"
#include "tarjan.hpp"
#include <omp.h>
#include <atomic>
#include <algorithm>
#include <chrono>

namespace scc {

std::vector<int> VGC::run_parallel_scc(const graph::Graph& g, int tau, benchmark::Breakdown* breakdown) {
    // Si el grafo es pequeño (menor que tau), usamos Tarjan secuencial directamente
    // como optimización de granularidad (VGC simplificado).
    if (g.num_vertices < tau) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = Tarjan::find_sccs(g);
        auto end = std::chrono::high_resolution_clock::now();
        if (breakdown) {
            breakdown->others = std::chrono::duration<double, std::milli>(end - start).count();
            breakdown->total = breakdown->others;
        }
        return result;
    }

    // Para grafos grandes, usamos el motor BGSS que ya tiene reachability paralela.
    return BGSS::find_sccs(g, false, breakdown);
}

} // namespace scc
