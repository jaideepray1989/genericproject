#include <iostream>
#include "withlocks/ThreadSafeQueue.cpp"
#include "lockless/LockFreeQueue.cpp"
#include "lockless/LockFreeStack.cpp"
#include "lockless/LockFreeHashMap.cpp"
using namespace std;




int main() {
    threadsafe_queue<int> *tq = new threadsafe_queue<int>();
    lock_free_queue<int> *lfq = new lock_free_queue<int>();
    lfq->push(89);
    lfq->push(90);
    lfq->push(23);
    lock_free_stack<int> *lfs = new lock_free_stack<int>();
    lfs->push(89);
    lfs->push(90);
    lfs->push(23);
    for(int i=0;i<3;i++) {
        cout << "lfq " << *(lfq->pop().get()) << endl;
        cout << "lfs " << *(lfs->pop().get()) << endl;
    }

    unsigned int sz = 64;

    HashTable* hashTable = new HashTable(sz);
    hashTable->set(15,-90);
    hashTable->set(16,-70);
    hashTable->set(15,-80);
    cout<<hashTable->get(15);
    cout<<hashTable->get(16);

    return 0;
}