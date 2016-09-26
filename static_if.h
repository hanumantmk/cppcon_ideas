#pragma once

#include <utility>

enum class StaticIfState {
    Resolved,
    True,
    False,
};

template <typename Parent, typename T, bool b>
struct Else;

template <typename Parent, typename T>
struct Else<Parent, T, false> {
    Else(Parent& p, T&& t) : p(p) {}
    Parent& p;

    template <typename... Ts>
    auto operator()(Ts&&... ts) {
        return p(std::forward<Ts>(ts)...);
    }
};

template <typename Parent, typename T>
struct Else<Parent, T, true> {
    Else(Parent& p, T&& t) : t(std::move(t)) {}
    T t;

    template <typename... Ts>
    auto operator()(Ts&&... ts) {
        return t(std::forward<Ts>(ts)...);
    }
};

template <typename Parent, typename T, StaticIfState state>
struct ElseIf {
    ElseIf(Parent& p, T&& t) : p(p) {}
    Parent& p;

    template <typename... Ts>
    auto operator()(Ts&&... ts) {
        return p(std::forward<Ts>(ts)...);
    }

    template <typename U>
    auto static_else(U&& u) {
        return Else < ElseIf, U,
               state == StaticIfState::Resolved ? false : true > {*this, std::forward<U>(u)};
    }

    template <bool b, typename U>
    auto static_else_if(U&& u) {
        return ElseIf < ElseIf, U, state == StaticIfState::Resolved ? StaticIfState::Resolved : b
                       ? StaticIfState::True
                       : StaticIfState::False > {*this, std::forward<U>(u)};
    }
};

template <typename Parent, typename T>
struct ElseIf<Parent, T, StaticIfState::True> {
    ElseIf(Parent& p, T&& t) : t(std::move(t)) {}
    T t;

    template <typename... Ts>
    auto operator()(Ts&&... ts) {
        return t(std::forward<Ts>(ts)...);
    }

    template <typename U>
    auto static_else(U&& u) {
        return Else<ElseIf, U, false>{*this, std::forward<U>(u)};
    }

    template <bool b, typename U>
    auto static_else_if(U&& u) {
        return ElseIf<ElseIf, U, StaticIfState::Resolved>{*this, std::forward<U>(u)};
    }
};

template <typename T, bool b>
struct Then;

template <typename T>
struct Then<T, true> {
    Then(T&& t) : t(std::move(t)) {}

    template <typename... Ts>
    auto operator()(Ts&&... ts) {
        return t(std::forward<Ts>(ts)...);
    }

    template <typename U>
    auto static_else(U&& u) {
        return Else<Then, U, false>{*this, std::forward<U>(u)};
    }

    template <bool b, typename U>
    auto static_else_if(U&& u) {
        return ElseIf<Then, U, StaticIfState::Resolved>{*this, std::forward<U>(u)};
    }

    T t;
};

template <typename T>
struct Then<T, false> {
    Then(T&&) {}

    template <typename... Ts>
    auto operator()(Ts&&... ts) {}

    template <typename U>
    auto static_else(U&& u) {
        return Else<Then, U, true>{*this, std::forward<U>(u)};
    }

    template <bool b, typename U>
    auto static_else_if(U&& u) {
        return ElseIf < Then, U,
               b ? StaticIfState::True : StaticIfState::False > {*this, std::forward<U>(u)};
    }
};

template <bool b, typename T>
auto static_if(T&& t) {
    return Then<T, b>{std::forward<T>(t)};
}
