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

#include "../core/core.h"
#include <sstream>
#include <algorithm>

namespace cppdatalib
{
    namespace netstrings
    {
        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(core::ostream_handle &output) : core::stream_writer(output) {}

            protected:
                size_t get_size(const core::value &v)
                {
                    struct traverser
                    {
                    private:
                        std::stack<size_t, std::vector<size_t>> size;

                    public:
                        traverser() {size.push(0);}

                        size_t get_size() const {return size.top();}

                        bool operator()(const core::value *arg, core::value::traversal_ancestry_finder, bool prefix)
                        {
                            switch (arg->get_type())
                            {
                                case core::null:
                                    if (prefix)
                                        size.top() += 3; // "0:,"
                                    break;
                                case core::boolean:
                                    if (prefix)
                                        size.top() += 7 + arg->get_bool_unchecked(); // "4:true," or "5:false,"
                                    break;
                                case core::integer:
                                {
                                    if (prefix)
                                    {
                                        core::ostringstream stream;
                                        stream << arg->get_int_unchecked();
                                        stream << stream.str().size();
                                        size.top() += 2 + stream.str().size();
                                    }
                                    break;
                                }
                                case core::uinteger:
                                {
                                    if (prefix)
                                    {
                                        core::ostringstream stream;
                                        stream << arg->get_uint_unchecked();
                                        stream << stream.str().size();
                                        size.top() += 2 + stream.str().size();
                                    }
                                    break;
                                }
                                case core::real:
                                {
                                    if (prefix)
                                    {
                                        core::ostringstream stream;
                                        stream.precision(CPPDATALIB_REAL_DIG);
                                        stream << arg->get_real_unchecked();
                                        stream << stream.str().size();
                                        size.top() += 2 + stream.str().size();
                                    }
                                    break;
                                }
                                case core::string:
                                {
                                    if (prefix)
                                        size.top() += 2 + arg->string_size() + std::to_string(arg->string_size()).size();
                                    break;
                                }
                                case core::array:
                                case core::object:
                                {
                                    if (prefix)
                                    {
                                        size.push(0);
                                    }
                                    else
                                    {
                                        size_t temp = size.top() + (size.size() > 2? std::to_string(size.top()).size() + 2: 0);
                                        size.pop();
                                        size.top() += temp;
                                    }

                                    break;
                                }
                            }

                            return true;
                        }
                    };

                    traverser t;

                    v.traverse(t);

                    return t.get_size();
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(core::ostream_handle output) : impl::stream_writer_base(output) {}

            unsigned int required_features() const {return requires_prefix_array_size | requires_prefix_object_size | requires_prefix_string_size;}

            std::string name() const {return "cppdatalib::netstrings::stream_writer";}

        protected:
            void null_(const core::value &) {stream().write("0:,", 3);}
            void bool_(const core::value &v) {v.get_bool_unchecked()? stream().write("4:true,", 7): stream().write("5:false,", 8);}

            void integer_(const core::value &v)
            {
                core::ostringstream strm;

                strm << v.get_int_unchecked();
                stream() << strm.str().size();
                stream().put(':');
                stream() << strm.str();
                stream().put(',');
            }

            void uinteger_(const core::value &v)
            {
                core::ostringstream strm;

                strm << v.get_uint_unchecked();
                stream() << strm.str().size();
                stream().put(':');
                stream() << strm.str();
                stream().put(',');
            }

            void real_(const core::value &v)
            {
                core::ostringstream strm;

                strm.precision(CPPDATALIB_REAL_DIG);
                strm << v.get_real_unchecked();
                stream() << strm.str().size();
                stream().put(':');
                stream() << strm.str();
                stream().put(',');
            }

            void begin_string_(const core::value &, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Netstrings - 'string' value does not have size specified");

                stream() << size;
                stream().put(':');
            }
            void string_data_(const core::value &v, bool) {stream().write(v.get_string_unchecked().c_str(), v.get_string_unchecked().size());}
            void end_string_(const core::value &, bool) {stream().put(',');}

            void begin_array_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Netstrings - 'array' value does not have size specified");
                else if (v.size() != static_cast<size_t>(size))
                    throw core::error("Netstrings - entire 'array' value must be buffered before writing");

                stream() << get_size(v);
                stream().put(':');
            }
            void end_array_(const core::value &, bool) {stream().put(',');}

            void begin_object_(const core::value &v, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Netstrings - 'object' value does not have size specified");
                else if (v.size() != static_cast<size_t>(size))
                    throw core::error("Netstrings - entire 'object' value must be buffered before writing");

                stream() << get_size(v);
                stream().put(':');
            }
            void end_object_(const core::value &, bool) {stream().put(',');}
        };

        inline std::string to_netstrings(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer w(stream);
            w << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_NETSTRINGS_H
