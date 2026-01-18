#pragma once

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "ugraph.hpp"

namespace ucosm {

    // Simple guard tag used by tests/examples
    template<int N>
    struct Guard {
        static constexpr int id = N;
        static constexpr bool is_guard = true;
    };

    template<auto _callable, uint8_t _priority>
    struct TaskData {
        constexpr void operator()() { _callable(); }
        static constexpr uint8_t priority = _priority;
        static constexpr bool is_guard = false;
    };

    // Use distinct ids for task nodes to avoid collision with guard ids.
    // We encode the task node id as (1ull << 32) | _priority (so guard ids remain small).
    template<typename _guard_t, auto _callable, uint8_t _priority>
    using Task = ugraph::Link<
        ugraph::NodeTag<_guard_t::id, _guard_t>,
        ugraph::NodeTag< (static_cast<std::size_t>(1ull) << 32) | static_cast<std::size_t>(_priority), TaskData<_callable, _priority> >
    >;

    // Builder: produce a ugraph::Topology whose nodes are the provided tasks
    // and whose edges connect tasks that share the same condition type. Within
    // each condition-group tasks are ordered by ascending numeric priority
    // (smaller value = higher priority), so edges go from higher-priority
    // to lower-priority tasks.
    template<typename... tasks_t>
    struct Builder {

        static constexpr std::size_t task_count = sizeof...(tasks_t);

        static_assert(task_count >= 1, "Builder requires at least one task");
        // Sort tasks by (guard id, priority) at compile-time and build a topology
        // whose edges connect tasks of the same guard in ascending priority order.

        // Sort the provided Link-types using ugraph::detail::type_list utilities.

        template<typename T>
        static constexpr std::size_t guard_id_v = T::first_type::id();

        template<typename T>
        static constexpr std::size_t priority_v = std::decay_t<typename T::second_type::module_type>::priority;

        // concat helper for ugraph::detail::type_list
        template<typename A, typename B>
        struct tl_concat;
        template<typename... A, typename... B>
        struct tl_concat<ugraph::detail::type_list<A...>, ugraph::detail::type_list<B...>> { using type = ugraph::detail::type_list<A..., B...>; };

        // insert Elem into sorted type_list L (insertion sort step)
        template<typename L, typename Elem>
        struct tl_insert_sorted;

        template<typename Elem>
        struct tl_insert_sorted<ugraph::detail::type_list<>, Elem> { using type = ugraph::detail::type_list<Elem>; };

        template<typename Head, typename... Tail, typename Elem>
        struct tl_insert_sorted<ugraph::detail::type_list<Head, Tail...>, Elem> {
            static constexpr bool less = (guard_id_v<Elem> < guard_id_v<Head>) || (guard_id_v<Elem> == guard_id_v<Head> && priority_v<Elem> < priority_v<Head>);
            using tail_list = ugraph::detail::type_list<Tail...>;
            using inserted_tail = typename tl_insert_sorted<tail_list, Elem>::type;
            using type = std::conditional_t< less, ugraph::detail::type_list<Elem, Head, Tail...>, typename tl_concat<ugraph::detail::type_list<Head>, inserted_tail>::type >;
        };

        // fold over input tasks to build a sorted type_list
        template<typename Acc, typename... Rem>
        struct tl_sort_fold;

        template<typename Acc>
        struct tl_sort_fold<Acc> { using type = Acc; };

        template<typename Acc, typename H, typename... R>
        struct tl_sort_fold<Acc, H, R...> { using type = typename tl_sort_fold<typename tl_insert_sorted<Acc, H>::type, R...>::type; };

        using sorted_list = typename tl_sort_fold<ugraph::detail::type_list<>, tasks_t...>::type;
        static constexpr std::size_t sorted_count = ugraph::detail::type_list_size<sorted_list>::value;

        template<typename L, std::size_t... I>
        static auto list_to_topology_impl(std::index_sequence<I...>) -> ugraph::Topology<typename ugraph::detail::type_list_at<I, L>::type...>;

        using system_t = decltype(list_to_topology_impl<sorted_list>(std::make_index_sequence<sorted_count>{}));

    };

} // namespace ucosm
