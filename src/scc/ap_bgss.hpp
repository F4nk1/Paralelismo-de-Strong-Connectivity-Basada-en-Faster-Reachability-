#ifndef AP_BGSS_HPP
#define AP_BGSS_HPP

#include "graph/graph.hpp"
#include "benchmark/metrics.hpp"
#include <vector>
#include <string>

namespace scc {

enum class PivotStrategy {
    SUM, // indegree + outdegree
    MAX, // max(indegree, outdegree)
    MUL  // indegree * outdegree
};

std::string strategy_to_string(PivotStrategy strategy);

struct AP_PivotMetric {
    std::string graph_name;
    int threads;
    std::string strategy;
    int pivot_index;
    int pivot_vertex;
    double pivot_score;
    int scc_found_size;
    int remaining_vertices;
    long long reachability_work;
};

class AP_BGSS {
public:
    static std::vector<int> find_sccs(const graph::Graph& g, 
                                      PivotStrategy strategy, 
                                      bool use_hashbag = false, 
                                      benchmark::Breakdown* breakdown = nullptr,
                                      std::vector<AP_PivotMetric>* pivot_metrics = nullptr,
                                      std::string graph_name = "unknown");
};

void save_ap_metrics_to_csv(const std::vector<AP_PivotMetric>& metrics, const std::string& filename);

} // namespace scc

#endif // AP_BGSS_HPP
