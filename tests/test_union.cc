#include <sstream>

#include "gtest/gtest.h"
#include "logic/union.hh"


struct union_a: logic::union_<16> {
    template <typename T>
    static logic::logic<15, 0> a(const T &v) {
        return v.template slice<15, 0>();
    }

    template <typename T>
    static logic::logic<7, 0> b(const T &v) {
        return v.template slice<7, 0>();
    }

    template <typename T, typename K>
    static void a(T &v, const K &k) {
        v.template update<15, 0>(k);
    }

    template <typename T, typename K>
    static void b(T &v, const K &k) {
        v.template update<7, 0>(k);
    }
};


TEST(union_, ctor) {    // NOLINT
    union_a::type u;
    auto l = logic::logic<15, 0>{"1111111111111111"};
    union_a::a(u, l);
    auto b = union_a::b(u);
    EXPECT_EQ(b.str(), "11111111");
}
