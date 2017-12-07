#include <iostream>

//#define CPPDATALIB_FAST_IO
#define CPPDATALIB_DISABLE_WRITE_CHECKS
#define CPPDATALIB_DISABLE_FAST_IO_GCOUNT
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

using namespace cppdatalib;

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
    size_t failed = 0;
    std::cout << "Testing " << name << "... " << std::flush;

    for (const auto &test: tests)
        if (compare(test.second, actual(test.first)))
        {
            std::cout << vt.red;
            std::cout << "FAILED!\n";
            std::cout << "\tInput: " << test.first << "\n";
            std::cout << "\tExpected output: " << test.second << "\n";
            std::cout << "\tActual output: " << actual(test.first) << std::endl;
            std::cout << vt.black;
            if (bail_early)
                return false;
            ++failed;
        }

    if (!failed)
    {
        std::cout << vt.green;
        std::cout << "done." << std::endl;
        std::cout << vt.black;
    }
    else
    {
        std::cout << vt.red;
        std::cout << "Test done. (" << failed << " failed out of " << tests.size() << ")" << std::endl;
        std::cout << vt.black;
    }

    return failed;
}

// `tests` is a vector of test cases. The test cases will be reversed (first item in tuple is result, second is parameter)
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<typename F, typename S, typename ResultCode, typename Compare = std::not_equal_to<S>>
bool ReverseTest(const char *name, const TestData<F, S> &tests, ResultCode actual, bool bail_early = true, Compare compare = Compare())
{
    vt100 vt;
    uintmax_t failed = 0;
    std::cout << "Testing " << name << "... " << std::flush;

    for (const auto &test: tests)
        if (compare(test.first, actual(test.second)))
        {
            std::cout << vt.red;
            std::cout << "FAILED!\n";
            std::cout << "\tInput: " << test.second << "\n";
            std::cout << "\tExpected output: " << test.first << "\n";
            std::cout << "\tActual output: " << actual(test.second) << std::endl;
            std::cout << vt.black;
            if (bail_early)
                return false;
            ++failed;
        }

    if (!failed)
    {
        std::cout << vt.green;
        std::cout << "done." << std::endl;
        std::cout << vt.black;
    }
    else
    {
        std::cout << vt.red;
        std::cout << "Test done. (" << failed << " failed out of " << tests.size() << ")" << std::endl;
        std::cout << vt.black;
    }

    return failed;
}

// `tests` is the maximum of a range, 0 -> tests, each element of which is passed to the test code
// ExpectedCode is a functor that should return the expected, valid, test result
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<uintmax_t, typename ExpectedOutput, typename ResultCode, typename Compare>
bool TestRange(const char *name, uintmax_t tests, ExpectedOutput expected, ResultCode actual, bool bail_early = true, Compare compare = Compare())
{
    vt100 vt;
    uintmax_t failed = 0;
    std::cout << "Testing " << name << "... " << std::flush;

    for (uintmax_t test = 0; test < tests; ++test)
        if (compare(expected(test), actual(test)))
        {
            std::cout << vt.red;
            std::cout << "FAILED!\n";
            std::cout << "\tInput: " << test << "\n";
            std::cout << "\tExpected output: " << expected(test) << "\n";
            std::cout << "\tActual output: " << actual(test) << std::endl;
            std::cout << vt.black;
            if (bail_early)
                return false;
            ++failed;
        }

    if (!failed)
    {
        std::cout << vt.green;
        std::cout << "done." << std::endl;
        std::cout << vt.black;
    }
    else
    {
        std::cout << vt.red;
        std::cout << "Test done. (" << failed << " failed out of " << tests << ")" << std::endl;
        std::cout << vt.black;
    }

    return failed;
}

// `tests` is the maximum of a range, 0 -> tests, each element of which is passed to the test code
// ExpectedCode is a functor that should return the expected, valid, test result
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<typename ExpectedCode, typename ResultCode, typename Compare>
bool TestRangeAndShowProgress(const char *name, uintmax_t tests, ExpectedCode expected, ResultCode actual, bool bail_early = true, Compare compare = Compare())
{
    vt100 vt;
    uintmax_t failed = 0;
    std::cout << "Testing " << name << "... " << vt.yellow << "0%" << std::flush;
    std::cout << vt.black;
    size_t percent = 0;

    for (uintmax_t test = 0; test < tests; ++test)
    {
        if (compare(expected(test), actual(test)))
        {
            std::cout << vt.erase_line << vt.move_cursor_home << vt.black;
            std::cout << "Testing " << name << "... " << std::flush;
            std::cout << vt.red << "FAILED!\n";
            std::cout << "\tInput: " << test << "\n";
            std::cout << "\tExpected output: " << expected(test) << "\n";
            std::cout << "\tActual output: " << actual(test) << std::endl;
            std::cout << vt.black;
            if (bail_early)
                return false;
            ++failed;
        }

        if (test * 100 / tests > percent)
        {
            percent = test * 100 / tests;
            std::cout << vt.erase_line << vt.move_cursor_home << vt.black;
            std::cout << "Testing " << name << "... " << vt.yellow << percent << "%" << std::flush;
            std::cout << vt.black;
        }
    }

    if (!failed)
    {
        std::cout << vt.erase_line << vt.move_cursor_home << vt.black;
        std::cout << "Testing " << name << "... " << vt.green << "done." << std::endl;
        std::cout << vt.black;
    }
    else
    {
        std::cout << vt.erase_line << vt.move_cursor_home << vt.black;
        std::cout << "Testing " << name << "... " << vt.red << "done. (" << failed << " failed out of " << tests << ")" << std::endl;
        std::cout << vt.black;
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
    {"5.0e-1", "0.5"},
    {"{\"key\":\"value\",\"key2\":null}", "{\"key\":\"value\",\"key2\":null}"},
    {"[null,true,false,-5,\"\"]", "[null,true,false,-5,\"\"]"}
};

int main()
{
    vt100 vt;
    std::cout << vt.attr_bright;
    Test("base64_encode", base64_encode_tests, base64::encode);
    ReverseTest("base64_decode", base64_encode_tests, base64::decode);
    Test("debug_hex_encode", debug_hex_encode_tests, hex::debug_encode);
    Test("hex_encode", hex_encode_tests, hex::encode);
#if 0
    TestRangeAndShowProgress("float_from_ieee_754", UINT32_MAX, core::float_cast_from_ieee_754,
                             core::float_from_ieee_754, true, [](const auto &f, const auto &s){return f != s && !isnan(f) && !isnan(s);});
    TestRangeAndShowProgress("float_to_ieee_754", UINT32_MAX, [](const auto &f){return f;},
                             [](const auto &f){return core::float_to_ieee_754(core::float_cast_from_ieee_754(f));}, true,
                             [](const auto &f, const auto &s){return f != s && !isnan(core::float_from_ieee_754(f)) && !isnan(core::float_from_ieee_754(s));});
#endif
    Test("JSON", json_tests, [](const auto &test){return json::to_json(json::from_json(test));}, false);
    std::cout << vt.attr_reset;
    return 0;
}
