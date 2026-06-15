#include "bgss.hpp"
#include "graph/graph.hpp"
#include "hashbag.hpp"
#include "multi_bgss.hpp"
#include <omp.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <random>

namespace scc {

// BFS Paralelo optimizado (estilo GBBS)
void parallel_bfs_optimized(int pivot, const graph::Graph& g, const std::vector<char>& handled, 
                           std::vector<char>& visited, std::vector<int>& reached_list) {
    reached_list.clear();
    if (handled[pivot] || visited[pivot]) return;

    visited[pivot] = 1;
    std::vector<int> frontier;
    frontier.push_back(pivot);
    reached_list.push_back(pivot);

    while (!frontier.empty()) {
        int num_threads = omp_get_max_threads();
        std::vector<std::vector<int>> local_frontiers(num_threads);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            auto& local_next = local_frontiers[tid];
            
            #pragma omp for nowait
            for (size_t i = 0; i < frontier.size(); ++i) {
                int u = frontier[i];
                for (long long j = g.offsets[u]; j < g.offsets[u + 1]; ++j) {
                    int v = g.neighbors[j];
                    if (!handled[v] && !visited[v]) {
                        if (__sync_bool_compare_and_swap(&visited[v], 0, 1)) {
                            local_next.push_back(v);
                        }
                    }
                }
            }
        }

        std::vector<size_t> offsets(num_threads + 1, 0);
        for (int i = 0; i < num_threads; i++) {
            offsets[i + 1] = offsets[i] + local_frontiers[i].size();
        }

        std::vector<int> next_frontier(offsets[num_threads]);
        #pragma omp parallel for
        for (int i = 0; i < num_threads; i++) {
            if (!local_frontiers[i].empty()) {
                std::copy(local_frontiers[i].begin(), local_frontiers[i].end(), 
                          next_frontier.begin() + offsets[i]);
            }
        }
        
        if (!next_frontier.empty()) {
            reached_list.insert(reached_list.end(), next_frontier.begin(), next_frontier.end());
        }
        frontier = std::move(next_frontier);
    }
}

std::vector<int> BGSS::find_sccs(const graph::Graph& g, bool use_hashbag, benchmark::Breakdown* breakdown) {
    auto total_start = std::chrono::high_resolution_clock::now();
    int n = g.num_vertices;
    std::vector<int> scc_map(n, -1);
    std::vector<char> handled(n, 0);
    std::atomic<int> handled_count(0);
    
    graph::Graph g_rev = g.transpose();

    // Phase 1: Trimming (Paralelo por oleadas)
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

    // Phase 2: Big SCC Optimization
    auto big_scc_start = std::chrono::high_resolution_clock::now();
    int big_pivot = -1;
    int max_deg = -1;

    #pragma omp parallel
    {
        int local_pivot = -1;
        int local_max_deg = -1;
        #pragma omp for nowait
        for (int i = 0; i < n; i++) {
            if (!handled[i]) {
                int deg = (g.offsets[i+1] - g.offsets[i]) + (g_rev.offsets[i+1] - g_rev.offsets[i]);
                if (deg > local_max_deg) {
                    local_max_deg = deg;
                    local_pivot = i;
                }
            }
        }
        #pragma omp critical
        {
            if (local_max_deg > max_deg) {
                max_deg = local_max_deg;
                big_pivot = local_pivot;
            }
        }
    }

    if (big_pivot != -1) {
        std::vector<char> visited_desc(n, 0);
        std::vector<char> visited_anc(n, 0);
        std::vector<int> reached_desc;
        std::vector<int> reached_anc;

        parallel_bfs_optimized(big_pivot, g, handled, visited_desc, reached_desc);
        parallel_bfs_optimized(big_pivot, g_rev, handled, visited_anc, reached_anc);

        #pragma omp parallel for
        for (int i = 0; i < (int)reached_anc.size(); i++) {
            int node = reached_anc[i];
            if (visited_desc[node]) {
                scc_map[node] = big_pivot;
                handled[node] = 1;
                handled_count.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
    auto big_scc_end = std::chrono::high_resolution_clock::now();
    if (breakdown) breakdown->big_scc += std::chrono::duration<double, std::milli>(big_scc_end - big_scc_start).count();

    // Phase 3: Multi-Search para el resto (Evita el bucle secuencial que mataba el rendimiento)
    if (handled_count < n) {
        auto multi_start = std::chrono::high_resolution_clock::now();
        
        // Obtenemos los SCC restantes usando la lógica de MultiBGSS
        // pero integrando los resultados en nuestro scc_map actual
        std::vector<int> remaining_p;
        for (int i = 0; i < n; i++) if (!handled[i]) remaining_p.push_back(i);
        std::shuffle(remaining_p.begin(), remaining_p.end(), std::mt19937(12345));

        size_t cur_offset = 0;
        size_t step_size = 1;
        double beta = 1.1;

        while (handled_count < n) {
            size_t end = std::min(cur_offset + step_size, remaining_p.size());
            std::vector<int> centers;
            for (size_t i = cur_offset; i < end; i++) {
                if (!handled[remaining_p[i]]) centers.push_back(remaining_p[i]);
            }

            if (centers.empty()) {
                if (end >= remaining_p.size()) break;
                cur_offset = end;
                step_size = static_cast<size_t>(step_size * beta) + 1;
                continue;
            }

            ConcurrentHashTable<int, int> fw_table(n + centers.size() * 2);
            ConcurrentHashTable<int, int> bw_table(n + centers.size() * 2);

            multi_search(g, handled, centers, fw_table);
            multi_search(g_rev, handled, centers, bw_table);

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
                        if (!handled[v]) {
                            handled[v] = 1;
                            scc_map[v] = label;
                            handled_count.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                }
            }
            cur_offset = end;
            step_size = static_cast<size_t>(step_size * beta) + 1;
        }
        auto multi_end = std::chrono::high_resolution_clock::now();
        if (breakdown) breakdown->reachability += std::chrono::duration<double, std::milli>(multi_end - multi_start).count();
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    if (breakdown) breakdown->total = std::chrono::duration<double, std::milli>(total_end - total_start).count();

    return scc_map;
}

} // namespace scc
