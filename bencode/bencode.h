/*
 * bencode.h
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

#ifndef CPPDATALIB_BENCODE_H
#define CPPDATALIB_BENCODE_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace bencode
    {
        class parser : public core::stream_parser
        {
        public:
            parser(core::istream_handle input) : core::stream_parser(input) {}

            bool provides_prefix_string_size() const {return true;}

            core::stream_input &convert(core::stream_handler &writer)
            {
                std::unique_ptr<char []> buffer(new char [core::buffer_size]);
                bool written = false;
                int chr;

                while (chr = stream().peek(), chr != EOF)
                {
                    written = true;

                    switch (chr)
                    {
                        case 'i':
                        {
                            stream().get(); // Eat 'i'

                            core::int_t i;
                            stream() >> i;
                            if (!stream()) throw core::error("Bencode - expected 'integer' value");

                            writer.write(i);
                            if (stream().get() != 'e') throw core::error("Bencode - invalid 'integer' value");
                            break;
                        }
                        case 'e':
                            stream().get(); // Eat 'e'
                            switch (writer.current_container())
                            {
                                case core::array: writer.end_array(core::array_t()); break;
                                case core::object: writer.end_object(core::object_t()); break;
                                default: throw core::error("Bencode - attempt to end element does not exist"); break;
                            }
                            break;
                        case 'l':
                            stream().get(); // Eat 'l'
                            writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                            break;
                        case 'd':
                            stream().get(); // Eat 'd'
                            writer.begin_object(core::object_t(), core::stream_handler::unknown_size);
                            break;
                        default:
                            if (isdigit(chr))
                            {
                                core::int_t size;

                                stream() >> size;
                                if (size < 0) throw core::error("Bencode - expected string size");
                                if (stream().get() != ':') throw core::error("Bencode - expected ':' separating string size and data");

                                writer.begin_string(core::string_t(), size);
                                while (size > 0)
                                {
                                    core::int_t buffer_size = std::min(core::int_t(core::buffer_size), size);
                                    stream().read(buffer.get(), buffer_size);
                                    if (stream().fail())
                                        throw core::error("Bencode - unexpected end of string");
                                    writer.append_to_string(core::string_t(buffer.get(), buffer_size));
                                    size -= buffer_size;
                                }
                                writer.end_string(core::string_t());
                            }
                            else
                                throw core::error("Bencode - expected value");
                            break;
                    }

                    if (writer.nesting_depth() == 0)
                        break;
                }

                if (!written)
                    throw core::error("Bencode - expected value");
                else if (writer.nesting_depth())
                    throw core::error("Bencode - unexpected end of stream");

                return *this;
            }
        };

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(core::ostream_handle output) : core::stream_writer(output) {}

            bool requires_prefix_string_size() const {return true;}

        protected:
            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("Bencode - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Bencode - 'null' value not allowed in output");}
            void bool_(const core::value &) {throw core::error("Bencode - 'boolean' value not allowed in output");}
            void integer_(const core::value &v)
            {
                stream().put('i');
                stream() << v.get_int_unchecked();
                stream().put('e');
            }
            void uinteger_(const core::value &v)
            {
                stream().put('i');
                stream() << v.get_uint_unchecked();
                stream().put('e');
            }
            void real_(const core::value &) {throw core::error("Bencode - 'real' value not allowed in output");}
            void begin_string_(const core::value &, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Bencode - 'string' value does not have size specified");
                stream() << size;
                stream().put(':');
            }
            void string_data_(const core::value &v, bool) {stream() << v.get_string_unchecked();}

            void begin_array_(const core::value &, core::int_t, bool) {stream().put('l');}
            void end_array_(const core::value &, bool) {stream().put('e');}

            void begin_object_(const core::value &, core::int_t, bool) {stream().put('d');}
            void end_object_(const core::value &, bool) {stream().put('e');}
        };

        inline core::value from_bencode(core::istream_handle stream)
        {
            parser p(stream);
            core::value v;
            p >> v;
            return v;
        }

        inline std::string to_bencode(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer w(stream);
            w << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_BENCODE_H
