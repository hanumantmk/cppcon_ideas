#include <tuple>
#include <iostream>
#include <utility>
#include <type_traits>

#define MAKE_FIELD(field) \
    makeField(&type::field, #field)

template <typename T>
struct Field {
};

template <typename Base, typename T>
struct Field<T Base::*> {
    const char* name;
    T Base::* ptr;
};

template <typename Base, typename T>
constexpr Field<T Base::*> makeField(T Base::*ptr, const char* name) {
    return Field<T Base::*>{name, ptr};
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
//    (void)std::initializer_list<int>{(cb(Idxs, std::get<Idxs>(std::forward<Tup>(tup))), 0)...};
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
        return tupleMap([&](auto&& x){ return std::forward<U>(object).*(x.ptr); }, fields);
    }

    const char* name;

    std::tuple<Fields...> fields;

    constexpr static size_t nFields = sizeof...(Fields);
};

template <typename T, typename ...Fields>
constexpr ReflectionData<T, Fields...> makeReflectionData(const char* name, Fields ...fields) {
    return ReflectionData<T, Fields...>(name, fields...);
}

struct Foo {
    long a;
    short b;
    int c;

    constexpr static auto reflectionData() {
        using type = Foo;
        return makeReflectionData<Foo>("Foo", MAKE_FIELD(a), MAKE_FIELD(b), MAKE_FIELD(c));
    }
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

template <typename T, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
std::ostream& operator<<(std::ostream& os, const T& t) {
    auto reflectionData = T::reflectionData();
    os << "(" << reflectionData.name << ") {\n";
    tupleForEach([&](size_t i, const auto& x) {
        os << "\t" << x.name << " : " << t.*(x.ptr);
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
    Foo foo{1,2,3};
    std::cout << std::get<2>(Foo::reflectionData().fields).name << "\n";
    std::cout << std::get<2>(Foo::reflectionData().getTupleLens(foo)) << "\n";
    std::cout << (Foo::reflectionData().getTupleLens(Foo{1,2,3}) < Foo::reflectionData().getTupleLens(Foo{2,3,4})) << "\n";
    std::cout << (Foo{1,2,3} < Foo{2,3,4}) << "\n";
    std::cout << Foo{1,2,3} << "\n";
}
