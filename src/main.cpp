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
#include "scc/vgc.hpp"
#include "benchmark/metrics.hpp"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

int count_sccs(const vector<int>& map) {
    int m = -1;
    for (int id : map) if (id > m) m = id;
    return (m == -1) ? 0 : m + 1;
}

int main(int argc, char** argv) {
    // Cargar configuración
    ifstream f_bench("config/benchmark.json");
    json config = json::parse(f_bench);
    
    ifstream f_params("config/parameters.json");
    json params = json::parse(f_params);

    vector<int> tau_values = config["tau_values"];
    vector<int> thread_counts = config["thread_counts"];
    vector<string> graph_paths = config["graphs"];

    vector<benchmark::Result> all_results;

    cout << "=======================================================" << endl;
    cout << "  SISTEMA DE EXPERIMENTACION CIENTIFICA: SCC PARALELO  " << endl;
    cout << "=======================================================" << endl;
    cout << "Hilos maximos disponibles: " << omp_get_max_threads() << endl;

    for (const auto& g_path : graph_paths) {
        if (!fs::exists(g_path)) {
            cout << "Skipping " << g_path << " (not found)" << endl;
            continue;
        }
        
        string g_name = fs::path(g_path).filename().string();
        cout << "\n>>> Procesando: " << g_name << endl;
        graph::Graph g = graph::Graph::load_from_file(g_path);

        // 1. Tarjan (Base para correctitud)
        auto start = chrono::high_resolution_clock::now();
        auto scc_tarjan = scc::Tarjan::find_sccs(g);
        auto end = chrono::high_resolution_clock::now();
        double t_tarjan = chrono::duration<double, milli>(end - start).count();
        int n_tarjan = count_sccs(scc_tarjan);
        
        benchmark::Result res_tarjan;
        res_tarjan.algorithm = "Tarjan";
        res_tarjan.graph_name = g_name;
        res_tarjan.vertices = g.num_vertices;
        res_tarjan.edges = g.num_edges;
        res_tarjan.threads = 1;
        res_tarjan.tau = -1;
        res_tarjan.time_ms = t_tarjan;
        res_tarjan.scc_count = n_tarjan;
        res_tarjan.correct = true;
        all_results.push_back(res_tarjan);
        cout << "  [REF] Tarjan: " << t_tarjan << "ms | SCCs: " << n_tarjan << endl;

        // 2. Escalabilidad (BGSS con diferentes hilos)
        for (int threads : thread_counts) {
            omp_set_num_threads(threads);
            benchmark::Breakdown bd;
            start = chrono::high_resolution_clock::now();
            auto scc_bgss = scc::BGSS::find_sccs(g, false, &bd);
            end = chrono::high_resolution_clock::now();
            double t_bgss = chrono::duration<double, milli>(end - start).count();
            int n_bgss = count_sccs(scc_bgss);
            
            benchmark::Result res;
            res.algorithm = "BGSS";
            res.graph_name = g_name;
            res.vertices = g.num_vertices;
            res.edges = g.num_edges;
            res.threads = threads;
            res.tau = -1;
            res.time_ms = t_bgss;
            res.scc_count = n_bgss;
            res.correct = (n_bgss == n_tarjan);
            res.breakdown = bd;
            all_results.push_back(res);
            cout << "  [PAR] BGSS (" << threads << " th): " << t_bgss << "ms" << endl;
        }

        // 3. Análisis de Tau (VGC)
        int default_threads = params["default_threads"];
        omp_set_num_threads(default_threads);
        for (int tau : tau_values) {
            benchmark::Breakdown bd;
            start = chrono::high_resolution_clock::now();
            auto scc_vgc = scc::VGC::run_parallel_scc(g, tau, &bd);
            end = chrono::high_resolution_clock::now();
            double t_vgc = chrono::duration<double, milli>(end - start).count();
            int n_vgc = count_sccs(scc_vgc);

            benchmark::Result res;
            res.algorithm = "VGC (tau=" + to_string(tau) + ")";
            res.graph_name = g_name;
            res.vertices = g.num_vertices;
            res.edges = g.num_edges;
            res.threads = default_threads;
            res.tau = tau;
            res.time_ms = t_vgc;
            res.scc_count = n_vgc;
            res.correct = (n_vgc == n_tarjan);
            res.breakdown = bd;
            all_results.push_back(res);
            if (tau == 512) cout << "  [VGC] Tau=512: " << t_vgc << "ms" << endl;
        }

        // 4. HashBag Comparison
        omp_set_num_threads(default_threads);
        for (bool use_hb : {false, true}) {
            benchmark::Breakdown bd;
            start = chrono::high_resolution_clock::now();
            auto scc_bgss = scc::BGSS::find_sccs(g, use_hb, &bd);
            end = chrono::high_resolution_clock::now();
            double t_hb = chrono::duration<double, milli>(end - start).count();
            
            benchmark::Result res;
            res.algorithm = use_hb ? "BGSS (HashBag)" : "BGSS (Vector)";
            res.graph_name = g_name;
            res.vertices = g.num_vertices;
            res.edges = g.num_edges;
            res.threads = default_threads;
            res.tau = -1;
            res.time_ms = t_hb;
            res.scc_count = count_sccs(scc_bgss);
            res.correct = true;
            res.breakdown = bd;
            all_results.push_back(res);
        }
    }

    // Guardar resultados en múltiples archivos como solicitó el usuario
    benchmark::save_results_to_csv(all_results, "results/csv/benchmarks.csv");
    benchmark::save_breakdown_to_csv(all_results, "results/csv/breakdown.csv");
    benchmark::save_tau_analysis_to_csv(all_results, "results/csv/tau_analysis.csv");
    benchmark::save_hashbag_comparison_to_csv(all_results, "results/csv/hashbag_vs_vector.csv");
    benchmark::save_scalability_to_csv(all_results, "results/csv/scalability.csv");

    cout << "\n[OK] Benchmarking finalizado. Archivos generados en results/csv/" << endl;

    return 0;
}
