# cppdatalib

Simple, header-only, C++11 data conversion library

## Features

### Formats

cppdatalib offers implementations of several different serialization formats designed for hierarchical data (and some that aren't).
cppdatalib is able to easily convert to and from a standard internal representation.
Supported formats include

   - [JSON](https://json.org/)
   - [UBJSON](http://ubjson.org/)
   - [Bencode](https://en.wikipedia.org/wiki/Bencode)
   - [plain text property lists](http://www.gnustep.org/resources/documentation/Developer/Base/Reference/NSPropertyList.html)
   - [CSV](https://tools.ietf.org/html/rfc4180)
   - [Binn](https://github.com/liteserver/binn/blob/master/spec.md)
   - XML property lists (write-only)
   - [XML-RPC](http://xmlrpc.scripting.com/spec.html) (write-only)
   - [XML-XLS](https://msdn.microsoft.com/en-us/library/aa140066(office.10).aspx) (write-only)
   - [MessagePack](https://msgpack.org/) (write-only)

### Filters

cppdatalib offers a variety of filters that can be applied to stream handlers. These include the following:

   - buffer_filter (Optionally buffers strings, arrays, and objects, or any combination of the same, or acts as a pass-through filter)
   - automatic_buffer_filter (Automatically determines the correct settings for the underlying buffer_filter based on the output stream handler)
   - tee_filter (Splits an input stream to two output stream handlers)
   - duplicate_key_check_filter (Ensures the input stream only provides unique keys in objects. This filter supports complex keys, including nested objects)
   - converter_filter (Converts from one internal type to another, for example, all integers to strings. This filter has built-in conversions)
   - custom_converter_filter (Converts the specified internal type, using the user-specified converter function. This filter supports varying output types, including the same type as the input)
   - generic_converter_filter (Sends all scalar values to a user-specified function for conversion. Arrays and objects cannot be converted with this filter)

Filters can be assigned on top of other filters. How many filters are permitted is limited only by the runtime environment.

### Large Data

cppdatalib supports streaming with a small memory footprint. Most conversions require no buffering or minimal buffering. Also, there is no limit to the nesting depth of arrays or objects. This makes cppdatalib much more suitable for large datasets.

## Limitations

   - Internal operation of complex object keys is recursive (due to STL constraints) and deeply nested arrays or objects in complex keys may result in undefined behavior. Note that this limitation does not apply to array elements or object values, just object keys.
   - Currently, comparison of `core::value` objects is recursive, thus comparison of deeply nested arrays or objects may result in undefined behavior.

## Usage

Using the library is simple. Everything is under the main namespace `cppdatalib`, and underneath is the `core` namespace and individual format namespaces (e.g. `json`).
If you only need one format, use `using` statements to include its namespace into your scope. You can also include the `core` namespace, as long as you don't have conflicting identifiers.

For example, the following program attempts to read a JSON structure from STDIN, and output it to STDOUT:

```c++
#include <cppdatalib/cppdatalib.h>

int main() {
    using namespace cppdatalib;             // Parent namespace
    using namespace json;                   // Format namespace
    
    core::value my_value;
    
    try {
        std::cin >> my_value;               // Read in to core::value as JSON
        std::cout << my_value;              // Write core::value out as JSON
    } catch (core::error e) {
        std::cerr << e.what() << std::endl; // Catch any errors that might have occured (syntax or logical)
    }

    return 0;
}
```

When using more than one format, you either should use the `to_xxx` and `from_xxx` string functions for a specific format,
or the `input` and `print` functions that take two parameters, instead of the `>>` and `<<` operators.
The operators are ambiguous when using more than one format.

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
        json::convert(std::cin, writer);      // Convert from JSON on standard input to UBJSON on standard output
                                              // Note that this DOES NOT READ the entire stream before writing!
                                              // The data is read and written at the same time

        json::convert(std::cin, builder);     // Convert from JSON on standard input to internal representation in
                                              // `my_value`. Note that my_value is also accessible by using `builder.value()`

        core::convert(my_value, writer);      // Convert from internal representation to UBJSON on standard output
    } catch (core::error e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    try {
        core::stream_filter<core::null, core::string> filter(writer);
                                              // Set up filter on UBJSON output that converts all `null` values to empty strings

        json::convert(std::cin, filter);      // Convert from JSON on standard input to UBJSON on standard output, converting `null`s to empty strings
                                              // When using a filter, write to the filter, instead of the handler the filter is modifying
                                              // (i.e. don't write to `writer` here unless you don't want to employ the filter)
                                              // Note that this DOES NOT READ the entire stream before writing!
                                              // The data is read and written at the same time
    } catch (core::error e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    try {
        core::stream_filter<core::null, core::string> filter(writer);
                                              // Set up filter on UBJSON output that converts all `null` values to empty strings

        auto lambda = [](core::value &v)
        {
            if (v.is_string() && v.get_string().find('a') == 0)
                v.set_string("");
        };
        core::generic_stream_filter<decltype(lambda)> generic_filter(filter, lambda);
                                              // Set up filter on top of previous filter that clears all strings beginning with lowercase 'a'

        json::convert(std::cin, generic_filter);
                                              // Convert from JSON on standard input to UBJSON on standard output,
                                              // converting `null`s to empty strings, and clearing all strings beginning with 'a'
                                              // Again, note that this does not read the entire stream before writing
                                              // The data is read and written at the same time
    } catch (core::error e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    try {
        core::stream_filter<core::boolean, core::integer> second_filter(writer);
                                              // Set up filter on UBJSON output that converts booleans to integers

        core::stream_filter<core::integer, core::real> first_filter(second_filter);
                                              // Set up filter on top of previous filter that converts all integers to reals

        json::convert(std::cin, first_filter);
                                              // Convert from JSON on standard input to UBJSON on standard output,
                                              // converting booleans to integers, and converting integers to reals
                                              // Note that order of filters is important. The last filter enabled will be the first to be called.
                                              // If the filter order was switched, all booleans and integers would become reals.
    } catch (core::error e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    try {
        core::tee_filter tee(writer, builder);
                                              // Set up tee filter. Tee filters split the input to two handlers, which can be stream_filters or other tee_filters.
                                              // In this case, we'll parse the JSON input once, and output to UBJSON on standard output and build an internal
                                              // representation simultaneously.

        json::convert(std::cin, first_filter);
                                              // Convert from JSON on standard input to UBJSON on standard output,
                                              // as well as building internal representation in my_value
    } catch (core::error e) {
        std::cerr << e.what() << std::endl;   // Catch any errors that might have occured (syntax or logical)
    }

    return 0;
}
```

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
       - `string` subtypes `date`, `time`, `datetime`, `bignum`, and `blob` are supported.
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
     
   - CSV supports `null`, `uint`, `bool`, `int`, `real`, `string`, and `array`.<br/>
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
       - `string` subtype `blob` is supported.
       - MessagePack timestamps are not currently supported.
