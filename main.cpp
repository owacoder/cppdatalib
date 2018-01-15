#include <iostream>

//#include "adapters/stl.h"

#define CPPDATALIB_FAST_IO
//#define CPPDATALIB_DISABLE_WRITE_CHECKS
#define CPPDATALIB_DISABLE_FAST_IO_GCOUNT
#define CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
#define CPPDATALIB_ENABLE_BOOST_DYNAMIC_BITSET
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
    } catch (core::error e) {
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
    } catch (core::error e) {
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
    } catch (core::error e) {
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
    operator cppdatalib::core::value() const {return cppdatalib::core::object_t{{"x", bind.x}, {"y", bind.y}};}
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
        p.x = bind["x"];
        p.y = bind["y"];
        return p;
    }
};

int readme_simple_test4()
{
    using namespace cppdatalib;             // Parent namespace
    using namespace json;                   // Format namespace

    core::value my_value, m2;               // Global cross-format value class
    core::value_builder builder(my_value);

    m2 = std::array<int, 3>{{0, 1, 4}};
    std::array<int, 2> axx = m2;
    m2 = axx;

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
        m2 = stack;
        m2 = map;
        point p2 = point();
        p2.x = 9;
        m2 = p2;
        p2 = m2;
        p2.x += 1000;
        m2 = p2;
        w << m2;                // Write core::value out to STDOUT as JSON
    } catch (core::error e) {
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

// Simple insertion sort.
// Iterator must be bidirectional, but does not need to be random-access
// Elements must be less-than comparable and have swap(element, element) defined
template<typename Iterator>
void insertion_sort(Iterator begin, Iterator end)
{
    using namespace std;
    auto i = begin;

    if (begin == end)
        return;

    for (++i; i != end; ++i) {
        auto j = i;
        auto j_1 = j; // To be set to j - 1 later

        while (j != begin) {
            --j_1;
            if (!(*j_1 < *j))
                break;

            swap(*j_1, *j);
            --j;
        }
    }
}

// Simple insertion sort.
// Iterator must be bidirectional, but does not need to be random-access
// Elements must have swap(element, element) defined
// `compare` is used to sort elements
template<typename Iterator, typename Predicate>
void insertion_sort(Iterator begin, Iterator end, Predicate compare)
{
    using namespace std;
    auto i = begin;

    if (begin == end)
        return;

    for (++i; i != end; ++i) {
        auto j = i;
        auto j_1 = j; // To be set to j - 1 later

        while (j != begin) {
            --j_1;
            if (!compare(*j_1, *j))
                break;

            swap(*j_1, *j);
            --j;
        }
    }
}

// Simple selection sort.
// Iterator must be bidirectional, but does not need to be random-access
// Elements be less-than comparable and must have swap(element, element) defined
template<typename Iterator>
void selection_sort(Iterator begin, Iterator end)
{
    using namespace std;
    auto i = begin;
    auto last = end;

    if (begin == end)
        return;

    for (--last; i != last; ++i) {
        auto least = i;
        auto j = least;

        for (++j; j != end; ++j) {
            if (*j < *least)
                least = j;
        }

        if (i != least)
            swap(*i, *least);
    }
}

// Simple selection sort.
// Iterator must be bidirectional, but does not need to be random-access
// Elements must have swap(element, element) defined
// `compare` is used to sort elements
template<typename Iterator, typename Predicate>
void selection_sort(Iterator begin, Iterator end, Predicate compare)
{
    using namespace std;
    auto i = begin;
    auto last = end;

    if (begin == end)
        return;

    for (--last; i != last; ++i) {
        auto least = i;
        auto j = least;

        for (++j; j != end; ++j) {
            if (compare(*j, *least))
                least = j;
        }

        if (i != least)
            swap(*i, *least);
    }
}

/*
 *  Key must have operator<() and operator==() defined, as well as the addition and subtraction operators
 *  Key also must be castable to DiffType and size_t (casting to size_t will only ever occur if <value of key> <= SIZE_MAX)
 *
 *  T may be of any type, as long as it is copy-constructable, default-constructable, and assignable
 *
 *  An invariant held throughout this implementation is that each map entry has at least one value stored in the
 *  "bucket", or vector, there.
 *
 *  For (very) sparse arrays (those without contiguous sections, or very few of them and small), the operations are:
 *
 *      Random element retrieval: O(logn)
 *      Random element existence: O(logn)
 *      Random element assigning: O(logn)
 *      Insertion at beginning: O(logn)
 *      Insertion in middle: O(logn)
 *      Insertion at end: O(logn)
 *      Erasing at beginning: O(logn)
 *      Erasing in middle: O(logn)
 *      Erasing at end: O(logn)
 *      Iteration: slower
 *      Everything else: O(1)
 *
 *  For very packed sparse arrays (those with massive contiguous sections), the operations are:
 *
 *      Random element retrieval: approaches O(1)
 *      Random element existence: approaches O(1)
 *      Random element assigning: approaches O(1)
 *      Insertion at beginning: approaches O(n)
 *      Insertion in middle: approaches O(n)
 *      Insertion at end: approaches O(1)
 *      Erasing at beginning: approaches O(n)
 *      Erasing in middle: approaches O(n)
 *      Erasing at end: approaches O(1)
 *      Iteration: faster
 *      Everything else: O(1)
 *
 * As you can see, read performance improves and insert and erasure performance declines
 * as contiguous sections grow, with the exception of inserting and deleting at the end.
 * Write performance for existing elements also improves when contiguous sections grow.
 *
 * All elements are strictly ordered, allowing O(n) ascending traversal of array keys.
 *
 */
template<typename Key, typename T, typename DiffType = uintmax_t>
class sparse_array {
    friend class const_iterator;

    static_assert(std::is_unsigned<DiffType>::value | std::is_class<DiffType>::value,
                  "sparse_array::DiffType must be an unsigned type or a user-defined class");

    T mDefaultValue;
    size_t mElements;
    std::map<Key, std::vector<T>> mMap;
    bool mUseRangeIters;

    friend void TestSparseArray();

    typedef std::map<Key, std::vector<T>> map_type;
    typedef typename map_type::iterator map_iterator;
    typedef typename map_type::const_iterator map_const_iterator;

public:
    class const_iterator : std::iterator<std::bidirectional_iterator_tag, T, std::ptrdiff_t, const T *, const T &>
    {
        friend class sparse_array;

        const sparse_array *mParent; // sparse array instance
        map_const_iterator mMapIter; // current map entry
        Key mKey; // Currently referenced key. May be invalid and out-of-bounds

        bool mIsRangeIter; // Set to true if non-existent (default-valued) elements should be iterated over

        const_iterator(const sparse_array *parent, map_const_iterator it, bool encompassAllValues)
            : mParent(parent)
            , mMapIter(it)
            , mKey{}
            , mIsRangeIter(encompassAllValues)
        {
            if (it != mParent->mMap.end())
                mKey = mMapIter->first;
        }

        const_iterator(const sparse_array *parent, map_const_iterator it, Key key, bool encompassAllValues)
            : mParent(parent)
            , mMapIter(it)
            , mKey(key)
            , mIsRangeIter(encompassAllValues)
        {}

        void advance() {
            if (mParent == nullptr)
                return;

            if (mIsRangeIter) {
                // If not at end, then check if we need to increment the map entry pointer
                // Otherwise, if at end, just increment
                if (mMapIter != mParent->mMap.end())
                {
                    auto nextIter = mMapIter;
                    ++nextIter;

                    // If the next entry is the end, bump mMapIter to the end
                    // Increment the key no matter what
                    if (nextIter == mParent->mMap.end()) {
                        if (mKey == mParent->vector_last_used_end_idx(mMapIter))
                            ++mMapIter;

                        mKey = mKey + 1;
                    }
                    // Otherwise, are we ready to reset and move to next vector?
                    else if (mKey == nextIter->first)
                        mKey = (++mMapIter)->first;
                    // Otherwise, just increment
                    else
                        mKey = mKey + 1;
                }
                else
                    mKey = mKey + 1;
            }
            else
            {
                // If at end of map entry, then obtain next map entry's initial value
                // If there is no next map entry, just increment
                if (mMapIter != mParent->mMap.end() && mKey == mParent->vector_last_used_end_idx(mMapIter) && ++mMapIter != mParent->mMap.end())
                    mKey = mMapIter->first;
                else
                    mKey = mKey + 1;
            }
        }

        void retreat() {
            if (mParent == nullptr)
                return;

            // If map entry is at end, then we need to reset back to the last map entry's final value
            // If there is no map entry, just decrement
            if (mMapIter == mParent->mMap.end()) {
                if (mMapIter != mParent->mMap.begin())
                    mKey = mParent->vector_last_used_end_idx(--mMapIter);
                else
                    mKey = mKey - 1;
            }
            // If key is equal to initial value of map entry, attempt to obtain previous map entry
            // Otherwise, if at beginning map entry, or not at beginning of current map entry, just decrement
            else if (mKey == mMapIter->first && mMapIter != mParent->mMap.begin()) {
                --mMapIter;

                // If a ranged iterator, just decrement,
                // Otherwise, get the last used index of the new map entry
                if (mIsRangeIter)
                    mKey = mKey - 1;
                else
                    mKey = mParent->vector_last_used_end_idx(mMapIter);
            }
            else
                mKey = mKey - 1;
        }

        bool atEnd() const {
            return mParent == nullptr || mMapIter == mParent->mMap.end();
        }

    public:
        typedef typename std::iterator<std::bidirectional_iterator_tag, T, std::ptrdiff_t, const T *, const T &>::reference reference;
        typedef typename std::iterator<std::bidirectional_iterator_tag, T, std::ptrdiff_t, const T *, const T &>::pointer pointer;

        const_iterator() : mParent(nullptr), mKey{} {}

        const_iterator &operator++() {advance(); return *this;}
        const_iterator &operator--() {retreat(); return *this;}
        const_iterator operator++(int) {const_iterator temp(*this); advance(); return temp;}
        const_iterator operator--(int) {const_iterator temp(*this); retreat(); return temp;}

        reference operator*() const
        {
            if (mIsRangeIter && mParent->is_not_in_vector(mMapIter, mKey))
                return mParent->default_value();

            return mMapIter->second[static_cast<size_t>(mKey - mMapIter->first)];
        }
        pointer operator->() const
        {
            if (mIsRangeIter && mParent->is_not_in_vector(mMapIter, mKey))
                return std::addressof(mParent->default_value());

            return std::addressof(mMapIter->second[static_cast<size_t>(mKey - mMapIter->first)]);
        }

        bool operator==(const const_iterator &other) const {
            if (atEnd() && other.atEnd())
                return true;

            return mParent == other.mParent &&
                    mMapIter == other.mMapIter &&
                    mKey == other.mKey;
        }

        bool operator!=(const const_iterator &other) const {
            return !(*this == other);
        }

        bool element_does_not_exist() const {return atEnd() || (mIsRangeIter && mParent->is_not_in_vector(mMapIter, mKey));}
        bool element_exists() const {return !element_does_not_exist();}
        const Key &index() const {return mKey;}
    };

    typedef T value_type;

    sparse_array(const T &defaultValue, const std::vector<T> &args, bool iteratorsEncompassAllValuesInSpan = true)
        : mDefaultValue(defaultValue)
        , mElements(0)
        , mUseRangeIters(iteratorsEncompassAllValuesInSpan)
    {
        mMap.insert(std::make_pair(0, args));
    }

    sparse_array(const T &defaultValue, bool iteratorsEncompassAllValuesInSpan = true)
        : mDefaultValue(defaultValue)
        , mElements(0)
        , mUseRangeIters(iteratorsEncompassAllValuesInSpan)
    {}

    // Complexity: O(1), constant-time
    // Set whether to use contiguous iterators (contiguous iterators include non-existent entries that take on the default value) for this class instance
    bool contiguous_iterators() const {return mUseRangeIters;}
    void set_contiguous_iterators(bool useContiguous) {mUseRangeIters = useContiguous;}

    // Returns either contiguous_xxx() or skip_xxx(), depending on the current iterator mode
    const_iterator begin() const {return const_iterator(this, mMap.begin(), mUseRangeIters);}
    const_iterator cbegin() const {return const_iterator(this, mMap.begin(), mUseRangeIters);}
    const_iterator end() const {return const_iterator(this, mMap.end(), mUseRangeIters);}
    const_iterator cend() const {return const_iterator(this, mMap.end(), mUseRangeIters);}

    // Returns iterators that iterator all values in the span, including non-existent entries that take on the default value
    // The key value is accessible using <iterator>.index()
    // Use <iterator>.element_exists() or <iterator>.element_does_not_exist() to determine whether the entry actually exists in the array or not
    const_iterator contiguous_begin() const {return const_iterator(this, mMap.begin(), true);}
    const_iterator contiguous_cbegin() const {return const_iterator(this, mMap.begin(), true);}
    const_iterator contiguous_end() const {return const_iterator(this, mMap.end(), true);}
    const_iterator contiguous_cend() const {return const_iterator(this, mMap.end(), true);}

    // Returns iterator that iterator only the values entered in the array, excluding non-existent entries that take on the default value
    // The key value is accessible using <iterator>.index()
    // <iterator>.element_exists() should always return true for valid iterators obtained from these functions
    const_iterator skip_begin() const {return const_iterator(this, mMap.begin(), false);}
    const_iterator skip_cbegin() const {return const_iterator(this, mMap.begin(), false);}
    const_iterator skip_end() const {return const_iterator(this, mMap.end(), false);}
    const_iterator skip_cend() const {return const_iterator(this, mMap.end(), false);}

    // Complexity: best case is O(1) (when decayed to a simple vector), worst case is O(logn) (when every element is in its own bucket)
    // If using contiguous iterators, the desired key iterator will be returned if the key is within the current span, end() otherwise
    // If using skip iterators, the desired key iterator will only be returned if a value is found in the array, end() otherwise
    const_iterator iterator_at(Key idx) const {
        return mUseRangeIters? contiguous_iterator_at(idx): skip_iterator_at(idx);
    }
    const_iterator contiguous_iterator_at(Key idx) const {
        auto upper_bound = mMap.upper_bound(idx);
        if (upper_bound == mMap.begin())
            return end();

        --upper_bound;

        if (is_not_in_vector(upper_bound, idx) && upper_bound == --mMap.end())
            return end();

        return const_iterator(this, upper_bound, idx, true);
    }
    const_iterator skip_iterator_at(Key idx) const {
        auto upper_bound = mMap.upper_bound(idx);
        if (upper_bound == mMap.begin())
            return end();

        --upper_bound;

        if (is_not_in_vector(upper_bound, idx))
            return end();

        return const_iterator(this, upper_bound, idx, false);
    }

    // Complexity: O(n)
    void clear() {mMap.clear();}

    // Complexity: best case is O(1) (when decayed to a simple vector), worst case is O(logn) (when every element is in its own bucket)
    const T &operator[](Key idx) const {return at(idx);}
    const T &at(Key idx) const {
        // Find map entry past the desired index
        auto upper_bound = mMap.upper_bound(idx);
        if (upper_bound == mMap.begin())
            return mDefaultValue;

        // Find map entry the desired index should be in
        --upper_bound;

        // Check if desired index is in bounds
        if (is_not_in_vector(upper_bound, idx))
            return mDefaultValue;

        // Return element
        return upper_bound->second[idx - upper_bound->first];
    }

    // Complexity: best case is O(1) (when decayed to a simple vector), worst case is O(logn) (when every element is in its own bucket)
    bool contains(Key idx) const {return !skip_iterator_at(idx).atEnd();}

    // Complexity: best case is O(1) (when entire array is decayed to a vector and the element already exists),
    //             average case is O(logn) (when every element is in its own bucket),
    //             worst case is O(n) (when entire array is decayed to a vector and the element doesn't exist)
    T &operator[](Key idx) {return write(idx, T{});}
    T &write(Key idx, const T &item) {
        // Find map entry past the desired index
        auto upper_bound = mMap.upper_bound(idx);
        auto lower_bound = upper_bound;

        // Determine whether the new index requires a vector merge
        if (upper_bound != mMap.begin())
            --lower_bound;

        // If lower_bound == upper_bound, the new item should be the first map entry
        // Insert at start of vector or create a prior map entry
        if (lower_bound == upper_bound) {
            auto it = mMap.insert(upper_bound, std::make_pair(idx, std::vector<T>({item})));
            ++mElements;
            compact(it, upper_bound); // Compact to merge the two vectors together if necessary
            return it->second[0];
        }
        // If lower_bound != upper_bound, the new item will be inserted in a map entry or between map entries
        else {
            // If not in vector, insert as standalone map entry or append to previous
            if (is_not_in_vector(lower_bound, idx)) {
                // Append to end of previous vector?
                if (should_append_to_vector(lower_bound, idx)) {
                    lower_bound->second.push_back(item);
                    ++mElements;
                    compact(lower_bound, upper_bound);
                    return lower_bound->second[idx - vector_begin_idx(lower_bound)];
                }
                // Insert as standalone map entry
                else {
                    auto it = mMap.insert(upper_bound, std::make_pair(idx, std::vector<T>({item})));
                    ++mElements;
                    compact(it, upper_bound);
                    return it->second[0];
                }
            }
            // Otherwise, the desired element is in the vector, so assign and return it
            else
                return lower_bound->second[idx - vector_begin_idx(lower_bound)] = item;
        }
    }

    // Complexity: best case is O(1) (when decayed to a vector and the specified element is at the end)
    //             average case is O(logn) (when every element is in a bucket by itself)
    //             worst case is O(n) (when decayed to a vector and the specified element is at the beginning)
    void erase(Key key) {
        // Look for element beyond specified element
        auto upper_bound = mMap.upper_bound(key);

        // If the first element, key is not set in the array
        if (upper_bound == mMap.begin())
            return;

        // Get the element that key would be in, if it existed
        --upper_bound;

        // Check if key is in the entry vector
        if (is_not_in_vector(upper_bound, key))
            return;

        // Decrement number of contained elements
        --mElements;

        // If at the end of the vector, just remove it from the vector
        // Falls through to the next case if only one element is in the vector
        if (key == vector_last_used_end_idx(upper_bound) && upper_bound->second.size() > 1) {
            upper_bound->second.pop_back();
        }
        // If at the beginning of the vector, a map rebuild is necessary to fix the start key
        else if (key == vector_begin_idx(upper_bound)) {
            auto it = upper_bound;

            // Only move map entry if there will be something in the vector
            if (upper_bound->second.size() > 1)
            {
                // Move existing vector to new vector
                std::vector<T> vec(std::move(upper_bound->second));
                // Erase beginning of new vector (getting rid of the specified key)
                vec.erase(vec.begin());
                // Insert new vector as a new map entry
                mMap.insert(++upper_bound, std::make_pair(key + 1, vec));
            }
            // Erase the existing (now invalid) map entry
            mMap.erase(it);
        }
        // Otherwise, split the vector into two map entries, removing the specified key
        else {
            size_t pivot = key - vector_begin_idx(upper_bound);

            std::vector<T> vec;
            // Copy end of existing vector into new vector
            std::move(upper_bound->second.begin() + pivot + 1, upper_bound->second.end(), std::back_inserter(vec));
            // Erase end of existing vector
            upper_bound->second.erase(upper_bound->second.begin() + pivot, upper_bound->second.end());
            // Insert new vector as a new map entry
            mMap.insert(++upper_bound, std::make_pair(key + 1, vec));
        }
    }

    // Complexity: O(1), constant-time
    const T &default_value() const {
        return mDefaultValue;
    }

    // Complexity: O(1), constant-time
    T &set_default_value(const T &value) {
        return mDefaultValue = value;
    }

    // Complexity: O(1), constant-time
    bool empty() const {return mMap.empty();}

    // Complexity: O(1), constant-time
    Key span_begin() const {
        if (empty())
            return Key{};

        return vector_begin_idx(mMap.begin());
    }

    // Complexity: O(1), constant-time
    // The key returned is the last actually used key, not one past the end
    Key span_end() const {
        if (empty())
            return Key{};

        return vector_last_used_end_idx(--mMap.end());
    }

    // Complexity: O(1), constant-time
    // Returns the number of elements this sparse array encompasses (even if they contain the default value)
    // If empty() returns false and this function returns 0, overflow has occured and the span is actually
    // one more than the maximum representable integer value. Keep this in mind when using span() on large key types.
    DiffType span() const {
        if (empty())
            return 0;

        Key first = vector_begin_idx(mMap.begin());
        Key last = vector_last_used_end_idx(--mMap.end());

        return static_cast<DiffType>(last) - static_cast<DiffType>(first) + 1;
    }

    // Complexity: O(1), constant-time
    // Returns true if the given index falls within the currently contained key span
    bool span_contains(Key idx) const {
        if (empty())
            return false;

        return !((idx < vector_begin_idx(mMap.begin())) || vector_last_used_end_idx(--mMap.end()) < idx);
    }

    // Complexity: O(1), constant-time
    // Returns the number of elements in this array
    // Note that though the span can cover the entire range of representable integers (or even more, depending on the key type),
    // the number of storable elements is limited to the range of size_t, no more
    size_t elements() const {return mElements;}
    size_t size() const {return mElements;}

    // Complexity: O(1), constant-time
    // Returns the number of runs (contiguous sections) in this array
    size_t runs() const {return mMap.size();}

    // Complexity: best-cast O(1), worst-case O(n)
    // Returns true if the two sparse arrays are equal, including the default element value
    template<typename K, typename V>
    friend bool operator==(const sparse_array<K, V> &lhs, const sparse_array<K, V> &rhs);

private:
    Key vector_begin_idx(map_const_iterator it) const {
        return it->first;
    }

    Key vector_end_idx(map_const_iterator it) const {
        return it->first + it->second.size();
    }

    Key vector_last_used_end_idx(map_const_iterator it) const {
        return it->first + (it->second.size() - 1);
    }

    bool is_not_in_vector(map_const_iterator it, Key key) const {
        return (key < vector_begin_idx(it) || vector_last_used_end_idx(it) < key);
    }

    bool should_append_to_vector(map_const_iterator it, Key key) const {
        return (vector_last_used_end_idx(it) < key && vector_end_idx(it) == key);
    }

    void compact(map_iterator first,
                 map_iterator second) {
        if (first == second || second == mMap.end() || !(vector_end_idx(first) == vector_begin_idx(second)))
            return;

        first->second.insert(first->second.end(), second->second.begin(), second->second.end());
        mMap.erase(second);
    }
};

template<typename Key, typename T>
bool operator==(const sparse_array<Key, T> &lhs, const sparse_array<Key, T> &rhs)
{
    return lhs.mDefaultValue == rhs.mDefaultValue && lhs.mMap == rhs.mMap;
}

template<typename Key, typename T>
bool operator!=(const sparse_array<Key, T> &lhs, const sparse_array<Key, T> &rhs)
{
    return !(lhs == rhs);
}

void TestSparseArray() {
    typedef signed long keytype;

    //map[0] = std::vector<int>{0, 8, 2, 0};
    //map[9] = std::vector<int>{7};

    sparse_array<keytype, std::string, uintmax_t> arr("");

    arr[std::numeric_limits<keytype>::lowest()] = "str";
    arr.write(std::numeric_limits<keytype>::max(), "str2");
    arr.write(0, "0");
    /*arr.write(1, "8");
    arr.write(2, "2");
    arr.write(3, "0");*/

    arr.write(-1, "-1");

    arr.write(-20, "-5");
    arr.write(-19, "-4");

    std::cout << "span = " << arr.span() << " elements\n";

    for (auto it = arr.mMap.begin(); it != arr.mMap.end(); ++it)
    {
        std::cout << "vector[" << it->first << "] = { ";
        for (size_t i = 0; i < it->second.size(); ++i)
            std::cout << it->second[i] << " ";
        std::cout << "}" << std::endl;
    }

    arr.erase(std::numeric_limits<keytype>::max());
    //arr.erase(0);
    std::cout << "Changed.\n";

    std::cout << "span = " << arr.span() << " elements\n";

    for (auto it = arr.mMap.begin(); it != arr.mMap.end(); ++it)
    {
        std::cout << "vector[" << it->first << "] = { ";
        for (size_t i = 0; i < it->second.size(); ++i)
            std::cout << it->second[i] << " ";
        std::cout << "}" << std::endl;
    }

    arr.set_contiguous_iterators(true);
    for (auto it = arr.span_begin(); it != arr.span_end(); ++it)
    {
#if 0
        if (it % 10000000 == 0)
            std::cout << "element at " << it.index() << " = " << *it << std::endl;
#else
        if ((it & ((1 << 23) - 1)) == 0)
            std::cout << "element at " << it << std::endl;
#endif
    }
}

#ifdef CPPDATALIB_ENABLE_MYSQL
void test_sql()
{
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
    catch (cppdatalib::core::error e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
}
#endif

#include "experimental/decision_trees.h"

int main()
{
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
        data.push_back(cppdatalib::core::object_t{{"size", "M"}, {"shape", "pillar"}, {"color", "red"}});
        results.push_back(true);

        auto tree = cppdatalib::experimental::make_decision_tree(data.get_array(), cppdatalib::core::array_t{"size", "shape", "color"}, results.get_array());
        std::cout << tree << std::endl;

        std::cout << "looking for {size: L, shape: pillar, color: blue} in tree: " << std::endl;
        std::cout << cppdatalib::experimental::test_decision_tree(tree, cppdatalib::core::object_t{{"size", "L"}, {"shape", "pillar"}, {"color", "blue"}}) << std::endl;
    } catch (cppdatalib::core::error e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;

    using namespace cppdatalib;
    using namespace json;
    boost::dynamic_bitset<> b;

    cppdatalib::core::value yx = b;
    yx = R"({"my string":""})"_json;

    std::tuple<unsigned int, const char *> myvec{44, "hello"};
    cppdatalib::core::dump::stream_writer(std::cout, 2) <<
    cppdatalib::core::generic_parser(myvec);
    std::cout << std::endl;
    std::cout << yx << std::endl;
    b = yx;



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

    sorter << cppdatalib::core::array_t{2, 0, 1, 2, 3, 4, 4};

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
    catch (cppdatalib::core::error e)
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
