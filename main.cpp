#include <iostream>

using namespace std;

template<typename T>
class threadsafe_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node *tail;

    node *get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) { return nullptr; }
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

public:
    threadsafe_queue() : head(new node), tail(head.get()) { }

    threadsafe_queue(const threadsafe_queue &other) = delete;

    threadsafe_queue &operator=(const threadsafe_queue &other) = delete;

    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head = pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    void push(T new_value) {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        node *const new_tail = p.get();
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        tail->data = new_data;
        tail->next = std::move(p);
        tail = new_tail;
    }
};

template<typename T>
class lock_free_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        node *next;

        node() : next(nullptr) { }
    };

    std::atomic<node *> head;
    std::atomic<node *> tail;

    node *pop_head() {
        node *const old_head = head.load();
        if (old_head == tail.load()) { return nullptr; }
        head.store(old_head->next);
        return old_head;
    }

public:
    lock_free_queue() : head(new node), tail(head.load()) { }

    lock_free_queue(const lock_free_queue &other) = delete;

    lock_free_queue &operator=(const lock_free_queue &other) = delete;

    ~lock_free_queue() {
        while (node *const old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }

    std::shared_ptr<T> pop() {
        node *old_head = pop_head();
        if (!old_head) { return std::shared_ptr<T>(); }
        std::shared_ptr<T> const res(old_head->data);
        delete old_head;
        return res;
    }

    void push(T new_value) {
        std::shared_ptr<T> new_data(std::make_shared<T>(new_value));
        node *p = new node;
        node *const old_tail = tail.load();
        old_tail->data.swap(new_data);
        old_tail->next = p;
        tail.store(p);
    }
};

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