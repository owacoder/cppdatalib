//#include <iostream>

//#include "adapters/stl.h"

/*
#ifndef CPPDATALIB_ENABLE_ATTRIBUTES
#define CPPDATALIB_ENABLE_ATTRIBUTES
#endif*/
#ifndef CPPDATALIB_ENABLE_XML
#define CPPDATALIB_ENABLE_XML
#endif
#define CPPDATALIB_ENABLE_FILESYSTEM
//#define CPPDATALIB_ENABLE_FAST_IO
//#define CPPDATALIB_DISABLE_WRITE_CHECKS
#define CPPDATALIB_DISABLE_FAST_IO_GCOUNT
#define CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
//#define CPPDATALIB_ENABLE_BOOST_DYNAMIC_BITSET
#define CPPDATALIB_DISABLE_WEAK_POINTER_CONVERSIONS
//#define CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
//#define CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS
//#define CPPDATALIB_THROW_IF_WRONG_TYPE
//#undef CPPDATALIB_ENABLE_ATTRIBUTES
#undef CPPDATALIB_ENABLE_MYSQL

#include "cppdatalib.h"
#include "test_framework.h"

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
        //using namespace cppdatalib::xml;

        cppdatalib::json::stream_writer writer(std::cout);

        writer.begin();
        writer.begin_array(cppdatalib::core::array_t(), 11 /*cppdatalib::core::stream_handler::unknown_size()*/);
        //writer.write_out_of_order(1000000, 100);
        /*
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
        writer.write_out_of_order(10, 9);*/
        writer.end_array(cppdatalib::core::array_t());
        writer.end();

        return;

        cppdatalib::http::parser(cppdatalib::core::value("http://owacoder.com"),
                                 cppdatalib::core::default_network_library,
                                 "GET",
                                 cppdatalib::core::istream_handle(),
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

//#include <qtapp.h>

int rmain(int argc, char **argv)
{
    (void) argc, (void) argv;

    //return readme_simple_test4();

    //test_mmap();

    //return qtapp_main(argc, argv, test_attributes);

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

    srand((unsigned int) std::time(NULL));

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

#ifdef CPPDATALIB_MSVC
	system("pause");
#endif
    return 0;
}

#include <qtapp.h>

void test_http()
{
    cppdatalib::http::http_initialize();

    cppdatalib::core::ostringstream stream;
    cppdatalib::json::stream_writer(stream) << cppdatalib::core::object_t{{"new object type", "not true"}, {"graduated sequence", 4412887}};
    std::cout << cppdatalib::http::parser("http://google.com");

    cppdatalib::http::http_deinitialize();
}

class JSBytecodeInterpreter
{
    typedef void (*ErrorCallback)(const std::string &);
    ErrorCallback err;

    // Arguments are pushed on the stack in the order they are encountered (first argument is deepest on the stack)
    // The return value should be left on the top of the stack, even if it is unused (it should be set to undefined then)
    typedef void (*ExternalFunction)(cppdatalib::core::value &stack,
                                     cppdatalib::core::value &callStack);

    static void Object_prototype_valueOf(cppdatalib::core::value &stack, cppdatalib::core::value &callStack)
    {

    }

    cppdatalib::core::value createObjectPrototype()
    {
        return cppdatalib::core::object_t{};
    }

    cppdatalib::core::value createObject()
    {
        auto prototype = newHeapReference(createObjectPrototype());
        return cppdatalib::core::object_t{
            {{}, prototype},
            {"prototype", prototype}
        };
    }

    cppdatalib::core::value createFunctionPrototype()
    {
        auto prototype = newHeapReference(createObject());
        return cppdatalib::core::object_t{
            {{}, prototype},
            {"prototype", prototype}
        };
    }

    cppdatalib::core::value createFunction(ExternalFunction func, const cppdatalib::core::array_t &args)
    {
        auto prototype = newHeapReference(createFunctionPrototype());
        return cppdatalib::core::object_t{
            {{}, prototype},
            {"prototype", prototype},
            {"originalBody", {}},
            {"body", {}},
            {"arguments", args},
            {"external", reinterpret_cast<uint64_t>(func)}
        };
    }

    cppdatalib::core::value createFunction(std::string body, std::string originalBody, const cppdatalib::core::array_t &args)
    {
        auto prototype = newHeapReference(createFunctionPrototype());
        return cppdatalib::core::object_t{
            {{}, prototype},
            {"prototype", prototype},
            {"originalBody", originalBody},
            {"body", body},
            {"arguments", args},
            {"external", 0u}
        };
    }

    std::string bytecode;

public:
    JSBytecodeInterpreter(const std::string &bytecode = "", ErrorCallback err = nullptr) : err(err), bytecode(bytecode)
    {
        initialize();
    }

    void setBytecode(const std::string &bytecode)
    {
        this->bytecode = bytecode;
        stream.str(bytecode);
        stream.seekg(0);
        while (callStack.array_size() > 1)
            callStack.erase_element(callStack.array_size() - 1);
    }

    cppdatalib::core::value run()
    {
        char c;

        stream.seekg(0);
        while (c = stream.get(), stream)
        {
            switch (c)
            {
                case 0x20: // ' ', print contents of stack
                {
                    auto stack = this->stack;

                    while (stack.size())
                    {
                        std::cout << stack.size() << ": " << stack.element(stack.size()-1) << "\n";
                        stack.erase_element(stack.size() - 1);
                    }
                    break;
                }
                case 'c': // 'c', print contents of call stack
                {
                    std::cout << callStack << std::endl;
                    break;
                }
                case '!': // '!', place following JSON value on stack
                {
                    cppdatalib::core::value value;
                    value << cppdatalib::json::parser(stream);
                    push(newHeapReference(std::move(value)));
                    break;
                }
                case 0x2A: // '*', multiply top two numbers on stack
                {
                    if (stack.size() < 2)
                    {
                        if (err)
                            err("JavaScript: Invalid arguments to '*' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    auto &first = stackFromTop(1).deref_all_links();
                    auto &second = stackFromTop(0).deref_all_links();
                    auto result = as_real(first) * as_real(second);
                    pop();
                    pop();
                    push(newHeapReference({result}));

                    break;
                }
                case 0x2B: // '+', add top two numbers on stack or concatenate top two strings
                {
                    if (stack.size() < 2)
                    {
                        if (err)
                            err("JavaScript: Invalid arguments to '+' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    auto &first = stackFromTop(1).deref_all_links();
                    auto &second = stackFromTop(0).deref_all_links();
                    if (first.get_type() == cppdatalib::core::string ||
                            second.get_type() == cppdatalib::core::string)
                    {
                        auto result = as_string(first) + as_string(second);
                        pop();
                        pop();
                        push(newHeapReference({result}));
                    }
                    else
                    {
                        auto result = first.get_real_unchecked() + second.get_real_unchecked();
                        pop();
                        pop();
                        push(newHeapReference({result}));
                    }

                    break;
                }
                case 0x2D: // '-', subtract top two numbers on stack
                {
                    if (stack.size() < 2)
                    {
                        if (err)
                            err("JavaScript: Invalid arguments to '-' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    auto &first = stackFromTop(1).deref_all_links();
                    auto &second = stackFromTop(0).deref_all_links();
                    auto result = as_real(first) + as_real(second);
                    pop();
                    pop();
                    push(newHeapReference({result}));

                    break;
                }
                case 0x2E: // '.', lookup member of object (second item on stack) with name (held at top of stack) and place reference on top of stack (pops name and object from stack)
                {
                    if (stack.size() < 2)
                    {
                        if (err)
                            err("JavaScript: Invalid arguments to 'member' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    auto &obj = stackFromTop(1).deref_all_links();
                    auto &name = stackFromTop(0).deref_all_links();

                    auto ref = findMember(obj, name);
                    if (ref.is_null())
                        ref = cppdatalib::core::value(&(obj.member(name) = newHeapReference(cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined)))); // Make own member in object (TODO: disregards getters and setters of prototypes at the moment)

                    if (stackFromTop(1).is_link() && !stackFromTop(1).is_strong_link()) // If we still will have access to the object...
                    {
                        pop();
                        pop();
                        push(std::move(ref)); // Push the member reference to the stack
                    }
                    else
                    {
                        if (err)
                            err("JavaScript: Invalid arguments to 'member' operator; the object has gone out of scope");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }
                    break;
                }
                case 0x2F: // '/', divide top two numbers on stack
                {
                    if (stack.size() < 2)
                    {
                        if (err)
                            err("JavaScript: Invalid arguments to '/' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    auto &first = stackFromTop(1).deref_all_links();
                    auto &second = stackFromTop(0).deref_all_links();
                    auto result = as_real(first) / as_real(second);
                    pop();
                    pop();
                    push(newHeapReference({result}));

                    break;
                }
                case 0x3D: // '=', assign top value on stack to second value on stack and pop second value from stack
                {
                    if (stack.size() < 2)
                    {
                        if (err)
                            err("JavaScript: Invalid arguments to '=' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    auto &first = stackFromTop(1).deref_all_links();
                    auto &second = stackFromTop(0).deref_to_depth(stackFromTop(0).link_depth()-1);

                    first = second;
                    pop();

                    break;
                }
                case 0x64: // 'd', define function (stack contains "...args, body, originalBody, numberOfArgs" with "numberOfArgs" on top) and pop all relevant values, pushing the resulting function object reference onto the stack
                {
                    if (stack.size() < 3)
                    {
                        if (err)
                            err("JavaScript: Invalid argument to 'define' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    auto numberOfArgs = stackFromTop(0).deref_all_links().as_uint();
                    auto originalBody = stackFromTop(1).deref_all_links().get_string();
                    auto body = stackFromTop(2).deref_all_links().get_string();

                    pop();
                    pop();
                    pop();

                    cppdatalib::core::array_t args;
                    while (numberOfArgs--)
                    {
                        args.data().insert(args.data().begin(), stackFromTop(0).deref_all_links());
                        pop();
                    }

                    push(newHeapReference(createFunction(static_cast<std::string>(body), static_cast<std::string>(originalBody), args)));
                    break;
                }
                case 0x67: // 'g', lookup variable (with name held at top of stack) and place reference on top of stack (pops name from stack)
                {
                    if (stack.size() < 1)
                    {
                        if (err)
                            err("JavaScript: Invalid argument to 'lookup' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    auto ref = findVariable(stackFromTop(0));
                    pop();
                    push(std::move(ref));
                    break;
                }
                case 0x6C: // 'l', declare variable (with name held at top of stack) in current local scope (pops name from stack)
                {
                    if (stack.size() < 1)
                    {
                        if (err)
                            err("JavaScript: Invalid argument to 'let' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    cppdatalib::core::value ref = newHeapReference(cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined));
                    currentScope()->member("variables").member(stackFromTop(0).deref_all_links()) = ref;
                    pop();
                    push(std::move(ref));
                    break;
                }
                case 0x6E: // 'n', push null onto stack
                {
                    push(newHeapReference({}));
                    break;
                }
                case 0x70: // 'p', pop from stack
                {
                    if (stack.size() < 1)
                    {
                        if (err)
                            err("JavaScript: Invalid argument to 'pop' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    pop();
                    break;
                }
                case 0x72: // 'r', return value from function (undefined value must be on stack already if needed)
                {
                    if (stack.size() < 1)
                    {
                        if (err)
                            err("JavaScript: Invalid argument to 'return' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    cppdatalib::core::value v = stackFromTop(0);
                    auto scope = currentFunctionScope();
                    if (scope->const_member("type") == cppdatalib::core::value("global", cppdatalib::core::domain_comparable))
                        return v.deref_all_links();
                    moveToScope(parentScopeOf(currentFunctionScope()));
                    push(newHeapReference(std::move(v)));
                    break;
                }
                case 0x75: // 'u', push undefined onto stack
                {
                    push(newHeapReference(cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined)));
                    break;
                }
                case 0x76: // 'v', declare variable (with name held at top of stack) in current function scope (pops name from stack)
                {
                    if (stack.size() < 1)
                    {
                        if (err)
                            err("JavaScript: Invalid argument to 'var' operator");
                        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
                    }

                    cppdatalib::core::value ref = newHeapReference(cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined));
                    currentFunctionScope()->member("variables").member(stackFromTop(0).deref_all_links()) = ref;
                    pop();
                    push(std::move(ref));
                    break;
                }
            }
        }

        return cppdatalib::core::value(cppdatalib::core::null_t(), cppdatalib::core::undefined);
    }

private:
    void initialize()
    {
        stack.set_null();
        callStack.set_null();
        heap.clear();
        stream.str(bytecode);

        addScope(cppdatalib::core::object_t{
                     {"body", bytecode},
                     {"variables",
                      cppdatalib::core::object_t{
                       {"Object", newHeapReference(cppdatalib::core::object_t{
                        {"valueOf", newHeapReference(createFunction(Object_prototype_valueOf, {}))},
                       })}
                      }
                     },
                     {"type", "global"}
                 });

        garbageCollect();
    }

    void finalize()
    {
        garbageCollect();
    }

    void garbageCollect()
    {

    }

    cppdatalib::core::value &stackFromTop(size_t offset) {return stack.element(stack.array_size() - 1 - offset);}
    cppdatalib::core::value &push(cppdatalib::core::value &&v) {stack.push_back(std::move(v)); return stack.element(stack.array_size()-1);}
    cppdatalib::core::value &pushNew() {stack.push_back({}); return stack.element(stack.array_size()-1);}
    void pop() {stack.erase_element(stack.array_size() - 1);}

    double as_real(const cppdatalib::core::value &value)
    {
        auto &ref = value.deref_all_links();

        switch (ref.get_type())
        {
            case cppdatalib::core::null: return ref.get_subtype() != cppdatalib::core::undefined? 0: NAN;
            case cppdatalib::core::boolean: return ref.get_bool_unchecked();
            case cppdatalib::core::integer: return ref.get_int_unchecked();
            case cppdatalib::core::uinteger: return ref.get_uint_unchecked();
            case cppdatalib::core::real: return ref.get_real_unchecked();
            case cppdatalib::core::string:
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            case cppdatalib::core::temporary_string:
#endif
            {
                size_t pos = ref.get_string_unchecked().find_first_not_of(" \t\n\r");

                if (ref.get_string_unchecked().empty())
                    return 0;
                else if (pos == std::string::npos || !isdigit(ref.get_string_unchecked()[pos]) ||
                         ref.get_string_unchecked().find_first_not_of("0123456789eE.+-") != std::string::npos)
                    return NAN;
                else
                {
                    char *after;
                    double val = cppdatalib::core::fp_from_in_string<double>(ref.get_string_unchecked().begin(), ref.get_string_unchecked().end(), &after);

                    if (after != ref.get_string_unchecked().end())
                        return NAN;
                    return val;
                }
            }
            case cppdatalib::core::array:
                if (ref.get_array_unchecked().empty())
                    return 0;
                else if (ref.get_array_unchecked()[0].get_type() == cppdatalib::core::real)
                    return ref.get_array_unchecked()[0].get_real_unchecked();
                else
                    return NAN;
            case cppdatalib::core::object:
                return NAN;
        }

        return NAN;
    }

    std::string as_string(const cppdatalib::core::value &value)
    {
        auto &ref = value.deref_all_links();

        switch (ref.get_type())
        {
            case cppdatalib::core::null: return ref.get_subtype() != cppdatalib::core::undefined? "null": "undefined";
            case cppdatalib::core::boolean: return ref.get_bool_unchecked()? "true": "false";
            case cppdatalib::core::integer:
            case cppdatalib::core::uinteger: return ref.as_string();
            case cppdatalib::core::real: return std::isnan(ref.get_real_unchecked())? "NaN": std::isinf(ref.get_real_unchecked())? ref.get_real_unchecked() < 0? "-Infinity": "Infinity": ref.as_string();
            case cppdatalib::core::string:
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            case cppdatalib::core::temporary_string:
#endif
                return static_cast<std::string>(ref.get_string_unchecked());
            case cppdatalib::core::array:
            {
                std::string result;
                uint64_t ctr = 0;
                for (const auto &ref: ref.get_array_unchecked())
                {
                    if (ctr++)
                        result.push_back(',');
                    result += as_string(ref);
                }
                return result;
            }
            case cppdatalib::core::object:
                return "[object Object]";
        }

        return "";
    }

    bool as_bool(const cppdatalib::core::value &value)
    {
        auto &ref = value.deref_all_links();

        switch (ref.get_type())
        {
            case cppdatalib::core::null: return false;
            case cppdatalib::core::boolean: return ref.get_bool_unchecked();
            case cppdatalib::core::integer: return ref.get_int_unchecked() != 0;
            case cppdatalib::core::uinteger: return ref.get_uint_unchecked() != 0;
            case cppdatalib::core::real: return std::isnan(ref.get_real_unchecked())? false: ref.get_real_unchecked() != 0;
            case cppdatalib::core::string:
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            case cppdatalib::core::temporary_string:
#endif
                return ref.get_string_unchecked().size();
            case cppdatalib::core::array: return true;
            case cppdatalib::core::object: return true;
        }

        return false;
    }

    /*
     * Memory management (TODO: newHeapReference needs some work!??!)
     */
    cppdatalib::core::value newHeapReference(cppdatalib::core::value &&value)
    {
        heap.push_back(std::move(value)); // Create value on heap
        return cppdatalib::core::value(&heap.back()); // Return weak link to it
    }

    // Returns a reference to the current non-block scope (could be the global scope)
    cppdatalib::core::value currentNonBlockScope()
    {
        for (size_t i = callStack.array_size(); i > 0; --i)
            if (callStack[i-1].const_member("type") != cppdatalib::core::value("block", cppdatalib::core::domain_comparable))
                return cppdatalib::core::value(&callStack.element(i-1));

        // Never reached (callStack must always have at least one entry)
        return {};
    }

    // Returns a reference to the current function scope, or global scope if not currently in a function
    cppdatalib::core::value currentFunctionScope()
    {
        for (size_t i = callStack.array_size(); i > 0; --i)
            if (callStack[i-1].const_member("type") == cppdatalib::core::value("function", cppdatalib::core::domain_comparable))
                return cppdatalib::core::value(&callStack.element(i-1));

        return cppdatalib::core::value(&callStack.element(0));
    }

    cppdatalib::core::value parentScopeOf(const cppdatalib::core::value &scope)
    {
        auto &ref = scope.deref_all_links();

        size_t idx = ref.const_member("callStackHeightAtEntry").as_uint();

        if (idx == 0)
            return scope;
        else
            return cppdatalib::core::value(&callStack.element(idx-1));
    }

    // Returns a reference to the current scope
    cppdatalib::core::value currentScope()
    {
        return cppdatalib::core::value(&callStack.element(callStack.array_size()-1));
    }

    // Enables the provided scope as the current scope
    void moveToScope(const cppdatalib::core::value &scope)
    {
        auto &ref = scope.deref_all_links();

        size_t callstackHeight = ref.const_member("callStackHeightAtEntry").as_uint() + 1;
        while (callStack.array_size() > callstackHeight)
            callStack.erase_element(callStack.array_size() - 1);

        size_t stackHeight = ref.const_member("stackHeight").as_uint();
        while (stack.array_size() > stackHeight)
            stack.erase_element(stack.array_size() - 1);

        // TODO: does this error-check properly?
        if (stack.array_size() < stackHeight) // Something strange happened in the function call
        {
            if (err)
                err("JavaScript - Too many stack entries popped off in call scope, undefined behavior may result");
        }

        stream.str(static_cast<std::string>(ref.const_member("body").get_string()));
        stream.seekg(ref.const_member("streamPosition").as_int());
    }

    // Add scope to the call stack
    void addScope(const cppdatalib::core::value &newScope,
                  const cppdatalib::core::value &thisRef = {},
                  const cppdatalib::core::array_t &args = {})
    {
        auto &ref = newScope.deref_all_links();

        if (callStack.array_size())
        {
            auto &current = currentScope().deref_all_links();

            current["stackHeight"] = stack.array_size();
            current["streamPosition"] = stream.tellg();
        }

        callStack.push_back(cppdatalib::core::object_t{
                                {"type", ref["type"]},
                                {"variables", ref["variables"]},
                                {"callStackHeightAtEntry", callStack.array_size()},
                                {"stackHeightAtEntry", stack.array_size()},
                                {"stackHeight", stack.array_size()},
                                {"streamPosition", 0},
                                {"body", ref["body"]},
                                {"this", thisRef},
                                {"arguments", args}
                            });
    }

    // Find variable name in any scope (returns reference on success, null on failure)
    static cppdatalib::core::value findMember(cppdatalib::core::value &obj,
                                              const cppdatalib::core::value &name)
    {
        auto &obj_deref = obj.deref_all_links();
        auto &name_deref = name.deref_all_links();

        auto *prototype = &obj_deref;
        while (prototype)
        {
            if (prototype->is_member(name_deref))
                return cppdatalib::core::value(&prototype->member(name_deref));

            if (!prototype->is_member(cppdatalib::core::null_t{}))
                return {};

            prototype = &prototype->member(cppdatalib::core::null_t{});
        }

        return {};
    }

    // Find variable name in any scope (returns reference on success, null on failure)
    cppdatalib::core::value findVariable(const cppdatalib::core::value &name)
    {
        auto &name_deref = name.deref_all_links();

        for (size_t i = callStack.size(); i > 0; --i)
        {
            auto &ref = callStack[i-1]["variables"];
            if (ref.is_member(name_deref))
                return ref.member(name_deref);
        }

        return {};
    }

    cppdatalib::core::istringstream stream;

    /* All objects contain a value with the key cppdatalib::core::null_t that refers to the prototype,
     * and normal objects have the cppdatalib::core::normal subtype:
     *
     *    {
     *        cppdatalib::core::null_t(): OBJECT_PROTOTYPE,
     *        ...
     *        NORMAL_PROPERTIES
     *    }
     *
     * Functions contain the following:
     *
     *    {
     *        cppdatalib::core::null_t(): OBJECT_PROTOTYPE,
     *        cppdatalib::core::object_t():
     *            {
     *                "originalBody": SOURCE_CODE as string, or cppdatalib::core::null_t() if no source code available
     *                "body": INTERNAL_CODE as string, or cppdatalib::core::null_t() if no source code available
     *                "argumentNames": LIST_OF_ARGUMENT_NAMES as array (a real array, not a JS array)
     *                "external": UINTEGER, interpreted as a `void (*fp)(cppdatalib::core::value &stack, cppdatalib::core::value &callStack)` (the return value should be left on the top of the stack), or 0 if not an external function
     *            }
     *        ...
     *        NORMAL_PROPERTIES
     *
     */

    /* callStack:
     *
     * Each level of callStack represents an instance of the following object:
     *
     * {
     *     "type": ONE OF "global/function/block"
     *     "variables": VARIABLE_MAP (maps variable names to values, including functions)
     *     "callStackHeightAtEntry": UINTEGER (equal to the index of this scope in `callStack`)
     *     "stackHeightAtEntry": UINTEGER
     *     "stackHeight": UINTEGER
     *     "streamPosition": UINTEGER
     *     "body": INTERNAL_CODE as string
     *     "this": OBJECT_REFERENCE, or null if not a "function" scope
     *     "arguments": LIST_OF_ARGUMENTS as array (a real array, not a JS array), or null if not a "function" scope
     * }
     *
     */

    cppdatalib::core::value callStack;
    cppdatalib::core::value stack;
    std::list<cppdatalib::core::value> heap;
};

void err(const std::string &err)
{
    std::cerr << "ERROR: " << err << std::endl;
}

int main(int argc, char **argv)
{
    /*make_testbed<std::string>("Bencode", std::cout, bencode_tests, [](const std::string &item) {return cppdatalib::bencode::to_bencode(cppdatalib::bencode::from_bencode(cppdatalib::core::istream_handle(item)));})();
    make_testbed<std::string>("JSON", std::cout, json_tests, [](const std::string &item) {return cppdatalib::json::to_json(cppdatalib::json::from_json(cppdatalib::core::istream_handle(item)));})();
    make_testbed<std::string>("MessagePack", std::cout, message_pack_tests, [](const std::string &item) {return cppdatalib::message_pack::to_message_pack(cppdatalib::message_pack::from_message_pack(cppdatalib::core::istream_handle(item)));})();
    make_range_testbed<uintmax_t>("identity", std::cout, 12000000000, [](uintmax_t v) {return v;}, [](uintmax_t v) {return v;})();*/

    std::chrono::seconds seconds(3599);
    cppdatalib::core::value nanoseconds(37000, cppdatalib::core::unix_timestamp);

    std::cout << cppdatalib::core::value(seconds) << std::endl;
    std::cout << cppdatalib::core::value(std::chrono::milliseconds(seconds)) << std::endl;
    std::cout << cppdatalib::core::value(std::chrono::microseconds(seconds)) << std::endl;
    std::cout << cppdatalib::core::value(std::chrono::nanoseconds(seconds)) << std::endl;
    std::cout << cppdatalib::core::value(std::chrono::duration_cast<std::chrono::minutes>(seconds)) << std::endl;
    std::cout << cppdatalib::core::value(std::chrono::duration_cast<std::chrono::hours>(seconds)) << std::endl;

    std::chrono::nanoseconds ns = nanoseconds;
    std::cout << "NS: " << ns.count() << std::endl;

    std::chrono::time_point<std::chrono::system_clock> sec_systemclock = std::chrono::system_clock::now();

    std::cout << cppdatalib::core::value(std::chrono::system_clock::now()) << std::endl;
    std::cout << cppdatalib::core::value(sec_systemclock) << std::endl;

    sec_systemclock = cppdatalib::core::value(std::chrono::system_clock::now());
    std::cout << "systemclock: " << std::chrono::duration_cast<std::chrono::nanoseconds>(sec_systemclock.time_since_epoch()).count() << std::endl;

    std::complex<float> cmp(30, 4);

    std::cout << cmp;

    std::cout << cppdatalib::core::value(cmp) << std::endl;

    cmp = cppdatalib::core::value(std::complex<float>(4.55, -7)).as<decltype(cmp)>();
    std::cout << cmp << std::endl;

    return 0;

#if 0
    if (argc > 1)
    {
        std::cout << JSBytecodeInterpreter((const char *) argv[1], err).run() << std::endl;
    }
    else
    {
        JSBytecodeInterpreter interpreter("", err);

        std::string line;
        while (std::getline(std::cin, line) && line.size())
        {
            interpreter.setBytecode(line);
            std::cout << interpreter.run() << std::endl;
        }
    }
    return 0;

    return qtapp_main(argc, argv, test_http);
#endif

#if 0
    cppdatalib::http::http_initialize();

    cppdatalib::core::ostringstream stream;
    cppdatalib::json::stream_writer(stream) << cppdatalib::core::object_t{{"new object type", "not true"}, {"graduated sequence", 4412887}};
    std::cout << cppdatalib::http::parser("http://admin:admin@localhost:5986/test_db",
                                          cppdatalib::core::default_network_library,
                                          "POST",
                                          cppdatalib::core::istream_handle(stream.str()),
                                          cppdatalib::core::object_t{{"content-type", "application/json"}}, 3);

    cppdatalib::http::http_deinitialize();
    return 0;
#endif

    std::unique_ptr<std::ifstream> infile;
    std::unique_ptr<std::ofstream> outfile;
    std::unique_ptr<cppdatalib::core::stream_input> input;
    std::unique_ptr<cppdatalib::core::stream_handler> output(new cppdatalib::core::stream_handler());
    cppdatalib::core::value outvalue;

    std::string in, out;
    std::string in_scheme, out_scheme;
    std::string in_extension, out_extension;

    if (argc != 3)
    {
        std::cerr << "usage: cppdatalib <input> <output>\n";
        return 1;
    }

    using namespace cppdatalib;
    core::value value(core::object_t{{"key1", core::null_t{}}});
    core::value lk(&value);
    core::value lk2(new core::value(value), core::strong_link);
    lk = lk2;
    lk2.set_weak_link(&lk);
    lk.set_weak_link(&lk2);
    lk.set_strong_link(new core::value(value));
    //lk.set_global_link_name(core::object_t{{"global link name", true}});
    //lk.set_local_link_name("local link name");

#if 0
    std::cout << lk;
    std::cout << lk2;
    if (!lk.link_cycle_exists())
        std::cout << lk.deref_all_links();
    if (!lk2.link_cycle_exists())
        std::cout << lk2.deref_all_links();
    return 0;
#endif

#if 0
    try
    {
        using namespace cppdatalib::json;

        cppdatalib::core::value tree, where;
        const char *sql = "select 'ALABASTER' = concat('alabaster', binary ''), power(ifnull(second, 2), 0.5) as '' where second != null";
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
#endif

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

    if (in == "-")
        input.reset(new cppdatalib::json::parser(std::cin));
    else if (in_scheme.empty() || in_scheme == "file")
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
        else if (in_extension == "bjson") input.reset(new cppdatalib::bjson::parser(*infile.get()));
        else if (in_extension == "binn") input.reset(new cppdatalib::binn::parser(*infile.get()));
        else if (in_extension == "bson") input.reset(new cppdatalib::bson::parser(*infile.get()));
        else if (in_extension == "cbor") input.reset(new cppdatalib::cbor::parser(*infile.get()));
        else if (in_extension == "csv") input.reset(new cppdatalib::csv::parser(*infile.get()));
        else if (in_extension == "json") input.reset(new cppdatalib::json::parser(*infile.get()));
        else if (in_extension == "line") input.reset(new cppdatalib::raw::line_parser(*infile.get()));
        else if (in_extension == "mpk") input.reset(new cppdatalib::message_pack::parser(*infile.get()));
        else if (in_extension == "pson") input.reset(new cppdatalib::pson::parser(*infile.get()));
        else if (in_extension == "raw") input.reset(new cppdatalib::raw::uint8_parser(*infile.get()));
        else if (in_extension == "tsv") input.reset(new cppdatalib::tsv::parser(*infile.get()));
        else if (in_extension == "ubjson") input.reset(new cppdatalib::ubjson::parser(*infile.get()));
#ifdef CPPDATALIB_ENABLE_XML
        else if (in_extension == "xml") input.reset(new cppdatalib::xml::parser(*infile.get(), false));
#endif
        else
        {
            std::cerr << "cppdatalib - unknown input file format\n";
            return 1;
        }
    }

    if (out == "internal")
        output.reset(new cppdatalib::core::value_builder(outvalue));
    else if (out == "dump")
        output.reset(new cppdatalib::core::dump::stream_writer(std::cout, 2));
    else if ((out_scheme.empty() || out_scheme == "file") && out != "null")
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
        else if (out_extension == "bson") output.reset(new cppdatalib::bson::stream_writer(*outfile.get()));
        else if (out_extension == "cbor") output.reset(new cppdatalib::cbor::stream_writer(*outfile.get()));
        else if (out_extension == "csv") output.reset(new cppdatalib::csv::stream_writer(*outfile.get()));
        else if (out_extension == "json") output.reset(new cppdatalib::json::stream_writer(*outfile.get()));
        else if (out_extension == "line") output.reset(new cppdatalib::raw::line_stream_writer(*outfile.get()));
        else if (out_extension == "mpk") output.reset(new cppdatalib::message_pack::stream_writer(*outfile.get()));
        else if (out_extension == "netstrings") output.reset(new cppdatalib::netstrings::stream_writer(*outfile.get()));
        else if (out_extension == "pson") output.reset(new cppdatalib::pson::stream_writer(*outfile.get()));
        else if (out_extension == "raw") output.reset(new cppdatalib::raw::uint8_stream_writer(*outfile.get()));
        else if (out_extension == "tsv") output.reset(new cppdatalib::tsv::stream_writer(*outfile.get()));
        else if (out_extension == "ubjson") output.reset(new cppdatalib::ubjson::stream_writer(*outfile.get()));
#ifdef CPPDATALIB_ENABLE_XML
        else if (out_extension == "xlsx") output.reset(new cppdatalib::xml_xls::document_writer(*outfile.get(), "Worksheet 1"));
        else if (out_extension == "xml") output.reset(new cppdatalib::xml::document_writer(*outfile.get()));
#endif
        else
        {
            std::cerr << "cppdatalib - unknown output file format\n";
            return 1;
        }
    }

    try
    {
#if 0
        input.get()->begin(*output.get());
        do
        {
            input.get()->write_one();
        } while (input.get()->busy());
        input.get()->end();
#else
        cppdatalib::core::automatic_buffer_filter buffer(*output.get());

        clock_t start = clock();

        *input.get() >> buffer;

		double dur = double(clock() - start) / CLOCKS_PER_SEC;
		std::cout << std::setprecision(16);
        std::cout << "Total Input: " << cppdatalib::core::used_buffer(dynamic_cast<cppdatalib::core::stream_parser*>(input.get())->stream()) << " bytes in " << dur << " seconds (" << uint64_t(cppdatalib::core::used_buffer(dynamic_cast<cppdatalib::core::stream_parser*>(input.get())->stream()) / dur) << " Bytes/S, or " << uint64_t(cppdatalib::core::used_buffer(dynamic_cast<cppdatalib::core::stream_parser*>(input.get())->stream()) / dur)*8/double(1000000) << " Mbits/S)\n";
		if (outfile.get())
            std::cout << "Total Output: " << uint64_t(outfile.get()->tellp()) << " bytes in " << dur << " seconds (" << uint64_t(outfile.get()->tellp() / dur) << " Bytes/S, or " << uint64_t(outfile.get()->tellp() / dur)*8/double(1000000) << " Mbits/S)\n";
#endif
    }
    catch (const std::exception &e) {
        std::cerr << "cppdatalib: " << e.what() << "\n";
        return 1;
    }
}
