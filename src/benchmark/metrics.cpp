#include "metrics.hpp"
#include <fstream>
#include <iomanip>
#include <map>

namespace benchmark {

void save_results_to_csv(const std::vector<Result>& results, const std::string& filename) {
    std::ofstream file(filename);
    file << "algorithm,graph,nodes,edges,threads,tau,time_ms,num_sccs,max_scc_size,throughput,trim_count,correct\n";
    for (const auto& res : results) {
        file << res.algorithm << "," << res.graph_name << "," << res.vertices << "," << res.edges << "," 
             << res.threads << "," << res.tau << "," << res.time_ms << "," 
             << res.scc_count << "," << res.max_scc_size << "," << res.throughput << "," << res.trim_count << "," 
             << (res.correct ? "YES" : "NO") << "\n";
    }
}

void save_breakdown_to_csv(const std::vector<Result>& results, const std::string& filename) {
    std::ofstream file(filename);
    file << "algorithm,graph,trimming,forward_reach,backward_reach,reachability,labeling,hash_bag,big_scc,others,total\n";
    for (const auto& res : results) {
        file << res.algorithm << "," << res.graph_name << "," 
             << res.breakdown.trimming << "," 
             << res.breakdown.forward_reachability << "," 
             << res.breakdown.backward_reachability << "," 
             << res.breakdown.reachability << "," 
             << res.breakdown.labeling << "," 
             << res.breakdown.hash_bag << "," 
             << res.breakdown.big_scc << "," 
             << res.breakdown.others << "," 
             << res.breakdown.total << "\n";
    }
}

void save_tau_analysis_to_csv(const std::vector<Result>& results, const std::string& filename) {
    std::ofstream file(filename);
    file << "graph,tau,time_ms\n";
    for (const auto& res : results) {
        if (res.algorithm.find("VGC") != std::string::npos) {
            file << res.graph_name << "," << res.tau << "," << res.time_ms << "\n";
        }
    }
}

void save_hashbag_comparison_to_csv(const std::vector<Result>& results, const std::string& filename) {
    std::ofstream file(filename);
    file << "graph,structure,time_ms\n";
    for (const auto& res : results) {
        if (res.algorithm == "BGSS (Vector)" || res.algorithm == "BGSS (HashBag)") {
            std::string structure = (res.algorithm == "BGSS (Vector)") ? "std::vector" : "HashBag";
            file << res.graph_name << "," << structure << "," << res.time_ms << "\n";
        }
    }
}

void save_scalability_to_csv(const std::vector<Result>& results, const std::string& filename) {
    std::ofstream file(filename);
    file << "algorithm,graph,threads,time_ms,speedup,efficiency\n";
    
    // Base times for speedup calculation (1 thread)
    std::map<std::pair<std::string, std::string>, double> base_times;
    for (const auto& res : results) {
        if (res.threads == 1) {
            base_times[{res.algorithm, res.graph_name}] = res.time_ms;
        }
    }

    for (const auto& res : results) {
        double speedup = 0.0;
        double efficiency = 0.0;
        if (base_times.count({res.algorithm, res.graph_name})) {
            speedup = base_times[{res.algorithm, res.graph_name}] / res.time_ms;
            efficiency = speedup / res.threads;
        }
        file << res.algorithm << "," << res.graph_name << "," << res.threads << "," 
             << res.time_ms << "," << speedup << "," << efficiency << "\n";
    }
}

} // namespace benchmark
