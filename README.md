# cppdatalib

Simple, header-only C++ data conversion library

cppdatalib offers implementations of several different serialization formats designed for hierarchical data (and some that aren't).
cppdatalib is able to easily convert to and from a standard internal representation.
Supported formats include

   - JSON
   - UBJSON
   - Bencode
   - plain text property lists
   - CSV
   - Binn (write-only)
   - XML property lists (write-only)
   - XML-RPC (write-only)
   - XML-XLS (write-only)

### Usage

Using the library is simple. Everything is under the main namespace `cppdatalib`, and underneath is the `core` namespace and individual format namespaces (e.g. `json`).
If you only need one format, use `using` statements to include its namespace into your scope. You can also include the `core` namespace, as long as you don't have conflicting identifiers.

For example, the following program attempts to read a JSON structure from STDIN, and output it to STDOUT:

```c++
#include <cppdatalib/core.h>

int main() {
    using namespace cppdatalib;
    using namespace json;
    
    core::value my_value;
    
    try {
        std::cin >> my_value;
        std::cout << my_value;
    } catch (core::error e) {
        std::cerr << e.what() << std::endl;
    }
}
```

When using more than one format, you either should use the `to_xxx` and `from_xxx` string functions for a specific format,
or the `input` and `print` functions that take two parameters, instead of the `>>` and `<<` operators.
The operators are ambiguous when using more than one format.

### Supported datatypes

If a type is unsupported in a serialization format, the type is not converted to something recognizable by the format, but an error is thrown instead, describing the failure. However, if a subtype is not supported, the value is processed as if it had none (i.e. if a value is a `string`, with unsupported subtype `date`, the value is processed as a meaningless string and the subtype metadata is removed).

If a format-defined limit is reached, such as an object key length limit, an error will be thrown.

   - JSON supports `null`, `bool`, `int` and `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `int`s are mapped onto `double`s in the implementation.
       - No subtypes are supported.
     
   - Bencode supports `int`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`, `bool`, and `real`s are **not** supported.
       - No subtypes are supported.
     
   - plain text property lists support `bool`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`s are **not** supported.<br/>
       - `string` subtypes `date`, `time`, and `datetime` are supported.
     
   - Binn supports `null`, `bool`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `string` subtypes `date`, `time`, `datetime`, `bignum`, and `blob` are supported.
       - `object` subtype `map` is supported.
       - Map keys are limited to signed 32-bit integers.
       - Object keys are limited to 255 characters or fewer.
       - Custom subtypes for `null`, `bool`, `int`, `string`, and `array` are supported, as long as they are at least equal to the value of `core::user`.
   
   - XML property lists support `bool`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`s are **not** supported.
       - `string` subtypes `date`, `time`, and `datetime` are supported.
     
   - XML-RPC supports `bool`, `int`, `real`, `string`, `array`, and `object`.<br/>
     Notes:
       - `null`s are **not** supported.
       - No subtypes are supported.
     
   - CSV supports `null`, `bool`, `int`, `real`, `string`, and `array`.<br/>
     Notes:
       - `object`s are **not** supported.
       - No subtypes are supported.
     
   - UBJSON supports `null`, `bool`, `int`, `real`, `string`, `array`, and `object`.
     Notes:
       - `string` subtype `bignum` is supported.

   - XML-XLS supports `null`, `bool`, `int`, `real`, `string`, and `array`.<br/>
     Notes:
       - `object`s are **not** supported.
       - `string` subtypes `date`, `time`, and `datetime` are supported.
