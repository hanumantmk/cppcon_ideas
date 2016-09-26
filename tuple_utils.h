#pragma once

#include <tuple>
#include <utility>

template <typename Callable, typename Tup, size_t... Idxs>
constexpr auto tupleMapImpl(Callable&& cb, Tup&& tup, std::index_sequence<Idxs...>) {
    return std::make_tuple(cb(std::get<Idxs>(std::forward<Tup>(tup)))...);
}

template <typename Callable, typename Tup>
constexpr auto tupleMap(Callable&& cb, Tup&& tup) {
    return tupleMapImpl(std::forward<Callable>(cb),
                        std::forward<Tup>(tup),
                        std::make_index_sequence<std::tuple_size<std::decay_t<Tup>>::value>{});
}

enum class Control {
    continue_t,
    break_t,
};

template <typename Callable, typename Tup, size_t... Idxs>
constexpr void tupleForEachImpl(Callable&& cb, Tup&& tup, std::index_sequence<Idxs...>) {
    Control flag = Control::continue_t;
    (void)std::initializer_list<int>{(
        [&] {
            if (flag == Control::continue_t) {
                flag = cb(std::integral_constant<size_t, Idxs>{},
                          std::get<Idxs>(std::forward<Tup>(tup)));
            }
        }(),
        0)...};
}

template <typename Callable, typename Tup>
constexpr void tupleForEach(Callable&& cb, Tup&& tup) {
    return tupleForEachImpl(std::forward<Callable>(cb),
                            std::forward<Tup>(tup),
                            std::make_index_sequence<std::tuple_size<std::decay_t<Tup>>::value>{});
}
