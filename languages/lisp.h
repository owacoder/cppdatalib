/*
 * lisp.h
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
 */

#ifndef CPPDATALIB_LISP_H
#define CPPDATALIB_LISP_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace lang
    {
        namespace lisp
        {
            class stream_writer : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer(core::ostream_handle output) : core::stream_writer(output) {}

                std::string name() const {return "cppdatalib::lang::lisp::stream_writer";}

            protected:
                void begin_key_(const core::value &)
                {
                    if (current_container_size() > 0)
                        stream().put(' ');
                    stream().put('(');
                }
                void end_key_(const core::value &) {stream().write(" .", 3);}
                void begin_item_(const core::value &)
                {
                    if (current_container_size() > 0 || current_container() == core::object)
                        stream().put(' ');
                }
                void end_item_(const core::value &)
                {
                    if (current_container() == core::object)
                        stream().put(')');
                }

                void null_(const core::value &) {throw core::error("Lisp - 'null' value not allowed in output");}
                void bool_(const core::value &) {throw core::error("Lisp - 'boolean' value not allowed in output");}
                void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
                void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
                void real_(const core::value &v)
                {
                    if (!std::isfinite(v.get_real_unchecked()))
                        throw core::error("Lisp - cannot write 'NaN' or 'Infinity' values");
                    stream() << v.get_real_unchecked();
                }

                // TODO: Strings should be escaped (if it's even possible) for Lisp
                void begin_string_(const core::value &, core::optional_size, bool) {stream().put('"');}
                void string_data_(const core::value &v, bool) {stream() << v.get_string_unchecked();}
                void end_string_(const core::value &, bool) {stream().put('"');}

                void begin_array_(const core::value &, core::optional_size, bool) {stream().write("(list ", 6);}
                void end_array_(const core::value &, bool) {stream().put(')');}

                void begin_object_(const core::value &, core::optional_size, bool) {stream().write("(list ", 6);}
                void end_object_(const core::value &, bool) {stream().put(')');}
            };

            inline std::string to_lisp(const core::value &v)
            {
                core::ostringstream stream;
                stream_writer w(stream);
                w << v;
                return stream.str();
            }
        }
    }
}

#endif // CPPDATALIB_LISP_H
