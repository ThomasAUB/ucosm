#pragma once

#include <iostream>
#include <stdint.h>
#include <array>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <limits>

namespace ucosm {

    struct IScheduler {
        virtual void yield() = 0;
    };

    inline IScheduler** getScheduler() {
        static IScheduler* sSched = nullptr;
        return &sSched;
    }

    inline void yield() {
        if (auto* s = *getScheduler()) {
            s->yield();
        }
    }

    using hook_id_t = uint8_t;
    using priority_t = uint8_t;
    constexpr hook_id_t max_hook_id = 31;

    template<hook_id_t _hook_id, uint8_t _priority, auto _callable>
    struct Job {
        static constexpr hook_id_t id = _hook_id;
        static constexpr priority_t priority = _priority;
        static constexpr auto callable = _callable;
    };

    template<typename ... jobs_t>
    struct System final : IScheduler {

        System() {
            *getScheduler() = this;
        }

        void run() {
            while (pending_mask) {
                // execute ready pipelines in priority order (lower value -> higher priority)
                for (auto idx : sorted_idx) {
                    const uint32_t mask = 1u << hooks_ids[idx];
                    if (pending_mask & mask) {
                        // clear this hook id flag once before running all hooks with this id
                        pending_mask &= ~mask;
                        // prevent reentry while jobs of this hook are executing
                        running_hook_mask |= mask;
                        // run all jobs that share this id (respecting priority order)
                        runners[idx](this);
                        // release running flag and reschedule if the hook fired while running
                        running_hook_mask &= ~mask;
                        if (deferred_mask & mask) {
                            pending_mask |= mask;
                            deferred_mask &= ~mask;
                        }
                    }
                }
            }
        }

        constexpr void signalHook(hook_id_t inID) {
            // ignore invalid or unused hook ids
            const uint32_t mask = 1u << inID;
            if (!(mask & hooks_mask)) { return; }
            // if hook is currently executing, defer until it finishes
            if (running_hook_mask & mask) {
                deferred_mask |= mask;
                return;
            }
            pending_mask |= mask;
        }

        void yield() override {
            // simple cooperative yield: run ready pipelines once
            if (!pending_mask) { return; }
            run();
        }

    private:

        static constexpr uint8_t computeUniqueHooks() {
            bool tHooks[max_hook_id + 1] {};
            // mark all hook ids referenced by jobs_t
            ((tHooks[jobs_t::id] = true), ...);
            uint8_t out = 0;
            for (auto b : tHooks) {
                if (b) {
                    out++;
                }
            }
            return out;
        }
        static constexpr auto hooks_count = computeUniqueHooks();

        static constexpr uint32_t computeHookMask() {
            uint32_t hookMask = 0;
            ((hookMask |= (1u << jobs_t::id)), ...);
            return hookMask;
        }
        static constexpr auto hooks_mask = computeHookMask();

        static constexpr size_t num_jobs = sizeof...(jobs_t);

        // helper to index job types by position
        template<size_t I, typename T, typename... Ts>
        struct type_at_impl { using type = typename type_at_impl<I - 1, Ts...>::type; };
        template<typename T, typename... Ts>
        struct type_at_impl<0, T, Ts...> { using type = T; };
        template<size_t I>
        using type_at = typename type_at_impl<I, jobs_t...>::type;

        // collect job ids and priorities into arrays
        static constexpr auto make_job_ids() {
            return std::array<hook_id_t, num_jobs>{ jobs_t::id... };
        }
        static constexpr auto make_job_priorities() {
            return std::array<priority_t, num_jobs>{ jobs_t::priority... };
        }
        static constexpr auto job_ids = make_job_ids();
        static constexpr auto job_priorities = make_job_priorities();

        // build unique hooks id list in ascending id order
        static constexpr auto make_hooks_ids() {
            std::array<hook_id_t, hooks_count> out {};
            size_t pos = 0;
            // proper implementation using fold over pack expansion
            pos = 0;
            for (hook_id_t hid = 0; hid <= max_hook_id; ++hid) {
                bool found = ((jobs_t::id == hid) || ...);
                if (found) {
                    out[pos++] = hid;
                }
            }
            return out;
        }
        static constexpr auto hooks_ids = make_hooks_ids();

        // For each hook, build ordered list of job indices (by ascending priority)
        static constexpr auto make_job_order() {
            using idx_t = uint8_t;
            std::array<std::array<idx_t, num_jobs>, hooks_count> out {};
            std::array<uint8_t, hooks_count> counts {};

            for (size_t h = 0; h < hooks_count; ++h) {
                const hook_id_t hid = hooks_ids[h];
                size_t pos = 0;
                for (idx_t j = 0; j < num_jobs; ++j) {
                    if (job_ids[j] == hid) {
                        out[h][pos++] = j;
                    }
                }
                // sort first pos elements by priority (ascending)
                for (size_t a = 0; a + 1 < pos; ++a) {
                    size_t best = a;
                    for (size_t b = a + 1; b < pos; ++b) {
                        if (job_priorities[out[h][b]] < job_priorities[out[h][best]]) {
                            best = b;
                        }
                    }
                    if (best != a) {
                        auto tmp = out[h][a];
                        out[h][a] = out[h][best];
                        out[h][best] = tmp;
                    }
                }
                counts[h] = static_cast<uint8_t>(pos);
            }
            return std::pair { out, counts };
        }
        static constexpr auto job_order_and_counts = make_job_order();
        static constexpr auto job_order = job_order_and_counts.first;
        static constexpr auto job_counts = job_order_and_counts.second;

        // produce sorted hook indices by their minimum priority among their jobs
        static constexpr auto make_sorted_indices() {
            std::array<uint8_t, hooks_count> out {};
            std::array<priority_t, hooks_count> minp {};
            for (size_t h = 0; h < hooks_count; ++h) {
                priority_t mp = std::numeric_limits<priority_t>::max();
                for (size_t k = 0; k < job_counts[h]; ++k) {
                    auto ji = job_order[h][k];
                    if (job_priorities[ji] < mp) mp = job_priorities[ji];
                }
                minp[h] = mp;
                out[h] = static_cast<uint8_t>(h);
            }
            // simple selection sort by minp
            for (size_t a = 0; a + 1 < hooks_count; ++a) {
                size_t best = a;
                for (size_t b = a + 1; b < hooks_count; ++b) {
                    if (minp[b] < minp[best]) best = b;
                }
                if (best != a) {
                    auto tmpm = minp[a];
                    minp[a] = minp[best];
                    minp[best] = tmpm;
                    auto tmpo = out[a];
                    out[a] = out[best];
                    out[best] = tmpo;
                }
            }
            return out;
        }
        static constexpr auto sorted_idx = make_sorted_indices();

        using runner_t = void(*)(System*);

        template<size_t I, size_t... Js>
        static constexpr runner_t make_runner_for_index_impl(std::index_sequence<Js...>) {
            // For each possible job index Js, if it's present in job_order[I], call its callable in order.
            // The runner accepts a `System*` to consult the runtime `job_skip_mask` so that if a job
            // calls `yield()` and `run()` is invoked nested, the currently executing job will be
            // skipped (avoiding re-entry into the same callable).
            return +[] (System* self) {
                // unfold calls for all jobs matching this hook in the precomputed order
                (([&] () {
                    const uint32_t bit = (1u << Js);
                    for (size_t p = 0; p < job_counts[I]; ++p) {
                        if (job_order[I][p] == static_cast<uint8_t>(Js)) {
                            // skip if this job is already running (prevents re-entry)
                            if (self->job_skip_mask & bit) {
                                continue;
                            }
                            // mark as running, call the callable, then unmark
                            self->job_skip_mask |= bit;
                            type_at<Js>::callable();
                            self->job_skip_mask &= ~bit;
                        }
                    }
                    }()), ...);
                };
        }

        template<size_t I>
        static constexpr runner_t make_runner_for_index() {
            return make_runner_for_index_impl<I>(std::make_index_sequence<num_jobs>{});
        }

        template<size_t... Is>
        static constexpr auto makeRunnerListImpl(std::index_sequence<Is...>) {
            return std::array<runner_t, hooks_count>{ make_runner_for_index<Is>()... };
        }

        static constexpr auto makeRunnerList() {
            return makeRunnerListImpl(std::make_index_sequence<hooks_count>{});
        }

        uint32_t pending_mask {};
        // hooks that fired while their jobs were running; re-armed after the run completes
        uint32_t deferred_mask {};
        // mask of hook ids currently executing
        uint32_t running_hook_mask {};
        // mask of job indices currently executing (used to prevent re-entry during nested run())
        uint32_t job_skip_mask {};

        static constexpr auto runners = makeRunnerList();

    };

} // namespace ucosm
