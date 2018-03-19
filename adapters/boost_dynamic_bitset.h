/*
 * boost_dynamic_bitset.h
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

#ifndef CPPDATALIB_BOOST_DYNAMIC_BITSET_H
#define CPPDATALIB_BOOST_DYNAMIC_BITSET_H

#include <boost/dynamic_bitset.hpp>

#include "../core/value_parser.h"

template<typename... Ts>
class cast_template_to_cppdatalib<boost::dynamic_bitset, Ts...>
{
    const boost::dynamic_bitset<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const boost::dynamic_bitset<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result = cppdatalib::core::value(cppdatalib::core::array_t());
        for (size_t i = 0; i < bind.size(); ++i)
            result.push_back(cppdatalib::core::value(bool(bind[i])));
        return result;
    }
};

template<typename... Ts>
class cast_template_from_cppdatalib<boost::dynamic_bitset, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator boost::dynamic_bitset<Ts...>() const
    {
        boost::dynamic_bitset<Ts...> result;
        size_t index = 0;
        result.resize(bind.array_size());
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result[index++] = item.operator bool();
        return result;
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<boost::dynamic_bitset, Ts...> : public generic_stream_input
    {
    protected:
        const boost::dynamic_bitset<Ts...> &bind;
        size_t idx;

    public:
        template_parser(const boost::dynamic_bitset<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {idx = 0;}

        void write_one_()
        {
            if (was_just_reset())
                get_output()->begin_array(core::array_t(), bind.size());
            else if (idx != bind.size())
                get_output()->write(bool(bind[idx++]));
            else
                get_output()->end_array(core::array_t());
        }
    };
}}

#endif // CPPDATALIB_BOOST_DYNAMIC_BITSET_H
