//
// Created by Jaideep Ray on 11/11/15.
//

template<typename T>
class lock_free_stack {
private:
    struct node {
        std::shared_ptr<T> data;
        node *next;

        node() : next(nullptr) { }
    };

    std::atomic<node *> head;

    node *pop_head() {
        node *const old_head = head.load();
        if (old_head == nullptr) { return nullptr;}
        head.store(old_head->next);
        return old_head;
    }

public:
    lock_free_stack() : head(nullptr) { }

    lock_free_stack(const lock_free_stack &other) = delete;

    lock_free_stack &operator=(const lock_free_stack &other) = delete;

    ~lock_free_stack() {
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
        node *const old_head = head.load();
        node *p = new node;
        p->data.swap(new_data);
        p->next = old_head;
        head.store(p);
    }
};
