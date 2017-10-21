
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "library_extensions.h"

using namespace xl;
using namespace std;


TEST(LibraryExtensions, erase_if) {
    vector<int> vi{1,2,3,4,5};
    erase_if(vi, [](int i){return i % 2;});
    EXPECT_EQ(vi.size(), 2);

    erase_if(vector<int>{1,2,3,4,5}, [](int i){return i % 2;});
    EXPECT_EQ(vi.size(), 2);
}

TEST(LibraryExtensions, copy_if) {
    {
        vector<int> vi{1, 2, 3, 4, 5};
        auto result = xl::copy_if(vi, [](int i) { return i % 2; });
        EXPECT_EQ(result.size(), 3);
    }
    {
        vector<int> vi{1, 2, 3, 4, 5};
        auto callback = [](int i) { return i % 2 == 0; };
        auto result = xl::copy_if(vi, callback);
        EXPECT_EQ(result.size(), 2);
    }
}


TEST(LibraryExtensions, each_i) {
    vector<int> v1{1,2,3,4,5};
    vector<int> v2{6,7,8,9,10};

    auto sum = 0;
    for(auto [e1, e2, i] : each_i(v1, v2)) {
//        std::cerr << fmt::format("adding {} {} {}", e1, e2, i) << std::endl;
        sum += e1 + e2 + i;
    }

    EXPECT_EQ(sum, 65);

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