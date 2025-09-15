# StdEx使用指南

## 关键内容说明

StdEx库是一个包含文件模式的C++扩展库，本库使用非外部链接，直接引用的方法进行使用。

什么是**单文件使用模式**：C++可以使用非项目的模式进行快速开发。在该场景下，通常只有一个cpp作为主文件并提供main方法（以使用Dev C++新建源文件为例）。如您在该情况下需要引用本库，请参考本模式（如需使用完整stdex库，请用**#include \<stdex.h>**）。

什么是**项目使用模式**：使用C++进行完整项目开发或项目类模式开发时，通常按照C++的头文件引用标准引入外部库（以Dev C++新建项目或使用CMake构建项目为例。如您在该情况下需要引用本库，请参考本模式（如使用完整stdex库，请用**#include \<cstdex>**）。

**通常**额外说明是给深度开发者展示的，如果您不是深度开发者，请无视该条目。

> [!WARNING]
>
> **<span style="color:red">所有库内宏均以\_STDEX\_开头，在使用本库时尽量避免指南中允许以外的以\_STDEX\_开头的宏定义，这可能会导致预期之外的结果。</span>**

被标记为`[[deprecated]]`的函数会在给出处明确使用“**不建议的**”字样标注，请注意。

## bitwise/flags.h(Version 2.0.2.1)

### 基本信息

#### 概要

flags.h旨在提供一个类型安全的位掩码标志管理的标志集功能，用于维护多个标志是否启用的状态。

使用flags.h提供的标志集，可以枚举标志的集合操作（添加/删除/检测），快速地对标志进行标识和清除，并检测特定标识是否启用。

同时，flags.h还提供一系列具有特定强化方向的增强标志集，可以通过设置标志之间的关系进行更复杂的管理。

#### 使用方法

单文件使用模式：`#include <bitwise/flags.h>`

项目使用模式：`#include <bitwise/flags.h>`

#### 使用场景

本库提供的标志集常常用于多个同类型标识需要分别记录状态的情况。

以文件系统的访问权限为例，使用本标志集可以同时完成对文件的读操作、写操作、执行操作等操作权限进行管理。

以游戏为例，如果一个伤害可能存在几种附加效果，使用本标志集可以完成对几种效果的快速记录。

额外提供的增强标志集可以设置标志之间的互斥、依赖和禁止关系，以完成更复杂的功能。

以选项选择为例，开发者可以将互为相反的选项设置为互斥组，并设置替换关系，则标志集可以自动管理选项之间的互斥维护。

以鉴权为例，如果一个项目的操作需要完成登入操作且不能是游客身份，开发者可以设置几个操作之间的依赖与禁止关系，使标志集自动维护该效果。

#### 使用限制

由于标志集本质上使用二进制的与或运算来记录，故对任意一个标志，应当将其设置为2的n次幂，以保证多个不同的标志之间不会相互影响，同时，可以设置一些集合为多个标志叠加后的情况，将一些常见的组合更快的进行标志集相关操作。

基于上述需求，标志集的模板要求其类型必须为枚举类型`enum`或`enum class`，否则会视为未定义行为，触发`static_assert`。

由于机器限制和标志集内部操作设置，标志集最多的可枚举标志数和机器字长一致，即前文的幂数n∈[0,机器字长)，如果使用枚举类`enum class`时，会受到枚举类类型制约。

使用`enum class`时，建议将枚举类类型设置为无符号数来避免出现预期外情况。

### 数据结构

#### `class flags<_Tp>`

##### 所属命名空间

`stdex::bitwise`

##### 功能简述

本文件的核心类之一。本类提供了本文件的基础标志集方法。

##### 成员说明

> [!WARNING]
>
> \_Tp：必须是枚举类，否则会触发static\_assert。**通常\_Tp需要满足其中每个枚举类型满足取值为2的若干次幂或部分标志的集合，<span style="color:red">除非你清楚本类会进行的行为</span>。**

##### 额外说明

对内部标志的管理使用的是std::underlying\_type\_t<\_Tp>。

##### 成员函数

| 方法                                                         | 说明                 | 复杂度 |
| :----------------------------------------------------------- | :------------------- | ------ |
| `flags()`                                                    | 创建空标志集         | O(1)   |
| `flags(_Tp e) noexcept`                                      | 用单个标志初始化     | O(1)   |
| `flags& operator =(_Tp e) noexcept`                          | 直接赋值             | O(1)   |
| `virtual flags& operator <<=(_Tp e)`<br>`virtual flags operator <<(_Tp e)` | 添加标志             | O(1)   |
| `flags& operator <<=(flags<_Tp> value) noexcept`<br/>`flags operator <<(flags<_Tp> value) noexcept` | 添加一组标志         | O(n)   |
| `flags& operator >>=(_Tp e) noexcept` <br>`flags operator <<(_Tp e) noexcept` | 移除标志             | O(1)   |
| `bool contains(_Tp e) const noexcept`                        | 检测标志是否存在     | O(1)   |
| `operator typename std::underlying_type_t<_Tp>() const noexcept` | 获取底层整数值       | O(1)   |
| `operator _Tp() const noexcept`                              | 获取枚举类型的底层值 | O(1)   |
| `void clear() noexcept`                                      | 清空所有标志         | O(1)   |
| `bool empty() const noexcept`                                | 检查是否为空集       | O(1)   |
| `template <typename _Func>`<br>`void for_each(_Func func) const` | 对每个生效值执行func | O(n)   |

**绝大多数函数均被标记为constexpr，其在性能上将非常优秀。**

> [!WARNING]
>
> **<span style="color:red">由于constexpr虚函数等行为只在C++20标准下才合法，所以operator <<=和operator <<使用了_STDEX_CONSTEXPR标记，该标记仅在C++20标准下被特化为constexpr。如果您需要对该函数的性能有所要求，请注意甄别所处的C++标准。</span>**

##### 其他方法

###### 增强宏

`_STDEX_ENABLE_FLAGS_ENHANCED(EnumType)`

使用宏后，会对`EnumType`启用全局运算符重载，使之可以使用额外的运算符。

> [!WARNING]
>
> **<span style="color:red">请不要手动使用`struct stdex::bitwise::flags<EnumType>::_flags_enhanced : std::true_type {};`的方法，这将导致未定义行为。</span>**

| 方法                                                         | 说明                                                         | 复杂度 |
| :----------------------------------------------------------- | :----------------------------------------------------------- | ------ |
| `flags<EnumType> operator <<(EnumType lhs,EnumType rhs) noexcept` | 相当于`flags<EnumType>(lhs)<<rhs`。<br>可以更高效地使用<<运算符。用标志枚举直接初始化 | O(1)   |
| `flags<EnumType> operator >>(EnumType lhs,EnumType rhs) noexcept` | 相当于`flags<EnumType>(lhs)>>rhs`。<br>可以更高效地使用>>运算符。通常在lhs表示为包含rhs和若干标志的集合时有效。 | O(1)   |

##### 使用样例

###### 以文件权限管理为例

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
	stdex::bitwise::flags<FilePermisson> permission_;
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

在上述代码中，我们创建了一个文件系统，其中`stdex::bitwise::flags<FilePermission>`用于管理文件权限。对于不同的权限，在`FilePermission`里给予了不同的取值，并提供了一个集合`FilePermission::All`来代表所有权限。

###### 以字体设置为例（使用增强宏）

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
    stdex::bitwise::flags<FontStyle> aHeadStyle=FontStyle::Bold<<FontStyle::Underline;
    stdex::bitwise::flags<FontStyle> aCommentStyle=FontStyle::All>>FontStyle::Italic>>FontStyleBold;
}
```

在上述代码中，我们使用`FontStyle`枚举来表示字体风格。启用了`_STDEX_ENABLE_FLAGS_ENHANCED`宏后，我们可以直接使用`FontStyle::Bold<<FontStyle::Underline`等类似的方式来完成标志集的创建。

#### `class exclusive_flags<_Tp,_DefaultPolicy>`

##### 所属命名空间

`stdex::bitwise`

##### 功能简述

本文件的核心类之一。本类提供了本文件的互斥标志集方法。

##### 成员说明

`enum relation_policy`

互斥集使用的关系策略。该策略枚举不是类内成员。取值包含`RP_FORCE`、`RP_REJECT`、`RP_EXCEPTION`，其中`RP_REJECT`是`_DefaultPolicy`的默认取值。

`std::map<_Tp,flags<_Tp>*> exclusions`_

互斥集的互斥组合管理。此成员仅能通过成员函数进行const访问。

`relation_policy exclusion_policy_`

选择的互斥关系策略。初始取值为`_DefaultPolicy`，随后仅能通过成员函数修改。

##### 额外说明

互斥策略影响添加标志出现互斥时的行为。选择`RP_FORCE`时，将强行添加当前标志并移除其他互斥标志；选择`RP_REJECT`时，将不进行任何操作；选择`RP_EXCEPTION`时，会抛出`std::invalid_argument`错误。

互斥关系的相互的：即将三个元素A、B、C使用两次绑定如AB、AC（或AB、BC，AC、BC）就可以将三者设置为互斥。互斥组可以理解为：每一个互斥组中最多同时有一个元素存在于标志集中；不同互斥组的交集为空集。

##### 成员函数

`exclusive_flags<_Tp,_DefaultPolicy>`继承了`flags<_Tp>`的全部方法。同时，其还有以下特殊方法。

| 方法                                                         | 说明                             | 复杂度 |
| :----------------------------------------------------------- | :------------------------------- | ------ |
| `exclusive_flags()`                                          | 创建空标志集，并赋予默认关系策略 | O(1)   |
| `template <relation_policy _OtherPolicy>`<br>`exclusive_flags(const exclusive_flags<_Tp,_OtherPolicy>& other)` | 拷贝构造函数                     | O(1)   |
| `~exclusive_flags()`                                         | 析构函数                         | O(n)   |
| `exclusive_flags& operator =(_Tp e) noexcept`                | **不建议的**直接赋值             | O(1)   |
| `template <relation_policy _OtherPolicy>`<br>`exclusive_flags& operator =(const exclusive_flags<_Tp,_OtherPolicy>& other)` | 拷贝赋值函数，但**不会**复制策略 | O(n)   |
| `void set_exclusion_policy(relation_policy policy)`          | 设置关系策略                     | O(1)   |
| `void set_exclusion(_Tp lhs,_Tp rhs)`                        | 设置两个标志的互斥关系           | O(1)   |
| `void clear_exclusion(_Tp e)`                                | 清除某个标志的互斥关系           | O(1)   |
| `virtual exclusive_flags& operator <<=(_Tp e)`               | 添加标志                         | O(1)   |
| `const std::map<_Tp,flags<_Tp>*>& exclusions() const`        | 获取互斥组成员                   | O(1)   |

**与flags<\_Tp>一样，绝大多数可以使用constexpr的函数都被标记了constexpr，这使得性能大大提升。有关\_STDEX\_CONSTEXPR的内容与前文保持一致。**

> [!WARNING]
>
> **<span style="color:red">除非你明确清楚使用operator =直接赋予某个\_Tp类型的值是不会经过互斥检查的，否则请不要使用标记为`[[deprecated]]`的operator =函数。如果你清楚上述行为，且需要使用operator =，你可以使用\_STDEX\_IGNORE\_BITWISE\_FLAGS\_WARNINGS宏或引入<macros/ignore_warnings.h>来避免抛出警告。</span>**

##### 使用样例



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