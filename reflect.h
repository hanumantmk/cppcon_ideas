#pragma once

#include <functional>
#include <tuple>

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include "tuple_utils.h"

#define PASTE(v) #v

#define TYPE(r, data, i, elem) \
    BOOST_PP_COMMA_IF(i) makeField<decltype(&data::elem), &data::elem>(PASTE(elem))

#define ARGS(classv, seq) BOOST_PP_SEQ_FOR_EACH_I(TYPE, classv, seq)

#define DECLARE(classv, ...)                                                                    \
    constexpr static auto reflectionData() {                                                    \
        return makeReflectionData<classv>(#classv,                                              \
                                          ARGS(classv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))); \
    }                                                                                           \
    using type = classv;

#define VALIDATE(name)                               \
    template <decltype(&type::name) ptr, typename T> \
    constexpr static bool validate(T&& name)


template <typename T, T t>
struct Field {
    constexpr T ptr() const {
        return t;
    }
    const char* name;
};

template <typename T, T t>
constexpr Field<T, t> makeField(const char* name) {
    return Field<T, t>{name};
}

template <typename T, typename... Fields>
struct ReflectionData {
    using type = T;

    constexpr ReflectionData(const char* name, Fields... fields) : name(name), fields(fields...) {}

    template <typename U>
    constexpr auto getTupleLens(U&& object) {
        return tupleMap([&](auto&& x) { return std::ref(std::forward<U>(object).*(x.ptr())); },
                        fields);
    }

    template <typename Callback>
    auto visit(const char* name, Callback&& cb) {
        tupleForEach(
            [&](auto i, auto&& x) {
                if (strcmp(x.name, name) == 0) {
                    cb(x);

                    return Control::break_t;
                }

                return Control::continue_t;
            },
            std::decay_t<T>::reflectionData().fields);
    }

    const char* name;

    std::tuple<Fields...> fields;

    constexpr static size_t nFields = sizeof...(Fields);
};

template <typename T, typename... Fields>
constexpr ReflectionData<T, Fields...> makeReflectionData(const char* name, Fields... fields) {
    return ReflectionData<T, Fields...>(name, fields...);
}

template <typename T>
constexpr auto hasReflectionData(int) -> decltype(T::reflectionData(), std::true_type{}) {
    return std::true_type{};
}

template <typename T>
constexpr auto hasReflectionData(...) -> std::false_type {
    return std::false_type{};
}

template <typename T>
constexpr bool hasReflectionData_v = decltype(hasReflectionData<T>(0))::value;
