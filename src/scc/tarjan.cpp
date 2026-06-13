#include "tarjan.hpp"
#include <algorithm>
#include <vector>

namespace scc {

struct StackFrame {
    int v;
    long long next_edge_idx;
};

std::vector<int> Tarjan::find_sccs(const graph::Graph& g) {
    int n = g.num_vertices;
    std::vector<int> indices(n, -1);
    std::vector<int> lowlink(n, -1);
    std::vector<char> on_stack(n, 0);
    std::vector<int> s; // Custom stack for SCC nodes
    std::vector<int> scc_map(n, -1);
    int index = 0;
    int scc_count = 0;

    std::vector<StackFrame> dfs_stack;

    for (int i = 0; i < n; ++i) {
        if (indices[i] == -1) {
            // Start iterative DFS from i
            dfs_stack.push_back({i, g.offsets[i]});
            indices[i] = lowlink[i] = index++;
            s.push_back(i);
            on_stack[i] = 1;

            while (!dfs_stack.empty()) {
                StackFrame& frame = dfs_stack.back();
                int v = frame.v;

                if (frame.next_edge_idx < g.offsets[v + 1]) {
                    int w = g.neighbors[frame.next_edge_idx];
                    frame.next_edge_idx++;

                    if (indices[w] == -1) {
                        // Tree edge
                        indices[w] = lowlink[w] = index++;
                        s.push_back(w);
                        on_stack[w] = 1;
                        dfs_stack.push_back({w, g.offsets[w]});
                    } else if (on_stack[w]) {
                        // Back edge
                        lowlink[v] = std::min(lowlink[v], indices[w]);
                    }
                } else {
                    // All edges processed for v
                    if (lowlink[v] == indices[v]) {
                        while (true) {
                            int w = s.back();
                            s.pop_back();
                            on_stack[w] = 0;
                            scc_map[w] = scc_count;
                            if (w == v) break;
                        }
                        scc_count++;
                    }
                    dfs_stack.pop_back();
                    if (!dfs_stack.empty()) {
                        int parent = dfs_stack.back().v;
                        lowlink[parent] = std::min(lowlink[parent], lowlink[v]);
                    }
                }
            }
        }
    }

    return scc_map;
}

} // namespace scc
