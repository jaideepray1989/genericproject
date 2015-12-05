//
// Created by Jaideep Ray on 12/4/15.
//

#include "HazardPointers.cpp"

template<typename T>
class lock_free_stack_with_hazard_pointers
{
private:
    struct node
    {
        // using smart pointer for exception safety while popping
        std::shared_ptr<T> data;
        node* next;
        node(T const& data_):
                data(std::make_shared<T>(data_))
        {}
    };
    std::atomic<node*> head;

public:
    void push(T const& data)
    {
        node* const new_node=new node(data);
        new_node->next=head.load();
        while(!head.compare_exchange_weak(new_node->next,new_node));
    }


    std::shared_ptr<T> pop()
    {
        // get the hazard pointers for the current thread
        std::atomic<void*>& hp=get_hazard_pointer_for_current_thread();
        node* old_head=head.load();
        do
        {
            node* temp;
            do
            {
                temp=old_head;
                // store old_head in the hazard pointer
                hp.store(old_head);
                old_head=head.load();
            } while(old_head!=temp);
        }
        // check old_head before deleting.
        while(old_head &&
              !head.compare_exchange_strong(old_head,old_head->next));
        hp.store(nullptr);
        std::shared_ptr<T> res;
        if(old_head)
        {
            res.swap(old_head->data);

            if(outstanding_hazard_pointers_for(old_head))
            {
                // old head being accessed by other threads.
                // add to reclaim later list
                reclaim_later(old_head);
            }
            else
            {
                delete old_head;
            }
            // traverse through the reclaim later list and delete nodes which are not hazardous.
            delete_nodes_with_no_hazards();
        }
        return res;
    }
};

