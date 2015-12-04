//
// Created by Jaideep Ray on 11/11/15.

// Based on the implementation provided in mintatomic library.

#include <atomic>
#include <iostream>
#include <cassert>

//----------------------------------------------
class HashTable
{
public:
    struct Entry
    {
        atomic<int> key;
        atomic<int> value;
    };

private:
    Entry* m_entries;
    uint32_t m_arraySize;

public:
    HashTable(unsigned int arraySize);
    ~HashTable();

    // Basic operations
    void set(int key, int value);
    int get(int key);
    int size();
    void clear();
};


//----------------------------------------------
// from code.google.com/p/smhasher/wiki/MurmurHash3
inline static int integerHash(uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}


//----------------------------------------------
HashTable::HashTable(unsigned int arraySize)
{
    // Initialize cells
    assert((arraySize & (arraySize - 1)) == 0);   // Must be a power of 2
    m_arraySize = arraySize;
    m_entries = new Entry[arraySize];
    clear();
}


//----------------------------------------------
HashTable::~HashTable()
{
    // Delete cells
    delete[] m_entries;
}


void HashTable::set(int key, int value)
{
    for (int idx = integerHash(key);; idx++)
    {
        idx &= m_arraySize - 1;

        // Load the key that was there.
        int probedKey = m_entries[idx].key.load(std::memory_order_relaxed);
        if (probedKey != key)
        {
            // The entry was either free, or contains another key.
            if (probedKey != 0)
                continue;           // Usually, it contains another key. Keep probing.

            // The entry was free. Now let's try to take it using a CAS.
            int expected = 0;
            auto prevKey = std::atomic_compare_exchange_strong(&m_entries[idx].key,&expected,key);
            if (!prevKey)
                continue;       // Another thread just stole it from underneath us.

            // Either we just added the key, or another thread did.
        }

        // Store the value in this array entry.
        m_entries[idx].value.store(value,std::memory_order_relaxed);
        return;
    }
}

//----------------------------------------------
int HashTable::get(int key)
{
    assert(key != 0);
    for (uint32_t idx = integerHash(key);; idx++)
    {
        idx &= m_arraySize - 1;
        uint32_t probedKey = m_entries[idx].key.load(std::memory_order_relaxed);
        if (probedKey == key)
            return m_entries[idx].value.load(std::memory_order_relaxed);
        if (probedKey == 0)
            return 0;
    }
}


//----------------------------------------------
int HashTable::size()
{
    uint32_t itemCount = 0;
    for (uint32_t idx = 0; idx < m_arraySize; idx++)
    {
        if ((m_entries[idx].key.load(std::memory_order_relaxed) != 0)
            && m_entries[idx].value.load(std::memory_order_relaxed) != 0)
            itemCount++;
    }
    return itemCount;
}


//----------------------------------------------
void HashTable::clear()
{
    memset(m_entries, 0, sizeof(Entry) * m_arraySize);
}

