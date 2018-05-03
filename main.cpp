//#include <iostream>

//#include "adapters/stl.h"

#define CPPDATALIB_ENABLE_FAST_IO
//#define CPPDATALIB_DISABLE_WRITE_CHECKS
#define CPPDATALIB_DISABLE_FAST_IO_GCOUNT
#define CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
//#define CPPDATALIB_ENABLE_BOOST_DYNAMIC_BITSET
#define CPPDATALIB_DISABLE_WEAK_POINTER_CONVERSIONS
//#define CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
//#define CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
#define CPPDATALIB_THROW_IF_WRONG_TYPE
#undef CPPDATALIB_ENABLE_MYSQL

#include "cppdatalib.h"

struct vt100
{
    const char * const reset_term = "\033c";
    const char * const enable_lwrap = "\033[7h";
    const char * const disable_lwrap = "\033[7l";

    const char * const default_font = "\033(";
    const char * const alternate_font = "\033)";

    const char * const move_cursor_screen_home = "\033[H"; // Moves to upper-left of screen, not beginning of line
    const char * const move_cursor_home = "\r"; // Moves to beginning of line, not beginning of screen
    std::string move_cursor(int row, int col)
    {
        return "\033[" + std::to_string(row) + ';' + std::to_string(col) + 'H';
    }
    const char * const force_move_cursor_home = "\033[f";
    std::string force_move_cursor(int row, int col)
    {
        return "\033[" + std::to_string(row) + ';' + std::to_string(col) + 'f';
    }
    const char * const move_cursor_up = "\033[A";
    std::string move_cursor_up_by(int rows)
    {
        return "\033[" + std::to_string(rows) + 'A';
    }
    const char * const move_cursor_down = "\033[B";
    std::string move_cursor_down_by(int rows)
    {
        return "\033[" + std::to_string(rows) + 'B';
    }
    const char * const move_cursor_right = "\033[C";
    std::string move_cursor_right_by(int cols)
    {
        return "\033[" + std::to_string(cols) + 'C';
    }
    const char * const move_cursor_left = "\033[D";
    std::string move_cursor_left_by(int cols)
    {
        return "\033[" + std::to_string(cols) + 'D';
    }
    const char * const save_cursor = "\033[s";
    const char * const restore_cursor = "\033[u";
    const char * const save_cursor_and_attrs = "\0337";
    const char * const restore_cursor_and_attrs = "\0338";

    const char * const enable_scroll = "\033[r";
    const char * const scroll_screen_down = "\033D";
    const char * const scroll_screen_up = "\033M";
    std::string scroll_range(int from, int to)
    {
        return "\033[" + std::to_string(from) + ';' + std::to_string(to) + 'r';
    }

    const char * const set_tab = "\033H";
    const char * const unset_tab = "\033[g";
    const char * const unset_all_tabs = "\033[3g";

    const char * const erase_to_end_of_line = "\033[K";
    const char * const erase_to_start_of_line = "\033[1K";
    const char * const erase_line = "\033[2K";
    const char * const erase_screen_down = "\033[J";
    const char * const erase_screen_up = "\033[1J";
    const char * const erase_screen = "\033[2J";

    const char * const attr_reset = "\033[0m";
    const char * const attr_bright = "\033[1m";
    const char * const attr_dim = "\033[2m";
    const char * const attr_underscore = "\033[4m";
    const char * const attr_blink = "\033[5m";
    const char * const attr_reverse = "\033[7m";
    const char * const attr_hidden = "\033[8m";

    const char * const black = "\033[30m";
    const char * const red = "\033[31m";
    const char * const green = "\033[32m";
    const char * const yellow = "\033[33m";
    const char * const blue = "\033[34m";
    const char * const magenta = "\033[35m";
    const char * const cyan = "\033[36m";
    const char * const white = "\033[37m";

    const char * const bg_black = "\033[40m";
    const char * const bg_red = "\033[41m";
    const char * const bg_green = "\033[42m";
    const char * const bg_yellow = "\033[43m";
    const char * const bg_blue = "\033[44m";
    const char * const bg_magenta = "\033[45m";
    const char * const bg_cyan = "\033[46m";
    const char * const bg_white = "\033[47m";
};

template<typename T>
std::ostream &operator<<(std::ostream &strm, const std::vector<T> &value)
{
    strm << "[";
    for (auto i = value.begin(); i != value.end(); ++i)
    {
        if (i != value.begin())
            strm << " ";
        strm << *i;
    }
    return strm << "]" << std::flush;
}

template<typename F, typename S = F>
struct TestData : public std::vector<std::pair<F, S>>
{
    using std::vector<std::pair<F, S>>::vector;
};

// `tests` is a vector of test cases. The test cases will be normal (first item in tuple is parameter, second is result)
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<typename F, typename S, typename ResultCode, typename Compare = std::not_equal_to<S>>
bool Test(const char *name, const TestData<F, S> &tests, ResultCode actual, bool bail_early = true, Compare compare = Compare())
{
    vt100 vt;
    size_t percent = 0;
    size_t current = 0;
    size_t failed = 0;

    std::cout << "Testing " << name << "... " << vt.yellow << "0%" << std::flush;
    std::cout << vt.attr_reset;

    for (const auto &test: tests)
    {
        if (compare(test.second, actual(test.first)))
        {
            std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
            std::cout << "Testing " << name << "... " << std::flush;
            std::cout << vt.red;
            std::cout << "Test " << (current+1) << " FAILED!\n";
            std::cout << "\tInput: " << test.first << "\n";
            std::cout << "\tExpected output: " << test.second << "\n";
            std::cout << "\tActual output: " << actual(test.first) << std::endl;
            std::cout << vt.attr_reset;
            if (bail_early)
                return false;
            ++failed;
        }

        if (current * 100 / tests.size() > percent)
        {
            percent = current * 100 / tests.size();
            std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
            std::cout << "Testing " << name << "... " << vt.yellow << percent << "%" << std::flush;
            std::cout << vt.attr_reset;
        }
        ++current;
    }

    if (!failed)
    {
        std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
        std::cout << "Testing " << name << "... " << vt.green << "done." << std::endl;
        std::cout << vt.attr_reset;
    }
    else
    {
        std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
        std::cout << "Testing " << name << "... " << vt.red << "done. (" << failed << " failed out of " << tests.size() << ")" << std::endl;
        std::cout << vt.attr_reset;
    }

    return failed;
}

// `tests` is a vector of test cases. The test cases will be reversed (first item in tuple is result, second is parameter)
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<typename F, typename S, typename ResultCode, typename Compare = std::not_equal_to<S>>
bool ReverseTest(const char *name, const TestData<F, S> &tests, ResultCode actual, bool bail_early = true, Compare compare = Compare())
{
    vt100 vt;
    size_t percent = 0;
    size_t current = 0;
    uintmax_t failed = 0;

    std::cout << "Testing " << name << "... " << vt.yellow << "0%" << std::flush;
    std::cout << vt.attr_reset;

    for (const auto &test: tests)
    {
        if (compare(test.first, actual(test.second)))
        {
            std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
            std::cout << "Testing " << name << "... " << std::flush;
            std::cout << vt.red;
            std::cout << "Test " << (current+1) << " FAILED!\n";
            std::cout << "\tInput: " << test.second << "\n";
            std::cout << "\tExpected output: " << test.first << "\n";
            std::cout << "\tActual output: " << actual(test.second) << std::endl;
            std::cout << vt.attr_reset;
            if (bail_early)
                return false;
            ++failed;
        }

        if (current * 100 / tests.size() > percent)
        {
            percent = current * 100 / tests.size();
            std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
            std::cout << "Testing " << name << "... " << vt.yellow << percent << "%" << std::flush;
            std::cout << vt.attr_reset;
        }
        ++current;
    }

    if (!failed)
    {
        std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
        std::cout << "Testing " << name << "... " << vt.green << "done." << std::endl;
        std::cout << vt.attr_reset;
    }
    else
    {
        std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
        std::cout << "Testing " << name << "... " << vt.red << "done. (" << failed << " failed out of " << tests.size() << ")" << std::endl;
        std::cout << vt.attr_reset;
    }

    return failed;
}

// `tests` is the maximum of a range, 0 -> tests, each element of which is passed to the test code
// ExpectedCode is a functor that should return the expected, valid, test result
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<typename ExpectedCode, typename ResultCode, typename Compare>
bool TestRange(const char *name, uintmax_t tests, ExpectedCode expected, ResultCode actual, bool bail_early = true, Compare compare = Compare())
{
    vt100 vt;
    size_t percent = 0;
    uintmax_t failed = 0;

    std::cout << "Testing " << name << "... " << vt.yellow << "0%" << std::flush;
    std::cout << vt.attr_reset;

    for (uintmax_t test = 0; test < tests; ++test)
    {
        if (compare(expected(test), actual(test)))
        {
            std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
            std::cout << "Testing " << name << "... " << std::flush;
            std::cout << vt.red;
            std::cout << "Test " << (test+1) << " FAILED!\n";
            std::cout << "\tInput: " << test << "\n";
            std::cout << "\tExpected output: " << expected(test) << "\n";
            std::cout << "\tActual output: " << actual(test) << std::endl;
            std::cout << vt.attr_reset;
            if (bail_early)
                return false;
            ++failed;
        }

        if (test * 100 / tests > percent)
        {
            percent = test * 100 / tests;
            std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
            std::cout << "Testing " << name << "... " << vt.yellow << percent << "%" << std::flush;
            std::cout << vt.attr_reset;
        }
    }

    if (!failed)
    {
        std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
        std::cout << "Testing " << name << "... " << vt.green << "done." << std::endl;
        std::cout << vt.attr_reset;
    }
    else
    {
        std::cout << vt.erase_line << vt.move_cursor_home << vt.attr_reset;
        std::cout << "Testing " << name << "... " << vt.red << "done. (" << failed << " failed out of " << tests << ")" << std::endl;
        std::cout << vt.attr_reset;
    }

    return failed;
}

TestData<std::string> debug_hex_encode_tests = {
    {"Hello World!", "H e l l o   W o r l d ! "},
    {"A", "A "},
    {"1234", "1 2 3 4 "},
    {"..\1\n", ". . 01 0A "},
    {"", ""}
};

TestData<std::string> hex_encode_tests = {
    {"Hello World!", "48656C6C6F20576F726C6421"},
    {"A", "41"},
    {"1234", "31323334"},
    {"..\1\n", "2E2E010A"},
    {"", ""}
};

TestData<std::string> base64_encode_tests = {
    {"Hello World!", "SGVsbG8gV29ybGQh"},
    {"A", "QQ=="},
    {"1234", "MTIzNA=="},
    {"..\1\n", "Li4BCg=="},
    {"", ""}
};

TestData<std::string> json_tests = {
    {"{}", "{}"},
    {"[]", "[]"},
    {"null", "null"},
    {"0", "0"},
    {"-123", "-123"},
    {"7655555", "7655555"},
    {"76555556666666666", "76555556666666666"},
    {"-76555556666666666021", "-76555556666666666021"},
    {"true", "true"},
    {"false", "false"},
    {"0.5", "0.5"},
    {"-5.0e-1", "-0.5"},
    {"{\"key\":\"value\",\"key2\":null}", "{\"key\":\"value\",\"key2\":null}"},
    {"[null,true,false,-5,\"\"]", "[null,true,false,-5,\"\"]"},
    {"\"Hello World!\"", "\"Hello World!\""}
};

TestData<std::string> bencode_tests = {
    {"de", "de"},
    {"le", "le"},
    {"i0e", "i0e"},
    {"i-123e", "i-123e"},
    {"i7655555e", "i7655555e"},
    {"i76555556666666666e", "i76555556666666666e"},
    {"17:76555556666666666", "17:76555556666666666"},
    {"d3:key5:value4:key2i0ee", "d3:key5:value4:key2i0ee"},
    {"li0e4:true5:falsei-5e0:e", "li0e4:true5:falsei-5e0:e"}
};

struct hex_string : public std::string
{
    hex_string() {}
    hex_string(const char *s) : std::string(s) {}
    hex_string(const char *s, size_t size) : std::string(s, size) {}
    hex_string(const std::string &str) : std::string(str) {}
};

std::ostream &operator<<(std::ostream &ostr, const hex_string &str)
{
    cppdatalib::core::ostream_handle wrap(ostr);
    cppdatalib::hex::debug_write(wrap, str);
    return ostr;
}

TestData<hex_string> message_pack_tests = {
    {"\x80", "\x80"},
    {"\x90", "\x90"},
    {"\x01", "\x01"},
    {"\xff", "\xff"},
    {"\xd0\x85", "\xd0\x85"},
    {"\xcc\x85", "\xcc\x85"},
    {"\x81\x01\x01", "\x81\x01\x01"},
    {"\x92\x01\x01", "\x92\x01\x01"}
};

int readme_simple_test()
{
    using namespace cppdatalib;             // Parent namespace
    using namespace json;                   // Format namespace

    core::value my_value;                   // Global cross-format value class

    try {
        json::parser p(std::cin);
        json::stream_writer w(std::cout);
        p >> my_value;                      // Read in to core::value from STDIN as JSON
        w << my_value;                      // Write core::value out to STDOUT as JSON
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl; // Catch any errors that might have occured (syntax or logical)
    }

    return 0;
}

int readme_simple_test2()
{
    using namespace cppdatalib;             // Parent namespace
    using namespace json;                   // Format namespace

    try {
        json::parser(std::cin) >> json::stream_writer(std::cout);        // Write core::value out to STDOUT as JSON
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl; // Catch any errors that might have occured (syntax or logical)
    }

    return 0;
}

int readme_simple_test3()
{
    using namespace cppdatalib;             // Parent namespace
    using namespace json;                   // Format namespace

    try {
        json::stream_writer(std::cout) << from_json(std::cin);        // Write core::value out to STDOUT as JSON
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl; // Catch any errors that might have occured (syntax or logical)
    }

    return 0;
}

struct point
{
    int x, y;
};

#include "adapters/stl.h"

template<>
class cast_to_cppdatalib<point>
{
    const point &bind;
public:
    cast_to_cppdatalib(const point &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(cppdatalib::core::object_t{{cppdatalib::core::value("x"), cppdatalib::core::value(bind.x)}, {cppdatalib::core::value("y"), cppdatalib::core::value(bind.y)}});}
};

template<>
struct cast_from_cppdatalib<point>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator point() const
    {
        point p;
        p.x = (int) bind["x"];
        p.y = (int) bind["y"];
        return p;
    }
};

int readme_simple_test4()
{
    using namespace cppdatalib;             // Parent namespace
    using namespace json;                   // Format namespace

    core::value my_value, m2;               // Global cross-format value class
    core::value_builder builder(my_value);

    m2 = cppdatalib::core::value(std::array<int, 3>{{0, 1, 4}});
    std::array<int, 2> axx = (decltype(axx)) m2;
    m2 = cppdatalib::core::value(axx);

    std::stack<int, std::vector<int>> stack;
    stack.push(0);
    stack.push(1);

    try {
        json::parser p(std::cin);
        json::stream_writer w(std::cout);

        p.begin(builder);
        do
            p.write_one();                  // Read in to core::value from STDIN as JSON
        while (p.busy());
        p.end();
        p >> my_value >> m2;

        core::value_parser vp(my_value);
        vp.begin(w);
        do
            vp.write_one();
        while (vp.busy());
        vp.end();

        std::multimap<const char *, int> map = {{"x", 1}, {"y", 2}};
        std::map<std::string, int> str;
        /*m2 = str;
        str = m2.as<decltype(str)>();
        m2 = stack;
        stack = m2;
        m2 = cppdatalib::core::value(map);
        point p2 = point();
        p2.x = 9;
        m2 = cppdatalib::core::value(p2);
        p2 = static_cast<decltype(p2)>(m2);
        p2.x += 1000;
        m2 = cppdatalib::core::value(p2);
        w << m2;                // Write core::value out to STDOUT as JSON*/
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl; // Catch any errors that might have occured (syntax or logical)
    }

    return 0;
}

#ifdef CPPDATALIB_ENABLE_BOOST_COMPUTE
#include "adapters/boost_compute.h"
#include <boost/compute.hpp>

void test_boost_compute()
{
    boost::compute::device device = boost::compute::system::default_device();
    boost::compute::context ctx(device);
    boost::compute::command_queue queue(ctx, device);

    boost::compute::vector<float> vec(3100000, ctx);
    cppdatalib::core::value compute_v;

    compute_v = cppdatalib::core::array_t();
    for (size_t i = 0; i < vec.size(); ++i)
        compute_v.push_back(i);
    vec = cppdatalib::core::userdata_cast(compute_v, queue);

    boost::compute::transform(vec.begin(),
                              vec.end(),
                              vec.begin(),
                              boost::compute::sqrt<float>(),
                              queue);

    compute_v = cppdatalib::core::userdata_cast(vec, queue);
    //std::cout << compute_v << std::endl;
}
#endif

#ifdef CPPDATALIB_ENABLE_QT
void test_qt()
{
	std::tuple<int, int, std::string, QVector<QString>> tuple;
	QStringList value = { "value", "" };
	xyz = std::make_tuple(0, 1.5, "hello", value, "stranger");
	std::cout << xyz << std::endl;
	tuple = xyz;
	xyz = tuple;
	std::cout << xyz << std::endl;
	std::array<int, 3> stdarr = xyz;
	xyz = stdarr;
	std::cout << xyz << std::endl;
}
#endif

void test_sql()
{
#ifdef CPPDATALIB_ENABLE_MYSQL
    try
    {
        MYSQL sql, sql2;

        if (!mysql_init(&sql) || !mysql_init(&sql2)) return;

        cppdatalib::mysql::table_parser p(&sql, "misc", "pet");
        cppdatalib::mysql::table_writer writer(&sql2, "misc", "new");

        p.connect("localhost", "oliver", NULL, "misc");
        writer.connect("localhost", "oliver", NULL, "misc");

        writer.set_metadata(p.refresh_metadata());
        cppdatalib::core::value val;
        p >> val;
        val >> writer;
        //std::cout << cppdatalib::json::to_pretty_json(val, 2) << std::endl;

        mysql_close(&sql);
        mysql_close(&sql2);
    }
    catch (cppdatalib::const core::error &e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
#endif
}

void test_select()
{
    using json_writer = cppdatalib::json::stream_writer;

    namespace cdlcore = cppdatalib::core;

    cppdatalib::core::value data;

    auto lambda = [](const cppdatalib::core::value &v) {return v.is_bool();};

    data.push_back(cppdatalib::core::value(true));//cppdatalib::core::object_t{{"size", cppdatalib::core::null_t()}, {"shape", "weird"}, {0, -1}, {cppdatalib::core::object_t{{"object", 1}}, 1}});
    data.push_back(cppdatalib::core::value(false));//cppdatalib::core::object_t{{"size", cppdatalib::core::null_t()}, {"shape", "weird"}, {0, -1}, {cppdatalib::core::object_t{{"object", 1}}, 1}});
    data.push_back(cppdatalib::core::value(true));
    data.push_back(cppdatalib::core::value(false));

    json_writer json(std::cout);
    data >> cdlcore::make_select_from_array_filter(json, lambda);
    //cppdatalib::core::object_keys_to_array_filter(json) << data;
}

#include "experimental/decision_trees.h"
#include "experimental/algorithm.h"
#include "experimental/knearest.h"
#include "languages/languages.h"

void test_decision_tree()
{
#if 0
    cppdatalib::core::value data;
    cppdatalib::core::value results;

    try
    {
        data.push_back(cppdatalib::core::object_t{{"size", "M"}, {"shape", "brick"}, {"color", "blue"}});
        results.push_back(true);
        data.push_back(cppdatalib::core::object_t{{"size", "S"}, {"shape", "wedge"}, {"color", "red"}});
        results.push_back(false);
        data.push_back(cppdatalib::core::object_t{{"size", "L"}, {"shape", "wedge"}, {"color", "red"}});
        results.push_back(false);
        data.push_back(cppdatalib::core::object_t{{"size", "S"}, {"shape", "sphere"}, {"color", "red"}});
        results.push_back(true);
        data.push_back(cppdatalib::core::object_t{{"size", "L"}, {"shape", "pillar"}, {"color", "green"}});
        results.push_back(true);
        data.push_back(cppdatalib::core::object_t{{"size", "L"}, {"shape", "pillar"}, {"color", "red"}});
        results.push_back(false);
        data.push_back(cppdatalib::core::object_t{{"size", "L"}, {"shape", "sphere"}, {"color", "green"}});
        results.push_back(true);

        auto tree = cppdatalib::experimental::make_decision_tree(data.get_array(), cppdatalib::core::array_t{"size", "shape", "color"}, results.get_array());
        std::cout << tree << std::endl;

        std::cout << "looking for {size: L, shape: block, color: blue} in tree: " << std::endl;
        std::cout << cppdatalib::experimental::test_decision_tree(tree, cppdatalib::core::object_t{{"size", "L"}, {"shape", "pillar"}, {"color", "blue"}}, true) << std::endl;

        data >> cppdatalib::lang::lisp::stream_writer(std::cout);
    } catch (const cppdatalib::core::error &e)
    {
        std::cerr << e.what() << std::endl;
    }
#endif
}

#include <fstream>

void make_k_nearest()
{
#if 0
    std::ofstream out;

    out.open("/shared/Test_Data/knearest_test.json");

    using namespace cppdatalib::json;
    cppdatalib::core::value data;
    cppdatalib::core::value results;

    for (size_t i = 0; i < static_cast<size_t>(rand() % 100); ++i)
    {
        data.push_back(cppdatalib::core::object_t{{"A", rand()}, {"B", rand()}});
        results.push_back(rand() % 4);
    }

    cppdatalib::json::pretty_stream_writer(out, 2) << cppdatalib::core::object_t{{"data", data}, {"results", results}};
#endif
}

void test_k_nearest()
{
    std::ifstream in;

    in.open("/shared/Test_Data/knearest_test.json");

    cppdatalib::core::value data, value;
    //data << cppdatalib::json::parser(in);
#if 0
    data["data"].push_back(cppdatalib::core::object_t{{"X", 0}, {"Y", 0}});
    data["results"].push_back(0);
    data["data"].push_back(cppdatalib::core::object_t{{"X", 3}, {"Y", 0}});
    data["results"].push_back(1);
    data["data"].push_back(cppdatalib::core::object_t{{"X", 3}, {"Y", 0}});
    data["results"].push_back(1);
#endif

    std::cout << "\n";

    while (true)
    {
        auto distance = [](const cppdatalib::core::value &lhs, const cppdatalib::core::value &rhs)
        {
            return lhs.as_real() - rhs.as_real();
        };

        auto weight = [](double val) -> double
        {
            if (val == 0)
                return INFINITY;
            return 1.0 / val;
        };

        size_t k;

        std::cout << "Enter desired value of k: " << std::flush;
        std::cin >> k;
        std::cout << "Enter your tuple to test in JSON format: " << std::flush;
        cppdatalib::json::parser(std::cin) >> value;

        std::cout << "Classification: ";
        cppdatalib::core::dump::stream_writer(std::cout, 2) << cppdatalib::experimental::k_nearest_neighbor_classify_weighted<double>(
                                                                    value,
                                                                    data["data"].as_array(),
                                                                    data["results"].as_array(),
                                                                    k,
                                                                    distance,
                                                                    weight);
    }

    std::cout << data << std::endl;
}

#include <ctime>

void test_attributes()
{
    try
    {
        using namespace cppdatalib::json;
        using namespace cppdatalib::bson;
        using namespace cppdatalib::xml;

        cppdatalib::json::stream_writer writer(std::cout);

        writer.begin();
        writer.begin_array(cppdatalib::core::array_t(), 11 /*cppdatalib::core::stream_handler::unknown_size*/);
        //writer.write_out_of_order(1000000, 100);
        writer.write_out_of_order(9, 10);
        writer.write_out_of_order(6, 6);
        writer.write_out_of_order(1, 1);
        writer.write_out_of_order(3, 3);
        writer.write_out_of_order(2, 2);
        writer.write_out_of_order(0, 0);
        writer.write_out_of_order(4, 4);
        //writer.write_out_of_order(7, 7);
        writer.write_out_of_order(5, 5);
        writer.write_out_of_order(8, 8);
        writer.write_out_of_order(10, 9);
        writer.end_array(cppdatalib::core::array_t());
        writer.end();

        return;

        cppdatalib::http::parser(cppdatalib::core::value("http://owacoder.com"),
                                 cppdatalib::core::default_network_library,
                                 "GET",
                                 cppdatalib::core::object_t(),
                                 1,
                                 cppdatalib::core::object_t{{"host", "localhost"},
                                                            {"port", 3128u},
                                                            {"username", ""},
                                                            {"password", ""}}) >> std::cout;
        std::cout << "\n";

#ifdef CPPDATALIB_ENABLE_FILESYSTEM
        cppdatalib::filesystem::stream_writer("/shared/Test_Data2")
         << cppdatalib::filesystem::parser("/shared/Test_Data",
                                           true,
                                           cppdatalib::filesystem::parser::skip_file_reading,
                                           cppdatalib::filesystem::fs::directory_options::skip_permission_denied);

        return;

        cppdatalib::filesystem::stream_writer("/shared/Test_Data/filesystem/", false, false)
                << "{\"directory\": {\"file 1\": 012354, \"file 2\": true, \"dir\": [\"0 contents\", \"1 contents\"]},"
                   "\"file\": \"Hello Worlds!\"}"_json;
#endif

        cppdatalib::core::value bson;

        cppdatalib::core::istringstream bstream(std::string("\x31\x00\x00\x00"
                                                             "\x04\x42SON\x00"
                                                             "\x26\x00\x00\x00"
                                                             "\x02\x30\x00\x08\x00\x00\x00\x61wesome\x00"
                                                             "\x01\x31\x00\x33\x33\x33\x33\x33\x33\x14\x40"
                                                             "\x10\x32\x00\xc2\x07\x00\x00"
                                                             "\x00"
                                                             "\x00", 0x31));

        std::cout << from_bson(bstream);

        cppdatalib::core::value xml;
        cppdatalib::core::istringstream stream("<?xml version = \"1.3\" encoding = \"UTF-8\"?>\n"
                                   "<!DOCTYPE greeting>"
                                   "<!ENTITY % pub    '&#xc9;ditions Gallimard' >\n"
                                   "<!ENTITY   left   'I was left behind!&amp;'> \n"
                                   "<!ENTITY   rights '<rights xml:lang=\"en-GB\" tag=&#39;&lt;&#39;>&left;All rights reserved</rights>' >\n"
                                   "<!ENTITY   book   'La Peste: Albert Camus,"
                                   " &#xA9; 1947 %pub;. &rights;'>\n"
                                   "<!-- This document (<head> and <body>) should not be used -->\n"
                                   "<greeting>&rights;<first><![CDATA[A CDATA SECTION!!<>]]>In first<tag id=\"my_id\" i = '&amp;none'>In tag</tag>Hello, world!</first>Before first");
        cppdatalib::core::istringstream stream2("</greeting>");
        std::string str;
        while (stream)
            str.push_back(stream.get());//, str.push_back(0);
        str.pop_back();
        //str.pop_back();
        str += cppdatalib::core::ucs_to_utf(0x1D11E, "UTF-8");
        while (stream2)
            str.push_back(stream2.get());//, str.push_back(0);
        str.pop_back();
        //str.pop_back();
        stream.str(str);
        std::cout << cppdatalib::core::ucs_to_utf(0xD1, "UTF-8");

        /*cppdatalib::core::iencodingstream encstream(stream, cppdatalib::core::encoding_from_name("UTF-8"));

        xml << cppdatalib::xml::parser(encstream, true);
        std::cout << xml;
        cppdatalib::xml::stream_writer(std::cout) << xml;*/

        return;

        /*
        auto attr = cppdatalib::core::object_t{{"attribute_1", true}, {"attribute_2", "My other attribute"}};

        cppdatalib::core::value value(2), v2("string");
        value.set_attributes(attr);
        v2.set_attributes(cppdatalib::core::object_t{{"type", "&none\n"}});

        value.add_attribute("my_attr").add_attribute("h23");

        cppdatalib::core::value e1 = cppdatalib::core::array_t{"some text ", cppdatalib::core::object_t{{"secondary", v2}}, cppdatalib::core::object_t{{"first", value}}};

        cppdatalib::xml::pretty_document_writer(std::cout, 2) << cppdatalib::core::object_t{{"root", e1}};*/
    } catch (const cppdatalib::core::error &e)
    {
        std::cerr << e.what() << std::endl;
    }
}

#ifdef CPPDATALIB_ENABLE_FAST_IO
#ifdef CPPDATALIB_LINUX
void test_mmap()
{
    std::ofstream out;
    out.open("/shared/Test_Data/huger.dif");
    cppdatalib::core::ostd_streambuf_wrapper wrap(out.rdbuf());

    cppdatalib::core::immapstream map("/shared/Test_Data/huge.json", true);
    cppdatalib::json::parser parser(map);
    parser >> cppdatalib::tsv::stream_writer(wrap);
}
#endif
#endif

#include <qtapp.h>

int rmain(int argc, char **argv)
{
    //return readme_simple_test4();

    //test_mmap();

    return qtapp_main(argc, argv, test_attributes);

    //return 0;

    cppdatalib::core::cache_vector_n<size_t*, 2> vec;

    vec.resize(10);
    for (size_t i = 0; i < 10; ++i)
        vec[i] = &i;

    std::cout << vec.size() << std::endl;

    for (auto it: vec)
        std::cout << it << "\n";

    //return vec.size();

    try
    {
        const char tuple[] = "2093";
        cppdatalib::json::stream_writer w(std::cout);
        cppdatalib::core::automatic_buffer_filter f(w);
        int i = 1;
        cppdatalib::core::generic_parser p(i);
        std::cout << f.name() << std::endl;
        f << p;
        std::cout << cppdatalib::core::value(tuple);
        std::unique_ptr<int, std::default_delete<int>> s = cppdatalib::core::value(tuple).as<decltype(s)>();
        f << cppdatalib::core::generic_parser(tuple);
        std::cout << *s << std::endl;
        //tuple = cppdatalib::core::value(f777);
        w.stream() << "\n";// std::endl;
    }
    catch (const cppdatalib::core::error &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;

#if 0
    cppdatalib::core::value data = cppdatalib::core::object_t{{"value", 1}, {cppdatalib::core::object_t{{cppdatalib::core::object_t{{"string", "null"}}, cppdatalib::core::object_t{{"string", "null"}}}}, cppdatalib::core::object_t{{cppdatalib::core::object_t{{"string", "null"}}, cppdatalib::core::object_t{{"string", "null"}}}}}, {"Object", cppdatalib::core::null_t()}};

    try
    {
        cppdatalib::core::dump::stream_writer out(std::cout, 2);
        cppdatalib::core::object_keys_to_array_filter(out) << data;
    }
    catch (const cppdatalib::core::error &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
#endif

    srand(std::time(NULL));

    test_decision_tree();
    //make_k_nearest();
    test_k_nearest();

    return 0;

    using namespace cppdatalib;
    using namespace json;
    //boost::dynamic_bitset<> b;

    cppdatalib::core::value yx;// = cppdatalib::core::value(b);
    yx = R"({"my string":""})"_json;

    std::tuple<unsigned int, const char *> myvec{44, "hello"};
    cppdatalib::core::dump::stream_writer(std::cout, 2) <<
    cppdatalib::core::generic_parser(myvec);
    std::cout << std::endl;
    std::cout << yx << std::endl;
    //b = yx;

    return 0;

#if 0
    TestSparseArray();
    return 0;
#endif

    //std::cout << cppdatalib::toml::to_toml(cppdatalib::json::from_json("{\"string\":0123}")) << std::endl;

    test_sql();
    return 0;

    cppdatalib::core::value xyz;

    std::cout << sizeof(cppdatalib::core::value) << std::endl;

    cppdatalib::core::dump::stream_writer dummy(std::cout, 2);
    cppdatalib::core::median_filter<cppdatalib::core::uinteger> median(dummy);
    cppdatalib::core::mode_filter<cppdatalib::core::uinteger> mode(median);
    cppdatalib::core::range_filter<cppdatalib::core::uinteger> range(mode);
    cppdatalib::core::dispersion_filter<cppdatalib::core::uinteger> dispersal(range);
    cppdatalib::core::array_sort_filter<> sorter(dispersal);

    //sorter << cppdatalib::core::value(cppdatalib::core::array_t{2, 0, 1, 2, 3, 4, 4});

    std::cout << median.get_median() << std::endl;
    std::cout << mode.get_modes() << std::endl;
    std::cout << range.get_max() << std::endl;
    std::cout << dispersal.get_arithmetic_mean() << std::endl << dispersal.get_standard_deviation() << std::endl;

    //return readme_simple_test4();

    vt100 vt;
    std::cout << vt.attr_bright;

#if 0
    Test("base64_encode", base64_encode_tests, cppdatalib::base64::encode);
    ReverseTest("base64_decode", base64_encode_tests, cppdatalib::base64::decode);
    Test("debug_hex_encode", debug_hex_encode_tests, cppdatalib::hex::debug_encode);
    Test("hex_encode", hex_encode_tests, cppdatalib::hex::encode);
#if 0
    TestRange("float_from_ieee_754", UINT32_MAX, cppdatalib::core::float_cast_from_ieee_754,
              cppdatalib::core::float_from_ieee_754, true, [](const auto &f, const auto &s){return f != s && !isnan(f) && !isnan(s);});
    TestRange("float_to_ieee_754", UINT32_MAX, [](const auto &f){return f;},
              [](const auto &f){return core::float_to_ieee_754(cppdatalib::core::float_cast_from_ieee_754(f));}, true,
              [](const auto &f, const auto &s){return f != s && !isnan(cppdatalib::core::float_from_ieee_754(f)) && !isnan(cppdatalib::core::float_from_ieee_754(s));});
#endif
    try
    {
        Test("JSON", json_tests, [](const auto &test){return cppdatalib::json::to_json(cppdatalib::json::from_json(test));}, false);
        Test("Bencode", bencode_tests, [](const auto &test){return cppdatalib::bencode::to_bencode(cppdatalib::bencode::from_bencode(test));}, false);
        Test("MessagePack", message_pack_tests, [](const auto &test) -> hex_string {return hex_string(cppdatalib::message_pack::to_message_pack(cppdatalib::message_pack::from_message_pack(test)));}, false);
    }
    catch (const cppdatalib::core::error &e)
    {
        std::cout << e.what() << std::endl;
    }
#endif
    std::cout << vt.attr_reset;

#ifdef CPPDATALIB_MSVC
	system("pause");
#endif
    return 0;
}

int main(int argc, char **argv)
{
    std::unique_ptr<std::ifstream> infile;
    std::unique_ptr<std::ofstream> outfile;
    std::unique_ptr<cppdatalib::core::stream_input> input;
    std::unique_ptr<cppdatalib::core::stream_handler> output;

    std::string in, out;
    std::string in_scheme, out_scheme;
    std::string in_extension, out_extension;

    if (argc != 3)
    {
        std::cerr << "usage: cppdatalib <input> <output>\n";
        return 1;
    }

    try
    {
        using namespace cppdatalib::json;

        cppdatalib::core::value tree, where;
        const char *sql = "select ELT(1, 'Aa', 'Bb', 'Cc', 'Dd'), power(ifnull(second, 2), 0.5) as '' where second != null";
        tree << cppdatalib::core::impl::sql_parser(sql, cppdatalib::core::impl::sql_parser::select_element);
        where << cppdatalib::core::impl::sql_parser(sql, cppdatalib::core::impl::sql_parser::where_element);
        cppdatalib::core::value data = "[{\"impl\": 3, \"second\": 76.3, \"third\": 3}, {\"impl\": 2, \"second\": 90.9987, \"third\": 1.4},  {\"impl\": 4, \"second\": null, \"third\": 2.4}]"_json;
        std::cout << tree << where << "\n";
        cppdatalib::core::dump::stream_writer writer(std::cout, 2);
        cppdatalib::core::sql_select_filter(writer, sql, false) << data;
    } catch (const cppdatalib::core::error &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;

    in = argv[1];
    out = argv[2];

    // Get input scheme
    size_t in_colon = in.find(':');
    if (in_colon != in.npos)
        if (in.substr(in_colon, 3) == "://")
            in_scheme = in.substr(0, in_colon);

    // Get output scheme
    size_t out_colon = out.find(':');
    if (out_colon != out.npos)
        if (out.substr(out_colon, 3) == "://")
            out_scheme = out.substr(0, out_colon);

    if (in_scheme.empty() || in_scheme == "file")
    {
        // Get input file extension
        size_t in_filetype = in.rfind('.');
        size_t in_path = in.rfind('/');

        if (in_filetype == in.npos || (in_path != in.npos && in_path > in_filetype))
        {
            std::cerr << "cppdatalib - input file has no type specified\n";
            return 1;
        }

        in_extension = in.substr(in_filetype + 1);
        if (in_scheme.size())
            in = in.substr(in_scheme.size() + 3);

        infile.reset(new std::ifstream());
        infile.get()->open(in, std::ios_base::binary | std::ios_base::in);
        if (!*infile.get())
        {
            std::cerr << "cppdatalib - input file is not readable\n";
            return 1;
        }

        if (in_extension == "benc") input.reset(new cppdatalib::bencode::parser(*infile.get()));
        else if (in_extension == "binn") input.reset(new cppdatalib::binn::parser(*infile.get()));
        else if (in_extension == "bson") input.reset(new cppdatalib::bson::parser(*infile.get()));
        else if (in_extension == "csv") input.reset(new cppdatalib::csv::parser(*infile.get()));
        else if (in_extension == "tsv") input.reset(new cppdatalib::tsv::parser(*infile.get()));
        else if (in_extension == "json") input.reset(new cppdatalib::json::parser(*infile.get()));
        else if (in_extension == "mpk") input.reset(new cppdatalib::message_pack::parser(*infile.get()));
        else if (in_extension == "ubjson") input.reset(new cppdatalib::ubjson::parser(*infile.get()));
        else if (in_extension == "xml") input.reset(new cppdatalib::xml::parser(*infile.get(), false));
        else
        {
            std::cerr << "cppdatalib - unknown input file format\n";
            return 1;
        }
    }

    if (out_scheme.empty() || out_scheme == "file")
    {
        // Get output file extension
        size_t out_filetype = out.rfind('.');
        size_t out_path = out.rfind('/');

        if (out_filetype == out.npos || (out_path != out.npos && out_path > out_filetype))
        {
            std::cerr << "cppdatalib - output file has no type specified\n";
            return 1;
        }

        out_extension = out.substr(out_filetype + 1);
        if (out_scheme.size())
            out = out.substr(out_scheme.size() + 3);

        outfile.reset(new std::ofstream());
        outfile.get()->open(out, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);

        if (out_extension == "benc") output.reset(new cppdatalib::bencode::stream_writer(*outfile.get()));
        else if (out_extension == "binn") output.reset(new cppdatalib::binn::stream_writer(*outfile.get()));
        else if (out_extension == "bjson") output.reset(new cppdatalib::bjson::stream_writer(*outfile.get()));
        else if (out_extension == "csv") output.reset(new cppdatalib::csv::stream_writer(*outfile.get()));
        else if (out_extension == "tsv") output.reset(new cppdatalib::tsv::stream_writer(*outfile.get()));
        else if (out_extension == "json") output.reset(new cppdatalib::json::stream_writer(*outfile.get()));
        else if (out_extension == "mpk") output.reset(new cppdatalib::message_pack::stream_writer(*outfile.get()));
        else if (out_extension == "netstrings") output.reset(new cppdatalib::netstrings::stream_writer(*outfile.get()));
        else if (out_extension == "ubjson") output.reset(new cppdatalib::ubjson::stream_writer(*outfile.get()));
        else if (out_extension == "xlsx") output.reset(new cppdatalib::xml_xls::document_writer(*outfile.get(), "Worksheet 1"));
        else if (out_extension == "xml") output.reset(new cppdatalib::xml::document_writer(*outfile.get()));
        else
        {
            std::cerr << "cppdatalib - unknown output file format\n";
            return 1;
        }
    }

    try
    {
        cppdatalib::core::value v;
        *input.get() >> v;
        std::cout << "Estimated memory usage: " << std::flush;
        std::cout << v.memory_consumed() << std::endl;
        *output.get() << v;
        /*input.get()->begin(*output.get());
        do
        {
            input.get()->write_one();
        } while (input.get()->busy());
        input.get()->end();*/
    } catch (const cppdatalib::core::error &e) {
        std::cerr << "cppdatalib: " << e.what() << "\n";
        return 1;
    }
}
