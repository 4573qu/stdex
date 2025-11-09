# StdEx使用指南

## 关键内容说明

StdEx库是一个包含文件模式的C++扩展库，本库使用非外部链接，直接引用的方法进行使用。

什么是**单文件使用模式**：C++可以使用非项目的模式进行快速开发。在该场景下，通常只有一个cpp作为主文件并提供main方法（以使用Dev C++新建源文件为例）。如您在该情况下需要引用本库，请参考本模式（如需使用完整stdex库，请用**#include \<stdex.h>**）。

什么是**项目使用模式**：使用C++进行完整项目开发或项目类模式开发时，通常按照C++的头文件引用标准引入外部库（以Dev C++新建项目或使用CMake构建项目为例。如您在该情况下需要引用本库，请参考本模式（如使用完整stdex库，请用**#include \<cstdex>**）。

若您无法找到上述的通用头文件，这说明该文件暂时不可用，请手动引用所需要的文件。

**通常**额外说明是给深度开发者展示的，如果您不是深度开发者，请无视该条目。

> [!WARNING]
>
> **<span style="color:red">所有库内宏均以\_STDEX\_开头，在使用本库时尽量避免指南中允许以外的以\_STDEX\_开头的宏定义，这可能会导致预期之外的结果。</span>**

被标记为`[[deprecated]]`的函数会在给出处明确使用“**不建议的**”字样标注，请注意。

## bitwise/flags.h(Version 2.0.2.6)

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

### 枚举类型

#### `enum relation_policy`

互斥集和增强集使用的关系策略。取值包含`RP_FORCE`、`RP_REJECT`、`RP_EXCEPTION`。

用于数据结构`class exclusive_flags<_Tp,DefaultPolicy>`和`class advanced_flags<_Tp,_DefaultExclusionPolicy,_DefaultForbiddenPolicy,_DefaultDependencyPolicy>`。

#### `enum consistency_type`

增强集检查依赖-禁止环的返回值的类型。取值包含`CT_CYCLE`（存在依赖环）、`CT_FORBIDDEN_WITH_DEPENDENCY`（依赖且禁止）、`CT_REVERSE_FORBIDDEN_WITH_DEPENDENCY`（依赖且禁止，但依赖关系与禁止关系在二者路径上方向相反）、`CT_FORBIDDEN_SELF`（禁止自己）。

### 数据结构

#### `class flags<_Tp>`

##### 所属命名空间

`stdex::bitwise`

##### 功能简述

本文件的核心类之一。本类提供了本文件的基础标志集方法。

##### 成员说明

> [!WARNING]
>
> `_Tp`：必须是枚举类，否则会触发`static_assert`。**通常\_Tp需要满足其中每个枚举类型满足取值为2的若干次幂或部分标志的集合（即若干枚举值与操作的结果），<span style="color:red">除非你清楚本类会进行的行为</span>。**

##### 额外说明

对内部标志的管理使用的是`std::underlying_type_t<_Tp>`。

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

本文件的核心类之一。本类提供了本文件的互斥标志集方法。标记为互斥的标志集无法同时被标记在集合内。可以对互斥行为进行不同的策略设置。

##### 成员说明

`std::map<_Tp,flags<_Tp>*> exclusions_`

互斥集的互斥组合管理。此成员仅能通过成员函数进行const访问。

`relation_policy exclusion_policy_`

选择的互斥关系策略。初始取值为`_DefaultPolicy`，随后仅能通过成员函数修改。`_DefaultPolicy`的默认值是`RP_REJECT`。

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

**与`flags<_Tp>`一样，绝大多数可以使用`constexpr`的函数都被标记了`constexpr`，这使得性能大大提升。有关`_STDEX_CONSTEXPR`的内容与前文保持一致。**

> [!WARNING]
>
> **<span style="color:red">除非你明确清楚使用`operator =`直接赋予某个`_Tp`类型的值是不会经过互斥检查的，否则请不要使用标记为`[[deprecated]]`的`operator =`函数。如果你清楚上述行为，且需要使用`operator =`，你可以使用`_STDEX_IGNORE_BITWISE_FLAGS_WARNINGS`宏或引入`<macros/ignore_warnings.h>`来避免抛出警告。</span>**

##### 使用样例

###### 以餐厅系统加料为例

```cpp
enum class SeasoningOptions : uint32_t {
    Salad=1,
    Tomato=2,
    Beef=4,
    Cheese=8,
}
//we assume salad and tomato is mutually exclusive
//beef and cheese is also excusive

stdex::bitwise::exclusive_flags<SeasoningOptions> seasoning;

void InitializeCanteen() {
    seasoning.clear();
    seasoning.clear_exclusion();
    seasoning.set_exclusion(SeasoningOptions::Salad,SeasoningOptions::Tomato);
    seasoning.set_exclusion(SeasoningOptions::Beef,SeasoningOptions::Cheese);
}

void TryAddSeasoning(SeasoningOptions s) {
    seasoning<<=s;
}

void MakeFood() {
    if (seasoning.contains(SeasoningOptions::Salad)) {
        //add salad...
    }
    //other seasoning operations
}
```

在上述代码中，我们创建了一个沙拉酱与番茄酱互斥，牛肉和奶酪的加料系统，并设置策略为拒绝。此时调用`TryAddSeasoning(s)`，若已经有一个与当前加料互斥的小料（比如存在沙拉酱的时候添加番茄酱），则会直接被拒绝不生效。

如果在`InitializeCanteen()`里使用`seasoning.set_exclusion_policy(stdex::bitwise::RP_FORCE)`，则用户后选择的小料，将覆盖掉先选择的小料；如果使用`seasoning.set_exclusion_policy(stdex::bitwise::RP_EXCEPTION)`，则会抛出错误，需要开发者额外用`try-catch`块处理，并提示给用户。

#### `class advanced_flags<_Tp,_DefaultExclusionPolicy,_DefaultForbiddenPolicy,_DefaultDependencyPolicy>`

##### 所属命名空间

`stdex::bitwise`

##### 功能简述

本文件的核心类之一。本类提供了本文件的增强标志集方法。增强标志集可以为不同的标志位设置互斥、依赖与禁止关系，还能检查是否存在依赖-禁止环等行为。对于互斥、依赖与禁止三种关系，还可以分别设置相应的策略。此类相对于互斥集`class exclusion_flags<_Tp,_DefaultPolicy>`性能略低，如果对性能要求较高且没有对依赖关系与禁止关系的需求，建议选用互斥集。

禁止关系和依赖关系均为单向关系。禁止关系(A→B)表示A存在于集合时集合B不被允许出现在集合中；依赖关系(A→B)表示A加入集合时B必须已经存在于集合中。

##### 成员说明

`std::map<_Tp,flags<_Tp>> forbiddens_`

增强集的禁止集管理。此成员仅能通过成员函数进行const访问。

`std::map<_Tp,flags<_Tp>> dependencies_`

增强集的依赖集管理。此成员仅能通过成员函数进行const访问。

`relation_policy forbidden_policy_`

选择的禁止关系策略。初始取值为`_DefaultPolicy`，随后仅能通过成员函数修改。`_DefaultPolicy`的默认值是`RP_REJECT`。

`relation_policy dependency_policy_`

选择的依赖关系策略。初始取值为`_DefaultPolicy`，随后仅能通过成员函数修改。`_DefaultPolicy`的默认值是`RP_REJECT`。

##### 额外说明

禁止策略影响添加标志出现禁止冲突时的行为。选择`RP_FORCE`时，将强行添加当前标志并移除该标志禁止的标志；选择`RP_REJECT`时，将不进行任何操作；选择`RP_EXCEPTION`时，会抛出`std::invalid_argument`错误。

依赖策略影响添加标志出现依赖不符合时的行为。选择`RP_FORCE`时，将强行将依赖加入集合，如果依赖被依赖环所影响，则不会加入且跳过后续所有检查和添加；选择`RP_REJECT`时，将不进行任何操作；选择`RP_EXCEPTION`时，会抛出`std::invalid_argument`错误。

##### 成员函数

`advanced_flags<_Tp,_DefaultExclusionPolicy,_DefaultForbiddenPolicy,_DefaultDependencyPolicy>`继承了`exclusive_flags<_Tp,_DefaultExclusionPolicy>`的全部方法。同时，其还有以下特殊方法。

| 方法                                                         | 说明                                                         | 复杂度       |
| :----------------------------------------------------------- | :----------------------------------------------------------- | ------------ |
| `advanced_flags()`                                           | 创建空标志集，并赋予默认关系策略                             | O(1)         |
| `template <relation_policy _OtherPolicy1,relation_policy _OtherPolicy2,relation_policy _OtherPolicy3>`<br>`advanced_flags(const advanced_flags<_Tp,_OtherPolicy1,_OtherPolicy2,_OtherPolicy3>& other)` | 拷贝构造函数                                                 | O(1)         |
| `template <relation_policy _OtherPolicy1,relation_policy _OtherPolicy2,relation_policy _OtherPolicy3>`<br>`advanced_flags& operator =(const advanced_flags<_Tp,_OtherPolicy1,_OtherPolicy2,_OtherPolicy3>& other)` | 拷贝赋值函数，但**不会**复制策略                             | O(n)         |
| `void set_forbidden_policy(relation_policy policy)`          | 设置禁止策略                                                 | O(1)         |
| `void set_dependency_policy(relation_policy policy)`         | 设置依赖策略                                                 | O(1)         |
| `void add_dependency(_Tp requirer,_Tp required)`             | 设置`requirer`→`required`的依赖关系                          | O(1)         |
| `void remove_dependency(_Tp requirer,_Tp required)`          | 取消`requirer`→`required`的依赖关系，无需保证关系存在        | O(1)         |
| `void clear_dependency(_Tp requirer)`                        | 清除`requirer`的全部依赖关系（不包含\*→`requirer`，仅包含`requirer`→\*) | O(1)         |
| `void add_forbidden(_Tp element,_Tp forbidden)`              | 添加`element`→`forbidden`的禁止关系                          | O(1)         |
| `void remove_forbidden(_Tp element,_Tp forbidden)`           | 取消`element`→`forbidden`的禁止关系，无需保证关系存在        | O(1)         |
| `void clear_forbidden(_Tp element)`                          | 清除`element`的全部禁止关系（不包含\*→`element`，仅包含`element`→\*） | O(1)         |
| `advanced_flags& operator <<=(_Tp e)`                        | 添加标志                                                     | **等待写入** |
| `std::vector<consistency_set<_Tp>> check_consistency()`      | 检测依赖-禁止环                                              | **等待写入** |
| `const std::map<_Tp,flags<_Tp>>& forbiddens() const`         | 查询禁止关系                                                 | O(1)         |
| `const std::map<_Tp,flags<_Tp>>& dependencies() const`       | 查询依赖关系                                                 | O(1)         |

对于`check_consistency()`的返回值中的`consistency_set<_Tp>`类型，请参阅下文。

**有关`__STDEX_CONSTEXPR`的内容与前文保持一致。**

##### 使用样例

暂无

#### `class advanced_flags<_Tp,_DefaultExclusionPolicy,_DefaultForbiddenPolicy,_DefaultDependencyPolicy>::struct consistency_set<_Up>`

##### 功能简述

用于表达增强集检测依赖-禁止环的返回值表达。

> [!WARNING]
>
> `_Up`：必须和`_Tp`为相同类型，否则会触发`static_assert`。通常不手动设置`_Up`，其将自动与`_Tp`保持一致。

##### 成员说明

`consistency_type type_`

表示当前冲突类型，其枚举值参考前文。

`std::vector<_Up> value_`

表达当前冲突对应的枚举值。`type_`为`CT_FORBIDDEN_WITH_DEPENDENCY`或`CT_REVERSE_FORBIDDEN_WITH_DEPENDENCY`时，包含2个值，表示冲突双方。请注意，A依赖B且A禁止B的冲突，`value_[0]=A,value_[1]=B,type_=CT_FORBIDDEN_WITH_DEPENDENCY`；A依赖B且B禁止A的冲突，`value_[0]=B,value_[1]=A,type_=CT_REVERSE_WITH_DEPENDENCY`；`type_`为`CT_CYCLE`时，包含强连通分量中环内的所有节点；`type_`为`CT_FORBIDDEN_SELF`时，仅包含禁止自身的节点本身。

`flags<_Up> extra_value_`

表达当前冲突带了额外影响的枚举值。`type_`为`CT_FORBIDDEN_WITH_DEPENDENCY`或`CT_REVERSE_FORBIDDEN_WITH_DEPENDENCY`时，包含若干个值，表达冲突双方的后继节点，即因冲突双方导致的额外受影响的枚举值集合；`type_`为`CT_CYCLE`时，包含强连通分量中，环外的，且前驱节点在`value_`内的所有节点；`type_`为`CT_FORBIDDEN_SELF`时，此成员为空。

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