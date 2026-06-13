#include "bgss.hpp"
#include "graph/graph.hpp"
#include "hashbag.hpp"
#include <omp.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>

namespace scc {

// Global or shared next frontier to avoid repeated allocation
// In a production environment, this would be handled more carefully.
static std::vector<int> g_next_frontier;

void parallel_bfs(int pivot, const graph::Graph& g, const std::vector<char>& handled, 
                  std::vector<char>& visited, std::vector<int>& reached_list) {
    reached_list.clear();
    if (handled[pivot]) return;

    visited[pivot] = 1;
    reached_list.push_back(pivot);
    
    std::vector<int> frontier;
    frontier.push_back(pivot);

    #pragma omp parallel
    {
        std::vector<int> local_next;
        while (true) {
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

            #pragma omp critical
            {
                g_next_frontier.insert(g_next_frontier.end(), local_next.begin(), local_next.end());
                reached_list.insert(reached_list.end(), local_next.begin(), local_next.end());
            }
            local_next.clear();

            #pragma omp barrier
            #pragma omp single
            {
                frontier = std::move(g_next_frontier);
                g_next_frontier.clear();
            }
            #pragma omp barrier
            
            if (frontier.empty()) break;
        }
    }
}

std::vector<int> BGSS::find_sccs(const graph::Graph& g, bool use_hashbag, benchmark::Breakdown* breakdown) {
    auto total_start = std::chrono::high_resolution_clock::now();
    int n = g.num_vertices;
    std::vector<int> scc_map(n, -1);
    std::atomic<int> scc_count(0);
    
    graph::Graph g_rev = g.transpose();
    std::vector<char> handled(n, 0);
    int handled_count = 0;

    // Phase 1: Recursive Trimming
    auto trim_start = std::chrono::high_resolution_clock::now();
    std::vector<int> in_deg(n, 0);
    std::vector<int> out_deg(n, 0);
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        out_deg[i] = g.offsets[i+1] - g.offsets[i];
    }
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (long long j = g.offsets[i]; j < g.offsets[i + 1]; ++j) {
            #pragma omp atomic
            in_deg[g.neighbors[j]]++;
        }
    }

    std::vector<int> q;
    #pragma omp parallel
    {
        std::vector<int> q_local;
        #pragma omp for nowait
        for (int i = 0; i < n; ++i) {
            if (in_deg[i] == 0 || out_deg[i] == 0) q_local.push_back(i);
        }
        #pragma omp critical
        q.insert(q.end(), q_local.begin(), q_local.end());
    }

    size_t head = 0;
    while(head < q.size()) {
        int u = q[head++];
        if (handled[u]) continue;
        handled[u] = 1;
        scc_map[u] = scc_count.fetch_add(1);
        handled_count++;

        if (out_deg[u] > 0) {
             for (long long j = g.offsets[u]; j < g.offsets[u + 1]; ++j) {
                 int v = g.neighbors[j];
                 if (--in_deg[v] == 0) q.push_back(v);
             }
        }
        for (long long j = g_rev.offsets[u]; j < g_rev.offsets[u + 1]; ++j) {
            int v = g_rev.neighbors[j];
            if (--out_deg[v] == 0) q.push_back(v);
        }
    }
    auto trim_end = std::chrono::high_resolution_clock::now();
    if (breakdown) breakdown->trimming += std::chrono::duration<double, std::milli>(trim_end - trim_start).count();

    // Phase 2: BGSS
    std::vector<char> visited_desc(n, 0);
    std::vector<char> visited_anc(n, 0);
    std::vector<int> reached_desc;
    std::vector<int> reached_anc;

    for (int i = 0; i < n; ++i) {
        if (handled[i]) continue;

        auto start_reach = std::chrono::high_resolution_clock::now();
        parallel_bfs(i, g, handled, visited_desc, reached_desc);
        auto mid_reach = std::chrono::high_resolution_clock::now();
        parallel_bfs(i, g_rev, handled, visited_anc, reached_anc);
        auto end_reach = std::chrono::high_resolution_clock::now();

        if (breakdown) {
            double d1 = std::chrono::duration<double, std::milli>(mid_reach - start_reach).count();
            double d2 = std::chrono::duration<double, std::milli>(end_reach - mid_reach).count();
            breakdown->forward_reachability += d1;
            breakdown->backward_reachability += d2;
            breakdown->reachability += (d1 + d2);
        }

        auto label_start = std::chrono::high_resolution_clock::now();
        int current_scc = scc_count.fetch_add(1);
        int found = 0;
        for (int node : reached_anc) {
            if (visited_desc[node]) {
                scc_map[node] = current_scc;
                handled[node] = 1;
                found++;
            }
        }
        handled_count += found;

        for (int node : reached_desc) visited_desc[node] = 0;
        for (int node : reached_anc) visited_anc[node] = 0;

        auto label_end = std::chrono::high_resolution_clock::now();
        if (breakdown) breakdown->labeling += std::chrono::duration<double, std::milli>(label_end - label_start).count();
        
        if (handled_count >= n) break;
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    if (breakdown) {
        breakdown->total = std::chrono::duration<double, std::milli>(total_end - total_start).count();
        breakdown->others = breakdown->total - (breakdown->trimming + breakdown->reachability + breakdown->labeling);
    }

    return scc_map;
}

} // namespace scc
