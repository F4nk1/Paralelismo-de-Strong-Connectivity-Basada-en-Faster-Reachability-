#include "ap_bgss.hpp"
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
#include <map>
#include <fstream>
#include <iostream>

namespace scc {

std::string strategy_to_string(PivotStrategy strategy) {
    switch (strategy) {
        case PivotStrategy::SUM: return "SUM";
        case PivotStrategy::MAX: return "MAX";
        case PivotStrategy::MUL: return "MUL";
    }
    return "UNKNOWN";
}

// BFS Paralelo optimizado (estilo GBBS)
static void parallel_bfs_optimized(int pivot, const graph::Graph& g, const std::vector<char>& handled, 
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

// BFS Paralelo con HashBag
static void parallel_bfs_hashbag(int pivot, const graph::Graph& g, const std::vector<char>& handled, 
                                 std::vector<char>& visited, std::vector<int>& reached_list) {
    reached_list.clear();
    if (handled[pivot] || visited[pivot]) return;

    visited[pivot] = 1;
    reached_list.push_back(pivot);
    
    std::vector<int> frontier;
    frontier.push_back(pivot);

    while (!frontier.empty()) {
        HashBag<int> next_frontier(g.num_vertices);

        #pragma omp parallel for
        for (size_t i = 0; i < frontier.size(); ++i) {
            int u = frontier[i];
            for (long long j = g.offsets[u]; j < g.offsets[u + 1]; ++j) {
                int v = g.neighbors[j];
                if (!handled[v] && !visited[v]) {
                    if (__sync_bool_compare_and_swap(&visited[v], 0, 1)) {
                        next_frontier.insert(v);
                    }
                }
            }
        }

        size_t next_size = next_frontier.get_size();
        if (next_size == 0) break;

        frontier.resize(next_size);
        const auto& data = next_frontier.get_data();
        #pragma omp parallel for
        for (size_t i = 0; i < (int)next_size; ++i) {
            frontier[i] = data[i];
        }
        reached_list.insert(reached_list.end(), frontier.begin(), frontier.end());
    }
}

std::vector<int> AP_BGSS::find_sccs(const graph::Graph& g, 
                                  PivotStrategy strategy, 
                                  bool use_hashbag, 
                                  benchmark::Breakdown* breakdown,
                                  std::vector<AP_PivotMetric>* pivot_metrics,
                                  std::string graph_name) {
    auto total_start = std::chrono::high_resolution_clock::now();
    int n = g.num_vertices;
    std::vector<int> scc_map(n, -1);
    std::vector<char> handled(n, 0);
    std::atomic<int> handled_count(0);
    int threads_num = omp_get_max_threads();

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

    // Preprocesar scores para la selección de pivotes
    std::vector<double> scores(n, 0.0);
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        double out_d = g.offsets[i+1] - g.offsets[i];
        double in_d = g_rev.offsets[i+1] - g_rev.offsets[i];
        if (strategy == PivotStrategy::SUM) {
            scores[i] = in_d + out_d;
        } else if (strategy == PivotStrategy::MAX) {
            scores[i] = std::max(in_d, out_d);
        } else if (strategy == PivotStrategy::MUL) {
            scores[i] = in_d * out_d;
        }
    }

    int next_pivot_index = 1;

    // Phase 2: Big SCC Optimization con Score Adaptativo
    auto big_scc_start = std::chrono::high_resolution_clock::now();
    int big_pivot = -1;
    double max_score = -1.0;

    #pragma omp parallel
    {
        int local_pivot = -1;
        double local_max_score = -1.0;
        #pragma omp for nowait
        for (int i = 0; i < n; i++) {
            if (!handled[i]) {
                double score = scores[i];
                if (score > local_max_score) {
                    local_max_score = score;
                    local_pivot = i;
                }
            }
        }
        #pragma omp critical
        {
            if (local_max_score > max_score) {
                max_score = local_max_score;
                big_pivot = local_pivot;
            }
        }
    }

    if (big_pivot != -1) {
        std::vector<char> visited_desc(n, 0);
        std::vector<char> visited_anc(n, 0);
        std::vector<int> reached_desc;
        std::vector<int> reached_anc;

        if (use_hashbag) {
            parallel_bfs_hashbag(big_pivot, g, handled, visited_desc, reached_desc);
            parallel_bfs_hashbag(big_pivot, g_rev, handled, visited_anc, reached_anc);
        } else {
            parallel_bfs_optimized(big_pivot, g, handled, visited_desc, reached_desc);
            parallel_bfs_optimized(big_pivot, g_rev, handled, visited_anc, reached_anc);
        }

        int scc_sz = 0;
        #pragma omp parallel for reduction(+:scc_sz)
        for (int i = 0; i < (int)reached_anc.size(); i++) {
            int node = reached_anc[i];
            if (visited_desc[node]) {
                scc_map[node] = big_pivot;
                handled[node] = 1;
                scc_sz++;
            }
        }
        handled_count.fetch_add(scc_sz, std::memory_order_relaxed);

        if (pivot_metrics) {
            AP_PivotMetric pm;
            pm.graph_name = graph_name;
            pm.threads = threads_num;
            pm.strategy = strategy_to_string(strategy);
            pm.pivot_index = next_pivot_index++;
            pm.pivot_vertex = big_pivot;
            pm.pivot_score = max_score;
            pm.scc_found_size = scc_sz;
            pm.remaining_vertices = n - handled_count.load();
            pm.reachability_work = reached_desc.size() + reached_anc.size();
            pivot_metrics->push_back(pm);
        }
    }
    auto big_scc_end = std::chrono::high_resolution_clock::now();
    if (breakdown) breakdown->big_scc += std::chrono::duration<double, std::milli>(big_scc_end - big_scc_start).count();

    // Phase 3: Multi-Search Adaptativo
    if (handled_count < n) {
        auto multi_start = std::chrono::high_resolution_clock::now();
        
        std::vector<int> remaining_p;
        for (int i = 0; i < n; i++) if (!handled[i]) remaining_p.push_back(i);
        
        // Ordenar los pivotes según su score en orden descendente
        std::sort(remaining_p.begin(), remaining_p.end(), [&](int u, int v) {
            return scores[u] > scores[v];
        });

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

            std::vector<int> newly_handled;
            auto raw_table = smaller->get_raw_table();
            #pragma omp parallel
            {
                std::vector<int> local_newly_handled;
                #pragma omp for nowait
                for (size_t i = 0; i < smaller->capacity(); i++) {
                    uint64_t entry = raw_table[i].load(std::memory_order_relaxed);
                    if (entry != smaller->combine(-1, -1)) {
                        auto pair = smaller->separate(entry);
                        int v = pair.first;
                        int label = pair.second;
                        if (larger->contains(v, label)) {
                            if (!handled[v]) {
                                if (__sync_bool_compare_and_swap(&handled[v], 0, 1)) {
                                    scc_map[v] = label;
                                    handled_count.fetch_add(1, std::memory_order_relaxed);
                                    local_newly_handled.push_back(label);
                                }
                            }
                        }
                    }
                }
                #pragma omp critical
                newly_handled.insert(newly_handled.end(), local_newly_handled.begin(), local_newly_handled.end());
            }

            if (pivot_metrics) {
                // Contar cuántos nodos se asignaron a cada label en esta iteración
                std::map<int, int> label_counts;
                for (int label : newly_handled) {
                    label_counts[label]++;
                }

                long long step_reachability_work = fw_table.size() + bw_table.size();
                int step_remaining = n - handled_count.load();

                // Registrar las métricas de cada pivote en esta tanda
                for (int c : centers) {
                    AP_PivotMetric pm;
                    pm.graph_name = graph_name;
                    pm.threads = threads_num;
                    pm.strategy = strategy_to_string(strategy);
                    pm.pivot_index = next_pivot_index++;
                    pm.pivot_vertex = c;
                    pm.pivot_score = scores[c];
                    pm.scc_found_size = label_counts[c];
                    pm.remaining_vertices = step_remaining;
                    pm.reachability_work = step_reachability_work;
                    pivot_metrics->push_back(pm);
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

void save_ap_metrics_to_csv(const std::vector<AP_PivotMetric>& metrics, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    file << "graph,threads,strategy,pivot_index,pivot_vertex,pivot_score,scc_found_size,remaining_vertices,reachability_work\n";
    for (const auto& pm : metrics) {
        file << pm.graph_name << "," << pm.threads << "," << pm.strategy << "," 
             << pm.pivot_index << "," << pm.pivot_vertex << "," << pm.pivot_score << "," 
             << pm.scc_found_size << "," << pm.remaining_vertices << "," << pm.reachability_work << "\n";
    }
}

} // namespace scc
