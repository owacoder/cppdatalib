# cppdatalib

A header-only, C++11 data conversion library

## Features

### Formats

cppdatalib offers implementations of several different serialization formats designed for hierarchical data (and some that aren't).
cppdatalib is able to easily convert to and from a standard internal representation. Adapters also exist to integrate smoothly with
the following frameworks:

   - [Boost.Compute](http://www.boost.org/doc/libs/1_66_0/libs/compute/doc/html/index.html)
   - [Boost.Container](http://www.boost.org/doc/libs/1_66_0/doc/html/container.html)
   - [Qt](https://www.qt.io/)
   - [POCO](https://pocoproject.org/)
   - [ETL](https://www.etlcpp.com/home.html)
   - The C++ STL

Supported formats include

   - [JSON](https://json.org/)
   - [UBJSON](http://ubjson.org/)
   - [Bencode](https://en.wikipedia.org/wiki/Bencode)
   - [plain text property lists](http://www.gnustep.org/resources/documentation/Developer/Base/Reference/NSPropertyList.html)
   - [CSV](https://tools.ietf.org/html/rfc4180)
   - [Binn](https://github.com/liteserver/binn/blob/master/spec.md)
   - [MessagePack](https://msgpack.org/)
   - MySQL (database/table retrieval and writing)
   - XML property lists (write-only)
   - [XML-RPC](http://xmlrpc.scripting.com/spec.html) (write-only)
   - [XML-XLS](https://msdn.microsoft.com/en-us/library/aa140066(office.10).aspx) (write-only)
   - [BJSON](http://bjson.org/) (write-only)
   - [Netstrings](https://en.wikipedia.org/wiki/Netstring) (write-only)

### Planned formats

   - Transenc
   - CBOR
   - TSV

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
     Pass-through filter that computes the maximum and minimum values of the specified type, as well as the midpoint (only applicable for numeric values)
   - `mean_filter`<br/>
     Pass-through filter that computes the arithmetic, geometric, and harmonic means of numeric values
   - `dispersal_filter`<br/>
     Pass-through filter that computes the variance and standard deviation of numeric values (subclass of `mean_filter`, so both central tendency and dispersal can be calculated with this class)
   - `array_sort_filter`<br/>
     Sorts all arrays deeper than the specified nesting level (or all arrays, if 0 is specified), in either ascending or descending order
   - `table_to_array_of_maps_filter`<br/>
     Converts a table to an array of maps, using an external column-name list. Also supports converting single-dimension arrays to object-wrapped values with specified column key
   - `duplicate_key_check_filter`<br/>
     Ensures the input stream only provides unique keys in objects. This filter supports complex keys, including nested objects
   - `converter_filter`<br/>
     Converts from one internal type to another, for example, all integers to strings. This filter has built-in conversions
   - `custom_converter_filter`<br/>
     Converts the specified internal type, using the user-specified converter function. This filter supports varying output types, including the same type as the input
   - `generic_converter_filter`<br/>
     Sends all scalar values to a user-specified function for conversion. Arrays and objects cannot be converted with this filter

Filters can be assigned on top of other filters. How many filters are permitted is limited only by the runtime environment.

### Large Data

cppdatalib supports streaming with a small memory footprint. Most conversions require no buffering or minimal buffering. Also, there is no limit to the nesting depth of arrays or objects. This makes cppdatalib much more suitable for large datasets.

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
    } catch (core::error e) {
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
    } catch (core::error e) {
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
    } catch (core::error e) {
        std::cerr << e.what() << std::endl; // Catch any errors that might have occured (syntax or logical)
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
    } catch (core::error e) {
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
    } catch (core::error e) {
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
    } catch (core::error e) {
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
    } catch (core::error e) {
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
    } catch (core::error e) {
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
      - `void begin_string_(const core::value &v, core::int_t size, bool is_key);` - Called when starting to parse a string. The size, if known, is passed in `size`. If the size is unknown, `size` is equal to `stream_handler::unknown_size`. If `v.size()` is equal to `size`, the entire string is available for analysis. `is_key` is true if the string is an object key. The default implementation does nothing.
      - `void string_data_(const core::value &v, bool is_key);` - Called when data is available to append to a string. The string is contained in `v`. `is_key` is true if the string currently being parsed is an object key. The default implementation does nothing.
      - `void end_string_(const core::value &v, bool is_key);` - Called when ending parsing of a string. `is_key` is true if the string is an object key. The default implementation does nothing.
      - `void begin_array_(const core::value &v, core::int_t size, bool is_key);` - Called when starting to parse an array. The size, if known, is passed in `size`. If the size is unknown, `size` is equal to `stream_handler::unknown_size`. If `v.size()` is equal to `size`, the entire array is available for analysis. `is_key` is true if the array is an object key. The default implementation does nothing.
      - `void end_array_(const core::value &v, bool is_key);` - Called when ending parsing of an array. `is_key` is true if the array is an object key. The default implementation does nothing.
      - `void begin_object_(const core::value &v, core::int_t size, bool is_key);` - Called when starting to parse an object. The size, if known, is passed in `size`. If the size is unknown, `size` is equal to `stream_handler::unknown_size`. If `v.size()` is equal to `size`, the entire object is available for analysis. `is_key` is true if the object is an object key. The default implementation does nothing.
      - `void end_object_(const core::value &v, bool is_key);` - Called when ending parsing of an object. `is_key` is true if the object is an object key. The default implementation does nothing.
   3. All data should be written via the member function `core::ostream &output_stream();`
   4. Required buffering features should be provided by member function `unsigned int required_features() const;` The available features are `const` members of the `core::stream_handler` class. The default implementation returns `stream_handler::requires_none`.
      
Since the default implementations of all these functions do nothing, an instance of `core::stream_handler` can be used as a dummy output sink that does nothing.

To create a new filter, follow the guidelines for output formats, but place them in the `cppdatalib::core` namespace. Inherit all filters from `core::stream_filter_base`. Writing should be done exclusively via member variable `output`. The reimplementation rules all still apply, but `core::stream_filter_base` provides pass-through writing of all values to `output`. This allows you to reimplement just what you need, and pass everything else through unmodified.

### Compile-time flags

Below is a list of compile-time type adjustments supported by cppdatalib:

   - `CPPDATALIB_BOOL_T` - The underlying boolean type of the implementation. Should be able to store a true and false value. Defaults to `bool`
   - `CPPDATALIB_INT_T` - The underlying integer type of the implementation. Should be able to store a signed integral value. Defaults to `int64_t`
   - `CPPDATALIB_UINT_T` - The underlying unsigned integer type of the implementation. Should be able to store an unsigned integral value. Defaults to `uint64_t`
   - `CPPDATALIB_REAL_T` - The underlying floating-point type of the implementation. Should be able to store at least an IEEE-754 value. Defaults to `double`
   - `CPPDATALIB_CSTRING_T` - The underlying C-style string type of the implementation. Defaults to `const char *`
   - `CPPDATALIB_STRING_T` - The underlying string type of the implementation. Defaults to `std::string`
   - `CPPDATALIB_ARRAY_T` - The underlying array type of the implementation. Defaults to `std::vector<cppdatalib::core::value>`
   - `CPPDATALIB_OBJECT_T` - The underlying object type of the implementation. Defaults to `std::multimap<cppdatalib::core::value, cppdatalib::core::value>`
   - `CPPDATALIB_SUBTYPE_T` - The underlying subtype type of the implementation. Must be able to store all subtypes specified in the `core` namespace. Default to `int16_t`

Enable/Disable flags are listed below:

   - `CPPDATALIB_ENABLE_MYSQL` - Enables inclusion of the MySQL interface library. If defined, the MySQL headers must be available in the include path
   - `CPPDATALIB_DISABLE_WRITE_CHECKS` - Disables nesting checks in the stream_handler class. If write checks are disabled, and the generating code is buggy, it may generate corrupted output without catching the errors, but can result in better performance. Use at your own risk
   - `CPPDATALIB_ENABLE_FAST_IO` - Swaps usage of the `std::ios` classes to trimmed-down, more performant, custom I/O classes. Although it acts as a drop-in replacement for the STL, it only implements a subset of the features (but the features it does implement should be usage-compatible). Use at your own risk
   - `CPPDATALIB_DISABLE_FAST_IO_GCOUNT` - Disables calculation of `gcount()` in the fast input classes. This removes the `gcount()` function altogether. This flag only has an effect if `CPPDATALIB_ENABLE_FAST_INPUT` is defined
   - `CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE` - Trims value sizes down to optimize space for large numeric arrays. This theoretically slows down string values somewhat, but saves space
   - `CPPDATALIB_ENABLE_BOOST_COMPUTE` - Enables the [Boost.Compute](http://www.boost.org/doc/libs/1_66_0/libs/compute/doc/html/index.html) adapters, to smoothly integrate with Boost.Compute types. The Boost source tree must be in the include path
   - `CPPDATALIB_ENABLE_BOOST_CONTAINER` - Enables the [Boost.Container](http://www.boost.org/doc/libs/1_66_0/doc/html/container.html) adapters, to smoothly integrate with Boost.Container types. The Boost source tree must be in the include path
   - `CPPDATALIB_ENABLE_QT` - Enables the [Qt](https://www.qt.io/) adapters, to smoothly integrate with the most common Qt types. The Qt source tree must be in the include path
   - `CPPDATALIB_ENABLE_POCO` - Enables the [POCO](https://pocoproject.org/) adapters, to smoothly integrate with POCO types. The "Poco" source tree must be in the include path
   - `CPPDATALIB_ENABLE_ETL` - Enables the [ETL](https://www.etlcpp.com/home.html) adapters, to smoothly integrate with ETL types. The "etl" source tree must be in the include path
   - `CPPDATALIB_ENABLE_STL` - Enables the STL adapters, to smoothly integrate with all STL types

Please note that custom datatypes are a work-in-progress. Defining custom types may work, or may not work at all.

## Supported datatypes

If a type is unsupported in a serialization format, the type is not converted to something recognizable by the format, but an error is thrown instead, describing the failure. However, if a subtype is not supported, the value is processed as if it had none (i.e. if a value is a `string`, with unsupported subtype `date`, the value is processed as a meaningless string and the subtype metadata is removed).

If a format-defined limit is reached, such as an object key length limit, an error will be thrown.

   - JSON supports `null`, `bool`, `uint` and `int` and `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtype `bignum` is fully supported, for both reading and writing.<br/>
       - There is no limit to the magnitude of numbers supported. `int` is attempted first, then `uint`, then `real`, then `bignum`.
       - Numerical metadata is lost when converting to and from JSON.
     
   - Bencode supports `uint`, `int`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`, `bool`, and `real`s are **not** supported.
       - integers are limited to two's complement 64-bit integers when reading.
       - No subtypes are supported.
       - Numerical metadata is lost when converting to and from Bencode.
     
   - plain text property lists support `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`s are **not** supported.<br/>
       - integers are limited to two's complement 64-bit integers when reading.
       - `string` subtypes `date`, `time`, and `datetime` are supported.
       - Numerical metadata is lost when converting to and from plain text property lists.
     
   - Binn supports `null`, `bool`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `date`, `time`, `datetime`, `bignum`, `blob`, and `clob` are supported.
       - `object` subtype `map` is supported.
       - Map keys are limited to signed 32-bit integers.
       - Object keys are limited to 255 characters or fewer.
       - Custom subtypes for `null`, `bool`, `int`, `string`, and `array` are supported, as long as they are at least equal to the value of `core::user`.
   
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
     
   - CSV supports `null`, `bool`, `uint`, `int`, `real`, `string`, and `array`.<br/>
     Notes:
       - `object`s are **not** supported.
       - `uint` values are fully supported when reading.
       - No subtypes are supported.
       - Numerical metadata is lost when converting to and from CSV.
     
   - UBJSON supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.
     Notes:
       - `string` subtype `bignum` is supported.
       - `uint` values are limited to the `int` value range when reading or writing, due to lack of format support for unsigned types

   - XML-XLS supports `null`, `bool`, `uint`, `int`, `real`, `string`, and `array`.<br/>
     Notes:
       - `object`s are **not** supported.
       - `string` subtypes `date`, `time`, and `datetime` are supported.
       - Numerical metadata is lost when converting to and from XML-XLS.

   - MessagePack supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - MessagePack extensions are not currently supported.
       - `string` subtypes `blob` and `clob` are supported.
       - MessagePack timestamps are not currently supported.

   - BJSON supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `blob` and `clob` are supported.
       - Numerical metadata is lost when converting to and from BJSON.

   - Netstrings supports `null`, `bool`, `uint`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - No subtypes are supported.
       - Type information is lost when converting to Netstrings.
