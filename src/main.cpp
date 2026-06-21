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

#include <set>

int count_sccs(const vector<int>& map_scc) {
    if (map_scc.empty()) return 0;
    set<int> unique_ids;
    for (int id : map_scc) if (id != -1) unique_ids.insert(id);
    return unique_ids.size();
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
    int default_tau = params["default_tau"];

    vector<benchmark::Result> all_results;

    cout << "=======================================================" << endl;
    cout << "  SISTEMA DE EXPERIMENTACION CIENTIFICA: SCC DUAL      " << endl;
    cout << "  (Tarjan, BGSS-Vector, BGSS-HashBag, MultiSearch, VGC) " << endl;
    cout << "=======================================================" << endl;
    cout << "Hilos maximos disponibles: " << omp_get_max_threads() << endl;

    for (const auto& g_path : graph_paths) {
        if (!fs::exists(g_path)) {
            cout << "Skipping " << g_path << " (not found)" << endl;
            continue;
        }
        
        string g_name = fs::path(g_path).filename().string();
        cout << "\n>>> Cargando grafo: " << g_name << " ..." << endl;
        graph::Graph g = graph::Graph::load_from_file(g_path);
        cout << "Nodos: " << g.num_vertices << ", Aristas: " << g.num_edges << endl;

        // 1. Tarjan (Base para correctitud)
        cout << "Ejecutando Tarjan secuencial..." << endl;
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

        // 2. Evaluaciones Paralelas con 1, 2, 4, 8, 16 hilos
        for (int threads : thread_counts) {
            omp_set_num_threads(threads);
            cout << "Ejecutando algoritmos paralelos con " << threads << " hilo(s)..." << endl;

            // 2.1 BGSS (Vector)
            benchmark::Breakdown bd_v;
            start = chrono::high_resolution_clock::now();
            auto scc_v = scc::BGSS::find_sccs(g, false, &bd_v);
            end = chrono::high_resolution_clock::now();
            t_ms = chrono::duration<double, milli>(end - start).count();
            
            benchmark::Result res_v;
            res_v.algorithm = "BGSS-BigSCC";
            res_v.graph_name = g_name;
            res_v.vertices = g.num_vertices;
            res_v.edges = g.num_edges;
            res_v.threads = threads;
            res_v.time_ms = t_ms;
            res_v.scc_count = count_sccs(scc_v);
            res_v.max_scc_size = get_max_scc_size(scc_v);
            res_v.throughput = (g.num_edges / 1000.0) / t_ms;
            res_v.correct = (res_v.scc_count == res_tarjan.scc_count);
            res_v.breakdown = bd_v;
            all_results.push_back(res_v);
            
            // Duplicar para la comparativa HashBag vs Vector
            benchmark::Result res_v_comp = res_v;
            res_v_comp.algorithm = "BGSS (Vector)";
            all_results.push_back(res_v_comp);
            print_gbbs_style_report(res_v);

            // 2.2 BGSS (HashBag)
            benchmark::Breakdown bd_h;
            start = chrono::high_resolution_clock::now();
            auto scc_h = scc::BGSS::find_sccs(g, true, &bd_h);
            end = chrono::high_resolution_clock::now();
            t_ms = chrono::duration<double, milli>(end - start).count();
            
            benchmark::Result res_h;
            res_h.algorithm = "BGSS-BigSCC-HashBag";
            res_h.graph_name = g_name;
            res_h.vertices = g.num_vertices;
            res_h.edges = g.num_edges;
            res_h.threads = threads;
            res_h.time_ms = t_ms;
            res_h.scc_count = count_sccs(scc_h);
            res_h.max_scc_size = get_max_scc_size(scc_h);
            res_h.throughput = (g.num_edges / 1000.0) / t_ms;
            res_h.correct = (res_h.scc_count == res_tarjan.scc_count);
            res_h.breakdown = bd_h;
            all_results.push_back(res_h);
            
            // Duplicar para la comparativa HashBag vs Vector
            benchmark::Result res_h_comp = res_h;
            res_h_comp.algorithm = "BGSS (HashBag)";
            all_results.push_back(res_h_comp);
            print_gbbs_style_report(res_h);

            // 2.3 BGSS-MultiSearch
            benchmark::Breakdown bd_m;
            start = chrono::high_resolution_clock::now();
            auto scc_multi = scc::MultiBGSS::find_sccs(g, 1.1, &bd_m);
            end = chrono::high_resolution_clock::now();
            t_ms = chrono::duration<double, milli>(end - start).count();

            benchmark::Result res_m;
            res_m.algorithm = "BGSS-MultiSearch";
            res_m.graph_name = g_name;
            res_m.vertices = g.num_vertices;
            res_m.edges = g.num_edges;
            res_m.threads = threads;
            res_m.time_ms = t_ms;
            res_m.scc_count = count_sccs(scc_multi);
            res_m.max_scc_size = get_max_scc_size(scc_multi);
            res_m.throughput = (g.num_edges / 1000.0) / t_ms;
            res_m.correct = (res_m.scc_count == res_tarjan.scc_count);
            res_m.breakdown = bd_m;
            all_results.push_back(res_m);
            print_gbbs_style_report(res_m);

            // 2.4 VGC (Vertical Granularity Control) con default_tau
            benchmark::Breakdown bd_vgc;
            start = chrono::high_resolution_clock::now();
            auto scc_vgc = scc::VGC::run_parallel_scc(g, default_tau, &bd_vgc);
            end = chrono::high_resolution_clock::now();
            t_ms = chrono::duration<double, milli>(end - start).count();

            benchmark::Result res_vgc;
            res_vgc.algorithm = "VGC";
            res_vgc.graph_name = g_name;
            res_vgc.vertices = g.num_vertices;
            res_vgc.edges = g.num_edges;
            res_vgc.threads = threads;
            res_vgc.tau = default_tau;
            res_vgc.time_ms = t_ms;
            res_vgc.scc_count = count_sccs(scc_vgc);
            res_vgc.max_scc_size = get_max_scc_size(scc_vgc);
            res_vgc.throughput = (g.num_edges / 1000.0) / t_ms;
            res_vgc.correct = (res_vgc.scc_count == res_tarjan.scc_count);
            res_vgc.breakdown = bd_vgc;
            all_results.push_back(res_vgc);
            print_gbbs_style_report(res_vgc);
        }
    }

    // 3. Analisis de Tau (VGC) - Solo para grafos sinteticos si son muy grandes, usando hilos por defecto
    cout << "\nEjecutando analisis de Tau (VGC) con hilos maximos..." << endl;
    for (const auto& g_path : graph_paths) {
        if (!fs::exists(g_path)) continue;
        string g_name = fs::path(g_path).filename().string();
        graph::Graph g = graph::Graph::load_from_file(g_path);
        
        for (int tau : config["tau_values"]) {
            auto start = chrono::high_resolution_clock::now();
            auto scc_vgc = scc::VGC::run_parallel_scc(g, tau);
            auto end = chrono::high_resolution_clock::now();
            double t_ms = chrono::duration<double, milli>(end - start).count();
            
            benchmark::Result res;
            res.algorithm = "VGC";
            res.graph_name = g_name;
            res.threads = omp_get_max_threads();
            res.tau = tau;
            res.time_ms = t_ms;
            all_results.push_back(res);
        }
    }

    // Guardar todos los CSV
    benchmark::save_results_to_csv(all_results, "results/csv/benchmarks.csv");
    benchmark::save_breakdown_to_csv(all_results, "results/csv/breakdown.csv");
    benchmark::save_scalability_to_csv(all_results, "results/csv/scalability.csv");
    benchmark::save_tau_analysis_to_csv(all_results, "results/csv/tau_analysis.csv");
    benchmark::save_hashbag_comparison_to_csv(all_results, "results/csv/hashbag_vs_vector.csv");
    
    cout << "\n[OK] Benchmarking finalizado. Archivos generados en results/csv/" << endl;

    return 0;
}
