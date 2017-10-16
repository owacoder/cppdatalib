#ifndef CPPDATALIB_NETSTRINGS_H
#define CPPDATALIB_NETSTRINGS_H

#include "../core/value_builder.h"

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
