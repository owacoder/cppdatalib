/*
 * boost_compute.h
 *
 * Copyright © 2018 Oliver Adams
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Disclaimer:
 * Trademarked product names referred to in this file are the property of their
 * respective owners. These trademark owners are not affiliated with the author
 * or copyright holder(s) of this file in any capacity, and do not endorse this
 * software nor the authorship and existence of this file.
 */

#ifndef CPPDATALIB_BOOST_COMPUTE_H
#define CPPDATALIB_BOOST_COMPUTE_H

#include <boost/compute/container.hpp>

// Requires the STL adapters
#include "stl.h"
#include "../core/value_builder.h"

BOOST_COMPUTE_TYPE_NAME(cppdatalib::core::value, cppdatalib_core_value)

// -------
//  array
// -------

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<boost::compute::array, T, N>
{
    const boost::compute::array<T, N> &bind;
    boost::compute::command_queue &queue;
public:
    cast_array_template_to_cppdatalib(const boost::compute::array<T, N> &bind,
                                      boost::compute::command_queue &queue)
        : bind(bind), queue(queue)
    {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::array_t array;
        array.data().resize(N);
        boost::compute::copy(bind.begin(), bind.end(), array.data().begin(), queue);
        return array;
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<boost::compute::array, T, N>
{
    const cppdatalib::core::value &bind;
    boost::compute::command_queue &queue;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind,
                                        boost::compute::command_queue &queue)
        : bind(bind), queue(queue)
    {}
    operator boost::compute::array<T, N>() const
    {
        boost::compute::array<T, N> result(queue.get_context());
        if (bind.is_array())
            boost::compute::copy_n(bind.get_array_unchecked().data().begin(), std::min(N, bind.array_size()), result.begin(), queue);
        return result;
    }
};

// --------
//  vector
// --------

template<typename T, typename... Ts>
class cast_template_to_cppdatalib<boost::compute::vector, T, Ts...>
{
    const boost::compute::vector<T, Ts...> &bind;
    boost::compute::command_queue &queue;
public:
    cast_template_to_cppdatalib(const boost::compute::vector<T, Ts...> &bind,
                                boost::compute::command_queue &queue)
        : bind(bind), queue(queue)
    {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::array_t array;
        array.data().resize(bind.size());
        boost::compute::copy(bind.begin(), bind.end(), array.data().begin(), queue);
        return array;
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<boost::compute::vector, T, Ts...>
{
    const cppdatalib::core::value &bind;
    boost::compute::command_queue &queue;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind,
                                  boost::compute::command_queue &queue)
        : bind(bind), queue(queue)
    {}
    operator boost::compute::vector<T, Ts...>() const
    {
        boost::compute::vector<T, Ts...> result(bind.array_size(), queue.get_context());
        if (bind.is_array())
            boost::compute::copy(bind.get_array_unchecked().data().begin(),
                                 bind.get_array_unchecked().data().end(),
                                 result.begin(),
                                 queue);
        return result;
    }
};

#endif // CPPDATALIB_BOOST_COMPUTE_H
