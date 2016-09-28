#pragma once

#include <bitset>
#include <functional>
#include <tuple>

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include "static_if.h"
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
struct LiftValue {
    static constexpr T value = t;
    using type = T;
};

#define LIFT_VALUE(x) LiftValue<decltype(x), x>

template <typename T>
struct UnpackMemberPointer;

template <typename Base, typename T>
struct UnpackMemberPointer<T Base::*> {
    using type = T;
};

template <typename T>
using UnpackMemberPointer_t = typename UnpackMemberPointer<T>::type;

template <typename T, T t>
struct Field {
    using MemberPointer = LiftValue<T, t>;

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

    struct CtorArgsBinder {
        CtorArgsBinder(const ReflectionData& reflectionData) : reflectionData(reflectionData) {}

        const ReflectionData& reflectionData;
        std::bitset<sizeof...(Fields)> setArgs;
        std::tuple<std::aligned_storage_t<sizeof(UnpackMemberPointer_t<typename Fields::MemberPointer::type>), alignof(UnpackMemberPointer_t<typename Fields::MemberPointer::type>)>...> args;

        template <size_t ...Idxs>
        T makeImpl(std::index_sequence<Idxs...>) {
            return T{[&](auto idx, auto&& x){
                using Field = UnpackMemberPointer_t<typename std::decay_t<decltype(std::get<std::decay_t<decltype(idx)>::value>(reflectionData.fields))>::MemberPointer::type>;

                Field field{std::move(*reinterpret_cast<Field*>(&x))};
                reinterpret_cast<Field*>(&x)->~Field();
                return field;
            }(LIFT_VALUE(Idxs){}, std::get<Idxs>(args))...};
        }

        T make() {
            return makeImpl(std::make_index_sequence<sizeof...(Fields)>{});
        }

        template <typename U, typename V>
        void set(U t, V&& v) {
            tupleForEach([&](auto idx, auto&& x){
                return static_if<std::is_same<typename std::decay_t<decltype(x)>::MemberPointer, U>::value>([&](auto&& y, auto&& z){
                    setArgs[idx] = true;
                    new (&std::get<std::decay_t<decltype(idx)>::value>(args))
                        UnpackMemberPointer_t<typename std::decay_t<decltype(x)>::MemberPointer::type>(
                            std::forward<decltype(z)>(z));
                    return Control::break_t;
                }).static_else([](auto ...){
                    return Control::continue_t;
                })(std::forward<decltype(x)>(x), std::forward<V>(v));
            }, reflectionData.fields);
        }
    };

    CtorArgsBinder getCtorArgsBinder() const {
        return CtorArgsBinder(*this);
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
