
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "library_extensions.h"

using namespace xl;
using namespace std;

struct NoInsertionOperator{};
struct InsertionOperator {
    int i = 5;
};
std::ostream & operator<<(std::ostream & os, InsertionOperator & io){
    os << io.i;
    return os;
}

TEST(LibraryExtensions, has_insertion_operator) {

    EXPECT_TRUE(has_insertion_operator_v<int>);
    EXPECT_TRUE(has_insertion_operator_v<bool>);

//    EXPECT_FALSE(has_insertion_operator_v<NoInsertionOperator>);

    EXPECT_TRUE(has_insertion_operator_v<InsertionOperator>);
}

TEST(LibraryExtensions, erase_if) {
    {
        vector<int> vi{1, 2, 3, 4, 5};
        erase_if(vi, [](int i) { return i % 2; });
        EXPECT_EQ(vi.size(), 2ul);
    }

    {
        auto size = erase_if(vector<int>{1,2,3,4,5}, [](int i){return i % 2;}).size();
        EXPECT_EQ(size, 2ul);
    }
    {
        auto size = erase_if(set<int>{1,2,3,4,5}, [](int i){return i % 2;}).size();
        EXPECT_EQ(size, 2ul);
    }
    {
        auto size = erase_if(xl::copy(set<int>{1,2,3,4,5}), [](int i){return i % 2;}).size();
        EXPECT_EQ(size, 2ul);
    }
    {
        // should compile
        erase_if(set<unique_ptr<int>>{}, [](auto &){return false;});

        set<unique_ptr<int>> v{};
        erase_if(v, [](auto &){return false;});

    }
    
}

TEST(LibraryExtensions, copy_if) {
    {
        vector<int> vi{1, 2, 3, 4, 5};
        auto result = xl::copy_if(vi, [](int i) { return i % 2; });
        EXPECT_EQ(result.size(), 3ul);
    }
    {
        vector<int> vi{1, 2, 3, 4, 5};
        auto callback = [](int i) { return i % 2 == 0; };
        auto result = xl::copy_if(vi, callback);
        EXPECT_EQ(result.size(), 2ul);
    }
}


TEST(LibraryExtensions, each_i) {
    {
        vector<int> v1{1, 2, 3, 4, 5};

        auto sum = 0;
        for (auto [e1, e2, i] : each_i(v1, vector<int>{6, 7, 8, 9, 10})) {
//        std::cerr << fmt::format("adding {} {} {}", e1, e2, i) << std::endl;
            sum += e1 + e2 + i;
        }

        EXPECT_EQ(sum, 65);
    }

    {
        // test uncopyable container to make sure no copies are being made
        vector<unique_ptr<int>> v;
        v.push_back(std::make_unique<int>(5));
        for (auto [e1, i] : each_i(v)) {
            EXPECT_EQ(e1.get(), v[0].get());
        }
    }
}


class ClassWithMemberFunction {
public:
    ClassWithMemberFunction(){}
    ClassWithMemberFunction(ClassWithMemberFunction const &) = delete;
    ClassWithMemberFunction & operator=(ClassWithMemberFunction const &) = delete;
    bool a(int){return true;}
    bool b(ClassWithMemberFunction &){return false;}
};




TEST(LibraryExtensions, EAC) {
    EXPECT_EQ(eac(&ClassWithMemberFunction::a)(5), true);

    ClassWithMemberFunction cwmf;
    EXPECT_EQ(eac(&ClassWithMemberFunction::b)(cwmf), false);

    EXPECT_EQ(eac(true)("this doesn't matter"), true);
}

TEST(LibraryExtensions, contains) {
    std::vector<int> vi = {1,2,3};
    EXPECT_TRUE(contains(vi, 1));
    EXPECT_FALSE(contains(vi, 4));

    EXPECT_TRUE(contains(std::vector<int>{1,2,3}, 1));
    EXPECT_FALSE(contains(std::vector<int>{1,2,3}, 4));


    std::map<int, bool> map = {{1, true}, {2, false}};
    EXPECT_TRUE(contains(map, 1));
    EXPECT_FALSE(contains(map, 3));

    EXPECT_TRUE(contains(std::map<int, bool>{{1, true}, {2, false}}, 1));
    EXPECT_FALSE(contains(std::map<int, bool>{{1, true}, {2, false}}, 3));
}

TEST(TransformIf, transform_if) {
    vector<int> v {1,2,3,4,5};

    auto v2 = transform_if(v, [](int i)->std::optional<double>{if (i%2) return i + 0.5; else return {};});
    static_assert(std::is_same_v<decltype(v2), vector<double>>);

    EXPECT_EQ(v2.size(), 3ul);
    EXPECT_EQ(v2[0], 1.5);
    EXPECT_EQ(v2[1], 3.5);
    EXPECT_EQ(v2[2], 5.5);
}


TEST(FilteredContainer, filtered_vector) {

    std::vector<int> v;
    auto fi = filtered_inserter([](int i){return i%2;}, v, std::end(v));
    EXPECT_EQ(v.size(), 0ul);

    fi = 1;
    EXPECT_EQ(v.size(), 1ul);
    fi = 2;
    EXPECT_EQ(v.size(), 1ul);
}

TEST(FilteredContainer, filtered_set) {

    std::set<int> s;
    auto fi = filtered_inserter([](int i){return i%2;}, s, std::end(s));
    EXPECT_EQ(s.size(), 0ul);

    fi = 1;
    EXPECT_EQ(s.size(), 1ul);
    fi = 2;
    EXPECT_EQ(s.size(), 1ul);
}

TEST(FilteredContainer, filtered_map) {
    std::map<string, int> m;
    auto fi = filtered_inserter([](pair<string, int> i){return i.second%2;}, m, std::end(m));
    EXPECT_EQ(m.size(), 0ul);

    fi = pair("a", 1);
    EXPECT_EQ(m.size(), 1ul);
    fi = pair("b", 2);
    EXPECT_EQ(m.size(), 1ul);
}

TEST(is_template_for, is_template_for) {
    {
        using UPI = std::unique_ptr<int>;
        auto result = is_template_for_v<std::unique_ptr, UPI>;
        EXPECT_TRUE(result);
    }
}


TEST(unique_ptr_type, unique_ptr_type) {
    {
        auto result = std::is_same_v<unique_ptr_type_t<std::unique_ptr<int>>, int>;
        EXPECT_TRUE(result);
    }
    {
        auto result = std::is_same_v<unique_ptr_type_t<std::unique_ptr<int> const>, int>;
        EXPECT_TRUE(result);
    }
    {
        auto result = std::is_same_v<unique_ptr_type_t<std::unique_ptr<int> &>, int>;
        EXPECT_TRUE(result);
    }
    {
        auto result = std::is_same_v<unique_ptr_type_t<std::unique_ptr<int> &&>, int>;
        EXPECT_TRUE(result);
    }
    {
        auto result = std::is_same_v<unique_ptr_type_t<std::unique_ptr<int> const volatile &&>, int>;
        EXPECT_TRUE(result);
    }
    {
        using UPI = std::unique_ptr<int>;
        auto result = std::is_same_v<unique_ptr_type_t<UPI const volatile &&>, int>;
        EXPECT_TRUE(result);
    }
}

const vector<int> v;
static_assert(is_same_v<match_const_of_t<decltype(v)::value_type, decltype(v)>, int const>);
static_assert(is_same_v<decltype(v)::value_type, int>);

static_assert(std::is_same_v<remove_reference_wrapper_t<std::reference_wrapper<int> const>, int>);
static_assert(std::is_same_v<remove_reference_wrapper_t<std::reference_wrapper<int const> const>, int const>);
static_assert(std::is_same_v<remove_reference_wrapper_t<std::reference_wrapper<int const>>, int const>);
static_assert(std::is_same_v<remove_reference_wrapper_t<int>, int>);
static_assert(std::is_same_v<remove_reference_wrapper_t<int const>, int const>);
static_assert(std::is_same_v<remove_refs_and_wrapper_t<std::reference_wrapper<int>&>, int>);
static_assert(std::is_same_v<remove_refs_and_wrapper_t<int&>, int>);
static_assert(std::is_same_v<remove_refs_and_wrapper_t<std::reference_wrapper<int>>, int>);

TEST(LibraryExtensions, remove_reference_wrapper) {
    {
        auto result = std::is_same_v<remove_reference_wrapper_t<std::reference_wrapper<int>>, int>;
        EXPECT_TRUE(result);
    }
    {
        auto result = std::is_same_v<remove_reference_wrapper_t<std::reference_wrapper<char>>, int>;
        EXPECT_FALSE(result);
    }
    {
        auto result = std::is_same_v<remove_reference_wrapper_t<int>, int>;
        EXPECT_TRUE(result);
    }
    {
        auto result = std::is_same_v<remove_reference_wrapper_t<int>, char>;
        EXPECT_FALSE(result);
    }
}


TEST(LibraryExtensions, join) {
    {
        vector<string> v = {"a", "b", "c"};

        EXPECT_EQ(join(v), "a, b, c");
        EXPECT_EQ(join(v, "|"), "a|b|c");
    }
    {
        vector<string> v = {"a"};

        EXPECT_EQ(join(v), "a");
        EXPECT_EQ(join(v, "|"), "a");
    }
    {
        vector<string> v = {};

        EXPECT_EQ(join(v), "");
        EXPECT_EQ(join(v, "|"), "");
    }

}


TEST(LibraryExtensions, IsStdArray) {
    {
        auto result = is_std_array_v<std::array<int, 5>>;
        EXPECT_TRUE(result);
    }
    {
        auto result = is_std_array_v<std::vector<int>>;
        EXPECT_FALSE(result);
    }

}

TEST(LibraryExtensions, Transform) {
    vector<int> vi{1,2,3};
    auto result = transform(vi, [](int i){return std::to_string(i);});

    auto expected_result = vector<string>{"1", "2", "3"};
    EXPECT_EQ(result, expected_result);
}



TEST(LibraryExtensions, Find) {
    std::vector<int> v{1,2,3,4,5};

    EXPECT_NE(xl::find(v, 1), v.end());
    EXPECT_EQ(xl::find(v, 0), v.end());
    {
        auto result = xl::find_if(v, [](int i){return i==5;});
        EXPECT_NE(result, v.end());
    }
    {
        auto result = xl::find_if(v, [](int i){return i==0;});
        EXPECT_EQ(result, v.end());
    }
}


static_assert(std::is_same_v<dereferenced_type_t<int*>, int>);
static_assert(std::is_same_v<dereferenced_type_t<std::unique_ptr<char>>, char>);
static_assert(std::is_same_v<dereferenced_type_t<int>, int>);


#include "member_function_type_traits.h"

struct MemberFunctionsClass {
    void f1() {}
    void f2() const {}
    void f3() volatile {}
    void f4() const volatile {}

    void f1_lv() & {}
    void f2_lv() const & {}
    void f3_lv() volatile & {}
    void f4_lv() const volatile & {}

    void f1_rv() && {}
    void f2_rv() const && {}
    void f3_rv() volatile && {}
    void f4_rv() const volatile && {}

    void f1_noexcept() noexcept {}
    void f2_noexcept() const noexcept {}
    void f3_noexcept() volatile noexcept {}
    void f4_noexcept() const volatile noexcept {}
};


TEST(member_function_type_traits, f1) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f1)>;
    EXPECT_TRUE(T::const_v == false);
    EXPECT_TRUE(T::volatile_v == false);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}
TEST(member_function_type_traits, f2) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f2)>;
    EXPECT_TRUE(T::const_v == true);
    EXPECT_TRUE(T::volatile_v == false);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}
TEST(member_function_type_traits, f3) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f3)>;
    EXPECT_TRUE(T::const_v == false);
    EXPECT_TRUE(T::volatile_v == true);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}
TEST(member_function_type_traits, f4) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f4)>;
    EXPECT_TRUE(T::const_v == true);
    EXPECT_TRUE(T::volatile_v == true);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}

// lvalue tests
TEST(member_function_type_traits, f1_lv) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f1_lv)>;
    EXPECT_TRUE(T::const_v == false);
    EXPECT_TRUE(T::volatile_v == false);
    EXPECT_TRUE(T::lvalue_qualified == true);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}
TEST(member_function_type_traits, f2_lv) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f2_lv)>;
    EXPECT_TRUE(T::const_v == true);
    EXPECT_TRUE(T::volatile_v == false);
    EXPECT_TRUE(T::lvalue_qualified == true);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}
TEST(member_function_type_traits, f3_lv) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f3_lv)>;
    EXPECT_TRUE(T::const_v == false);
    EXPECT_TRUE(T::volatile_v == true);
    EXPECT_TRUE(T::lvalue_qualified == true);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}
TEST(member_function_type_traits, f4_lv) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f4_lv)>;
    EXPECT_TRUE(T::const_v == true);
    EXPECT_TRUE(T::volatile_v == true);
    EXPECT_TRUE(T::lvalue_qualified == true);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}

// rvalue tests
TEST(member_function_type_traits, f1_rv) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f1_rv)>;
    EXPECT_TRUE(T::const_v == false);
    EXPECT_TRUE(T::volatile_v == false);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == true);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);
}

TEST(member_function_type_traits, f2_rv) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f2_rv)>;
    EXPECT_TRUE(T::const_v == true);
    EXPECT_TRUE(T::volatile_v == false);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == true);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}
TEST(member_function_type_traits, f3_rv) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f3_rv)>;
    EXPECT_TRUE(T::const_v == false);
    EXPECT_TRUE(T::volatile_v == true);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == true);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}
TEST(member_function_type_traits, f4_rv) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f4_rv)>;
    EXPECT_TRUE(T::const_v == true);
    EXPECT_TRUE(T::volatile_v == true);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == true);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_FALSE(T::noexcept_v);

}

// noexcept
TEST(member_function_type_traits, f1_noexcept) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f1_noexcept)>;
    EXPECT_TRUE(T::const_v == false);
    EXPECT_TRUE(T::volatile_v == false);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_TRUE(T::noexcept_v);
}
TEST(member_function_type_traits, f2_noexcept) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f2_noexcept)>;
    EXPECT_TRUE(T::const_v == true);
    EXPECT_TRUE(T::volatile_v == false);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_TRUE(T::noexcept_v);

}
TEST(member_function_type_traits, f3_noexcept) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f3_noexcept)>;
    EXPECT_TRUE(T::const_v == false);
    EXPECT_TRUE(T::volatile_v == true);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_TRUE(T::noexcept_v);

}
TEST(member_function_type_traits, f4_noexcept) {
    using T = pointer_to_member_function<decltype(&MemberFunctionsClass::f4_noexcept)>;
    EXPECT_TRUE(T::const_v == true);
    EXPECT_TRUE(T::volatile_v == true);
    EXPECT_TRUE(T::lvalue_qualified == false);
    EXPECT_TRUE(T::rvalue_qualified == false);
    static_assert(std::is_same_v<T::return_type, void>);
    static_assert(std::is_same_v<T::class_type, MemberFunctionsClass>);
    EXPECT_EQ(T::arity, 0);
    EXPECT_TRUE(T::noexcept_v);
}

