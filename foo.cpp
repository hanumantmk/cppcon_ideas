#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

#include "reflect.h"
#include "static_if.h"
#include "tuple_utils.h"

template <typename T, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
bool operator<(const T& lhs, const T& rhs) {
    return T::reflectionData().getTupleLens(lhs) < T::reflectionData().getTupleLens(rhs);
}

template <typename T, typename Tup, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
auto load(T&& dst, Tup&& from) {
    T::reflectionData().getTupleLens(dst) = std::forward<Tup>(from);
    return std::move(dst);
}

template <typename T, std::enable_if_t<hasReflectionData_v<T>, int> = 0>
struct Binder {
    T& t;
    const char* name;

    template <typename U>
    Binder& operator=(U&& u) {
        std::decay_t<T>::reflectionData().visit(name, [&](auto&& field) {
            auto&& x = t.*(field.ptr());

            static_if<std::is_convertible<U, std::decay_t<decltype(x)>>::value>([&](auto&& y) {
                y = std::forward<U>(u);
            }).static_else([&](auto&& y) {
                throw std::runtime_error("bad assignment type");
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
    tupleForEach(
        [&](auto i, const auto& x) {
            os << "\t" << x.name << " : " << t.*(x.ptr());
            if (reflectionData.nFields != i + 1) {
                os << ",";
            }
            os << "\n";
            return Control::continue_t;
        },
        reflectionData.fields);
    os << "}\n";

    return os;
}

template <typename T>
void print(T&& t) {
    static_if<std::is_same<std::decay_t<T>, std::string>::value>([](auto&& x) {
        static_assert(std::is_same<std::decay_t<decltype(x)>, std::string>::value, "");
        std::cout << "string\n";
    })
        .template static_else_if<std::is_same<std::decay_t<T>, int32_t>::value>([](auto&& x) {
            static_assert(std::is_same<std::decay_t<decltype(x)>, int32_t>::value, "");
            std::cout << "int32_t\n";
        })
        .template static_else_if<std::is_same<std::decay_t<T>, char>::value>([](auto&& x) {
            static_assert(std::is_same<std::decay_t<decltype(x)>, char>::value, "");
            std::cout << "char\n";
        })
        .static_else([](auto&& x) { std::cout << "magic: " << x << "\n"; })(std::forward<T>(t));
}

struct MustConstruct {
    MustConstruct(int x) : x(x) {}

    int x;

    DECLARE(MustConstruct, x);
};

class Foo {
public:
    long a;
    short b;
    MustConstruct c;
    std::string d;

    DECLARE(Foo, a, b, c, d)

    VALIDATE(a) {
        return a > 0 && a < 10;
    }
};

int main() {
    std::cout << (Foo{1, 2, 3, "a"} < Foo{2, 3, 4, "b"}) << "\n";
    std::cout << Foo{1, 2, 3, "a"} << "\n";
    std::cout << load(Foo{0, 0, 0, ""}, std::make_tuple(1, 2, 3, "fun")) << "\n";

    Foo foo{0, 0, 0, ""};
    wrap(foo)["a"] = 9001;
    wrap(foo)["d"] = "hi";
    std::cout << foo << "\n";

    std::cout << "trying to throw...\n";
    try {
        wrap(foo)["d"] = 50;
    } catch (std::exception& e) {
        std::cout << e.what() << "\n";
    }
    std::cout << "\n";

    auto n = Foo::reflectionData().getCtorArgsBinder();
    n.b = 2;
    n.a = 1;
    n.c = 3;
    n.d = "wow";

    std::cout << n.make() << "\n";

    print(std::string{"foo"});
    print(int32_t{10});
    print('a');
    print(0.0);
}
