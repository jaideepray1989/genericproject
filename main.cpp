#include <iostream>
#include "withlocks/ThreadSafeQueue.cpp"
#include "lockless/LockFreeQueue.cpp"
#include "lockless/LockFreeStack.cpp"
#include "lockless/LockFreeHashMap.cpp"
#include "withlocks/WriteRarelyReadManyMap.cpp"
#include "withlocks/ThreadSafeQueue2.cpp"
#include "lockless/LockFreeStackWithRefCount.cpp"
#include "lockless/LockFreeStackWithHazardPointers.cpp"
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

    WRRMMap<int,int> *map = new WRRMMap<int,int>();
    map->set(10,-69);
    map->set(10,9);
    cout<<map->get(10);

    lock_free_queue<int> *b1 = new lock_free_queue<int>();
    threadsafe_data_queue<int> * b2 = new threadsafe_data_queue<int>();
    for(int i=0;i<10;i++)
    {
        b1->push(i);
        b2->push(i);
    }

//    for(int i=0;i<10;i++)
//    {
//        cout<< *( b1->pop().get() )<<" : "<< *(b2->wait_and_pop().get()) << " | ";
//    }

    lock_free_stack_with_ref_count<int> * rlfs = new lock_free_stack_with_ref_count<int>();
    rlfs->push(10);
    rlfs->push(11);
    rlfs->push(12);
    auto x = *(rlfs->pop().get());
    cout<<x;

    lock_free_stack_with_hazard_pointers<int> * hlfs = new lock_free_stack_with_hazard_pointers<int>();
    hlfs->push(10);
    hlfs->push(11);
    hlfs->push(12);
    auto y = *(hlfs->pop().get());
    cout<<y;

    return 0;
}