#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <omp.h>
#include <map>
#include <filesystem>
#include <fstream>
#include "graph/graph.hpp"
#include "scc/tarjan.hpp"
#include "scc/bgss.hpp"
#include "scc/multi_bgss.hpp"
#include "scc/vgc.hpp"
#include "benchmark/metrics.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

int count_sccs(const vector<int>& map_scc) {
    int m = -1;
    for (int id : map_scc) if (id > m) m = id;
    return (m == -1) ? 0 : m + 1;
}

int get_max_scc_size(const vector<int>& scc_map) {
    if (scc_map.empty()) return 0;
    map<int, int> counts;
    for (int id : scc_map) if (id != -1) counts[id]++;
    int max_s = 0;
    for (auto const& [id, count] : counts) if (count > max_s) max_s = count;
    return max_s;
}

void print_gbbs_style_report(const benchmark::Result& res) {
    cout << "### Application: StronglyConnectedComponents" << endl;
    cout << "### Algorithm: " << res.algorithm << endl;
    cout << "### Graph: " << res.graph_name << endl;
    cout << "### Threads: " << res.threads << endl;
    cout << "### n: " << res.vertices << ", m: " << res.edges << endl;
    cout << "### Running Time: " << fixed << setprecision(3) << res.time_ms << " ms" << endl;
    cout << "### Throughput: " << res.throughput << " M-edges/s" << endl;
    cout << "### Stats: #SCCs: " << res.scc_count << ", Max SCC: " << res.max_scc_size << endl;
    cout << "### ------------------------------------" << endl;
}

int main(int argc, char** argv) {
    // Cargar configuración
    ifstream f_bench("config/benchmark.json");
    if (!f_bench.is_open()) { cerr << "Error: config/benchmark.json not found" << endl; return 1; }
    json config = json::parse(f_bench);
    
    ifstream f_params("config/parameters.json");
    if (!f_params.is_open()) { cerr << "Error: config/parameters.json not found" << endl; return 1; }
    json params = json::parse(f_params);

    vector<int> thread_counts = config["thread_counts"];
    vector<string> graph_paths = config["graphs"];

    vector<benchmark::Result> all_results;

    cout << "=======================================================" << endl;
    cout << "  SISTEMA DE EXPERIMENTACION CIENTIFICA: SCC DUAL      " << endl;
    cout << "  (Inspirado en GBBS: FW-BW vs Multi-Search)           " << endl;
    cout << "=======================================================" << endl;
    cout << "Hilos maximos disponibles: " << omp_get_max_threads() << endl;

    for (const auto& g_path : graph_paths) {
        if (!fs::exists(g_path)) {
            cout << "Skipping " << g_path << " (not found)" << endl;
            continue;
        }
        
        string g_name = fs::path(g_path).filename().string();
        graph::Graph g = graph::Graph::load_from_file(g_path);

        // 1. Tarjan (Base para correctitud)
        auto start = chrono::high_resolution_clock::now();
        auto scc_tarjan = scc::Tarjan::find_sccs(g);
        auto end = chrono::high_resolution_clock::now();
        double t_ms = chrono::duration<double, milli>(end - start).count();
        
        benchmark::Result res_tarjan;
        res_tarjan.algorithm = "Tarjan";
        res_tarjan.graph_name = g_name;
        res_tarjan.vertices = g.num_vertices;
        res_tarjan.edges = g.num_edges;
        res_tarjan.threads = 1;
        res_tarjan.time_ms = t_ms;
        res_tarjan.scc_count = count_sccs(scc_tarjan);
        res_tarjan.max_scc_size = get_max_scc_size(scc_tarjan);
        res_tarjan.throughput = (g.num_edges / 1000.0) / t_ms;
        res_tarjan.correct = true;
        all_results.push_back(res_tarjan);
        print_gbbs_style_report(res_tarjan);

        // 2. BGSS Optimizado (FW-BW con Big SCC)
        for (int threads : thread_counts) {
            omp_set_num_threads(threads);
            benchmark::Breakdown bd;
            start = chrono::high_resolution_clock::now();
            auto scc_bgss = scc::BGSS::find_sccs(g, false, &bd);
            end = chrono::high_resolution_clock::now();
            t_ms = chrono::duration<double, milli>(end - start).count();
            
            benchmark::Result res;
            res.algorithm = "BGSS-BigSCC";
            res.graph_name = g_name;
            res.vertices = g.num_vertices;
            res.edges = g.num_edges;
            res.threads = threads;
            res.time_ms = t_ms;
            res.scc_count = count_sccs(scc_bgss);
            res.max_scc_size = get_max_scc_size(scc_bgss);
            res.throughput = (g.num_edges / 1000.0) / t_ms;
            res.correct = (res.scc_count == res_tarjan.scc_count);
            res.breakdown = bd;
            all_results.push_back(res);
            print_gbbs_style_report(res);
        }

        // 3. Multi-Search BGSS
        for (int threads : thread_counts) {
            omp_set_num_threads(threads);
            benchmark::Breakdown bd;
            start = chrono::high_resolution_clock::now();
            auto scc_multi = scc::MultiBGSS::find_sccs(g, 1.1, &bd);
            end = chrono::high_resolution_clock::now();
            t_ms = chrono::duration<double, milli>(end - start).count();

            benchmark::Result res;
            res.algorithm = "BGSS-MultiSearch";
            res.graph_name = g_name;
            res.vertices = g.num_vertices;
            res.edges = g.num_edges;
            res.threads = threads;
            res.time_ms = t_ms;
            res.scc_count = count_sccs(scc_multi);
            res.max_scc_size = get_max_scc_size(scc_multi);
            res.throughput = (g.num_edges / 1000.0) / t_ms;
            res.correct = (res.scc_count == res_tarjan.scc_count);
            res.breakdown = bd;
            all_results.push_back(res);
            print_gbbs_style_report(res);
        }
    }

    benchmark::save_results_to_csv(all_results, "results/csv/benchmarks_dual.csv");
    benchmark::save_breakdown_to_csv(all_results, "results/csv/breakdown_dual.csv");
    cout << "\n[OK] Benchmarking finalizado. Archivos generados en results/csv/" << endl;

    return 0;
}
