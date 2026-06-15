#include "multi_bgss.hpp"
#include "hashbag.hpp"
#include <omp.h>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <random>

namespace scc {

/**
 * @brief Realiza una búsqueda multi-pivote paralela (estilo GBBS).
 */
void multi_search(const graph::Graph& g, const std::vector<char>& handled, 
                  std::vector<int>& labels, const std::vector<int>& centers, 
                  ConcurrentHashTable<int, int>& table) {
    int n = g.num_vertices;
    std::vector<int> frontier = centers;
    
    // Inicializar la tabla con los centros
    for (int c : centers) {
        table.insert(c, c); // Usamos el ID del centro como su etiqueta
    }

    while (!frontier.empty()) {
        std::vector<int> next_frontier;
        #pragma omp parallel
        {
            std::vector<int> local_next;
            #pragma omp for nowait
            for (size_t i = 0; i < frontier.size(); i++) {
                int u = frontier[i];
                // Obtener la etiqueta de u de la tabla
                int u_label = -1;
                // Nota: Esto es simplificado. GBBS maneja múltiples etiquetas por nodo.
                // Aquí asumiremos que el primer pivote que llega a un nodo le asigna su etiqueta.
                // Buscamos la etiqueta de u en la tabla (asumimos que existe porque u estaba en la frontera)
                size_t h = table.hash(u);
                while (table.get_table()[h].key != -1) {
                    if (table.get_table()[h].key == u) {
                        u_label = table.get_table()[h].value;
                        break;
                    }
                    h = (h + 1) % table.get_table().size();
                }

                if (u_label == -1) continue;

                for (long long j = g.offsets[u]; j < g.offsets[u + 1]; j++) {
                    int v = g.neighbors[j];
                    if (!handled[v] && labels[v] == -1) {
                        // Intentar insertar en la tabla. Si es la primera vez que v es alcanzado, se añade a local_next.
                        if (table.insert(v, u_label)) {
                            // En una implementación real, verificaríamos si v ya fue añadido a la frontera en este paso.
                            // Por ahora, usamos un enfoque de buffer local.
                            local_next.push_back(v);
                        }
                    }
                }
            }
            #pragma omp critical
            {
                next_frontier.insert(next_frontier.end(), local_next.begin(), local_next.end());
            }
        }
        frontier = std::move(next_frontier);
    }
}

std::vector<int> MultiBGSS::find_sccs(const graph::Graph& g, double beta, benchmark::Breakdown* breakdown) {
    auto total_start = std::chrono::high_resolution_clock::now();
    int n = g.num_vertices;
    std::vector<int> scc_map(n, -1);
    std::vector<int> labels(n, -1);
    std::vector<char> handled(n, 0);
    std::atomic<int> scc_count(0);
    int handled_count = 0;

    graph::Graph g_rev = g.transpose();

    // 1. Trimming (igual que en BGSS para consistencia)
    auto trim_start = std::chrono::high_resolution_clock::now();
    // ... (Podríamos reutilizar el código de trimming o simplificarlo aquí)
    auto trim_end = std::chrono::high_resolution_clock::now();
    if (breakdown) breakdown->trimming += std::chrono::duration<double, std::milli>(trim_end - trim_start).count();

    // 2. Selección de Pivotes Aleatorios (Estrategia GBBS)
    std::vector<int> p;
    for (int i = 0; i < n; i++) if (!handled[i]) p.push_back(i);
    std::shuffle(p.begin(), p.end(), std::mt19937(std::random_device()()));

    size_t cur_offset = 0;
    size_t step_size = 1;
    
    while (handled_count < n && cur_offset < p.size()) {
        size_t end = std::min(cur_offset + step_size, p.size());
        std::vector<int> centers;
        for (size_t i = cur_offset; i < end; i++) {
            if (!handled[p[i]]) centers.push_back(p[i]);
        }
        
        if (centers.empty()) {
            cur_offset = end;
            step_size = static_cast<size_t>(step_size * beta) + 1;
            continue;
        }

        ConcurrentHashTable<int, int> fw_table(n * 2);
        ConcurrentHashTable<int, int> bw_table(n * 2);

        auto reach_start = std::chrono::high_resolution_clock::now();
        multi_search(g, handled, scc_map, centers, fw_table);
        multi_search(g_rev, handled, scc_map, centers, bw_table);
        auto reach_end = std::chrono::high_resolution_clock::now();
        if (breakdown) breakdown->reachability += std::chrono::duration<double, std::milli>(reach_end - reach_start).count();

        // Intersección de etiquetas para encontrar SCCs
        auto label_start = std::chrono::high_resolution_clock::now();
        #pragma omp parallel for
        for (int i = 0; i < n; i++) {
            if (handled[i]) continue;
            // Si un nodo i fue alcanzado por el mismo pivote en FW y BW, es un SCC
            int fw_label = -1, bw_label = -1;
            // (Búsqueda en tablas simplificada)
            size_t h_fw = fw_table.hash(i);
            while (fw_table.get_table()[h_fw].key != -1) {
                if (fw_table.get_table()[h_fw].key == i) { fw_label = fw_table.get_table()[h_fw].value; break; }
                h_fw = (h_fw + 1) % fw_table.get_table().size();
            }
            size_t h_bw = bw_table.hash(i);
            while (bw_table.get_table()[h_bw].key != -1) {
                if (bw_table.get_table()[h_bw].key == i) { bw_label = bw_table.get_table()[h_bw].value; break; }
                h_bw = (h_bw + 1) % bw_table.get_table().size();
            }

            if (fw_label != -1 && fw_label == bw_label) {
                scc_map[i] = fw_label; // Simplificación: usamos el ID del pivote como ID de SCC
                handled[i] = 1;
                #pragma omp atomic
                handled_count++;
            }
        }
        auto label_end = std::chrono::high_resolution_clock::now();
        if (breakdown) breakdown->labeling += std::chrono::duration<double, std::milli>(label_end - label_start).count();

        cur_offset = end;
        step_size = static_cast<size_t>(step_size * beta) + 1;
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    if (breakdown) breakdown->total = std::chrono::duration<double, std::milli>(total_end - total_start).count();

    return scc_map;
}

} // namespace scc
