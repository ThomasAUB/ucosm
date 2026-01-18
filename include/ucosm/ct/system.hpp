#pragma once

#include <iostream>
#include <stdint.h>
#include <array>
#include <cstddef>
#include <utility>

namespace ucosm {

    struct IScheduler {
        virtual void yield() = 0;
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


    using hook_id_t = uint8_t;
    constexpr uint8_t max_hook_id = 31;


    template<hook_id_t _id>
    struct Hook {
        static_assert(_id <= max_hook_id, "Hook ID must be < 32");
        static constexpr hook_id_t id = _id;
        //static constexpr bool is_hook = true;
    };

    template<auto _callable>
    struct Job {
        constexpr void operator()() { _callable(); }
        //static constexpr bool is_hook = false;
    };

    template<uint8_t _priority, typename _hook_t, typename ... jobs_t>
    struct Pipeline {

        using hook_t = _hook_t;

        static constexpr uint8_t priority = _priority;

        //static_assert(hook_t::is_hook, "Hook error");
        //static_assert((!jobs_t::is_hook && ...), "Hook error");

        static constexpr void run() {
            constexpr auto runJob =
                [] (auto job) {
                job();
                };
            (runJob(jobs_t {}), ...);
        }

    };


    template<typename ... pipelines_t>
    struct System final : IScheduler {

        System() {
            *getScheduler() = this;
        }

        constexpr void run() {
            while (ready_mask) {
                // execute ready pipelines in priority order (lower value -> higher priority)
                for (auto idx : sorted_ids) {
                    const uint32_t mask = 1 << hooks_ids[idx];
                    if (ready_mask & mask) {
                        ready_mask &= ~mask;
                        runners[idx]();
                    }
                }
            }
        }

        constexpr void signalHook(hook_id_t inID) {
            if (inID > max_hook_id) { return; }
            ready_mask |= (1 << inID);
        }

        void yield() override {
            // simple cooperative yield: run ready pipelines once
            run();
        }

    private:

        static constexpr auto makeSortedIDs() {

            constexpr size_t N = sizeof...(pipelines_t);
            if constexpr (N == 0) {
                return std::array<uint8_t, 0>{};
            }

            constexpr std::array<uint8_t, N> priorities = { pipelines_t::priority... };

            std::array<uint8_t, N> idx {};
            for (size_t i = 0; i < N; ++i) {
                idx[i] = static_cast<uint8_t>(i);
            }

            for (size_t i = 0; i < N; ++i) {
                size_t min = i;
                for (size_t j = i + 1; j < N; ++j) {
                    if (priorities[idx[j]] < priorities[idx[min]]) {
                        min = j;
                    }
                }
                if (min != i) {
                    auto tmp = idx[i]; idx[i] = idx[min]; idx[min] = tmp;
                }
            }

            return idx;
        }

        using runner_t = void(*)();
        static constexpr auto sorted_ids = makeSortedIDs();
        static constexpr hook_id_t hooks_ids[] = { pipelines_t::hook_t::id... };
        static constexpr runner_t runners[] = { &pipelines_t::run... };

        uint32_t ready_mask {};

    };

} // namespace ucosm
