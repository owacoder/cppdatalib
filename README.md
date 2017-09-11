# cppdatalib

Simple, header-only C++ data conversion library

cppdatalib offers implementations of several different serialization formats designed for heirarchical data (and some that aren't).
cppdatalib is able to easily convert to and from a standard internal representation.
Supported formats include

   - JSON
   - Bencode
   - plain text property lists
   - XML property lists (write-only)
   - XML-RPC (write-only)
   - CSV (write-only)
   - UBJSON (write-only)

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

Note: If an `input` or `print` function takes more than two parameters, ignore the extra parameters. They are only needed for internal use.
