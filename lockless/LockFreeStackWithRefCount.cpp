//
// Created by Jaideep Ray on 12/4/15.
//

/*Reference counting tackles the problem of ABA and leaking nodes by storing a count of the number
of threads accessing each node*/

/*
 * if std::atomic_is_lock_free(&some_shared_ptr) is true then we don't need to do ref count
 * */

template<typename T>
class lock_free_stack_with_ref_count
{
private:
    struct node;
    struct ref_counted_node_ptr
    {
        node* ptr;
        int weak_count;
    };
    struct node
    {
        std::shared_ptr<T> data;
        std::atomic<int> strong_count;
        ref_counted_node_ptr next;
        node(T const& data_):
                data(std::make_shared<T>(data_)),
                strong_count(0)
        {}
    };
    std::atomic<ref_counted_node_ptr> head;

    void increment_weak_ref_count(ref_counted_node_ptr &old_counter)
    {
        ref_counted_node_ptr new_counter;
        do
        {new_counter=old_counter;
            ++new_counter.weak_count;
        }
        while(!head.compare_exchange_strong(old_counter,new_counter,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed));
        old_counter.weak_count=new_counter.weak_count;
    }
public:
    ~lock_free_stack_with_ref_count()
    {
        while(pop());
    }

    // external count = 1 for push
    void push(T const& data)
    {
        ref_counted_node_ptr new_node;
        new_node.ptr=new node(data);
        new_node.weak_count=1;
        new_node.ptr->next=head.load(std::memory_order_relaxed);
        while(!head.compare_exchange_weak(new_node.ptr->next,new_node,
                                          std::memory_order_release,
                                          std::memory_order_relaxed));
    }


    std::shared_ptr<T> pop()
    {
        ref_counted_node_ptr old_head=
                head.load(std::memory_order_relaxed);

        while(true)
        {
            // all the thread trying to pop concurrently will increment the weak counter.
            increment_weak_ref_count(old_head);
            node* const ptr=old_head.ptr;
            // someone has deleted it
            if(!ptr)
            {
                return std::shared_ptr<T>();
            }
            // if compare_exchange_swap is successful, we have successfully
            // pointed head to old_head->next.
            if(head.compare_exchange_strong(old_head,ptr->next,
                                            std::memory_order_relaxed))
            {
                // copy ptr data
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                // removed old_head weak_ptr decrement by one
                // no longer reading this node, weak_ptr decrement by one more
                // so a total of 2.
                int const count=old_head.weak_count-2;
                // fetch_add = adds a non-atomic value to an atomic object and obtains the previous value
                if(ptr->strong_count.fetch_add(count,
                                                 std::memory_order_release)==-count)
                {
                    delete ptr;
                }
                return res;
            }
            else if(ptr->strong_count.fetch_add(-1,
                                                  std::memory_order_relaxed)==1)
            {
                /* compare_exchange_failed so another thread has removed the node before we did
                or another thread added a node to the stack. Either way you need to start again.
                If it is the last thread to hold a reference (because
                another thread removed it from the stack), the strong reference count will be 1,
                being the last thread holding the ref, you can delete the pointer
                 * */
                ptr->strong_count.load(std::memory_order_acquire);
                delete ptr;
            }
        }
    }
};
