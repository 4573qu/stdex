**StdEx** is an extension library for Std. It contains components that may be used across various technical domains.

For detailed usage instructions, please refer to: [help](/Documents/StdEx.md)

For contributing a new module to STDEX, please refer to: [submitting](/Documents/StdEx_Submitting.md)

//Submitting should contains 哪些模块会被接受 & 编码规范

---

For single-file development (non-project mode), use:  
`#include <cstdex>`

For project reference development mode, use:  
`#include <stdex.h>`

---

### Directory Overview
- **bitwise**: Bitwise operations  
- **container**: Containers like enhanced any or function
- **crypto**: Encodings and encryptions
- **logging**: Logging operations *(Temporarily unavailable)*  
- **machine**: Machine-level operations *(Currently Windows-only)* *(Temporarily unavailable)* 
- **macros**: General MACROS for library files and developers 
- **math**: Mathematical operations  
- **memory**: Operations of memory level
- **meta**: Metadata operations *(Temporarily unavailable)*  
- **other**: Uncategorized items *(Includes modified `diff_match_patch` from [Google OSS](https://github.com/google/diff-match-patch))* *(Temporarily unavailable)*  
- **profiling**: Profiling tools
- **structure**: Common data structures *(DisjointSet & NaryTree is Temporarily unavailable)*  
- **syntax**: Lexical & syntax operations *(Lexer is Temporarily unavailable)*  
- **type**: Data type operations 
  *(Currently supports bitmap/ini/json; `json` library modified from [nlohmann OSS](https://github.com/nlohmann/json))* *(Temporarily unavailable)*  
- **utility**: Quick tools like syntactic sugar
- **vision**: Vision-related operations *(Temporarily unavailable)*  
