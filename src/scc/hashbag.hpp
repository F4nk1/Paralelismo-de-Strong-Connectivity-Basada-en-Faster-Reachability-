#ifndef HASHBAG_HPP
#define HASHBAG_HPP

#include <vector>
#include <atomic>
#include <omp.h>
#include <tuple>

namespace scc {

/**
 * @brief Implementación de una Tabla de Hash Concurrente mínima para Multi-Search.
 * Basada en el concepto de resizable_table de GBBS.
 */
template <typename K, typename V>
class ConcurrentHashTable {
public:
    struct Entry {
        K key;
        V value;
    };

    ConcurrentHashTable(size_t capacity) : m_capacity(capacity), m_size(0) {
        m_table.resize(m_capacity, {static_cast<K>(-1), static_cast<V>(-1)});
    }

    // Hash simple de 32 bits (ajustable)
    size_t hash(K key) const {
        size_t x = static_cast<size_t>(key);
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x % m_capacity;
    }

    bool insert(K key, V value) {
        size_t h = hash(key);
        while (true) {
            K current_key = __sync_val_compare_and_swap(&m_table[h].key, static_cast<K>(-1), key);
            if (current_key == static_cast<K>(-1)) {
                // Nueva entrada
                m_table[h].value = value;
                m_size++;
                return true;
            }
            if (current_key == key) {
                // Llave existente, intentar actualizar si el nuevo valor es menor
                V old_val = m_table[h].value;
                while (value < old_val) {
                    if (__sync_bool_compare_and_swap(&m_table[h].value, old_val, value)) {
                        return true;
                    }
                    old_val = m_table[h].value;
                }
                return false;
            }
            h = (h + 1) % m_capacity; // Linear probing
        }
    }

    bool contains(K key, V value) const {
        size_t h = hash(key);
        while (m_table[h].key != static_cast<K>(-1)) {
            if (m_table[h].key == key && m_table[h].value == value) return true;
            h = (h + 1) % m_capacity;
        }
        return false;
    }

    const std::vector<Entry>& get_table() const { return m_table; }
    size_t size() const { return m_size.load(); }

private:
    std::vector<Entry> m_table;
    size_t m_capacity;
    std::atomic<size_t> m_size;
};

/**
 * @brief HashBag original (manteniendo compatibilidad)
 */
template <typename T>
class HashBag {
public:
    HashBag(size_t capacity) : data(capacity), size(0) {}

    void insert(T item) {
        size_t idx = size.fetch_add(1, std::memory_order_relaxed);
        if (idx < data.size()) {
            data[idx] = item;
        }
    }

    const std::vector<T>& get_data() const { return data; }
    size_t get_size() const { return std::min(size.load(), data.size()); }
    void clear() { size.store(0); }

private:
    std::vector<T> data;
    std::atomic<size_t> size;
};

} // namespace scc

#endif // HASHBAG_HPP
