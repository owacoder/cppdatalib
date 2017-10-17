#ifndef CPPDATALIB_STREAM_FILTERS_H
#define CPPDATALIB_STREAM_FILTERS_H

#include "stream_base.h"

namespace cppdatalib
{
    namespace core
    {
        template<core::type from, core::type to>
        class stream_filter : public core::stream_handler
        {
            core::stream_handler &output;
            core::value str;

        public:
            stream_filter(core::stream_handler &output) : output(output) {}

        protected:
            void begin_() {output.begin();}
            void end_() {output.end();}

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                if (v.get_type() == from && from != to)
                {
                    value copy(v);
                    copy.convert_to_type(to);
                    return output.write(copy);
                }
                return output.write(v);
            }

            void begin_array_(const value &v, int_t size, bool) {output.begin_array(v, size);}
            void end_array_(const value &v, bool) {output.end_array(v);}

            void begin_object_(const value &v, int_t size, bool) {output.begin_object(v, size);}
            void end_object_(const value &v, bool) {output.end_object(v);}

            void begin_string_(const value &v, int_t size, bool)
            {
                if (from == core::string && from != to)
                    str.set_string("");
                else
                    output.begin_string(v, size);
            }
            void string_data_(const value &v)
            {
                if (from == core::string && from != to)
                    str.get_string() += v.get_string();
                else
                    output.append_to_string(v);
            }
            void end_string_(const value &v, bool)
            {
                if (from == core::string && from != to)
                {
                    str.convert_to_type(to);
                    output.write(str);
                }
                else
                    output.end_string(v);
            }
        };
    }
}

#endif // CPPDATALIB_STREAM_FILTERS_H
