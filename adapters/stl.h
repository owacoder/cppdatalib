#ifndef CPPDATALIB_STL_H
#define CPPDATALIB_STL_H

#include <string>
#include <array>
#include <vector>
#include <list>
#include <forward_list>
#include <queue>
#include <valarray>
#include <bitset>
#include <tuple>

#include <set>
#include <unordered_set>

#include <map>
#include <unordered_map>

#include <stack>
#include <queue>

#if __cplusplus >= 201703L
#include <variant>
#include <optional>
#include <any>
#endif

#include "../core/value_parser.h"

template<typename... Ts>
class cast_template_to_cppdatalib<std::basic_string, char, Ts...>
{
    const std::basic_string<char, Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::basic_string<char, Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::string_t(bind.c_str(), bind.size());}
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
            get_output()->write(core::value(bind));
        }
    };
}}

template<typename T, typename... Ts>
class cast_template_to_cppdatalib<std::unique_ptr, T, Ts...>
{
    const std::unique_ptr<T, Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::unique_ptr<T, Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        if (!bind)
            return cppdatalib::core::null_t();
        return *bind;
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
            if (bind)
            {
                compose_parser(*bind);
                write_next();
            }
            else
                get_output()->write(core::value());
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
        if (!bind)
            return cppdatalib::core::null_t();
        return *bind;
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
            if (bind)
            {
                compose_parser(*bind);
                write_next();
            }
            else
                get_output()->write(core::value());
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
        if (!bind)
            return cppdatalib::core::null_t();
        return *bind;
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
            if (bind)
            {
                compose_parser(*bind);
                write_next();
            }
            else
                get_output()->write(core::value());
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (size_t i = 0; i < bind.size(); ++i)
            result.push_back(bind[i]);
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (size_t i = 0; i < bind.size(); ++i)
            result.push_back(cppdatalib::core::value(bind[i]));
        return result;
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
            {
                compose_parser(bind[idx++]);
                write_next();
            }
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
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind);}
};

template<typename... Ts>
class cast_template_to_cppdatalib<std::stack, Ts...>
{
    std::stack<Ts...> bind;
public:
    cast_template_to_cppdatalib(std::stack<Ts...> bind) : bind(bind) {}
    operator cppdatalib::core::value()
    {
        using namespace std;
        cppdatalib::core::value result = cppdatalib::core::array_t();
        while (!bind.empty())
        {
            result.push_back(cppdatalib::core::value(bind.top()));
            bind.pop();
        }
        reverse(result.get_array_ref().begin().data(), result.get_array_ref().end().data());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        while (!bind.empty())
        {
            result.push_back(cppdatalib::core::value(bind.front()));
            bind.pop();
        }
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        while (!bind.empty())
        {
            result.push_back(cppdatalib::core::value(bind.front()));
            bind.pop();
        }
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (const auto &item: bind)
            result.add_member_at_end(cppdatalib::core::value(item.first),
                                     cppdatalib::core::value(item.second));
        return result;
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
                write_next();
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
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (const auto &item: bind)
            result.add_member_at_end(cppdatalib::core::value(item.first),
                                     cppdatalib::core::value(item.second));
        return result;
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
                write_next();
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
        cppdatalib::core::value result = cppdatalib::core::object_t();
        result.set_subtype(cppdatalib::core::hash);
        for (const auto &item: bind)
            result.add_member(cppdatalib::core::value(item.first),
                              cppdatalib::core::value(item.second));
        return result;
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
                write_next();
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
        cppdatalib::core::value result = cppdatalib::core::object_t();
        result.set_subtype(cppdatalib::core::hash);
        for (const auto &item: bind)
            result.add_member(cppdatalib::core::value(item.first),
                              cppdatalib::core::value(item.second));
        return result;
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
                write_next();
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
            {
                compose_parser(bind.second);
                write_next();
            }
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
        cppdatalib::core::array_t result;
        cppdatalib::core::impl::push_back<std::tuple_size<std::tuple<Ts...>>::value>(bind, result);
        return result;
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

#if __cplusplus >= 201703L
template<typename... Ts>
class cast_template_to_cppdatalib<std::optional, Ts...>
{
    const std::optional<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const std::optional<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        if (bind)
            return cppdatalib::core::value(*bind);
        return cppdatalib::core::null_t();
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
                    get_output()->write(core::null_t());
                else
                {
                    compose_parser(*bind);
                    write_next();
                }
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
        if (bind.valueless_by_exception())
            return core::null_t();
        return cppdatalib::core::value(std::get<bind.index()>(bind));
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
                {
                    compose_parser(std::get<bind.index()>(bind));
                    write_next();
                }
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
        cppdatalib::core::string_t str = bind.as_string();
        return std::basic_string<char, Ts...>(str.c_str(), str.size());
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
        return new T(bind.operator T());
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
        return new T(bind.operator T());
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
        if (bind.is_array())
        {
            auto it = bind.get_array_unchecked().begin();
            for (size_t i = 0; i < std::min(bind.array_size(), N); ++i)
                result[i] = (*it++).operator T();
        }
        return result;
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
        if (bind.is_array())
        {
            auto it = bind.get_array_unchecked().begin();
            for (size_t i = 0; i < std::min(bind.array_size(), N); ++i)
                result[i] = *it++;
        }
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push(item.operator T());
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push(item.operator T());
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push(item.operator T());
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_front(item.operator T());
        result.reverse();
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        size_t index = 0;
        result.resize(bind.array_size());
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result[index++] = item.operator T();
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.insert(item.operator T());
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.insert(item.operator T());
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.insert(item.operator T());
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.insert(item.operator T());
        return result;
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
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert({item.first.operator K(),
                               item.second.operator V()});
        return result;
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
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert({item.first.operator K(),
                               item.second.operator V()});
        return result;
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
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert({item.first.operator K(),
                               item.second.operator V()});
        return result;
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
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert({item.first.operator K(),
                               item.second.operator V()});
        return result;
    }
};

template<typename T1, typename T2>
class cast_template_from_cppdatalib<std::pair, T1, T2>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::pair<T1, T2>() const {return std::pair<T1, T2>{bind.element(0).operator T1(), bind.element(1).operator T2()};}
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
        if (bind.is_array())
            cppdatalib::core::impl::tuple_push_back<std::tuple_size<std::tuple<Ts...>>::value>(bind.get_array_unchecked(), result);
        return result;
    }
};

#if __cplusplus >= 201703L
template<typename... Ts>
class cast_template_from_cppdatalib<std::optional, Ts...>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator std::optional<Ts...>() const
    {
        typedef typename cppdatalib::core::impl::unpack_first<Ts...>::type contained_type;
        std::optional<Ts...> result;
        if (!bind.is_null())
            result = bind.operator contained_type();
        return result;
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
        switch (bind.get_type())
        {
            default:
            case core::null: result = std::any(); break;
            case core::boolean: result = bind.get_bool_unchecked(); break;
            case core::integer: result = bind.get_int_unchecked(); break;
            case core::uinteger: result = bind.get_uint_unchecked(); break;
            case core::real: result = bind.get_real_unchecked(); break;
            case core::string: result = bind.get_string_unchecked(); break;
            case core::array: result = bind.get_array_unchecked(); break;
            case core::object: result = bind.get_object_unchecked(); break;
        }
        return result;
    }
};
#endif

#endif // CPPDATALIB_STL_H
