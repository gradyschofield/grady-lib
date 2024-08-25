//
// Created by Grady Schofield on 8/17/24.
//

#include<catch2/catch_test_macros.hpp>

#include<iostream>

#include<gradylib/OpenHashSet.hpp>

using namespace std;
using namespace gradylib;

TEST_CASE("Open hash set"){
    OpenHashSet<int> s;
    s.insert(4);
    s.insert(4);
    s.insert(3);
    s.insert(999);
    REQUIRE(!s.contains(1));
    REQUIRE(s.contains(4));
    REQUIRE(s.contains(3));
    REQUIRE(s.contains(999));
}