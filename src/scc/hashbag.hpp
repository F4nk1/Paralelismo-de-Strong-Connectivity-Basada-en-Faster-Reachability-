#ifndef HASHBAG_HPP
#define HASHBAG_HPP

#include <vector>
#include <atomic>
#include <omp.h>
#include <tuple>
#include <cstdint>

namespace scc {

/**
 * @brief Implementación de una Tabla de Hash Concurrente para Multi-Search.
 * Soporta múltiples valores para la misma llave, almacenando pares (key, value).
 * Optimizado para 64 bits usando atomics.
 */
template <typename K, typename V>
class ConcurrentHashTable {
public:
    struct Entry {
        K key;
        V value;
    };

    ConcurrentHashTable(size_t capacity) : m_capacity(capacity), m_size(0) {
        // Aseguramos que capacity sea potencia de 2 para hash rápido (opcional, pero recomendado)
        m_table = new std::atomic<uint64_t>[m_capacity];
        for (size_t i = 0; i < m_capacity; ++i) {
            m_table[i].store(combine(-1, -1), std::memory_order_relaxed);
        }
    }

    ~ConcurrentHashTable() {
        delete[] m_table;
    }

    // No permitir copia por el manejo de memoria manual
    ConcurrentHashTable(const ConcurrentHashTable&) = delete;
    ConcurrentHashTable& operator=(const ConcurrentHashTable&) = delete;

    inline uint64_t combine(K key, V value) const {
        return (static_cast<uint64_t>(static_cast<uint32_t>(key)) << 32) | 
               static_cast<uint64_t>(static_cast<uint32_t>(value));
    }

    inline std::pair<K, V> separate(uint64_t val) const {
        return {static_cast<K>(val >> 32), static_cast<V>(val & 0xFFFFFFFF)};
    }

    // Hash de la pareja (key, value) para distribuir mejor
    inline size_t hash_pair(K key, V value) const {
        uint64_t x = combine(key, value);
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return x % m_capacity;
    }

    bool insert(K key, V value) {
        uint64_t new_entry = combine(key, value);
        uint64_t empty_entry = combine(-1, -1);
        size_t h = hash_pair(key, value);

        while (true) {
            uint64_t expected = empty_entry;
            if (m_table[h].compare_exchange_strong(expected, new_entry, std::memory_order_relaxed)) {
                m_size.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
            if (expected == new_entry) {
                return false; // Ya existe el par (key, value)
            }
            h = (h + 1) % m_capacity;
        }
    }

    bool contains(K key, V value) const {
        uint64_t target = combine(key, value);
        uint64_t empty_entry = combine(-1, -1);
        size_t h = hash_pair(key, value);

        while (true) {
            uint64_t current = m_table[h].load(std::memory_order_relaxed);
            if (current == target) return true;
            if (current == empty_entry) return false;
            h = (h + 1) % m_capacity;
        }
    }

    size_t size() const { return m_size.load(); }
    size_t capacity() const { return m_capacity; }

    // Para iterar sobre la tabla
    std::atomic<uint64_t>* get_raw_table() { return m_table; }

private:
    std::atomic<uint64_t>* m_table;
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
