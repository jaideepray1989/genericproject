//
// Created by Jaideep Ray on 12/4/15.
//

#include <thread>
#include <mutex>

/* Problem 1 - ABA
 Any lock-free data structure that uses the compare-and-swap primitive must deal with the ABA problem.
 For example, in a lock-free stack represented as an intrusively linked list, one thread may be attempting to pop an
 item from the front of the stack (A → B → C). The new head should be 'B'.It remembers the second-from-top value "B",
 and then performs compare_and_swap(target=&head, newvalue=B, expected=A). Unfortunately,
 in the middle of this operation, another
 thread may have done two pops and then pushed A back on top, resulting in the stack (A → C). The compare-and-swap
 succeeds in swapping `head` with `B`,and the result is that the stack now contains garbage
 (a pointer to the freed element "B").*/


/* Problem 2 =
Furthermore, any lock-free algorithm containing code of the form
 Node* currentNode = this->head;  // assume the load from "this->head" is atomic
 Node* nextNode = currentNode->next;  // assume this load is also atomic
 suffers from another major problem, in the absence of automatic garbage collection.
 In between those two lines, it is possible that another thread may pop the node pointed to by this->head and
 deallocate it, meaning that the memory access through currentNode on the second line reads deallocated memory
 (which may in fact already be in use by some other thread for a completely different purpose). */

/*
 * Hazard pointers can be used to address both of these problems. In a hazard-pointer system, each thread keeps a list
 * of hazard pointers indicating which nodes the thread is currently accessing.
 * Nodes on the hazard pointer list must not be modified or deallocated by any other thread.
 * */

/*
 * Each reader thread owns a single-writer/multi-reader shared pointer called "hazard pointer." When a reader thread
 * assigns the address of a map to its hazard pointer, it is basically announcing to other threads (writers),
 * "I am reading this map. You can replace it if you want, but don't change its contents and certainly
 * keep your deleteing hands off it."
 * */



unsigned const max_hazard_pointers = 100;

struct hazard_pointer {
    // id for thread which owns it
    std::atomic<std::thread::id> id;
    // pointer
    std::atomic<void *> pointer;
};
// array of hazard pointers
hazard_pointer hazard_pointers[max_hazard_pointers];

// owner of hazard pointers - threads.
class hp_owner {
    hazard_pointer *hp;
public:
    hp_owner(hp_owner const &) = delete;

    hp_owner operator=(hp_owner const &) = delete;

    hp_owner() :
            hp(nullptr) {
        for (unsigned i = 0; i < max_hazard_pointers; ++i) {
            std::thread::id old_id = std::thread::id();
            // if any of the hazard pointer slots is free
            // assign it to the thread
            if (hazard_pointers[i].id.compare_exchange_strong(
                    old_id, std::this_thread::get_id())) {
                hp = &hazard_pointers[i];
                break;
            }
        }
        // no hazard pointer found
        if (!hp) {
            throw std::runtime_error("No hazard pointers available");
        }
    }

    // get the hazard pointer
    std::atomic<void *> &get_pointer() {
        return hp->pointer;
    }

    ~hp_owner() {
        // set pointer to null
        hp->pointer.store(nullptr);
        // set thread id to default thread id
        hp->id.store(std::thread::id());
    }
};

std::atomic<void *> &get_hazard_pointer_for_current_thread() {
    static hp_owner hazard;
    return hazard.get_pointer();
}


template<typename T>
void do_delete(void *p) {
    delete static_cast<T *>(p);
}

// data to reclaim contains data, function template deleter
struct data_to_reclaim {
    void *data;
    std::function<void(void *)> deleter;
    data_to_reclaim *next;

    template<typename T>
    data_to_reclaim(T *p) :
            data(p),
            deleter(&do_delete<T>),
            next(0) { }

    ~data_to_reclaim() {
        deleter(data);
    }
};

// reclaim later list
std::atomic<data_to_reclaim *> nodes_to_reclaim;

// add to reclaim later list
void add_to_reclaim_list(data_to_reclaim *node) {
    node->next = nodes_to_reclaim.load();
    while (!nodes_to_reclaim.compare_exchange_weak(node->next, node));
}

// wrap data into data_to_reclaim
template<typename T>
void reclaim_later(T *data) {
    add_to_reclaim_list(new data_to_reclaim(data));
}

// checks if the given pointer data belongs to a hazard pointer.
bool outstanding_hazard_pointers_for(void *p) {
    // iterate through the hazard pointer list for checking if the
    // pointer is being accessed by other threads.
    for (unsigned i = 0; i < max_hazard_pointers; ++i) {
        if (hazard_pointers[i].pointer.load() == p) {
            return true;
        }
    }
    return false;
}

// travel through the reclaim list, check if it is hazardous to delete a node
// if not delete it
// This portion is computation intensive and needs to be optimized.
void delete_nodes_with_no_hazards() {
    data_to_reclaim *current = nodes_to_reclaim.exchange(nullptr);
    while (current) {
        data_to_reclaim *const next = current->next;
        if (!outstanding_hazard_pointers_for(current->data)) {
            // not a hazard pointer, delete node
            delete current;
        }
        else {
            // chain to another list to delete later
            add_to_reclaim_list(current);
        }
        current = next;
    }
}

