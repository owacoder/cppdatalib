#ifndef CPPDATALIB_BENCODE_H
#define CPPDATALIB_BENCODE_H

#include "../core/value_builder.h"

namespace cppdatalib
{
    namespace bencode
    {
        inline std::istream &convert(std::istream &stream, core::stream_handler &writer)
        {
            char buffer[core::buffer_size];
            bool written = false;
            int chr;

            writer.begin();

            while (chr = stream.peek(), chr != EOF)
            {
                written = true;

                switch (chr)
                {
                    case 'i':
                    {
                        stream.get(); // Eat 'i'

                        core::int_t i;
                        stream >> i;
                        if (!stream) throw core::error("Bencode - expected 'integer' value");

                        writer.write(i);
                        if (stream.get() != 'e') throw core::error("Bencode - invalid 'integer' value");
                        break;
                    }
                    case 'e':
                        stream.get(); // Eat 'e'
                        switch (writer.current_container())
                        {
                            case core::array: writer.end_array(core::array_t()); break;
                            case core::object: writer.end_object(core::object_t()); break;
                            default: throw core::error("Bencode - attempt to end element does not exist"); break;
                        }
                        break;
                    case 'l':
                        stream.get(); // Eat 'l'
                        writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                        break;
                    case 'd':
                        stream.get(); // Eat 'd'
                        writer.begin_object(core::object_t(), core::stream_handler::unknown_size);
                        break;
                    default:
                        if (isdigit(chr))
                        {
                            core::int_t size;

                            stream >> size;
                            if (size < 0) throw core::error("Bencode - expected string size");
                            if (stream.get() != ':') throw core::error("Bencode - expected ':' separating string size and data");

                            writer.begin_string(core::string_t(), size);
                            while (size > 0)
                            {
                                core::int_t buffer_size = std::min(core::int_t(core::buffer_size), size);
                                stream.read(buffer, buffer_size);
                                if (!stream)
                                    throw core::error("Bencode - unexpected end of string");
                                writer.append_to_string(core::string_t(buffer, buffer_size));
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

            writer.end();
            return stream;
        }

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("Bencode - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("Bencode - 'null' value not allowed in output");}
            void bool_(const core::value &) {throw core::error("Bencode - 'boolean' value not allowed in output");}
            void integer_(const core::value &v) {output_stream << 'i' << v.get_int() << 'e';}
            void real_(const core::value &) {throw core::error("Bencode - 'real' value not allowed in output");}
            void begin_string_(const core::value &, core::int_t size, bool)
            {
                if (size == unknown_size)
                    throw core::error("Bencode - 'string' value does not have size specified");
                output_stream << size << ':';
            }
            void string_data_(const core::value &v) {output_stream << v.get_string();}

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << 'l';}
            void end_array_(const core::value &, bool) {output_stream << 'e';}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << 'd';}
            void end_object_(const core::value &, bool) {output_stream << 'e';}
        };

        inline std::istream &operator>>(std::istream &stream, core::value &v)
        {
            core::value_builder builder(v);
            convert(stream, builder);
            return stream;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::istream &input(std::istream &stream, core::value &v) {return stream >> v;}
        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline core::value from_bencode(const std::string &bencode)
        {
            std::istringstream stream(bencode);
            core::value v;
            stream >> v;
            return v;
        }

        inline std::string to_bencode(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_BENCODE_H
