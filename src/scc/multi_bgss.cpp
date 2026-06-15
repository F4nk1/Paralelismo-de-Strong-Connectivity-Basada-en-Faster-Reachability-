#include "multi_bgss.hpp"
#include "hashbag.hpp"
#include <omp.h>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <random>

namespace scc {

struct FrontierNode {
    int v;
    int label;
};

/**
 * @brief Realiza una búsqueda multi-pivote paralela optimizada.
 */
void multi_search(const graph::Graph& g, const std::vector<char>& handled, 
                  const std::vector<int>& centers, 
                  ConcurrentHashTable<int, int>& table) {
    std::vector<FrontierNode> frontier;
    frontier.reserve(centers.size());
    for (int c : centers) {
        if (table.insert(c, c)) {
            frontier.push_back({c, c});
        }
    }

    while (!frontier.empty()) {
        int num_threads = omp_get_max_threads();
        std::vector<std::vector<FrontierNode>> local_frontiers(num_threads);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            auto& local_next = local_frontiers[tid];
            
            #pragma omp for nowait
            for (size_t i = 0; i < frontier.size(); i++) {
                int u = frontier[i].v;
                int u_label = frontier[i].label;

                for (long long j = g.offsets[u]; j < g.offsets[u + 1]; j++) {
                    int v = g.neighbors[j];
                    if (!handled[v]) {
                        if (table.insert(v, u_label)) {
                            local_next.push_back({v, u_label});
                        }
                    }
                }
            }
        }

        // Combinación paralela de fronteras
        std::vector<size_t> offsets(num_threads + 1, 0);
        for (int i = 0; i < num_threads; i++) {
            offsets[i + 1] = offsets[i] + local_frontiers[i].size();
        }

        std::vector<FrontierNode> next_frontier(offsets[num_threads]);
        #pragma omp parallel for
        for (int i = 0; i < num_threads; i++) {
            if (!local_frontiers[i].empty()) {
                std::copy(local_frontiers[i].begin(), local_frontiers[i].end(), 
                          next_frontier.begin() + offsets[i]);
            }
        }
        
        frontier = std::move(next_frontier);
    }
}

std::vector<int> MultiBGSS::find_sccs(const graph::Graph& g, double beta, benchmark::Breakdown* breakdown) {
    auto total_start = std::chrono::high_resolution_clock::now();
    int n = g.num_vertices;
    std::vector<int> scc_map(n, -1);
    std::vector<char> handled(n, 0);
    std::atomic<int> handled_count(0);

    graph::Graph g_rev = g.transpose();

    // 1. Trimming (Paralelo por oleadas)
    auto trim_start = std::chrono::high_resolution_clock::now();
    std::vector<int> in_deg(n);
    std::vector<int> out_deg(n);
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        out_deg[i] = g.offsets[i+1] - g.offsets[i];
        in_deg[i] = g_rev.offsets[i+1] - g_rev.offsets[i];
    }

    std::vector<int> q;
    #pragma omp parallel
    {
        std::vector<int> local_q;
        #pragma omp for nowait
        for (int i = 0; i < n; i++) {
            if (out_deg[i] == 0 || in_deg[i] == 0) local_q.push_back(i);
        }
        #pragma omp critical
        q.insert(q.end(), local_q.begin(), local_q.end());
    }

    while (!q.empty()) {
        int num_threads = omp_get_max_threads();
        std::vector<std::vector<int>> local_next_qs(num_threads);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            auto& local_next = local_next_qs[tid];

            #pragma omp for nowait
            for (size_t i = 0; i < q.size(); i++) {
                int u = q[i];
                if (handled[u]) continue;
                handled[u] = 1;
                scc_map[u] = u;
                handled_count.fetch_add(1, std::memory_order_relaxed);

                for (long long j = g_rev.offsets[u]; j < g_rev.offsets[u + 1]; j++) {
                    int v = g_rev.neighbors[j];
                    if (__sync_sub_and_fetch(&out_deg[v], 1) == 0) local_next.push_back(v);
                }
                for (long long j = g.offsets[u]; j < g.offsets[u + 1]; j++) {
                    int v = g.neighbors[j];
                    if (__sync_sub_and_fetch(&in_deg[v], 1) == 0) local_next.push_back(v);
                }
            }
        }

        std::vector<size_t> offsets(num_threads + 1, 0);
        for (int i = 0; i < num_threads; i++) {
            offsets[i+1] = offsets[i] + local_next_qs[i].size();
        }

        std::vector<int> next_q(offsets[num_threads]);
        #pragma omp parallel for
        for (int i = 0; i < num_threads; i++) {
            if (!local_next_qs[i].empty()) {
                std::copy(local_next_qs[i].begin(), local_next_qs[i].end(), next_q.begin() + offsets[i]);
            }
        }
        q = std::move(next_q);
    }
    auto trim_end = std::chrono::high_resolution_clock::now();
    if (breakdown) breakdown->trimming += std::chrono::duration<double, std::milli>(trim_end - trim_start).count();

    // 2. Selección de Pivotes Aleatorios
    std::vector<int> p;
    for (int i = 0; i < n; i++) if (!handled[i]) p.push_back(i);
    std::shuffle(p.begin(), p.end(), std::mt19937(12345));

    size_t cur_offset = 0;
    size_t step_size = 1;
    
    while (handled_count < n) {
        size_t end = std::min(cur_offset + step_size, p.size());
        std::vector<int> centers;
        for (size_t i = cur_offset; i < end; i++) {
            if (!handled[p[i]]) centers.push_back(p[i]);
        }
        
        if (centers.empty()) {
            if (end >= p.size()) break;
            cur_offset = end;
            step_size = static_cast<size_t>(step_size * beta) + 1;
            continue;
        }

        // Capacidad estimada: n + centers.size() * avg_reach (usamos 2n como heurística)
        ConcurrentHashTable<int, int> fw_table(n * 2 + centers.size());
        ConcurrentHashTable<int, int> bw_table(n * 2 + centers.size());

        auto reach_start = std::chrono::high_resolution_clock::now();
        multi_search(g, handled, centers, fw_table);
        multi_search(g_rev, handled, centers, bw_table);
        auto reach_end = std::chrono::high_resolution_clock::now();
        if (breakdown) breakdown->reachability += std::chrono::duration<double, std::milli>(reach_end - reach_start).count();

        // Intersección (Estilo GBBS)
        auto label_start = std::chrono::high_resolution_clock::now();
        
        // Iterar sobre la tabla más pequeña para encontrar intersecciones
        ConcurrentHashTable<int, int>* smaller = &fw_table;
        ConcurrentHashTable<int, int>* larger = &bw_table;
        if (bw_table.size() < fw_table.size()) std::swap(smaller, larger);

        auto raw_table = smaller->get_raw_table();
        #pragma omp parallel for
        for (size_t i = 0; i < smaller->capacity(); i++) {
            uint64_t entry = raw_table[i].load(std::memory_order_relaxed);
            if (entry != smaller->combine(-1, -1)) {
                auto pair = smaller->separate(entry);
                int v = pair.first;
                int label = pair.second;
                
                if (larger->contains(v, label)) {
                    // SCC encontrada
                    if (!handled[v]) {
                        handled[v] = 1;
                        scc_map[v] = label;
                        handled_count++;
                    }
                }
            }
        }
        auto label_end = std::chrono::high_resolution_clock::now();
        if (breakdown) breakdown->labeling += std::chrono::duration<double, std::milli>(label_end - label_start).count();

        cur_offset = end;
        step_size = static_cast<size_t>(step_size * beta) + 1;
        if (cur_offset >= p.size() && handled_count < n) {
            // Re-shuffle remaining
            p.clear();
            for (int i = 0; i < n; i++) if (!handled[i]) p.push_back(i);
            std::shuffle(p.begin(), p.end(), std::mt19937(std::random_device()()));
            cur_offset = 0;
            step_size = 1;
        }
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    if (breakdown) breakdown->total = std::chrono::duration<double, std::milli>(total_end - total_start).count();

    return scc_map;
}

} // namespace scc
