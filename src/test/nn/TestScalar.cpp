//
// Created by Grady Schofield on 11/15/24.
//

#include<catch2/catch_test_macros.hpp>

#include<vector>

#include<gradylib/nn4/Tensor.hpp>
#include<gradylib/nn4/Initializer.hpp>
#include<gradylib/nn4/Evaluator.hpp>

using namespace std;

TEST_CASE("Basic Scalar") {
    using namespace gradylib::nn::activation;
    auto z = 1 + log(x) + exp(x*x);
    cout << z.evaluate(0.2) << "\n";
}

TEST_CASE("Basic scalar derivative") {
    using namespace gradylib::nn::activation;
    auto z = 1 + log(x) + exp(x*x);
    auto dz = z.derivative();
    auto z2 = h / x + 2 * x * exp(x*x) * h;
    //REQUIRE(z == z2);
    //cout << z.derivative() << "\n";
}

TEST_CASE("Basic factor") {
    using namespace gradylib::nn::activation;
    auto z = x * h + h * x;
    z.simplify();
    auto z3 = x * h + h * x;
    //REQUIRE(z == z2);
    //cout << z.derivative() << "\n";
}

TEST_CASE("Basic expand") {
    using namespace gradylib::nn::activation;
    auto z = x * (x + h);
    z.expand();
    auto z3 = x * h + h * x;
    //REQUIRE(z == z2);
    //cout << z.derivative() << "\n";
}

TEST_CASE("Expand exercising postfix design") {
    using namespace gradylib::nn::activation;
    auto z = x[1] * x[2] * x[3] * (x[4] + x[5]);
    z.expand();
    auto z3 = x * h + h * x;
    //REQUIRE(z == z2);
    //cout << z.derivative() << "\n";
}

TEST_CASE("Expand exercising call on newly created multiplies") {
    using namespace gradylib::nn::activation;
    auto z = (x[1] + x[2]) * (x[3] + x[4]);
    z.expand();
    auto z3 = x * h + h * x;
    //REQUIRE(z == z2);
    //cout << z.derivative() << "\n";
}
