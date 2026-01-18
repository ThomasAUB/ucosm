#pragma once

#include "builder.hpp"
#include <iostream>

namespace ucosm {

    struct IScheduler {
        virtual void yield() noexcept = 0;
    };

    inline IScheduler** getScheduler() {
        static IScheduler* sSched = nullptr;
        return &sSched;
    }

    inline void yield() {
        if (auto** sched = getScheduler()) {
            (*sched)->yield();
        }
    }

    // System builds a compile-time Topology from the provided task descriptors.
    // Each task descriptor is expected to expose `using tag` and `static constexpr int priority`.
    template<typename ... tasks_t>
    struct System final : IScheduler {
    private:

        using system_t = typename Builder<tasks_t...>::system_t;
        static_assert(!system_t::is_cyclic(), "Task resource conflict detected");

    public:

        System() {
            *getScheduler() = this;
        }

        void schedule() noexcept {

            system_t::for_each(

                [] (auto tag) {

                    using mod_t = typename decltype(tag)::module_type;

                    if constexpr (mod_t::is_guard) {
                        std::cout << "guard :" << mod_t::id << std::endl;
                    }
                    else {
                        std::cout << "task : ";
                        mod_t m;
                        m();
                    }

                }

            );

        }
        void yield() noexcept override { schedule(); }
        void signalInterrupt(int /*task_id*/) noexcept {}
    };

} // namespace ucosm
