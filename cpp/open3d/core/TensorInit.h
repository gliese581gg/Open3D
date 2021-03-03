// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#pragma once

#include <initializer_list>
#include <utility>

#include "open3d/core/SizeVector.h"

namespace open3d {
namespace core {
namespace tensor_init {

template <typename T, size_t D>
struct NestedInitializerImpl {
    using type = std::initializer_list<
            typename NestedInitializerImpl<T, D - 1>::type>;
};

template <typename T>
struct NestedInitializerImpl<T, 0> {
    using type = T;
};

template <typename T, size_t D>
using NestedInitializerList = typename NestedInitializerImpl<T, D>::type;

template <typename T>
struct InitializerDim {
    static constexpr size_t value = 0;
};

template <typename T>
struct InitializerDim<std::initializer_list<T>> {
    static constexpr size_t value = 1 + InitializerDim<T>::value;
};

template <size_t D>
struct InitializerShapeImpl {
    template <typename T>
    static constexpr size_t value(T t) {
        if (t.size() == 0) {
            return 0;
        }
        size_t dim = InitializerShapeImpl<D - 1>::value(*t.begin());
        for (auto it = t.begin(); it != t.end(); ++it) {
            if (dim != InitializerShapeImpl<D - 1>::value(*it)) {
                utility::LogError(
                        "Input contains ragged nested sequences"
                        "(nested lists with unequal sizes or shapes).");
            }
        }
        return dim;
    }
};

template <>
struct InitializerShapeImpl<0> {
    template <typename T>
    static constexpr size_t value(T t) {
        return t.size();
    }
};

template <typename T, size_t... D>
SizeVector InitializerShape(T t, std::index_sequence<D...>) {
    return SizeVector{
            static_cast<int64_t>(InitializerShapeImpl<D>::value(t))...};
}

template <typename T, typename D>
void NestedCopy(T&& iter, const D& s) {
    *iter++ = s;
}

template <typename T, typename D>
void NestedCopy(T&& iter, std::initializer_list<D> s) {
    for (auto it = s.begin(); it != s.end(); ++it) {
        NestedCopy(std::forward<T>(iter), *it);
    }
}

template <typename T>
SizeVector InferShape(T t) {
    SizeVector shape = InitializerShape<T>(
            t, std::make_index_sequence<InitializerDim<T>::value>());

    // Handle 0-dimensional inputs.
    size_t last_dim = 0;
    while (shape.size() > (last_dim + 1) && shape[last_dim] != 0) {
        last_dim++;
    }
    shape.resize(last_dim + 1);

    return shape;
}

template <typename T, size_t D>
std::vector<T> ToFlatVector(
        const SizeVector& shape,
        const tensor_init::NestedInitializerList<T, D>& nested_list) {
    std::vector<T> values(shape.NumElements());
    tensor_init::NestedCopy(values.begin(), nested_list);
    return values;
}

}  // namespace tensor_init
}  // namespace core
}  // namespace open3d