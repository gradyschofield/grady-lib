#ifndef CONTROL_BFGS_HPP
#define CONTROL_BFGS_HPP

#pragma once

#include<iostream>
#include<list>
#include<vector>

#include "SparseMatrix.hpp"

namespace gradylib {
    class BFGS {
        std::vector<double> lastX;
        std::vector<double> lastF;
        std::list<std::vector<double>> s;
        std::list<std::vector<double>> y;
        std::list<double> sTy;
        int maxMemory;

        std::vector<double> apply(std::vector<double> && in, decltype(s.rbegin()) & sIter, decltype(y.rbegin()) & yIter, decltype(sTy.rbegin()) & sTyIter) {
            using std::vector;
            using namespace gradylib::std_vector_operators;
            if (sIter == s.rend()) {
                return in;
            }
            vector<double> const & s = *sIter;
            vector<double> const & y = *yIter;
            double sTyInv = 1.0 / *sTyIter;
            //apply RHS
            double sDotIn = dot(s, in);
            vector<double> rhsOut = std::move(in);
            for (size_t i = 0; i < rhsOut.size(); ++i) {
                rhsOut[i] -= y[i] * sDotIn * sTyInv;
            }
            ++sIter;
            ++yIter;
            ++sTyIter;
            vector<double> lhsOut = apply(std::move(rhsOut), sIter, yIter, sTyIter);
            // apply LHS
            double yDotLhsOut = dot(y, lhsOut);
            for (size_t i = 0; i < lhsOut.size(); ++i) {
                lhsOut[i] -= s[i] * yDotLhsOut * sTyInv;
            }
            for (size_t i = 0; i < lhsOut.size(); ++i) {
                lhsOut[i] += s[i] * sDotIn * sTyInv;
            }
            return lhsOut;
        }


    public:
        BFGS(int maxMemory)
            : maxMemory(maxMemory)
        {
        }

        void update(std::vector<double> x, std::vector<double> F) {
            using std::vector;
            using std::cout;
            if (lastX.empty()) {
               lastX = std::move(x);
               lastF = std::move(F);
            } else {
                vector<double> sn(x);
                vector<double> yn(F);
                double dot = 0;
                for (size_t i = 0; i < sn.size(); ++i) {
                    yn[i] -= lastF[i];
                    sn[i] -= lastX[i];
                    dot += sn[i] * yn[i];
                }
                if (dot == 0) {
                    lastX.clear();
                    lastF.clear();
                    s.clear();
                    y.clear();
                    sTy.clear();
                    return;
                    //cout << "Dot ZERO" << std::endl;
                    //cout.flush();
                    //exit(1);
                }
                s.push_back(std::move(sn));
                y.push_back(std::move(yn));
                sTy.push_back(dot);
                lastX = std::move(x);
                lastF = std::move(F);
            }
            if (s.size() > maxMemory) {
                s.pop_front();
                y.pop_front();
                sTy.pop_front();
            }
        }

        bool isReady() const {
            return true;
        }

        std::vector<double> step(std::vector<double> x) {
            if (s.empty()) {
                return x;
            }
            auto sIter = s.rbegin();
            auto yIter = y.rbegin();
            auto sTyIter = sTy.rbegin();
            auto ret = apply(std::move(x), sIter, yIter, sTyIter);
            return ret;
        }

    };

    class Broyden {
        std::list<std::vector<double>> lastX;
        std::list<std::vector<double>> lastF;
        std::list<std::vector<double>> s;
        std::list<std::vector<double>> Bss;
        std::list<std::vector<double>> y;
        std::list<double> sTs;
        int maxMemory;

        std::vector<double> apply(std::vector<double> const &in,
                                  decltype(s.rbegin()) sIter,
                                  decltype(y.rbegin()) yIter,
                                  decltype(Bss.rbegin()) BsIter,
                                  decltype(sTs.rbegin()) sTsIter) {
            using std::vector;
            using namespace gradylib::std_vector_operators;
            if (sIter == s.rend()) {
                return in;
            }
            vector<double> const & s = *sIter;
            vector<double> const & y = *yIter;
            double sTsInv = 1.0 / *sTsIter;
            double sDotIn = dot(s, in);
            vector<double> const & Bs = *BsIter;
            ++sIter;
            ++yIter;
            ++BsIter;
            ++sTsIter;
            vector<double> Bin = apply(in, sIter, yIter, BsIter, sTsIter);
            //vector<double> Bs = apply(s, sIter, yIter, BsIter, sTsIter);
            for (size_t i = 0; i < in.size(); ++i) {
                Bin[i] += (y[i] - Bs[i]) * sDotIn * sTsInv;
            }
            return Bin;
        }


    public:
        Broyden(int maxMemory)
                : maxMemory(maxMemory)
        {
        }

        bool isReady() const {
            return s.size() > 0;
        }

        void forgetLastStep() {
            s.pop_back();
            y.pop_back();
            Bss.pop_back();
            sTs.pop_back();
            lastX.pop_back();
            lastF.pop_back();
        }

        void update(std::vector<double> x, std::vector<double> F) {
            using std::vector;
            using std::cout;
            if (lastX.empty()) {
                lastX.push_back(x);
                lastF.push_back(F);
            } else {
                vector<double> sn(x);
                vector<double> yn(F);
                double dot = 0;
                for (size_t i = 0; i < sn.size(); ++i) {
                    yn[i] -= lastF.back()[i];
                    sn[i] -= lastX.back()[i];
                    dot += sn[i] * sn[i];
                }
                vector<double> Bs = step(sn);
                Bss.push_back(std::move(Bs));
                s.push_back(std::move(sn));
                y.push_back(std::move(yn));
                sTs.push_back(dot);
                lastX.push_back(x);
                lastF.push_back(F);
            }
            if (s.size() > maxMemory) {
                s.pop_front();
                y.pop_front();
                Bss.pop_front();
                sTs.pop_front();
                lastX.pop_front();
                lastF.pop_front();
            }
            cout << "broyden mem size: " << s.size() << std::endl;
        }

        std::vector<double> step(std::vector<double> x) {
            if (s.empty()) {
                return x;
            }
            auto sIter = s.rbegin();
            auto yIter = y.rbegin();
            auto BsIter = Bss.rbegin();
            auto sTsIter = sTs.rbegin();
            return apply(x, sIter, yIter, BsIter, sTsIter);
        }
    };
}

#endif //CONTROL_BFGS_HPP
