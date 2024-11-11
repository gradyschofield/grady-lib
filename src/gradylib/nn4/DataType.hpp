/*
MIT License

Copyright (c) 2024 Grady Schofield

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include<type_traits>

namespace gradylib {
    namespace nn {
        enum ParameterPurpose {
            InputParameter,
            FreeParameter
        };

        enum DataType {
            Bfloat,
            Float,
            Double,
            I8,
            I16,
            I32,
            I64,
            U8,
            U16,
            U32,
            U64,
            Derived
        };

        template<DataType dataType>
        struct DataTypeTraits;

        template<>
        struct DataTypeTraits<Float> {
            using type = float;
            static int const sizeInBytes = sizeof(type);
        };

        template<>
        struct DataTypeTraits<Derived> {
            using type = std::void_t<>;
            static int const sizeInBytes = 0;
        };

        int sizeInBytes(DataType dataType) {
            switch(dataType) {
                case Float: return DataTypeTraits<Float>::sizeInBytes;
                default: return 0;
            }
        }

    }
}