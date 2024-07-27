//
// Created by Grady Schofield on 7/26/24.
//

#ifndef GRADY_LIB_THREADPOOL_HPP
#define GRADY_LIB_THREADPOOL_HPP

namespace gradylib {
    class Receipt {
    };

    class ReceiptBasket {
        /*
         * have a wait free queue here.  Worker puts the Receipt on the queue.
         *
         */
    public:
        void wait() {
        }
    };

    class ThreadPool {
    public:
        template<typename Func>
        Receipt add(Func &&f) {
        }
    };
    /*
     * ThreadPool tp(64);
     * OpenHashMap<,> map;
     * //fill map
     * ThreadPool::WorkSet ts = parallelForEach<OpenHashMap>(tp, map, []() {});
     * ts.get(duration<milliseconds>(500));
     *    OR
     * ts.wait();
     */
}

#endif //GRADY_LIB_THREADPOOL_HPP
