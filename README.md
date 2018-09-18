# cppdatalib

A header-only, C++11 data conversion library. cppdatalib includes multiple input and output formats, filters, extensions, and adapters for
use with a variety of frameworks and tools. Things it supports:

   - Parsing a variety of formats to output streams (SAX-style) or internal representations (DOM-style)
   - Writing a variety of formats
   - Conversions to and from built-in and custom types with low syntax overhead, including complex data trees
   - Filters for performing a number of operations on data before or after it's used
   - A limited SQL engine for greater flexibility
   - Extremely low memory overhead when converting most formats

## Features

### Formats

cppdatalib offers implementations of several different serialization formats designed for hierarchical data (and some that aren't).
cppdatalib is able to easily convert to and from a standard internal representation. Adapters also exist to integrate smoothly with
the following frameworks:

   - [Boost.Compute](http://www.boost.org/doc/libs/1_66_0/libs/compute/doc/html/index.html)
   - [Boost.Container](http://www.boost.org/doc/libs/1_66_0/doc/html/container.html)
   - [Boost.DynamicBitset](http://www.boost.org/doc/libs/1_66_0/libs/dynamic_bitset/dynamic_bitset.html)
   - [ETL](https://www.etlcpp.com/home.html)
   - [POCO](https://pocoproject.org/)
   - [Qt](https://www.qt.io/)
   - The C++ (03/11/14/17) STL

Supported formats include

   - [Bencode](https://en.wikipedia.org/wiki/Bencode)
   - [Binn](https://github.com/liteserver/binn/blob/master/spec.md)
   - [BJSON](http://bjson.org/)
   - [BSON](http://bsonspec.org/spec.html)
   - [CBOR](http://cbor.io/)
   - [CSV](https://tools.ietf.org/html/rfc4180) (allows user-defined delimiter)
   - [DIF](https://en.wikipedia.org/wiki/Data_Interchange_Format) (write-only, requires table dimensions prior to writing to create well-formed output)
   - [Filesystem I/O](http://en.cppreference.com/w/cpp/filesystem) (`std::experimental::filesystem` or `std::filesystem`)
   - Raw [HTTP](https://en.wikipedia.org/wiki/Http) (client-only, as string with attributes - if they're enabled - and plain string otherwise)<br/>
     Supports sending data from input stream (i.e. PUT with string buffer)<br/>
     Supports Qt, Poco, or curl backends
   - [JSON](https://json.org/)
   - [MessagePack](https://msgpack.org/)
   - MySQL (database/table retrieval and writing)
   - [Netstrings](https://en.wikipedia.org/wiki/Netstring) (write-only)
   - [plain text property lists](http://www.gnustep.org/resources/documentation/Developer/Base/Reference/NSPropertyList.html)
   - [PSON](https://github.com/dcodeIO/PSON)
   - [TSV](https://en.wikipedia.org/wiki/Tab-separated_values) (allows user-defined delimiter)
   - [UBJSON](http://ubjson.org/)
   - Raw [XML](https://www.w3.org/TR/xml/) (write-only)
   - XML property lists (write-only)
   - [XML-RPC](http://xmlrpc.scripting.com/spec.html) (write-only)
   - [XML-XLS](https://msdn.microsoft.com/en-us/library/aa140066(office.10).aspx) (write-only)

Language formats include

   - Lisp (write-only)

### Planned formats

   - Transenc

### Unimplemented adapter features

The adapter for the STL is missing some features to seamlessly integrate with standard types. For example, the following libraries and technologies are not currently supported as seamless conversions out-of-the-box:

   - Function objects (i.e. `std::function`)
   - The Locale library
   - `std::auto_ptr` which is deprecated anyway
   - The random-number generators
   - Ratio values
   - Regular expressions
   - I/O streams

Obviously the other adapters for third-party libraries are not complete either, but are there to get you started.

### Filters

cppdatalib offers a variety of filters that can be applied to stream handlers. These include the following:

   - `buffer_filter`<br/>
     Optionally buffers strings, arrays, and objects, or any combination of the same, or acts as a pass-through filter
   - `automatic_buffer_filter`<br/>
     Automatically determines the correct settings for the underlying buffer_filter based on the output stream handler
   - `tee_filter`<br/>
     Splits an input stream to two output stream handlers
   - `view_filter`<br/>
     Applies a view function to every element of the specified type. Essentially the same as `custom_converter_filter`, but can't edit the value and is more efficient.
   - `range_filter`<br/>
     Pass-through filter that computes the maximum and minimum values of the specified type, as well as the midpoint (the midpoint is only applicable for numeric values, but the min and max can be computed for any type)
   - `mean_filter`<br/>
     Pass-through filter that computes the arithmetic, geometric, and harmonic means of numeric values
   - `dispersal_filter`<br/>
     Pass-through filter that computes the variance and standard deviation of numeric values (subclass of `mean_filter`, so both central tendency and dispersal can be calculated with this class)
   - `array_sort_filter`<br/>
     Sorts all arrays deeper than the specified nesting level (or all arrays, if 0 is specified), in either ascending or descending order
   - `select_from_array_filter`<br/>
     Selects values from an array using the user-specified selection functor. The selection functor should return either true/non-zero, if the value should be included in the output, or false/zero otherwise
   - `object_keys_to_array_filter`<br/>
     Extracts keys of objects in the input stream. Keys are written in arrays as rows of an output table. The table lists objects in the order they were found, and the rows list keys (in ascending order) in an array. Complex object keys are permitted, including those which contain/are objects themselves
   - `table_to_array_of_objects_filter`<br/>
     Converts a table to an array of objects, using an external column-name list. Also supports converting single-dimension arrays to object-wrapped values with specified column key
   - `duplicate_key_check_filter`<br/>
     Ensures the input stream only provides unique keys in objects. This filter supports complex keys, including nested objects
   - `converter_filter`<br/>
     Converts from one internal type to another, for example, all integers to strings. This filter has built-in conversions
   - `custom_converter_filter`<br/>
     Converts the specified internal type, using the user-specified converter functor. This filter supports varying output types, including the same type as the input
   - `generic_converter_filter`<br/>
     Sends all scalar values to a user-specified functor for conversion. Arrays and objects cannot be converted with this filter
   - `sql_select_where_filter`<br/>
     Basically executes a SQL query equivalent to `SELECT * WHERE <predicate>`, where `<predicate>` is any specified structure. Note the absence of and a lack of support for a `FROM` clause.
   - `sql_select_filter`<br/>
     Executes a SQL query equivalent to `SELECT <selection> [WHERE <predicate>]`. The `<selection>` can be any combination and ordering of field names or supported expressions (including the `AS` operator for renaming expressions), and the optional `<predicate>` specifies which fields to include in the output. Note the absence of and a lack of support for a `FROM` clause.

Filters can be assigned on top of other filters. How many filters are permitted is limited only by the runtime environment.

### Large Data

cppdatalib supports streaming with a small memory footprint. Most conversions require no buffering or minimal buffering. Also, there is no limit to the nesting depth of arrays or objects (misnomer: technically, there *is* a limit, it's just imposed by available memory and the addressable memory space). This makes cppdatalib much more suitable for large datasets.

## Usage

Using the library is simple. Everything is under the main namespace `cppdatalib`, and underneath is the `core` namespace and individual format namespaces (e.g. `json`).
It is recommended to use `using` statements to pull in format namespaces.

For example, the following programs are identical attempts to read a JSON structure from STDIN, and output it to STDOUT:

Read through value class:

```c++
#include <cppdatalib/cppdatalib.h>

int main() {
    using namespace cppdatalib;             // Parent namespace
    using namespace json;                   // Format namespace

    core::value my_value;                   // Global cross-format value class

    try {
        json::parser p(std::cin);           // Initialize parser
        json::stream_writer w(std::cout);   // Initialize writer
        p >> my_value;                      // Read in to core::value as JSON
        w << my_value;                      // Write core::value out as JSON
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl; // Catch any errors that might have occured (syntax or logical)
    }

    return 0;
}
```

Read without parser (still uses intermediate value - result of `from_json`):

```c++
#include <cppdatalib/cppdatalib.h>

int main()
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
```

Read without intermediate value (extremely memory efficient):

```c++
#include <cppdatalib/cppdatalib.h>

int main()
{
    using namespace cppdatalib;             // Parent namespace
    using namespace json;                   // Format namespace

    try {
        json::parser(std::cin) >> json::stream_writer(std::cout);        // Write core::value out to STDOUT as JSON
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl; // Catch ainnny errors that might have occured (syntax or logical)
    }

    return 0;
}
```

### Advanced Usage

To use the lower-level stream-handling (parse-on-the-fly) classes, see the below example:

```c++
#include <cppdatalib/cppdatalib.h>

int main() {
    using namespace cppdatalib;               // Parent namespace
    using namespace json;                     // Format namespace
    using namespace ubjson; 	              // Another format namespace
    
    core::value my_value;

    core::value_builder builder(my_value);    // Set up value builder stream handler
                                              // It acts as a stream handler that writes all data into the
                                              // internal value structure

    ubjson::stream_writer writer(std::cout);  // UBJSON writer to standard output
    
    try {
        json::parser parser(std::cin);
        parser >> writer;                     // Convert from JSON on standard input to UBJSON on standard output
                                              // Note that this DOES NOT READ the entire stream before writing!
                                              // The data is read and written at the same time

        parser >> builder;                    // Convert from JSON on standard input to internal representation in
                                              // `my_value`. Note that my_value is also accessible by using `builder.value()`

        core::convert(my_value, writer);      // Convert from internal representation to UBJSON on standard output
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    try {
        json::parser parser(std::cin);
        core::stream_filter<core::null, core::string> filter(writer);
                                              // Set up filter on UBJSON output that converts all `null` values to empty strings

        parser >> filter;                     // Convert from JSON on standard input to UBJSON on standard output, converting `null`s to empty strings
                                              // When using a filter, write to the filter, instead of the handler the filter is modifying
                                              // (i.e. don't write to `writer` here unless you don't want to employ the filter)
                                              // Note that this DOES NOT READ the entire stream before writing!
                                              // The data is read and written at the same time
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    try {
        json::parser parser(std::cin);
        core::stream_filter<core::null, core::string> filter(writer);
                                              // Set up filter on UBJSON output that converts all `null` values to empty strings

        auto lambda = [](core::value &v)
        {
            if (v.is_string() && v.get_string().find('a') == 0)
                v.set_string("");
        };
        core::generic_stream_filter<decltype(lambda)> generic_filter(filter, lambda);
                                              // Set up filter on top of previous filter that clears all strings beginning with lowercase 'a'

        parser >> generic_filter;             // Convert from JSON on standard input to UBJSON on standard output,
                                              // converting `null`s to empty strings, and clearing all strings beginning with 'a'
                                              // Again, note that this does not read the entire stream before writing
                                              // The data is read and written at the same time
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    try {
        json::parser parser(std::cin);
        core::stream_filter<core::boolean, core::integer> second_filter(writer);
                                              // Set up filter on UBJSON output that converts booleans to integers

        core::stream_filter<core::integer, core::real> first_filter(second_filter);
                                              // Set up filter on top of previous filter that converts all integers to reals

        parser >> first_filter;               // Convert from JSON on standard input to UBJSON on standard output,
                                              // converting booleans to integers, and converting integers to reals
                                              // Note that order of filters is important. The last filter enabled will be the first to be called.
                                              // If the filter order was switched, all booleans and integers would become reals.
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    try {
        json::parser parser(std::cin);
        core::tee_filter tee(writer, builder);
                                              // Set up tee filter. Tee filters split the input to two handlers, which can be stream_filters or other tee_filters.
                                              // In this case, we'll parse the JSON input once, and output to UBJSON on standard output and build an internal
                                              // representation simultaneously.

        parser >> tee_filter;                 // Convert from JSON on standard input to UBJSON on standard output,
                                              // as well as building internal representation in my_value
    } catch (const core::error &e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    return 0;
}
```

### Extending the library

Almost every part of cppdatalib is extensible.

To create a new output format, follow the following guidelines:

   1. Create a new namespace under `cppdatalib` for the format
   2. In the new namespace, define a class `stream_writer` that inherits `core::stream_writer` and `core::stream_handler`. The stream writer class can reimplement the following functions:
      - `void begin_();` - Called to initialize the format. The default implementation does nothing
      - `void end_();` - Called to deinitalize the format. The default implementation does nothing
      - `bool write_(const core::value &v, bool is_key);` - Called when a value is written by `write()`, with the value to be written passed in `v`. `is_key` is true if the specified value is an object key. Reimplementations of this function should return `true` if the value was written, and `false` if the value still needs to be processed. The default implementation returns `false`
      - `void begin_item_(const core::value &v);` - Called when starting to parse any non-key value. The opposite of `begin_key_()`. The default implementation does nothing.
      - `void end_item_(const core::value &v);` - Called when ending parsing of any non-key value. The opposite of `end_key_()`. The default implementation does nothing.
      - `void begin_scalar_(const core::value &v, bool is_key);` - Called when starting to parse any scalar value. This includes all value types except arrays and objects. `is_key` is true if the specified value is an object key. The default implementation does nothing.
      - `void end_scalar_(const core::value &v, bool is_key);` - Called when ending parsing of any scalar value. This includes all value types except arrays and objects. `is_key` is true if the specified value is an object key. The default implementation does nothing.
      - `void begin_key_(const core::value &v);` - Called when starting to parse any key value. The opposite of `begin_item_()`. The default implementation does nothing.
      - `void end_key_(const core::value &v);` - Called when ending parsing of any key value. The opposite of `end_item_()`. The default implementation does nothing.
      - `void null_(const core::value &v);` - Called when a scalar `null` is written. The default implementation does nothing.
      - `void bool_(const core::value &v);` - Called when a scalar `boolean` is written. The default implementation does nothing.
      - `void integer_(const core::value &v);` - Called when a scalar `integer` is written. The default implementation does nothing.
      - `void uinteger_(const core::value &v);` - Called when a scalar `uinteger` is written. The default implementation does nothing.
      - `void real_(const core::value &v);` - Called when a scalar `real` is written. The default implementation does nothing.
      - `void begin_string_(const core::value &v, core::optional_size size, bool is_key);` - Called when starting to parse a string. The size, if known, is passed in `size`. If the size is unknown, `size` is equal to `stream_handler::unknown_size()`. If `v.size()` is equal to `size`, the entire string is available for analysis. `is_key` is true if the string is an object key. The default implementation does nothing.
      - `void string_data_(const core::value &v, bool is_key);` - Called when data is available to append to a string. The string is contained in `v`. `is_key` is true if the string currently being parsed is an object key. The default implementation does nothing.
      - `void end_string_(const core::value &v, bool is_key);` - Called when ending parsing of a string. `is_key` is true if the string is an object key. The default implementation does nothing.
      - `void begin_array_(const core::value &v, core::optional_size size, bool is_key);` - Called when starting to parse an array. The size, if known, is passed in `size`. If the size is unknown, `size` is equal to `stream_handler::unknown_size()`. If `v.size()` is equal to `size`, the entire array is available for analysis. `is_key` is true if the array is an object key. The default implementation does nothing.
      - `void end_array_(const core::value &v, bool is_key);` - Called when ending parsing of an array. `is_key` is true if the array is an object key. The default implementation does nothing.
      - `void begin_object_(const core::value &v, core::optional_size size, bool is_key);` - Called when starting to parse an object. The size, if known, is passed in `size`. If the size is unknown, `size` is equal to `stream_handler::unknown_size()`. If `v.size()` is equal to `size`, the entire object is available for analysis. `is_key` is true if the object is an object key. The default implementation does nothing.
      - `void end_object_(const core::value &v, bool is_key);` - Called when ending parsing of an object. `is_key` is true if the object is an object key. The default implementation does nothing.
   3. All data should be written via the member function `core::ostream &output_stream();`
   4. Required buffering features should be provided by member function `unsigned int required_features() const;` The available features are `const` members of the `core::stream_handler` class. The default implementation returns `stream_handler::requires_none`.
      
Since the default implementations of all these functions do nothing, an instance of `core::stream_handler` can be used as a dummy output sink that does nothing.

To create a new filter, follow the guidelines for output formats, but place them in the `cppdatalib::core` namespace. Inherit all filters from `core::stream_filter_base`. Writing should be done exclusively via member variable `output`. The reimplementation rules all still apply, but `core::stream_filter_base` provides pass-through writing of all values to `output`. This allows you to reimplement just what you need, and pass everything else through unmodified.

### Compile-time flags

Below is a list of compile-time adjustments supported by cppdatalib:

   - `CPPDATALIB_BOOL_T` - The underlying boolean type of the implementation. Should be able to store a true and false value. Defaults to `bool`
   - `CPPDATALIB_INT_T` - The underlying integer type of the implementation. Should be able to store a signed integral value. Defaults to `int64_t`
   - `CPPDATALIB_UINT_T` - The underlying unsigned integer type of the implementation. Should be able to store an unsigned integral value. Defaults to `uint64_t`
   - `CPPDATALIB_REAL_T` - The underlying floating-point type of the implementation. Should be able to store at least an IEEE-754 value. Defaults to `double`
   - `CPPDATALIB_CSTRING_T` - The underlying C-style string type of the implementation. Defaults to `const char *`
   - `CPPDATALIB_STRING_T` - The underlying string type of the implementation. Defaults to `std::string`
   - `CPPDATALIB_ARRAY_T` - The underlying array type of the implementation. Defaults to `std::vector<cppdatalib::core::value>`
   - `CPPDATALIB_OBJECT_T` - The underlying object type of the implementation. Defaults to `std::multimap<cppdatalib::core::value, cppdatalib::core::value>`
   - `CPPDATALIB_SUBTYPE_T` - The underlying subtype type of the implementation. Must be able to store all subtypes specified in the `core` namespace. Defaults to `int16_t`
   - `CPPDATALIB_BUFFER_SIZE` - The size of underlying buffers in the implementation. Defaults to 4096
   - `CPPDATALIB_CACHE_SIZE` - The cache depth of traversal in the implementation. Defaults to 3
   - `CPPDATALIB_DEFAULT_NETWORK_LIBRARY` - The default network interface to use. Defaults to the first interface defined. To redefine this value, check what value is introduced by the defining of the `CPPDATALIB_ENABLE_<XXX>_NETWORK` flag (e.g. `qt_network_library` for QtNetwork).
   - `CPPDATALIB_EVENT_LOOP_CALLBACK` - Defines a section of code to run while a parser is busy parsing. If defined, this should be either empty or a valid function call (including parentheses and arguments, if any). For example, for the Qt framework: `#define CPPDATALIB_EVENT_LOOP_CALLBACK qApp->processEvents();`

Enable/Disable flags are listed below:

   - `CPPDATALIB_ENABLE_MYSQL` - Enables inclusion of the MySQL interface library. If defined, the MySQL headers must be available in the include path
   - `CPPDATALIB_ENABLE_XML` - Enables inclusion of the standard XML library. If defined, `CPPDATALIB_ENABLE_ATTRIBUTES` must also be defined
   - `CPPDATALIB_DISABLE_WRITE_CHECKS` - Disables nesting checks in the stream_handler class. If write checks are disabled, and the generating code is buggy, it may generate corrupted output without catching the errors, but can result in better performance. Use at your own risk (this is *very* risky)
   - `CPPDATALIB_ENABLE_FAST_IO` - Swaps usage of the `std::ios` classes to trimmed-down, more performant, custom I/O classes. Although it acts as a drop-in replacement for the STL, it only implements a subset of the features (but the features it does implement should be usage-compatible). It is preferred that this option be enabled.
   - `CPPDATALIB_DISABLE_FAST_IO_GCOUNT` - Disables calculation of `gcount()` in the fast input classes. This removes the `gcount()` function altogether. This flag only has an effect if `CPPDATALIB_ENABLE_FAST_INPUT` is defined
   - `CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE` - Trims value sizes down to optimize space for large numeric arrays. This theoretically (and practically) slows down string values somewhat, but saves space
   - `CPPDATALIB_DISABLE_WEAK_POINTER_CONVERSIONS` - Disables raw pointer and std::weak_ptr conversions to core::values. Note that defining this flag will not result in compile-time errors when attempting to convert a weakly-defined pointer. It will be implicitly converted to bool instead. As this may not be what you want, use at your own risk.
   - `CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS` - Disables implicit conversions between types and removes the convert_to_xxx() member functions of core::value. The as_xxx() member functions of core::value are remapped to be identical to the get_xxx() member functions. Some formats require that this be undefined
   - `CPPDATALIB_THROW_IF_WRONG_TYPE` - Makes the get_xxx() member functions of core::value throw an error if the currently stored type is not the requested type. The "default value" parameter is irrelevant if this is defined. Some internal library code may throw errors with this flag defined, so use at your own risk
   - `CPPDATALIB_ENABLE_ATTRIBUTES` - Enables API for attachment of attributes to any core::value instance. Attributes consist of an object with arbitrary-type keys and values, and each of those keys or values may have attributes of their own. NOTE: This must be defined for the standard XML format. The filesystem library requires this to be defined for file metadata (permissions and modification time) to be available. WARNING: Stack-overflow protection is not extended to attributes having attributes having attributes, etc. Please limit how many nesting levels of attributes a given attribute has. NOTE: defining this value increases the size of a `value` instance
   - `CPPDATALIB_DISABLE_TEMP_STRING` - Disables the use of a standard or custom `string_view` class. If defined, all strings will be of type `string`. If not defined, strings may be of type `string` or `temporary_string`. Saves space if not defined, saves time if defined
   - `CPPDATALIB_DISABLE_IMPLICIT_TYPE_CONVERSIONS` - If defined, this flag makes all core::value constructors (except for core::null_t) and all core::value type conversion operators `explicit`. User code and future formats should compile with this flag defined, although the library is much easier to use with it undefined
   - `CPPDATALIB_DISABLE_CONVERSION_LOSS` - If defined, this flag makes internal core::value converters throw errors if data loss would result, including loss of precision by converting to or from "real"
   - `CPPDATALIB_ENABLE_BOOST_COMPUTE` - Enables the [Boost.Compute](http://www.boost.org/doc/libs/1_66_0/libs/compute/doc/html/index.html) adapters, to smoothly integrate with Boost.Compute types. The Boost source tree must be in the include path
   - `CPPDATALIB_ENABLE_BOOST_CONTAINER` - Enables the [Boost.Container](http://www.boost.org/doc/libs/1_66_0/doc/html/container.html) adapters, to smoothly integrate with Boost.Container types. The Boost source tree must be in the include path
   - `CPPDATALIB_ENABLE_BOOST_DYNAMIC_BITSET` - Enables the [Boost.DynamicBitset](http://www.boost.org/doc/libs/1_66_0/libs/dynamic_bitset/dynamic_bitset.html) adapters, to smoothly integrate with Boost.DynamicBitset. The Boost source tree must be in the include path
   - `CPPDATALIB_ENABLE_QT` - Enables the [Qt](https://www.qt.io/) adapters, to smoothly integrate with the most common Qt types. The Qt source tree must be in the include path
   - `CPPDATALIB_ENABLE_POCO` - Enables the [POCO](https://pocoproject.org/) adapters, to smoothly integrate with POCO types. The "Poco" source tree must be in the include path
   - `CPPDATALIB_ENABLE_ETL` - Enables the [ETL](https://www.etlcpp.com/home.html) adapters, to smoothly integrate with ETL types. The "etl" source tree must be in the include path
   - `CPPDATALIB_ENABLE_STL` - Enables the STL adapters, to smoothly integrate with all STL types
   - `CPPDATALIB_ENABLE_FILESYSTEM` - Enables the `std::filesystem` (or `std::experimental::filesystem`) parser and stream writer. If the C++ version is reported to be less than C++17, the `std::experimental` version is used
   - `CPPDATALIB_DISABLE_HTTPS_IF_POSSIBLE` - To reduce dependencies, define this flag if HTTPS support is not needed for your application.
   - `CPPDATALIB_ENABLE_QT_NETWORK` - Enables the [Qt](https://www.qt.io/) network adapter, for use with the `cppdatalib::http::parser` class. This defines the value `qt_network_library` to be used with `CPPDATALIB_DEFAULT_NETWORK_LIBRARY`. The Qt source tree must be in the include path
   - `CPPDATALIB_ENABLE_POCO_NETWORK` - Enables the [POCO](https://pocoproject.org/) network adapter, for use with the `cppdatalib::http::parser` class. This defines the value `poco_network_library` to be used with `CPPDATALIB_DEFAULT_NETWORK_LIBRARY`. The "Poco" source tree must be in the include path

For best space savings, do the following before including `cppdatalib.h`:

    #define CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
    #undef CPPDATALIB_ENABLE_ATTRIBUTES
    #define CPPDATALIB_DISABLE_TEMP_STRING

For best compatibility and time savings, do the following before including `cppdatalib.h`:

    #undef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
    #define CPPDATALIB_ENABLE_ATTRIBUTES
    #undef CPPDATALIB_DISABLE_TEMP_STRING

Some features do require attributes, so keep that in mind when working with compile-time flags.

Please note that custom datatypes are a work-in-progress. Defining custom types may work, or may not work at all.

## Supported datatypes

Top-level types include:
   - null
   - boolean
   - integer
   - uinteger
   - real
   - string
   - array
   - object
   - link

Subtypes include (all ranges inclusive on both ends):

   - 0 to 2^32-1: format- or user-specified subtypes
   - -9 to -1: generic subtypes applicable to all types
   - -19 to -10: subtypes applicable to booleans
   - -39 to -20: subtypes applicable to integers, either signed or unsigned
   - -44 to -40: subtypes applicable only to signed integers
   - -49 to -45: subtypes applicable only to unsigned integers
   - -59 to -50: subtypes applicable to floating-point values
   - -129 to -60: subtypes applicable to strings or temporary strings, encoded as some form of text
   - -199 to -130: subtypes applicable to strings or temporary strings, encoded as some form of binary value
   - -209 to -200: subtypes applicable to arrays
   - -219 to -200: subtypes applicable to objects
   - -229 to -220: subtypes applicable to links
   - -239 to -230: subtypes applicable to nulls
   - -255 to -240: undefined, reserved
   - -2^32 to -256: format-specified reserved subtypes

There are two special subtypes, `generic_subtype_comparable`, which compares as if its subtype is equal to whatever it is compared against,
and `domain_comparable`, which compares as if its type *and* subtype are equal to whatever it is compared against (with a few restrictions).
NOTE: it is *not* recommended to use these special subtypes in actual data. Their purpose is to make it easier to find specific data, whatever the type.

Domains are special comparison modes, to compare similar values of different types. To compare using domains, use the special subtype
`domain_comparable` as the test subtype. Domains are separated into the following at the moment:
   - null
   - boolean
   - number (integer, uinteger, real)
   - string (string, temporary_string)
   - object
   - array
   - link

This means that, for example, an object and an array will never, under any circumstances, be found equal.

Generic strings have three main subtypes:

   - `normal` - Only use this for strings known to be UTF-8 encoded. This is the default string type.
   - `clob` - Use this for strings that have an unknown text encoding (that could possibly be UTF-8).
   - `blob` - Use this for strings that are known to be binary encoded, or if you want to remove any encoding metadata from the string.

If a type is unsupported in a serialization format, the type is not converted to something recognizable by the format. An error is thrown instead, describing the failure. However, if a subtype is not supported, the value is processed as if it had none (i.e. if a value is a `string`, with unsupported subtype `date`, the value is processed as a meaningless string and the subtype metadata is removed).

If a format-defined limit is reached, such as an object key length limit, an error will be thrown.
     
   - Bencode supports `uint`, `int`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`, `bool`, and `real`s are **not** supported.
       - integers are limited to two's complement 64-bit integers when reading.
       - Strings are read as `clob` subtype.
       - No subtypes are supported; strings are written without modification (i.e. as binary data). If the string is already UTF-8 encoded, it will stay UTF-8 encoded, but be read as `clob`.
       - Numerical metadata is lost when converting to and from Bencode.

   - Binn supports `null`, `bool`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `date`, `time`, `datetime`, `bignum`, `blob`, and `clob` are supported.
       - `object` subtype `map` is supported.
       - Map keys are limited to signed 32-bit integers.
       - The Binn `string` type is used to identify UTF-8 strings, and the Binn `blob` type identifies other strings. No strings are modified, either when reading or writing. The correctness of UTF-8 strings is not verified.
       - Object keys are limited to 255 characters or fewer.
       - Custom subtypes for `null`, `bool`, `int`, `string`, and `array` are supported, as long as they are at least equal to the value of `core::user`.

   - BJSON supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `blob` and `clob` are supported.
       - The BJSON `string` type is used to identify UTF-8 strings, and the BJSON `binary` type identifies other strings. No strings are modified, either when reading or writing. The correctness of UTF-8 strings is not verified.
       - Negative values too large for the internal type are converted to `bignum` internally.
       - Numerical metadata of integers is lost when converting to and from BJSON.

   - BSON supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `blob` and `clob` are supported.
       - The BSON `string` type is used to identify UTF-8 strings, and the BSON `binary` type identifies other strings. No strings are modified, either when reading or writing. The correctness of UTF-8 strings is not verified.
       - When writing, the entire output must be written in a single call.

   - CBOR supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `blob` and `clob` are supported.
       - CBOR tags are not currently supported.
       - Since the CBOR specification supports negative numbers less than -2^63, these integers are converted to decimal bignums when reading.
       - The CBOR `string` type is used to identify UTF-8 strings, and the CBOR `binary` type identifies other strings. No strings are modified, either when reading or writing. The correctness of UTF-8 strings is not verified.

   - CSV supports `null`, `bool`, `uint`, `int`, `real`, `string`, and `array`.<br/>
     Notes:
       - `object`s are **not** supported.
       - `uint` values are fully supported when reading.
       - All strings are converted to UTF-8 when read, regardless of the input encoding.<br/>
       - No subtypes are supported; strings are written without modification (i.e. as binary data). If the string is already UTF-8 encoded, it will stay UTF-8 encoded.
       - Numerical metadata is lost when converting to and from CSV.

   - DIF supports `null`, `bool`, `uint`, `int`, `real`, `string`, and `array`.<br/>
     Notes:
       - `object`s are **not** supported.
       - `uint` values are fully supported when reading.
       - No subtypes are supported; strings are written without modification (i.e. as binary data). If the string is already UTF-8 encoded, it will stay UTF-8 encoded.
       - Numerical metadata is lost when converting to and from DIF.
     
   - JSON supports `null`, `bool`, `uint` and `int` and `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - All strings are converted to UTF-8 when read, regardless of the input encoding.<br/>
       - `string` subtype `bignum` is fully supported, for both reading and writing.<br/>
       - `string` subtypes `clob` and `blob` are supported when writing, as well as UTF-8 strings.<br/>
       - There is no limit to the magnitude of numbers supported. `int` is attempted first, then `uint`, then `real`, then `bignum`.
       - Numerical metadata is lost when converting to and from JSON.

   - Lisp supports `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - No subtypes are supported; strings are written without modification (i.e. as binary data). If the string is already UTF-8 encoded, it will stay UTF-8 encoded.
       - Numerical metadata is lost when converting to Lisp.

   - MessagePack supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - MessagePack extensions are not currently supported.
       - `string` subtypes `blob` and `clob` are supported.
       - The MessagePack `str` type is used to identify UTF-8 strings, and the MessagePack `bin` type identifies other strings. No strings are modified, either when reading or writing. The correctness of UTF-8 strings is not verified.
       - MessagePack timestamps are not currently supported.

   - MySQL supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `blob`, `bignum`, `date`, `time`, and `datetime` are supported.
       - MySQL datatypes `ENUM` and `SET` are not supported natively, although they are extracted as raw strings.
       - Due to a limitation with the MySQL C API, it is not possible to read a table/db and write to another table/db using the same connection at the same time. Doing so results in undefined behavior. An intermediate representation is required (i.e. parse table to core::value, then output core::value to the table/db), or else use multiple connections to the MySQL server.

   - Netstrings supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - No subtypes are supported.
       - Type information is lost when converting to Netstrings.

   - plain text property lists support `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`s are **not** supported.<br/>
       - integers are limited to two's complement 64-bit integers when reading.
       - `string` subtypes `date`, `time`, and `datetime` are supported.
       - Numerical metadata is lost when converting to and from plain text property lists.

   - PSON supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `blob` and `clob` are supported.
       - The PSON `string` type is used to identify UTF-8 strings, and the PSON `binary` type identifies other strings. No strings are modified, either when reading or writing. The correctness of UTF-8 strings is not verified.
       - integers are limited to two's complement 64-bit integers when reading and writing.
       - Numerical metadata of integers is lost when converting to and from PSON.

   - TSV supports `null`, `bool`, `uint`, `int`, `real`, `string`, and `array`.<br/>
     Notes:
       - `object`s are **not** supported.
       - `uint` values are fully supported when reading.
       - All strings are converted to UTF-8 when read, regardless of the input encoding.<br/>
       - No subtypes are supported; strings are written without modification (i.e. as binary data). If the string is already UTF-8 encoded, it will stay UTF-8 encoded.
       - Numerical metadata is lost when converting to and from TSV.

   - UBJSON supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.
     Notes:
       - `string` subtype `bignum` is supported.
       - `string` subtypes `blob` and `clob` are **not** supported. These subtypes cannot be written. Convert all strings to UTF-8 before writing them to UBJSON (and don't just change the subtype, unless you know that doing so will result in a valid UTF-8 string).
       - `uint` values are limited to the `int` value range when reading or writing, due to lack of format support for unsigned types.

   - XML supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - All strings are converted to UTF-8 when read, regardless of the input encoding.<br/>
       - `string` subtypes `clob` and `blob` are supported when writing, as well as UTF-8 strings.<br/>
       - Numerical metadata is lost when converting to and from XML.
       - Value attributes are supported when writing (and in fact required, if you want attributes attached to your elements), either on object keys or object values, or both. No protection is provided when writing against duplicate keys existing in both an object key and the associated object value.
       - XML cannot compile without CPPDATALIB_ENABLE_ATTRIBUTES being defined.

   - XML property lists support `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`s are **not** supported.
       - `string` subtypes `date`, `time`, and `datetime` are supported.
       - Numerical metadata is lost when converting to XML property lists.
     
   - XML-RPC supports `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`s are **not** supported.
       - No subtypes are supported.
       - Numerical metadata is lost when converting to XML-RPC.

   - XML-XLS supports `null`, `bool`, `uint`, `int`, `real`, `string`, and `array`.<br/>
     Notes:
       - `object`s are **not** supported.
       - `string` subtypes `date`, `time`, and `datetime` are supported.
       - Numerical metadata is lost when converting to and from XML-XLS.
