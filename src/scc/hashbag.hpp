#ifndef HASHBAG_HPP
#define HASHBAG_HPP

#include <vector>
#include <atomic>
#include <omp.h>

namespace scc {

/**
 * @brief Implementación simplificada de un Parallel Hash Bag.
 * Permite inserciones concurrentes y procesamiento en paralelo.
 */
template <typename T>
class HashBag {
public:
    HashBag(size_t capacity) : data(capacity), size(0) {
        // Inicializar con valores que indiquen "vacío" si es necesario
    }

    void insert(T item) {
        // En una implementación real de Hash Bag, usaríamos hashing para distribuir.
        // Aquí usaremos un enfoque de buffer local por hilo para simplicidad y rendimiento.
        size_t idx = size.fetch_add(1, std::memory_order_relaxed);
        if (idx < data.size()) {
            data[idx] = item;
        }
    }

    const std::vector<T>& get_data() const { return data; }
    size_t get_size() const { return std::min(size.load(), data.size()); }

    void clear() {
        size.store(0);
    }

private:
    std::vector<T> data;
    std::atomic<size_t> size;
};

} // namespace scc

#endif // HASHBAG_HPP
