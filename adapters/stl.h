#ifndef CPPDATALIB_STL_H
#define CPPDATALIB_STL_H

#include <string>
#include <locale>

#include <vector>
#include <list>
#include <queue>
#include <valarray>
#include <bitset>

#include <set>
#include <unordered_set>

#include <map>
#include <unordered_map>

#include <stack>
#include <queue>

#ifdef CPPDATALIB_CPP11
#include <array>
#include <codecvt>
#include <forward_list>
#include <tuple>
#include <ratio>

#include <chrono>

#include <complex>

#include <atomic>
#endif

#ifdef CPPDATALIB_CPP17
#include <variant>
#include <optional>
#include <any>
#endif

#include "../core/value_parser.h"

#ifdef CPPDATALIB_CPP11

template<typename... Ts>
class cast_template_to_cppdatalib<std::basic_string, char, Ts...>
{
    const std::basic_string<char, Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::basic_string<char, Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(cppdatalib::core::string_t(bind.c_str(), bind.size()),
                                                                             cppdatalib::core::clob);}
    void convert(cppdatalib::core::value &dest) const {dest.set_string(cppdatalib::core::string_t(bind.c_str(), bind.size()),
                                                                                      cppdatalib::core::clob);}
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::basic_string, char, Ts...> : public generic_stream_input
    {
    protected:
        const std::basic_string<char, Ts...> &bind;

    public:
        template_parser(const std::basic_string<char, Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {}

        void write_one_()
        {
            get_output()->write(core::value(bind, core::clob));
        }
    };
}}

// This overload handles both 16- and 32-bit wide strings
// It is assumed that 16-bit strings are UTF-16 encoded
template<typename CharType, typename... Ts>
class cast_template_to_cppdatalib<std::basic_string, CharType, Ts...>
{
    const std::basic_string<CharType, Ts...> &bind;

    static_assert(sizeof(CharType) == sizeof(uint32_t) || sizeof(CharType) == sizeof(uint16_t), "Size of character type must be 16 or 32 bits");

public:
    cast_template_to_cppdatalib(const std::basic_string<CharType, Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (sizeof(CharType) == sizeof(uint32_t)) // 32 bit?
        {
            auto &ref = dest.get_owned_string_ref();
            ref.clear();
            for (CharType c: bind)
            {
                std::string temp = cppdatalib::core::ucs_to_utf8(c);
                if (temp.empty()) // Conversion error
                    throw cppdatalib::core::error("Invalid UTF-32");
                ref += temp;
            }
            dest.set_subtype(cppdatalib::core::normal);
        }
        else if (sizeof(CharType) == sizeof(uint16_t)) // 16 bit?
        {
            // TODO: remove necessity of splitting UTF-16 characters into individual bytes?
            std::string split;
            for (CharType c: bind)
            {
                split.push_back((uint32_t) c >> 8);
                split.push_back((uint32_t) c & 0xff);
            }

            auto &ref = dest.get_owned_string_ref();
            ref.clear();
            for (size_t i = 0; i < split.size(); )
            {
                uint32_t codepoint = cppdatalib::core::utf_to_ucs(split, cppdatalib::core::utf16_big_endian, i, i);
                std::string temp = cppdatalib::core::ucs_to_utf8(codepoint);
                if (temp.empty())
                    throw cppdatalib::core::error("Invalid UTF-16");
                ref += temp;
            }

            dest.set_subtype(cppdatalib::core::normal);
        }
    }
};

template<typename T, typename... Ts>
class cast_template_to_cppdatalib<std::unique_ptr, T, Ts...>
{
    const std::unique_ptr<T, Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::unique_ptr<T, Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (!bind)
            dest.set_null(cppdatalib::core::normal);
        else
            dest = *bind;
    }
};

namespace cppdatalib { namespace core {
    template<typename T, typename... Ts>
    class template_parser<std::unique_ptr, T, Ts...> : public generic_stream_input
    {
    protected:
        const std::unique_ptr<T, Ts...> &bind;

    public:
        template_parser(const std::unique_ptr<T, Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {}

        void write_one_()
        {
            if (was_just_reset())
            {
                if (bind)
                    compose_parser(*bind);
                else
                    get_output()->write(core::value());
            }
        }
    };
}}

template<typename T>
class cast_template_to_cppdatalib<std::shared_ptr, T>
{
    const std::shared_ptr<T> &bind;
public:
    cast_template_to_cppdatalib(const std::shared_ptr<T> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (!bind)
            dest.set_null(cppdatalib::core::normal);
        else
            dest = *bind;
    }
};

namespace cppdatalib { namespace core {
    template<typename T>
    class template_parser<std::shared_ptr, T> : public generic_stream_input
    {
    protected:
        const std::shared_ptr<T> &bind;

    public:
        template_parser(const std::shared_ptr<T> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {}

        void write_one_()
        {
            if (was_just_reset())
            {
                if (bind)
                    compose_parser(*bind);
                else
                    get_output()->write(core::value());
            }
        }
    };
}}

#ifndef CPPDATALIB_DISABLE_WEAK_POINTER_CONVERSIONS
template<typename T>
class cast_template_to_cppdatalib<std::weak_ptr, T>
{
    const std::weak_ptr<T> &bind;
public:
    cast_template_to_cppdatalib(const std::weak_ptr<T> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (!bind)
            dest.set_null(cppdatalib::core::normal);
        else
            dest = *bind;
    }
};

namespace cppdatalib { namespace core {
    template<typename T>
    class template_parser<std::weak_ptr, T> : public generic_stream_input
    {
    protected:
        const std::weak_ptr<T> &bind;

    public:
        template_parser(const std::weak_ptr<T> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {}

        void write_one_()
        {
            if (was_just_reset())
            {
                if (bind)
                    compose_parser(*bind);
                else
                    get_output()->write(core::value());
            }
        }
    };
}}
#endif

template<typename T, size_t N>
class cast_array_template_to_cppdatalib<std::array, T, N>
{
    const std::array<T, N> &bind;
public:
    cast_array_template_to_cppdatalib(const std::array<T, N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<size_t N>
class cast_sized_template_to_cppdatalib<std::bitset, N>
{
    const std::bitset<N> &bind;
public:
    cast_sized_template_to_cppdatalib(const std::bitset<N> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (size_t i = 0; i < bind.size(); ++i)
            dest.push_back(cppdatalib::core::value(bind[i]));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::list, Ts...>
{
    const std::list<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::list<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::forward_list, Ts...>
{
    const std::forward_list<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::forward_list<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::deque, Ts...>
{
    const std::deque<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::deque<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::vector, Ts...>
{
    const std::vector<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::vector<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::set, Ts...>
{
    const std::set<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::set<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::unordered_set, Ts...>
{
    const std::unordered_set<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::unordered_set<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::multiset, Ts...>
{
    const std::multiset<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::multiset<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::unordered_multiset, Ts...>
{
    const std::unordered_multiset<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::unordered_multiset<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::valarray, Ts...>
{
    const std::valarray<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::valarray<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (size_t i = 0; i < bind.size(); ++i)
            dest.push_back(cppdatalib::core::value(bind[i]));
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::valarray, Ts...> : public generic_stream_input
    {
    protected:
        const std::valarray<Ts...> &bind;
        size_t idx;

    public:
        template_parser(const std::valarray<Ts...> &bind, generic_parser &parser)
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
                compose_parser(bind[idx++]);
            else
                get_output()->end_array(core::array_t());
        }
    };
}}

template<typename... Ts>
class cast_template_to_cppdatalib<std::initializer_list, Ts...>
{
    cppdatalib::core::array_t bind;
public:
    cast_template_to_cppdatalib(std::initializer_list<Ts...> bind) : bind(bind) {}
    operator cppdatalib::core::value() {return cppdatalib::core::value(std::move(bind));}
    void convert(cppdatalib::core::value &dest) {dest = cppdatalib::core::value(std::move(bind));}
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::stack, Ts...>
{
    std::stack<Ts...> bind;
public:
    cast_template_to_cppdatalib(std::stack<Ts...> bind) : bind(bind) {}
    operator cppdatalib::core::value()
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest)
    {
        dest.set_array({}, cppdatalib::core::normal);
        using namespace std;
        while (!bind.empty())
        {
            dest.push_back(cppdatalib::core::value(bind.top()));
            bind.pop();
        }
        reverse(dest.get_array_ref().begin().data(), dest.get_array_ref().end().data());
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::queue, Ts...>
{
    std::queue<Ts...> bind;
public:
    cast_template_to_cppdatalib(std::queue<Ts...> bind) : bind(bind) {}
    operator cppdatalib::core::value()
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest)
    {
        dest.set_array({}, cppdatalib::core::normal);
        while (!bind.empty())
        {
            dest.push_back(cppdatalib::core::value(bind.front()));
            bind.pop();
        }
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::priority_queue, Ts...>
{
    std::priority_queue<Ts...> bind;
public:
    cast_template_to_cppdatalib(std::priority_queue<Ts...> bind) : bind(bind) {}
    operator cppdatalib::core::value()
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest)
    {
        dest.set_array({}, cppdatalib::core::normal);
        while (!bind.empty())
        {
            dest.push_back(cppdatalib::core::value(bind.front()));
            bind.pop();
        }
    }
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::map, Ts...>
{
    const std::map<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::map<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.add_member_at_end(cppdatalib::core::value(item.first),
                                   cppdatalib::core::value(item.second));
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::map, Ts...> : public generic_stream_input
    {
    protected:
        const std::map<Ts...> &bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const std::map<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind.begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::object_t(), bind.size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

template<typename... Ts>
class cast_template_to_cppdatalib<std::multimap, Ts...>
{
    const std::multimap<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::multimap<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.add_member_at_end(cppdatalib::core::value(item.first),
                                   cppdatalib::core::value(item.second));
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::multimap, Ts...> : public generic_stream_input
    {
    protected:
        const std::multimap<Ts...> &bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const std::multimap<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind->begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::object_t(), bind->size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

template<typename... Ts>
class cast_template_to_cppdatalib<std::unordered_map, Ts...>
{
    const std::unordered_map<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::unordered_map<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        dest.set_subtype(cppdatalib::core::hash);
        for (const auto &item: bind)
            dest.add_member(cppdatalib::core::value(item.first),
                            cppdatalib::core::value(item.second));
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::unordered_map, Ts...> : public generic_stream_input
    {
    protected:
        const std::unordered_map<Ts...> &bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const std::unordered_map<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind.begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::value(core::object_t(), core::hash), bind->size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

template<typename... Ts>
class cast_template_to_cppdatalib<std::unordered_multimap, Ts...>
{
    const std::unordered_multimap<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::unordered_multimap<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        dest.set_subtype(cppdatalib::core::hash);
        for (const auto &item: bind)
            dest.add_member(cppdatalib::core::value(item.first),
                            cppdatalib::core::value(item.second));
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::unordered_multimap, Ts...> : public generic_stream_input
    {
    protected:
        const std::unordered_multimap<Ts...> &bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const std::unordered_multimap<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind.begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::value(core::object_t(), core::hash), bind.size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

template<typename... Ts>
class cast_template_to_cppdatalib<std::pair, Ts...>
{
    const std::pair<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::pair<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::array_t{cppdatalib::core::value(bind.first), cppdatalib::core::value(bind.second)};}
    void convert(cppdatalib::core::value &dest) const {dest.set_array({cppdatalib::core::value(bind.first), cppdatalib::core::value(bind.second)}, cppdatalib::core::normal);}
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::pair, Ts...> : public generic_stream_input
    {
    protected:
        const std::pair<Ts...> &bind;
        size_t idx;

    public:
        template_parser(const std::pair<Ts...> &bind, generic_parser &parser)
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
            {
                get_output()->begin_array(core::array_t(), 2);
                compose_parser(bind.first);
            }
            else if (++idx == 1)
                compose_parser(bind.second);
            else
                get_output()->end_array(core::array_t());
        }
    };
}}

namespace cppdatalib
{
    namespace core
    {
        namespace impl
        {
            template<size_t tuple_size>
            struct push_back : public push_back<tuple_size - 1>
            {
                template<typename... Ts>
                push_back(const std::tuple<Ts...> &tuple, core::array_t &result) : push_back<tuple_size - 1>(tuple, result) {
                    result.data().push_back(cppdatalib::core::value(std::get<tuple_size - 1>(tuple)));
                }

                template<typename T, typename... Ts>
                push_back(const std::tuple<Ts...> &tuple, std::vector<T> &result, core::stream_handler &output) : push_back<tuple_size - 1>(tuple, result, output) {
                    result.emplace_back(std::get<tuple_size - 1>(tuple), output);
                }

                template<typename T, typename... Ts>
                push_back(const std::tuple<Ts...> &tuple, std::vector<T> &result) : push_back<tuple_size - 1>(tuple, result) {
                    result.emplace_back(std::get<tuple_size - 1>(tuple));
                }
            };

            template<>
            struct push_back<0>
            {
                template<typename... Ts>
                push_back(const std::tuple<Ts...> &, core::array_t &) {}

                template<typename T, typename... Ts>
                push_back(const std::tuple<Ts...> &, std::vector<T> &, core::stream_handler &) {}

                template<typename T, typename... Ts>
                push_back(const std::tuple<Ts...> &, std::vector<T> &) {}
            };
        }
    }
}

template<typename... Ts>
class cast_template_to_cppdatalib<std::tuple, Ts...>
{
    const std::tuple<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::tuple<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        cppdatalib::core::impl::push_back<std::tuple_size<std::tuple<Ts...>>::value>(bind, dest.get_array_ref());
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::tuple, Ts...> : public generic_stream_input
    {
    protected:
        const std::tuple<Ts...> &bind;
        std::vector<generic_parser> parsers;
        decltype(parsers.begin()) iterator;

    public:
        template_parser(const std::tuple<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void output_changed_()
        {
            for (auto &parser: parsers)
                parser.set_output(*get_output());
        }

        void reset_()
        {
            parsers.clear();
            if (get_output() == NULL)
                cppdatalib::core::impl::push_back<std::tuple_size<std::tuple<Ts...>>::value>(bind, parsers);
            else
                cppdatalib::core::impl::push_back<std::tuple_size<std::tuple<Ts...>>::value>(bind, parsers, *get_output());
            iterator = parsers.begin();
        }

        void write_one_()
        {
            if (was_just_reset())
                get_output()->begin_array(core::array_t(), parsers.size());
            else if (iterator != parsers.end() && (iterator->was_just_reset() || iterator->busy() || ++iterator != parsers.end()))
                iterator->write_one();
            else
                get_output()->end_array(core::array_t());
        }
    };
}}

#ifdef CPPDATALIB_CPP17
template<typename... Ts>
class cast_template_to_cppdatalib<std::optional, Ts...>
{
    const std::optional<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::optional<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (bind)
            dest = *bind;
        else
            dest.set_null(cppdatalib::core::normal);
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::optional, Ts...> : public generic_stream_input
    {
    protected:
        const std::optional<Ts...> &bind;

    public:
        template_parser(const std::optional<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {}

        void write_one_()
        {
            if (was_just_reset())
            {
                if (bind)
                    compose_parser(*bind);
                else
                    get_output()->write(core::null_t());
            }
        }
    };
}}

template<typename... Ts>
class cast_template_to_cppdatalib<std::variant, Ts...>
{
    const std::variant<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::variant<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (bind.valueless_by_exception())
            dest.set_null(cppdatalib::core::normal);
        else
            dest = cppdatalib::core::value(std::get<bind.index()>(bind));
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<std::variant, Ts...> : public generic_stream_input
    {
    protected:
        const std::variant<Ts...> &bind;

    public:
        template_parser(const std::variant<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {}

        void write_one_()
        {
            if (was_just_reset())
            {
                if (bind.valueless_by_exception())
                    get_output()->write(core::null_t());
                else
                    compose_parser(std::get<bind.index()>(bind));
            }
        }
    };
}}
#endif

template<typename... Ts>
class cast_template_from_cppdatalib<std::basic_string, char, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::basic_string<char, Ts...>() const
    {
        std::basic_string<char, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::basic_string<char, Ts...> &dest) const
    {
        cppdatalib::core::string_t str = bind.as_string();
        dest.assign(str.c_str(), str.size());
    }
};

// This overload handles both 16- and 32-bit wide strings
// It is assumed that 16-bit strings are UTF-16 encoded
template<typename CharType, typename... Ts>
class cast_template_from_cppdatalib<std::basic_string, CharType, Ts...>
{
    const cppdatalib::core::value &bind;

    static_assert(sizeof(CharType) == sizeof(uint32_t) || sizeof(CharType) == sizeof(uint16_t), "Size of character type must be 16 or 32 bits");

public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::basic_string<CharType, Ts...>() const
    {
        std::basic_string<CharType, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::basic_string<CharType, Ts...> &dest) const
    {
        std::string s = bind.as_string();

        if (sizeof(CharType) == sizeof(uint32_t)) // 32 bit?
        {
            dest.clear();
            for (size_t i = 0; i < s.size(); )
            {
                uint32_t codepoint = cppdatalib::core::utf8_to_ucs(s, i, i);
                if (codepoint == std::numeric_limits<uint32_t>::max())
                    throw cppdatalib::core::error("Invalid UTF-8");
                dest.push_back(codepoint);
            }
        }
        else if (sizeof(CharType) == sizeof(uint16_t)) // 16 bit?
        {
            dest.clear();
            for (size_t i = 0; i < s.size(); )
            {
                uint32_t codepoint = cppdatalib::core::utf8_to_ucs(s, i, i);
                if (codepoint == std::numeric_limits<uint32_t>::max())
                    throw cppdatalib::core::error("Invalid UTF-8");

                // TODO: remove dependence on creating a secondary byte-granular string?
                std::string temp = cppdatalib::core::ucs_to_utf(codepoint, cppdatalib::core::utf16_big_endian);
                for (size_t j = 0; j < temp.size(); j += 2)
                    dest.push_back(((temp[j] & 0xff) << 8) | uint8_t(temp[j+1]));
            }
        }
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::unique_ptr, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::unique_ptr<T, Ts...>() const
    {
        return std::unique_ptr<T, Ts...>(new T(bind.operator T()));
    }
    void convert(std::unique_ptr<T, Ts...> &dest) const
    {
        dest.reset(new T(bind.operator T()));
    }
};

template<typename T>
class cast_template_from_cppdatalib<std::shared_ptr, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::shared_ptr<T>() const
    {
        return std::shared_ptr<T>(new T(bind.operator T()));
    }
    void convert(std::shared_ptr<T> &dest) const
    {
        dest = std::shared_ptr<T>(new T(bind.operator T()));
    }
};

template<typename T, size_t N>
class cast_array_template_from_cppdatalib<std::array, T, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_array_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::array<T, N>() const
    {
        std::array<T, N> result{};
        convert(result);
        return result;
    }
    void convert(std::array<T, N> &dest) const
    {
        if (bind.is_array())
        {
            size_t i = 0;
            auto it = bind.get_array_unchecked().begin();
            for (; i < std::min(bind.array_size(), N); ++i)
                dest[i] = (*it++).operator T();
            for (; i < N; ++i)
                dest[i] = {};
        }
        else
            std::fill(dest.begin(), dest.end(), T{});
    }
};

template<size_t N>
class cast_sized_template_from_cppdatalib<std::bitset, N>
{
    const cppdatalib::core::value &bind;
public:
    cast_sized_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::bitset<N>() const
    {
        std::bitset<N> result{};
        convert(result);
        return result;
    }
    void convert(std::bitset<N> &dest) const
    {
        if (bind.is_array())
        {
            size_t i = 0;
            auto it = bind.get_array_unchecked().begin();
            for (; i < std::min(bind.array_size(), N); ++i)
                dest[i] = *it++;
            for (; i < N; ++i)
                dest[i] = {};
        }
        else
            std::fill(dest.begin(), dest.end(), {});
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::stack, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::stack<T, Ts...>() const
    {
        std::stack<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::stack<T, Ts...> &dest) const
    {
        dest = {};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push(item.operator T());
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::queue, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::queue<T, Ts...>() const
    {
        std::queue<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::queue<T, Ts...> &dest) const
    {
        dest = {};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push(item.operator T());
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::priority_queue, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::priority_queue<T, Ts...>() const
    {
        std::priority_queue<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::priority_queue<T, Ts...> &dest) const
    {
        dest = {};
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push(item.operator T());
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::list, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::list<T, Ts...>() const
    {
        std::list<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::list<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::forward_list, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::forward_list<T, Ts...>() const
    {
        std::forward_list<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::forward_list<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_front(item.operator T());
        dest.reverse();
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::deque, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::deque<T, Ts...>() const
    {
        std::deque<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::deque<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::vector, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::vector<T, Ts...>() const
    {
        std::vector<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::vector<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

template<typename T>
class cast_template_from_cppdatalib<std::valarray, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::valarray<T>() const
    {
        std::valarray<T> result;
        convert(result);
        return result;
    }
    void convert(std::valarray<T> &dest) const
    {
        size_t index = 0;
        dest.resize(bind.array_size());
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest[index++] = item.operator T();
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::set, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::set<T, Ts...>() const
    {
        std::set<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::set<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.insert(item.operator T());
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::multiset, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::multiset<T, Ts...>() const
    {
        std::multiset<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::multiset<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.insert(item.operator T());
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::unordered_set, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::unordered_set<T, Ts...>() const
    {
        std::unordered_set<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::unordered_set<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.insert(item.operator T());
    }
};

template<typename T, typename... Ts>
class cast_template_from_cppdatalib<std::unordered_multiset, T, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::unordered_multiset<T, Ts...>() const
    {
        std::unordered_multiset<T, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::unordered_multiset<T, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.insert(item.operator T());
    }
};

template<typename K, typename V, typename... Ts>
class cast_template_from_cppdatalib<std::map, K, V, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::map<K, V, Ts...>() const
    {
        std::map<K, V, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::map<K, V, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert({item.first.operator K(),
                             item.second.operator V()});
    }
};

template<typename K, typename V, typename... Ts>
class cast_template_from_cppdatalib<std::multimap, K, V, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::multimap<K, V, Ts...>() const
    {
        std::multimap<K, V, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::multimap<K, V, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert({item.first.operator K(),
                             item.second.operator V()});
    }
};

template<typename K, typename V, typename... Ts>
class cast_template_from_cppdatalib<std::unordered_map, K, V, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::unordered_map<K, V, Ts...>() const
    {
        std::unordered_map<K, V, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::unordered_map<K, V, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert({item.first.operator K(),
                             item.second.operator V()});
    }
};

template<typename K, typename V, typename... Ts>
class cast_template_from_cppdatalib<std::unordered_multimap, K, V, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::unordered_multimap<K, V, Ts...>() const
    {
        std::unordered_multimap<K, V, Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::unordered_multimap<K, V, Ts...> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert({item.first.operator K(),
                             item.second.operator V()});
    }
};

template<typename T1, typename T2>
class cast_template_from_cppdatalib<std::pair, T1, T2>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::pair<T1, T2>() const {return {bind.element(0).operator T1(), bind.element(1).operator T2()};}
    void convert(std::pair<T1, T2> &dest) const {dest = {bind.element(0).operator T1(), bind.element(1).operator T2()};}
};

namespace cppdatalib
{
    namespace core
    {
        namespace impl
        {
            template<size_t tuple_size>
            struct tuple_push_back : public tuple_push_back<tuple_size - 1>
            {
                template<typename... Ts>
                tuple_push_back(const core::array_t &list, std::tuple<Ts...> &result) : tuple_push_back<tuple_size - 1>(list, result) {
                    typedef typename std::tuple_element<tuple_size-1, std::tuple<Ts...>>::type element_type;
                    if (tuple_size <= list.size()) // If the list contains enough elements, assign it
                        std::get<tuple_size-1>(result) = list[tuple_size - 1].operator element_type();
                    else // Otherwise, clear the tuple's element value
                        std::get<tuple_size-1>(result) = element_type{};
                }
            };

            template<>
            struct tuple_push_back<0>
            {
                template<typename... Ts>
                tuple_push_back(const core::array_t &, std::tuple<Ts...> &) {}
            };
        }
    }
}

template<typename... Ts>
class cast_template_from_cppdatalib<std::tuple, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::tuple<Ts...>() const
    {
        std::tuple<Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::tuple<Ts...> &dest) const
    {
        if (bind.is_array())
            cppdatalib::core::impl::tuple_push_back<std::tuple_size<std::tuple<Ts...>>::value>(bind.get_array_unchecked(), dest);
        else
            dest = {};
    }
};

// ATOMIC

template<typename T>
class cast_template_to_cppdatalib<std::atomic, T>
{
    const std::atomic<T> &bind;

public:
    cast_template_to_cppdatalib(const std::atomic<T> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest = cppdatalib::core::value(bind.load());
    }
};

template<typename T>
class cast_template_from_cppdatalib<std::atomic, T>
{
    const cppdatalib::core::value &bind;

public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    void convert(std::atomic<T> &dest) const
    {
        dest.store(bind.operator T());
    }
};

// COMPLEX

template<typename T>
class cast_template_to_cppdatalib<std::complex, T>
{
    const std::complex<T> &bind;

public:
    cast_template_to_cppdatalib(const std::complex<T> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        return cppdatalib::core::value(cppdatalib::core::array_t{cppdatalib::core::value(bind.real()),
                                                                 cppdatalib::core::value(bind.imag())});
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest = cppdatalib::core::array_t{cppdatalib::core::value(bind.real()), cppdatalib::core::value(bind.imag())};
    }
};

template<typename T>
class cast_template_from_cppdatalib<std::complex, T>
{
    const cppdatalib::core::value &bind;

public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::complex<T>() const
    {
        std::complex<T> result;
        convert(result);
        return result;
    }
    void convert(std::complex<T> &dest) const
    {
        if (bind.is_array())
            dest = {bind.element(0).operator T(), bind.element(1).operator T()};
        else
            dest = {};
    }
};

// CHRONO

template<typename Clock, typename Duration>
class cast_template_to_cppdatalib<std::chrono::time_point, Clock, Duration>
{
    const std::chrono::time_point<Clock, Duration> &bind;

public:
    cast_template_to_cppdatalib(const std::chrono::time_point<Clock, Duration> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        typedef typename Duration::period Period;

        if (Period::num < Period::den) // Sub-second precision
        {
            if (Period::den <= 1000)
                dest = cppdatalib::core::value(std::chrono::duration_cast<std::chrono::milliseconds>(bind.time_since_epoch()), cppdatalib::core::unix_timestamp_ms);
            else
                dest = cppdatalib::core::value(std::chrono::duration_cast<std::chrono::nanoseconds>(bind.time_since_epoch()), cppdatalib::core::unix_timestamp_ns);
        }
        else // Second or super-second precision
            dest = cppdatalib::core::value(std::chrono::duration_cast<std::chrono::milliseconds>(bind.time_since_epoch()), cppdatalib::core::unix_timestamp);
    }
};

template<typename Rep, typename Period>
class cast_template_to_cppdatalib<std::chrono::duration, Rep, Period>
{
    const std::chrono::duration<Rep, Period> &bind;

public:
    cast_template_to_cppdatalib(const std::chrono::duration<Rep, Period> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        if (Period::num < Period::den) // Sub-second precision
        {
            if (Period::den <= 1000)
                dest = cppdatalib::core::value(bind.count() * (1000 / Period::den), cppdatalib::core::duration_ms);
            else if (Period::den <= 1000000000)
                dest = cppdatalib::core::value(bind.count() * (1000000000 / Period::den), cppdatalib::core::duration_ns);
            else // Period::den > 1000000000
                dest = cppdatalib::core::value(bind.count() / (Period::den / 1000000000ull), cppdatalib::core::duration_ns);
        }
        else // Second or super-second precision
            dest = cppdatalib::core::value(bind.count() * Period::num / Period::den, cppdatalib::core::duration);
    }
};

template<typename Clock, typename Duration>
class cast_template_from_cppdatalib<std::chrono::time_point, Clock, Duration>
{
    const cppdatalib::core::value &bind;

    template<typename T>
    std::chrono::time_point<Clock, Duration> from_int(T v, cppdatalib::core::subtype_t subtype) const
    {
        switch (subtype)
        {
            case cppdatalib::core::unix_timestamp:
            case cppdatalib::core::utc_timestamp:
            case cppdatalib::core::duration:
                return std::chrono::time_point<Clock, Duration>(std::chrono::duration_cast<Duration>(std::chrono::seconds(v)));
            case cppdatalib::core::unix_timestamp_ms:
            case cppdatalib::core::utc_timestamp_ms:
            case cppdatalib::core::duration_ms:
                return std::chrono::time_point<Clock, Duration>(std::chrono::duration_cast<Duration>(std::chrono::milliseconds(v)));
            case cppdatalib::core::unix_timestamp_ns:
            case cppdatalib::core::utc_timestamp_ns:
            case cppdatalib::core::duration_ns:
                return std::chrono::time_point<Clock, Duration>(std::chrono::duration_cast<Duration>(std::chrono::nanoseconds(v)));
            default: return std::chrono::time_point<Clock, Duration>(std::chrono::duration_cast<Duration>(std::chrono::seconds(v)));
        }
    }

public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::chrono::time_point<Clock, Duration>() const
    {
        std::chrono::time_point<Clock, Duration> result;
        convert(result);
        return result;
    }
    void convert(std::chrono::time_point<Clock, Duration> &dest) const
    {
        if (bind.is_int())
            dest = from_int(bind.get_int_unchecked(), bind.get_subtype());
        else if (bind.is_uint())
            dest = from_int(bind.get_uint_unchecked(), bind.get_subtype());
        else
            dest = {};
    }
};

template<typename Rep, typename Period>
class cast_template_from_cppdatalib<std::chrono::duration, Rep, Period>
{
    const cppdatalib::core::value &bind;

    template<typename T>
    std::chrono::duration<Rep, Period> from_int(T v, cppdatalib::core::subtype_t subtype) const
    {
        switch (subtype)
        {
            case cppdatalib::core::unix_timestamp:
            case cppdatalib::core::utc_timestamp:
            case cppdatalib::core::duration:
                return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::seconds(v));
            case cppdatalib::core::unix_timestamp_ms:
            case cppdatalib::core::utc_timestamp_ms:
            case cppdatalib::core::duration_ms:
                return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::milliseconds(v));
            case cppdatalib::core::unix_timestamp_ns:
            case cppdatalib::core::utc_timestamp_ns:
            case cppdatalib::core::duration_ns:
                return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::nanoseconds(v));
            default: return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::seconds(v));
        }
    }

public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::chrono::duration<Rep, Period>() const
    {
        std::chrono::duration<Rep, Period> result;
        convert(result);
        return result;
    }
    void convert(std::chrono::duration<Rep, Period> &dest) const
    {
        if (bind.is_int())
            dest = from_int(bind.get_int_unchecked(), bind.get_subtype());
        else if (bind.is_uint())
            dest = from_int(bind.get_uint_unchecked(), bind.get_subtype());
        else
            dest = {};
    }
};

#ifdef CPPDATALIB_CPP17
#include <filesystem>

template<typename... Ts>
class cast_template_from_cppdatalib<std::optional, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::optional<Ts...>() const
    {
        std::optional<Ts...> result;
        convert(result);
        return result;
    }
    void convert(std::optional<Ts...> &dest) const
    {
        typedef typename cppdatalib::core::impl::unpack_first<Ts...>::type contained_type;
        if (!bind.is_null())
            dest = bind.operator contained_type();
        else
            dest = {};
    }
};

template<>
class cast_from_cppdatalib<std::any>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::any() const
    {
        std::any result;
        convert(result);
        return result;
    }
    void convert(std::any &dest) const
    {
        switch (bind.get_type())
        {
            case core::null: dest = std::any(); break;
            case core::boolean: dest = bind.get_bool_unchecked(); break;
            case core::integer: dest = bind.get_int_unchecked(); break;
            case core::uinteger: dest = bind.get_uint_unchecked(); break;
            case core::real: dest = bind.get_real_unchecked(); break;
            case core::string:
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            case cppdatalib::core::temporary_string:
#endif
                dest = bind.get_string_unchecked();
                break;
            case core::link: dest = bind.get_link_unchecked(); break;
            case core::array: dest = bind.get_array_unchecked(); break;
            case core::object: dest = bind.get_object_unchecked(); break;
        }
    }
};
#endif // CPPDATALIB_CPP17

#endif // CPPDATALIB_CPP11

#endif // CPPDATALIB_STL_H
