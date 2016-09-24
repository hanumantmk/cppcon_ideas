#include <tuple>
#include <iostream>
#include <utility>
#include <type_traits>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>

#define PASTE(v) #v

#define TYPE(r, data, i, elem) \
    BOOST_PP_COMMA_IF(i) makeField<decltype(&data::elem), &data::elem>(PASTE(elem))

#define ARGS(classv, seq) \
    BOOST_PP_SEQ_FOR_EACH_I(TYPE, classv, seq)

#define DECLARE(classv, ...) \
    constexpr static auto reflectionData() { \
        return makeReflectionData<classv>(#classv, ARGS(classv, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))); \
    } \
    using type = classv;

#define VALIDATE(name) \
    template <decltype(&type::name) ptr, typename T> \
    constexpr static bool validate(T&& name)

template <bool b>
struct StaticIf;

template <>
struct StaticIf<true> {
    template <typename T>
    auto then(T&& t) {
        return t;
    }
};

template <>
struct StaticIf<false> {
    template <typename T>
    auto then(T&& t) {
        return [](auto&& ...){
            std::cout << "in a dumb place\n";
        };
    }
};

template <typename T>
auto static_if(T&& t) {
    return StaticIf<T::value>{};
}


template <typename T, T t>
struct Field {
    constexpr T ptr() const { return t; }
    const char* name;
};

template <typename T, T t>
constexpr Field<T, t> makeField(const char* name) {
    return Field<T, t>{name};
}

template <typename Callable, typename Tup, size_t ...Idxs>
constexpr auto tupleMapImpl(Callable&& cb, Tup&& tup, std::index_sequence<Idxs...>) {
    return std::make_tuple(cb(std::get<Idxs>(std::forward<Tup>(tup)))...);
}

template <typename Callable, typename Tup>
constexpr auto tupleMap(Callable&& cb, Tup&& tup) {
    return tupleMapImpl(std::forward<Callable>(cb), std::forward<Tup>(tup), std::make_index_sequence<std::tuple_size<std::decay_t<Tup>>::value>{});
}

enum class Control {
    continue_t,
    break_t,
};

template <typename Callable, typename Tup, size_t ...Idxs>
constexpr void tupleForEachImpl(Callable&& cb, Tup&& tup, std::index_sequence<Idxs...>) {
    Control flag = Control::continue_t;
    (void)std::initializer_list<int>{([&]{
        if (flag == Control::continue_t) {
            flag = cb(Idxs, std::get<Idxs>(std::forward<Tup>(tup)));
        }
    }(), 0)...};
}

template <typename Callable, typename Tup>
constexpr void tupleForEach(Callable&& cb, Tup&& tup) {
    return tupleForEachImpl(std::forward<Callable>(cb), std::forward<Tup>(tup), std::make_index_sequence<std::tuple_size<std::decay_t<Tup>>::value>{});
}

template <typename T, typename ...Fields>
struct ReflectionData {
    using type = T;

    constexpr ReflectionData(const char* name, Fields ...fields) : name(name), fields(fields...) {}

    template <typename U>
    constexpr auto getTupleLens(U&& object) {
        return tupleMap([&](auto&& x){ return std::ref(std::forward<U>(object).*(x.ptr())); }, fields);
    }

    const char* name;

    std::tuple<Fields...> fields;

    constexpr static size_t nFields = sizeof...(Fields);
};

template <typename T, typename ...Fields>
constexpr ReflectionData<T, Fields...> makeReflectionData(const char* name, Fields ...fields) {
    return ReflectionData<T, Fields...>(name, fields...);
}

class Foo {
public:
    Foo() = default;

    Foo(long a, short b, int c) : a(a), b(b), c(c) {}

private:
    long a;
    short b;
    int c;
    std::string d;

public:
    DECLARE(Foo, a, b, c, d)

    VALIDATE(a) {
        return a > 0 && a < 10;
    }

    bool check() const { return a && b && c; }
};

template <typename T>
constexpr auto hasReflectionData(int) -> decltype(T::reflectionData(), std::true_type{}) { return std::true_type{}; }

template <typename T>
constexpr auto hasReflectionData(...) -> std::false_type { return std::false_type{}; }

template <typename T>
constexpr bool hasReflectionData_v = decltype(hasReflectionData<T>(0))::value;

template <typename T, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
bool operator<(const T& lhs, const T& rhs) {
    return T::reflectionData().getTupleLens(lhs) < T::reflectionData().getTupleLens(rhs);
}

template <typename T, typename Tup, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
auto load(T&& dst, Tup&& from) {
    T::reflectionData().getTupleLens(dst) = std::forward<Tup>(from);
    return std::move(dst);
}

template <typename T, typename Callback>
void visit(T&& t, const char* name, Callback&& cb) {
    tupleForEach([&](size_t i, auto&& x) {
        if (strcmp(x.name, name) == 0) {
            cb(t.*(x.ptr()));

            return Control::break_t;
        }

        return Control::continue_t;
    }, std::decay_t<T>::reflectionData().fields);
}

template <typename T, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
struct Binder {
    T& t;
    const char* name;

    template <typename U>
    Binder& operator=(U&& u) {
        visit(t, name, [&](auto&& x){
            static_if(std::is_convertible<U, std::decay_t<decltype(x)>>{}).then([&](auto&& y){
                y = std::forward<U>(u);
            })(std::forward<decltype(x)>(x));
        });
        return *this;
    }
};


template <typename T>
struct Wrap {
    T& t;

    auto operator[](const char* name) {
        return Binder<T>{t, name};
    }
};

template <typename T, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
Wrap<T> wrap(T& t) {
    return Wrap<T>{t};
}

template <typename T, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
std::ostream& operator<<(std::ostream& os, const T& t) {
    auto reflectionData = T::reflectionData();
    os << "(" << reflectionData.name << ") {\n";
    tupleForEach([&](size_t i, const auto& x) {
        os << "\t" << x.name << " : " << t.*(x.ptr());
        if (reflectionData.nFields != i + 1) {
            os << ",";
        }
        os << "\n";
        return Control::continue_t;
    }, reflectionData.fields);
    os << "}\n";

    return os;
}

int main() {
    std::cout << (Foo{1,2,3} < Foo{2,3,4}) << "\n";
    std::cout << Foo{1,2,3} << "\n";
    std::cout << load(Foo{}, std::make_tuple(1,2,3, "fun")) << "\n";

    Foo foo{};
    wrap(foo)["a"] = 9001;
    wrap(foo)["d"] = "hi";
    std::cout << foo << "\n";
}
