**StdEx** is an extension library for std. It contains components that may be used across various technical domains.

For detailed usage instructions, please refer to: [help](/Documents/StdEx.md)

For contributing a new module to STDEX, please refer to: [submitting](/Documents/StdEx_Submitting.md)

---

### License

This Project is license under the MIT License.

See the [LICENSE](LICENSE) file for details.

- **Copyright (c) 2025 4573qu**

- [type/notation.h](type/notation.h) is inspired by implementations from the [nlohmann/json](https://github.com/nlohmann/json) library (MIT License).

### General Headers(Currently Unavailable)

For single-file development (non-project mode), use:  
`#include <cstdex>`

For project reference development mode, use:  
`#include <stdex.h>`

---

### Directory Overview
- **bitwise**: Bitwise operations
- **container**: Containers like enhanced any or function
- **crypto**: Encodings and encryptions
- **logging**: Logging operations *(Currently unavailable)*
- **machine**: Machine-level operations *(Currently unavailable)*
- **macros**: General MACROS for library files and developers
- **math**: Mathematical operations
- **memory**: Operations of memory level
- **meta**: Metadata operations *(Currently unavailable)*
- **profiling**: Profiling tools
- **structure**: Common data structures
- **syntax**: Lexical & syntax operations 
- **type**: Data type operations*(Currently supports ini)*
- **utility**: Quick tools like syntactic sugar
- **vision**: Vision-related operations *(Currently unavailable)*  



### See future features

You can check branch:dev to see which may be added to branch:main next.

The integrity and security of any files not in branch:main are not guaranteed.

The branch:dev can only show that which is in the preparation list and which is not.



### Third-party

- break_eternity.js (MIT), maintained by Patashu.

  Used as the basis for [decimal.h](math/decimal.h) (only).

  License: [break_eternity.js MIT License](third_party_licenses/break_eternity.js_MIT.txt)

  Upstream: [github repository](https://github.com/Patashu/break_eternity.js)
