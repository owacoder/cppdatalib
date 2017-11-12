/*
 * netstrings.h
 *
 * Copyright © 2017 Oliver Adams
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
 */

#ifndef CPPDATALIB_NETSTRINGS_H
#define CPPDATALIB_NETSTRINGS_H

#include "../core/value_builder.h"

// TODO: Refactor into stream_parser API and impl::stream_writer_base API

namespace cppdatalib
{
    namespace netstrings
    {
        inline size_t get_size(const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: return 3; // "0:,"
                case core::boolean: return 7 + v.get_bool(); // "4:true," or "5:false,"
                case core::integer:
                {
                    std::ostringstream str;
                    str << ":" << v.get_int() << ',';
                    str << (str.str().size() - 2);
                    return str.str().size();
                }
                case core::uinteger:
                {
                    std::ostringstream str;
                    str << ":" << v.get_uint() << ',';
                    str << (str.str().size() - 2);
                    return str.str().size();
                }
                case core::real:
                {
                    std::ostringstream str;
                    str << ":" << v.get_real() << ',';
                    str << (str.str().size() - 2);
                    return str.str().size();
                }
                case core::string:
                    return floor(log10(std::max(v.size(), size_t(1))) + 1) + v.size() + 2;
                case core::array:
                {
                    size_t size = 0;

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        size += get_size(*it);

                    return floor(log10(std::max(size, size_t(1))) + 1) + size + 2;
                }
                case core::object:
                {
                    size_t size = 0;

                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        size += get_size(it->first) + get_size(it->second);

                    return floor(log10(std::max(size, size_t(1))) + 1) + size + 2;
                }
            }

            // Control will never get here
            return 0;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            switch (v.get_type())
            {
                case core::null: return stream << "0:,";
                case core::boolean: return stream << (v.get_bool()? "4:true,": "5:false,");
                case core::integer: return stream << std::to_string(v.get_int()).size() << ':' << v.get_int() << ',';
                case core::uinteger: return stream << std::to_string(v.get_uint()).size() << ':' << v.get_uint() << ',';
                case core::real: return stream << std::to_string(v.get_real()).size() << ':' << v.get_real() << ',';
                case core::string: return stream << v.size() << ':' << v.get_string() << ',';
                case core::array:
                {
                    size_t size = 0;

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        size += get_size(*it);

                    stream << size << ':';

                    for (auto it = v.get_array().begin(); it != v.get_array().end(); ++it)
                        stream << *it;

                    return stream << ',';
                }
                case core::object:
                {
                    size_t size = 0;

                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        size += get_size(it->first) + get_size(it->second);

                    stream << size << ':';

                    for (auto it = v.get_object().begin(); it != v.get_object().end(); ++it)
                        stream << it->first << it->second;

                    return stream << ',';
                }
            }

            // Control will never get here
            return stream;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_netstrings(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_NETSTRINGS_H
