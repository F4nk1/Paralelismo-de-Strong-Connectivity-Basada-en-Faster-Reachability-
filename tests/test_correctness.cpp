#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include "graph/graph.hpp"
#include "scc/tarjan.hpp"
#include "scc/bgss.hpp"
#include "scc/vgc.hpp"
#include "scc/multi_bgss.hpp"
#include "scc/ap_bgss.hpp"

using namespace std;

// Normaliza el mapeo de SCCs para que sea fácil compararlos
// Dos mapeos son iguales si agrupan los mismos nodos en los mismos conjuntos
vector<int> normalize_scc_map(const vector<int>& scc_map) {
    int n = scc_map.size();
    if (n == 0) return {};
    
    map<int, int> id_map;
    int next_id = 0;
    vector<int> normalized(n);
    
    for (int i = 0; i < n; ++i) {
        int old_id = scc_map[i];
        if (id_map.find(old_id) == id_map.end()) {
            id_map[old_id] = next_id++;
        }
        normalized[i] = id_map[old_id];
    }
    return normalized;
}

bool compare_sccs(const vector<int>& map1, const vector<int>& map2) {
    if (map1.size() != map2.size()) return false;
    return normalize_scc_map(map1) == normalize_scc_map(map2);
}

void test_graph(const string& name, const graph::Graph& g) {
    cout << "Testing graph: " << name << " (" << g.num_vertices << " vertices, " << g.num_edges << " edges)" << endl;
    
    auto scc_tarjan = scc::Tarjan::find_sccs(g);
    auto scc_bgss = scc::BGSS::find_sccs(g);
    auto scc_vgc = scc::VGC::run_parallel_scc(g, 10); // Tau pequeño para forzar paralelo
    auto scc_multi = scc::MultiBGSS::find_sccs(g);
    auto scc_ap_sum = scc::AP_BGSS::find_sccs(g, scc::PivotStrategy::SUM);
    auto scc_ap_max = scc::AP_BGSS::find_sccs(g, scc::PivotStrategy::MAX);
    auto scc_ap_mul = scc::AP_BGSS::find_sccs(g, scc::PivotStrategy::MUL);
    
    bool bgss_ok = compare_sccs(scc_tarjan, scc_bgss);
    bool vgc_ok = compare_sccs(scc_tarjan, scc_vgc);
    bool multi_ok = compare_sccs(scc_tarjan, scc_multi);
    bool ap_sum_ok = compare_sccs(scc_tarjan, scc_ap_sum);
    bool ap_max_ok = compare_sccs(scc_tarjan, scc_ap_max);
    bool ap_mul_ok = compare_sccs(scc_tarjan, scc_ap_mul);
    
    cout << "  BGSS:   " << (bgss_ok ? "PASSED" : "FAILED") << endl;
    cout << "  VGC:    " << (vgc_ok ? "PASSED" : "FAILED") << endl;
    cout << "  Multi:  " << (multi_ok ? "PASSED" : "FAILED") << endl;
    cout << "  AP-SUM: " << (ap_sum_ok ? "PASSED" : "FAILED") << endl;
    cout << "  AP-MAX: " << (ap_max_ok ? "PASSED" : "FAILED") << endl;
    cout << "  AP-MUL: " << (ap_mul_ok ? "PASSED" : "FAILED") << endl;
    
    if (!bgss_ok || !vgc_ok || !multi_ok || !ap_sum_ok || !ap_max_ok || !ap_mul_ok) {
        exit(1);
    }
}

int main() {
    // Grafo 1: Un ciclo simple 0->1->2->0
    graph::Graph g1;
    g1.num_vertices = 3;
    g1.num_edges = 3;
    g1.offsets = {0, 1, 2, 3};
    g1.neighbors = {1, 2, 0};
    test_graph("Simple Cycle", g1);

    // Grafo 2: Dos componentes fuertemente conexas 0->1, 1->0, 2->3, 3->2, 1->2
    graph::Graph g2;
    g2.num_vertices = 4;
    g2.num_edges = 5;
    g2.offsets = {0, 1, 3, 4, 5};
    g2.neighbors = {1, 0, 2, 3, 2};
    test_graph("Two SCCs", g2);

    // Grafo 3: Grafo acíclico 0->1, 1->2
    graph::Graph g3;
    g3.num_vertices = 3;
    g3.num_edges = 2;
    g3.offsets = {0, 1, 2, 2};
    g3.neighbors = {1, 2};
    test_graph("DAG", g3);

    cout << "\nAll correctness tests passed!" << endl;
    return 0;
}
