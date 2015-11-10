#include <iostream>
#include "withlocks/ThreadSafeQueue.cpp"
#include "lockless/LockFreeQueue.cpp"
using namespace std;




int main() {
    threadsafe_queue<int> *tq = new threadsafe_queue<int>();
    lock_free_queue<int> *lfq = new lock_free_queue<int>();
    tq->push(89);
    tq->push(90);
    lfq->push(23);
    cout << *(lfq->pop().get())<<endl;
    cout << *(tq->try_pop().get());
    return 0;
}