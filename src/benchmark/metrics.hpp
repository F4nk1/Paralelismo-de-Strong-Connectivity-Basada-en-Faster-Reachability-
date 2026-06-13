#ifndef METRICS_HPP
#define METRICS_HPP

#include <string>
#include <vector>

namespace benchmark {

struct Breakdown {
    double trimming = 0.0;
    double forward_reachability = 0.0;
    double backward_reachability = 0.0;
    double reachability = 0.0; // Total
    double labeling = 0.0;
    double hash_bag = 0.0;
    double others = 0.0;
    double total = 0.0;

    void reset() {
        trimming = forward_reachability = backward_reachability = reachability = labeling = hash_bag = others = total = 0.0;
    }
};

struct Result {
    std::string algorithm;
    std::string graph_name;
    int vertices;
    long long edges;
    int threads;
    int tau;
    double time_ms;
    int scc_count;
    bool correct;
    Breakdown breakdown;
};

void save_results_to_csv(const std::vector<Result>& results, const std::string& filename);
void save_breakdown_to_csv(const std::vector<Result>& results, const std::string& filename);
void save_tau_analysis_to_csv(const std::vector<Result>& results, const std::string& filename);
void save_hashbag_comparison_to_csv(const std::vector<Result>& results, const std::string& filename);
void save_scalability_to_csv(const std::vector<Result>& results, const std::string& filename);

} // namespace benchmark

#endif // METRICS_HPP
