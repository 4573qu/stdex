# StdEx使用指南

## 关键内容说明

StdEx库是一个包含文件模式的C++扩展库，本库使用非外部链接，直接引用的方法进行使用。

什么是**单文件使用模式**：C++可以使用非项目的模式进行快速开发。在该场景下，通常只有一个cpp作为主文件并提供main方法（以使用Dev C++新建源文件为例）。如您在该情况下需要引用本库，请参考本模式（如需使用完整stdex库，请用**#include \<stdex.h>**）。

什么是**项目使用模式**：使用C++进行完整项目开发或项目类模式开发时，通常按照C++的头文件引用标准引入外部库（以Dev C++新建项目或使用CMake构建项目为例。如您在该情况下需要引用本库，请参考本模式（如使用完整stdex库，请用**#include \<cstdex>**）。

**通常**额外说明是给深度开发者展示的，如果您不是深度开发者，请无视该条目。

## bitmask/flags.h(Version 1.1.0.1)

### 基本信息

#### 概要

flags.h旨在提供一个标志集，提供类型安全的位掩码标志管理，用于维护多个标志是否启用的状态。

使用flags.h提供的标志集，可以枚举标志的集合操作（添加/删除/检测），快速地对标志进行标识和清除，并检测特定标识是否启用。

#### 使用方法

单文件使用模式：`#include <bitmask/flags.h>`

项目使用模式：`#include <bitmask/flags.h>`

#### 使用场景

本库提供的标志集常常用于多个同类型标识需要分别记录状态的情况。

以文件系统的访问权限为例，使用本标志集可以同时完成对文件的读操作、写操作、执行操作等操作权限进行管理。

以游戏为例，如果一个伤害可能存在几种附加效果，使用本标志集可以完成对几种效果的快速记录。

#### 使用限制

由于标志集本质上使用二进制的与或运算来记录，故对任意一个标志，应当将其设置为2的n次幂，以保证多个不同的标志之间不会相互影响，同时，可以设置一些集合为多个标志叠加后的情况，将一些常见的组合更快的进行标志集相关操作。

基于上述需求，标志集的模板要求其类型必须为枚举类型`enum`或`enum class`，否则会视为未定义行为，触发`static_assert`。

由于机器限制和标志集内部操作设置，标志集最多的可枚举标志数和机器字长一致，即前文的幂数n∈[0,机器字长)，如果使用枚举类`enum class`时，会受到枚举类类型制约。

使用`enum class`时，建议将枚举类类型设置为无符号数来避免出现预期外情况。

### 数据结构

#### `class flags<_Tp>`

##### 所属命名空间

`stdex::bitmask`

##### 功能简述

本文件的核心类。本类提供了本文件所有的标志集方法。

##### 成员说明

> [!WARNING]
>
> \_Tp：必须是枚举类，否则会触发static\_assert。**通常\_Tp需要满足其中每个枚举类型满足取值为2的若干次幂或部分标志的集合，<span style="color:red">除非你清楚本类会进行的行为</span>。**

##### 额外说明

对内部标志的管理使用的是std::underlying\_type\_t<\_Tp>。

##### 成员函数

| 方法                                                         | 说明             | 复杂度 |
| :----------------------------------------------------------- | :--------------- | ------ |
| `flags()`                                                    | 创建空标志集     | O(1)   |
| `flags(_Tp e) noexcept`                                      | 用单个标志初始化 | O(1)   |
| `flags& operator =(_Tp e) noexcept`                          | 设置为特定标志   | O(1)   |
| `flags& operator <<=(_Tp e) noexcept`<br>`flags operator <<(_Tp e) noexcept` | 添加标志         | O(1)   |
| `flags& operator >>=(_Tp e) noexcept` <br>`flags operator <<(_Tp e) noexcept` | 移除标志         | O(1)   |
| `bool contains(_Tp e) const noexcept`                        | 检测标志是否存在 | O(1)   |
| `operator typename std::underlying_type_t<_Tp>() const noexcept` | 获取底层整数值   | O(1)   |
| `void clear() noexcept`                                      | 清空所有标志     | O(1)   |
| `bool empty() const noexcept`                                | 检查是否为空集   | O(1)   |

### 其他方法

#### 增强宏

##### `_STDEX_ENABLE_FLAGS_ENHANCED(EnumType)`

使用宏后，会对`EnumType`启用全局运算符重载，使之可以使用额外的运算符。

> [!WARNING]
>
> **<span style="color:red">请不要手动使用`struct stdex::bitmask::flags<EnumType>::_flags_enhanced : std::true_type {};`的方法，这将导致未定义行为。</span>**

| 方法                                                         | 说明                                                         | 复杂度 |
| :----------------------------------------------------------- | :----------------------------------------------------------- | ------ |
| `flags<EnumType> operator <<(EnumType lhs,EnumType rhs) noexcept` | 相当于`flags<EnumType>(lhs)<<rhs`。<br>可以更高效地使用<<运算符。用标志枚举直接初始化 | O(1)   |
| `flags<EnumType> operator >>(EnumType lhs,EnumType rhs) noexcept` | 相当于`flags<EnumType>(lhs)>>rhs`。<br/>可以更高效地使用>>运算符。通常在lhs表示为包含rhs和若干标志的集合时有效。 | O(1)   |

### 使用样例

##### 以文件权限管理为例

```cpp
enum class FilePermission : uint32_t {
	None=0,		//0000
	Read=1,		//0001
	Write=2,	//0010
	Execute=4,	//0100
	Delete=8,	//1000
    All=15,		//1111
}

struct file{`
	stdex::bitmask::flags<FilePermisson> permission_;
	//other members...
	void add_permission(FilePermission permission) { permission_<<=permission; }
	void set_admin_permission() { permission_=FilePermission::All; }
	bool read_file(std::string& content) {
		if (!permission_.contains(FilePermission::Read)) return false;
		//read something...
		return true;
	}
	//other functions...
}
```

在上述代码中，我们创建了一个文件系统，其中`stdex::bitmask::flags<FilePermission>`用于管理文件权限。对于不同的权限，在`FilePermission`里给予了不同的取值，并提供了一个集合`FilePermission::All`来代表所有权限。

##### 以字体设置为例（使用增强宏）

```cpp
enum class FontStyle : uint32_t {
    None=0,			//000
    Bold=1,			//001
    Italic=2,		//010
    Underline=4,	//100
    All=7,			//111
}
_STDEX_ENABLE_FLAGS_ENHANCED(FontStyle)
int main() {
    stdex::bitmask::flags<FontStyle> aHeadStyle=FontStyle::Bold<<FontStyle::Underline;
    stdex::bitmask::flags<FontStyle> aCommentStyle=FontStyle::All>>FontStyle::Italic>>FontStyleBold;
}
```

在上述代码中，我们使用`FontStyle`枚举来表示字体风格。启用了`_STDEX_ENABLE_FLAGS_ENHANCED`宏后，我们可以直接使用`FontStyle::Bold<<FontStyle::Underline`等类似的方式来完成标志集的创建。

## math/math.h

## math/matrix.h

## math/geometry/graphics.h

## math/geometry/trajectory.h

## meta/database.h-(V1.0)

### 简述

本文件的主要功能是提供一个支持类SQL语句和自定义数据结构的数据库。

单文件使用模式：#include <meta/database.h>

项目使用模式：#include <meta/database.h>

### 数据结构

#### class database

##### 功能简述

## meta/dynamic_struct.h-(V1.21)

### 简述

本文件的主要功能是提供一个动态提供类型的数据结构。

单文件使用模式：#include <meta/dynamic_struct.h>

项目使用模式：#include <meta/dynamic_struct.h>

## other/diff_match_patch.h

## structure/nary_tree.h

## syntax/lexer.h-(V1.41)

### 简述

本文件的主要功能是提供一个可以自定义文法的词法分析器。

单文件使用模式：#include <syntax/lexer.h>

项目使用模式：#include <syntax/lexer.h>

## syntax/parser.h

## type/bitmap.h

## type/json.h

## vision/easing.h