#ifndef CPPDATALIB_CSV_H
#define CPPDATALIB_CSV_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace csv
    {
        struct options {virtual ~options() {}};
        struct convert_all_fields_as_strings : options {};
        struct convert_fields_by_deduction : options {};

        inline void deduce_type(const std::string &buffer, core::stream_handler &writer)
        {
            if (buffer.empty() ||
                    buffer == "~" ||
                    buffer == "null" ||
                    buffer == "Null" ||
                    buffer == "NULL")
                writer.write(core::null_t());
            else if (buffer == "Y" ||
                     buffer == "y" ||
                     buffer == "yes" ||
                     buffer == "Yes" ||
                     buffer == "YES" ||
                     buffer == "on" ||
                     buffer == "On" ||
                     buffer == "ON" ||
                     buffer == "true" ||
                     buffer == "True" ||
                     buffer == "TRUE")
                writer.write(true);
            else if (buffer == "N" ||
                     buffer == "n" ||
                     buffer == "no" ||
                     buffer == "No" ||
                     buffer == "NO" ||
                     buffer == "off" ||
                     buffer == "Off" ||
                     buffer == "OFF" ||
                     buffer == "false" ||
                     buffer == "False" ||
                     buffer == "FALSE")
                writer.write(false);
            else
            {
                // Attempt to read as an integer
                {
                    std::istringstream temp_stream(buffer);
                    core::int_t value;
                    temp_stream >> value;
                    if (!temp_stream.fail() && temp_stream.get() == EOF)
                    {
                        writer.write(value);
                        return;
                    }
                }

                // Attempt to read as an unsigned integer
                {
                    std::istringstream temp_stream(buffer);
                    core::uint_t value;
                    temp_stream >> value;
                    if (!temp_stream.fail() && temp_stream.get() == EOF)
                    {
                        writer.write(value);
                        return;
                    }
                }

                // Attempt to read as a real
                {
                    std::istringstream temp_stream(buffer);
                    core::real_t value;
                    temp_stream >> value;
                    if (!temp_stream.fail() && temp_stream.get() == EOF)
                    {
                        writer.write(value);
                        return;
                    }
                }

                // Revert to string
                writer.write(buffer);
            }
        }

        inline std::istream &read_string(std::istream &stream, core::stream_handler &writer, bool parse_as_strings)
        {
            int chr;
            std::string buffer;

            if (parse_as_strings)
            {
                writer.begin_string(core::string_t(), core::stream_handler::unknown_size);

                // buffer is used to temporarily store whitespace for reading strings
                while (chr = stream.get(), chr != EOF && chr != ',' && chr != '\n')
                {
                    if (isspace(chr))
                    {
                        buffer.push_back(chr);
                        continue;
                    }
                    else if (buffer.size())
                    {
                        writer.append_to_string(buffer);
                        buffer.clear();
                    }

                    writer.append_to_string(core::string_t(1, chr));
                }

                if (chr != EOF)
                    stream.unget();

                writer.end_string(core::string_t());
            }
            else // Unfortunately, one cannot deduce the type of the incoming data without first loading the field into a buffer
            {
                while (chr = stream.get(), chr != EOF && chr != ',' && chr != '\n')
                    buffer.push_back(chr);

                if (chr != EOF)
                    stream.unget();

                while (!buffer.empty() && isspace(buffer.back() & 0xff))
                    buffer.pop_back();

                deduce_type(buffer, writer);
            }

            return stream;
        }

        // Expects that the leading quote has already been parsed out of the stream
        inline std::istream &read_quoted_string(std::istream &stream, core::stream_handler &writer, bool parse_as_strings)
        {
            int chr;
            std::string buffer;

            if (parse_as_strings)
            {
                writer.begin_string(core::string_t(), core::stream_handler::unknown_size);

                // buffer is used to temporarily store whitespace for reading strings
                while (chr = stream.get(), chr != EOF)
                {
                    if (chr == '"')
                    {
                        if (stream.peek() == '"')
                            chr = stream.get();
                        else
                            break;
                    }

                    if (isspace(chr))
                    {
                        buffer.push_back(chr);
                        continue;
                    }
                    else if (buffer.size())
                    {
                        writer.append_to_string(buffer);
                        buffer.clear();
                    }

                    writer.append_to_string(core::string_t(1, chr));
                }

                writer.end_string(core::string_t());
            }
            else // Unfortunately, one cannot deduce the type of the incoming data without first loading the field into a buffer
            {
                while (chr = stream.get(), chr != EOF)
                {
                    if (chr == '"')
                    {
                        if (stream.peek() == '"')
                            chr = stream.get();
                        else
                            break;
                    }

                    buffer.push_back(chr);
                }

                while (!buffer.empty() && isspace(buffer.back() & 0xff))
                    buffer.pop_back();

                deduce_type(buffer, writer);
            }

            return stream;
        }

        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                int c = str[i] & 0xff;

                if (c == '"')
                    stream.put('"');

                stream.put(str[i]);
            }

            return stream;
        }

        inline std::istream &convert(std::istream &stream, core::stream_handler &writer, const options &opts = convert_fields_by_deduction())
        {
            const bool parse_as_strings = dynamic_cast<const convert_all_fields_as_strings *>(&opts) != NULL;
            int chr;
            bool comma_just_parsed = true;
            bool newline_just_parsed = true;

            writer.begin();
            writer.begin_array(core::array_t(), core::stream_handler::unknown_size);

            while (chr = stream.get(), chr != EOF)
            {
                if (newline_just_parsed)
                {
                    writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                    newline_just_parsed = false;
                }

                switch (chr)
                {
                    case '"':
                        read_quoted_string(stream, writer, parse_as_strings);
                        comma_just_parsed = false;
                        break;
                    case ',':
                        if (comma_just_parsed)
                        {
                            if (parse_as_strings)
                                writer.write(core::string_t());
                            else // parse by deduction of types, assume `,,` means null instead of empty string
                                writer.write(core::null_t());
                        }
                        comma_just_parsed = true;
                        break;
                    case '\n':
                        if (comma_just_parsed)
                        {
                            if (parse_as_strings)
                                writer.write(core::string_t());
                            else // parse by deduction of types, assume `,,` means null instead of empty string
                                writer.write(core::null_t());
                        }
                        comma_just_parsed = newline_just_parsed = true;
                        writer.end_array(core::array_t());
                        break;
                    default:
                        if (!isspace(chr))
                        {
                            stream.unget();
                            read_string(stream, writer, parse_as_strings);
                            comma_just_parsed = false;
                        }
                        break;
                }
            }

            if (!newline_just_parsed)
            {
                if (comma_just_parsed)
                {
                    if (parse_as_strings)
                        writer.write(core::string_t());
                    else // parse by deduction of types, assume `,,` means null instead of empty string
                        writer.write(core::null_t());
                }

                writer.end_array(core::array_t());
            }


            writer.end_array(core::array_t());
            writer.end();
            return stream;
        }

        class row_writer : public core::stream_handler, public core::stream_writer
        {
            char separator;

        public:
            row_writer(std::ostream &output, char separator = ',') : core::stream_writer(output), separator(separator) {}

        protected:
            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                    output_stream.put(separator);
            }

            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void uinteger_(const core::value &v) {output_stream << v.get_uint();}
            void real_(const core::value &v) {output_stream << std::setprecision(CPPDATALIB_REAL_DIG) << v.get_real();}
            void begin_string_(const core::value &, core::int_t, bool) {output_stream.put('"');}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream.put('"');}

            void begin_array_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'array' value not allowed in row output");}
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'object' value not allowed in output");}
        };

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
            char separator;

        public:
            stream_writer(std::ostream &output, char separator = ',') : core::stream_writer(output), separator(separator) {}

        protected:
            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                {
                    if (nesting_depth() == 1)
                        output_stream << "\r\n";
                    else
                        output_stream.put(separator);
                }
            }

            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void uinteger_(const core::value &v) {output_stream << v.get_uint();}
            void real_(const core::value &v) {output_stream << std::setprecision(CPPDATALIB_REAL_DIG) << v.get_real();}
            void begin_string_(const core::value &, core::int_t, bool) {output_stream.put('"');}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream.put('"');}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() == 2)
                    throw core::error("CSV - 'array' value not allowed in row output");
            }
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("CSV - 'object' value not allowed in output");}
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

        inline std::istream &input_table(std::istream &stream, core::value &v, const options &opts = convert_fields_by_deduction())
        {
            core::value_builder builder(v);
            convert(stream, builder, opts);
            return stream;
        }

        inline std::ostream &print_table(std::ostream &stream, const core::value &v, char separator = ',')
        {
            stream_writer writer(stream, separator);
            core::convert(v, writer);
            return stream;
        }
        inline std::ostream &print_row(std::ostream &stream, const core::value &v, char separator = ',')
        {
            row_writer writer(stream, separator);
            core::convert(v, writer);
            return stream;
        }

        inline core::value from_csv_table(const std::string &csv, const options &opts = convert_fields_by_deduction())
        {
            std::istringstream stream(csv);
            core::value v;
            core::value_builder builder(v);
            convert(stream, builder, opts);
            return v;
        }

        inline std::string to_csv_row(const core::value &v, char separator = ',')
        {
            std::ostringstream stream;
            print_row(stream, v, separator);
            return stream.str();
        }

        inline std::string to_csv_table(const core::value &v, char separator = ',')
        {
            std::ostringstream stream;
            print_table(stream, v, separator);
            return stream.str();
        }

        inline std::istream &input(std::istream &stream, core::value &v, const options &opts = convert_fields_by_deduction()) {return input_table(stream, v, opts);}
        inline core::value from_csv(const std::string &csv, const options &opts = convert_fields_by_deduction()) {return from_csv_table(csv, opts);}
        inline std::ostream &print(std::ostream &stream, const core::value &v, char separator = ',') {return print_table(stream, v, separator);}
        inline std::string to_csv(const core::value &v, char separator = ',') {return to_csv_table(v, separator);}
    }
}

#endif // CPPDATALIB_CSV_H
