//
// Created by Jaideep Ray on 11/20/15.
//
#include<unordered_map>
#include<mutex>
#include<thread>
using namespace std;

template <class K, class V>
class WRRMMap {
    std::mutex mtx_;
    unordered_map<K, V> map_;
public:
    V get(const K& k) {
        std::lock_guard<std::mutex> lock(mtx_);
        return map_[k];
    }
    void set(const K& k,
                const V& v) {
        std::lock_guard<std::mutex> lock(mtx_);
        map_[k] = v;
    }
};