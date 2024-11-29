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

TEST_CASE("Basic derivative") {
    using namespace gradylib::nn::activation;
    auto z = x * x;
    auto dz = z.derivative();
    dz.simplify();
    auto z3 = x * h * 2;
    REQUIRE(dz.equalPolynomialTerms(z3));
}

TEST_CASE("Derivative x^3") {
    using namespace gradylib::nn::activation;
    auto z = x * x * x;
    auto dz = z.derivative();
    dz.simplify();
    auto z3 = x * x * h * 3;
    REQUIRE(dz.equalPolynomialTerms(z3));
}

TEST_CASE("Derivative x^2 + x^3") {
    using namespace gradylib::nn::activation;
    auto z = x * x * x + x*x;
    auto dz = z.derivative();
    dz.simplify();
    dz.expand();
    auto z3 = x * x * h * 3 + 2*x*h;
    REQUIRE(dz.equalPolynomialTerms(z3));
}

TEST_CASE("Derivative t*x^3") {
    using namespace gradylib::nn::activation;
    double t = (0.5 + 0.125);
    auto z = t*x * x * x;
    auto dz = z.derivative();
    dz.simplify();
    auto z3 = (x * x * h * 3)*t;
    REQUIRE(dz.equalPolynomialTerms(z3));
}

TEST_CASE("Basic factor") {
    using namespace gradylib::nn::activation;
    auto z = x * h + h * x;
    z.simplify();
    auto z3 = x * h * 2;
    REQUIRE(z.equalPolynomialTerms(z3));
}

TEST_CASE("Basic expand") {
    using namespace gradylib::nn::activation;
    auto z = x * (x + h);
    z.expand();
    auto z3 = x * x + h * x;
    REQUIRE(z.equalPolynomialTerms(z3));
    //cout << z.derivative() << "\n";
}

TEST_CASE("Expand exercising postfix design") {
    using namespace gradylib::nn::activation;
    auto z = x[1] * x[2] * x[3] * (x[4] + x[5]);
    z.expand();
    auto z3 = x[1]*x[2]*x[3]*x[4] + x[1]*x[2]*x[3]*x[5];
    REQUIRE(z.equalPolynomialTerms(z3));
    //cout << z.derivative() << "\n";
}

TEST_CASE("Expand exercising call on newly created multiplies") {
    using namespace gradylib::nn::activation;
    auto z = (x[1] + x[2]) * (x[3] + x[4]);
    z.expand();
    auto z3 = x[1]*x[3] + x[1]*x[4] + x[2]*x[3] + x[2]*x[4];
    REQUIRE(z.equalPolynomialTerms(z3));
    //cout << z.derivative() << "\n";
}

TEST_CASE("Expand 3") {
    using namespace gradylib::nn::activation;
    auto z = (x[1] + x[2]) * (x[3] + x[4] * x[5] * x[6] * (x[7] + x[8] + x[9]));
    z.expand();
    auto z3 = x[1] * x[3] +
            x[1] * x[4] * x[5] * x[6] * x[7] +
            x[1] * x[4] * x[5] * x[6] * x[8] +
            x[1] * x[4] * x[5] * x[6] * x[9] +
            x[2] * x[3] +
            x[2] * x[4] * x[5] * x[6] * x[7] +
            x[2] * x[4] * x[5] * x[6] * x[8] +
            x[2] * x[4] * x[5] * x[6] * x[9];
    REQUIRE(z.equalPolynomialTerms(z3));
}

TEST_CASE("Expand 4") {
    using namespace gradylib::nn::activation;
    auto z = x[1] * (x[6] * (x[7] + x[8] + x[9]));
    z.expand();
    auto z3 =
              x[1] * x[6] * x[7] +
              x[1] * x[6] * x[8] +
              x[1] * x[6] * x[9];
    REQUIRE(z.equalPolynomialTerms(z3));
}

TEST_CASE("Simplify/Expand 1") {
    using namespace gradylib::nn::activation;
    auto z = x[1] * (x[6] * (x[7] + x[8] + x[9])) + x[6] * (x[7] - x[8]) * x[9] * (x[10] - x[11]);
    z.simplify();
    z.expand();
    auto z3 =
            x[1] * x[6] * x[7] +
            x[1] * x[6] * x[8] +
            x[1] * x[6] * x[9] +
            x[6] * x[7] * x[9] * x[10] -
            x[6] * x[7] * x[9] * x[11] -
            x[6] * x[8] * x[9] * x[10] +
            x[6] * x[8] * x[9] * x[11];
    REQUIRE(z.equalPolynomialTerms(z3));
}

TEST_CASE("Exp large") {
    double t = 50;
    cout << -t - log(1+exp(-t)) << "\n";
    double s = 1 / (1 + exp(-t));
    cout << s << " " << exp(-t) << "\n";
    cout << log(1-s) << "\n";
}
