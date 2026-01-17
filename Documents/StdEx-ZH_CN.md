# StdEx使用指南

## 关键内容说明

StdEx库是一个包含文件模式的C++扩展库，本库使用非外部链接，直接引用的方法进行使用。本库基于C++17标准，对于更高版本标准的内容会在文中额外注明。

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
> `_Tp`：必须是枚举类，否则会触发`static_assert`。**通常\_Tp需要满足其中每个枚举类型满足取值为2的若干次幂或部分标志的集合（即若干枚举值进行或操作的结果），<span style="color:red">除非你清楚本类会进行的行为</span>。**

##### 额外说明

对内部标志的管理使用的是`std::underlying_type_t<_Tp>`。

##### 成员函数

| 方法                                                         | 说明                 | 复杂度 |
| :----------------------------------------------------------- | :------------------- | ------ |
| `flags()`                                                    | 创建空标志集         | $O(1)$ |
| `flags(_Tp e) noexcept`                                      | 用单个标志初始化     | $O(1)$ |
| `flags& operator =(_Tp e) noexcept`                          | 直接赋值             | $O(1)$ |
| `virtual flags& operator <<=(_Tp e)`<br>`virtual flags operator <<(_Tp e)` | 添加标志             | $O(1)$ |
| `flags& operator <<=(flags<_Tp> value) noexcept`<br/>`flags operator <<(flags<_Tp> value) noexcept` | 添加一组标志         | $O(n)$ |
| `flags& operator >>=(_Tp e) noexcept` <br>`flags operator <<(_Tp e) noexcept` | 移除标志             | $O(1)$ |
| `bool contains(_Tp e) const noexcept`                        | 检测标志是否存在     | $O(1)$ |
| `operator typename std::underlying_type_t<_Tp>() const noexcept` | 获取底层整数值       | $O(1)$ |
| `operator _Tp() const noexcept`                              | 获取枚举类型的底层值 | $O(1)$ |
| `void clear() noexcept`                                      | 清空所有标志         | $O(1)$ |
| `bool empty() const noexcept`                                | 检查是否为空集       | $O(1)$ |
| `template <typename _Func>`<br>`void for_each(_Func func) const` | 对每个生效值执行func | $O(n)$ |

**绝大多数函数均被标记为`constexpr`，其在性能上将非常优秀。**

> [!WARNING]
>
> **<span style="color:red">由于`constexpr`虚函数等行为只在C++20标准下才合法，所以`operator <<=`和`operator <<`使用了`_STDEX_CONSTEXPR`标记，该标记仅在C++20标准下被特化为`constexpr`。如果您需要对该函数的性能有所要求，请注意甄别所处的C++标准。</span>**

##### 其他方法

###### 增强宏

`_STDEX_ENABLE_FLAGS_ENHANCED(EnumType)`

使用宏后，会对`EnumType`启用全局运算符重载，使之可以使用额外的运算符。

> [!WARNING]
>
> **<span style="color:red">请不要手动使用`struct stdex::bitwise::flags<EnumType>::_flags_enhanced : std::true_type {};`的方法，这将导致未定义行为。</span>**

| 方法                                                         | 说明                                                         | 复杂度 |
| :----------------------------------------------------------- | :----------------------------------------------------------- | ------ |
| `flags<EnumType> operator <<(EnumType lhs,EnumType rhs) noexcept` | 相当于`flags<EnumType>(lhs)<<rhs`。<br>可以更高效地使用<<运算符。用标志枚举直接初始化 | $O(1)$ |
| `flags<EnumType> operator >>(EnumType lhs,EnumType rhs) noexcept` | 相当于`flags<EnumType>(lhs)>>rhs`。<br>可以更高效地使用>>运算符。通常在lhs表示为包含rhs和若干标志的集合时有效。 | $O(1)$ |

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

<<<<<<< HEAD
struct file{`
	stdex::bitwise::flags<FilePermisson> permission_;
=======
struct file{
	stdex::bitwise::flags<FilePermission> permission_;
>>>>>>> 5383089 (Update Document)
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
| `exclusive_flags()`                                          | 创建空标志集，并赋予默认关系策略 | $O(1)$ |
| `template <relation_policy _OtherPolicy>`<br>`exclusive_flags(const exclusive_flags<_Tp,_OtherPolicy>& other)` | 拷贝构造函数                     | $O(1)$ |
| `~exclusive_flags()`                                         | 析构函数                         | $O(n)$ |
| `exclusive_flags& operator =(_Tp e) noexcept`                | **不建议的**直接赋值             | $O(1)$ |
| `template <relation_policy _OtherPolicy>`<br>`exclusive_flags& operator =(const exclusive_flags<_Tp,_OtherPolicy>& other)` | 拷贝赋值函数，但**不会**复制策略 | $O(n)$ |
| `void set_exclusion_policy(relation_policy policy)`          | 设置关系策略                     | $O(1)$ |
| `void set_exclusion(_Tp lhs,_Tp rhs)`                        | 设置两个标志的互斥关系           | $O(1)$ |
| `void clear_exclusion(_Tp e)`                                | 清除某个标志的互斥关系           | $O(1)$ |
| `virtual exclusive_flags& operator <<=(_Tp e)`               | 添加标志                         | $O(1)$ |
| `const std::map<_Tp,flags<_Tp>*>& exclusions() const`        | 获取互斥组成员                   | $O(1)$ |

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
};
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

互斥关系和禁止关系的区别在于：互斥是双向的，禁止是单向的。如果需要管理双向互斥的行为，建议优先使用互斥关系，否则建议优先使用禁止关系。

##### 成员函数

`advanced_flags<_Tp,_DefaultExclusionPolicy,_DefaultForbiddenPolicy,_DefaultDependencyPolicy>`继承了`exclusive_flags<_Tp,_DefaultExclusionPolicy>`的全部方法。同时，其还有以下特殊方法。

| 方法                                                         | 说明                                                         | 复杂度                                     |
| :----------------------------------------------------------- | :----------------------------------------------------------- | ------------------------------------------ |
| `advanced_flags()`                                           | 创建空标志集，并赋予默认关系策略                             | $O(1)$                                     |
| `template <relation_policy _OtherPolicy1,relation_policy _OtherPolicy2,relation_policy _OtherPolicy3>`<br>`advanced_flags(const advanced_flags<_Tp,_OtherPolicy1,_OtherPolicy2,_OtherPolicy3>& other)` | 拷贝构造函数                                                 | $O(1)$                                     |
| `template <relation_policy _OtherPolicy1,relation_policy _OtherPolicy2,relation_policy _OtherPolicy3>`<br>`advanced_flags& operator =(const advanced_flags<_Tp,_OtherPolicy1,_OtherPolicy2,_OtherPolicy3>& other)` | 拷贝赋值函数，但**不会**复制策略                             | $O(n)$                                     |
| `void set_forbidden_policy(relation_policy policy)`          | 设置禁止策略                                                 | $O(1)$                                     |
| `void set_dependency_policy(relation_policy policy)`         | 设置依赖策略                                                 | $O(1)$                                     |
| `void add_dependency(_Tp requirer,_Tp required)`             | 设置`requirer`→`required`的依赖关系                          | $O(1)$                                     |
| `void remove_dependency(_Tp requirer,_Tp required)`          | 取消`requirer`→`required`的依赖关系，无需保证关系存在        | $O(1)$                                     |
| `void clear_dependency(_Tp requirer)`                        | 清除`requirer`的全部依赖关系（不包含\*→`requirer`，仅包含`requirer`→\*) | $O(1)$                                     |
| `void add_forbidden(_Tp element,_Tp forbidden)`              | 添加`element`→`forbidden`的禁止关系                          | $O(1)$                                     |
| `void remove_forbidden(_Tp element,_Tp forbidden)`           | 取消`element`→`forbidden`的禁止关系，无需保证关系存在        | $O(1)$                                     |
| `void clear_forbidden(_Tp element)`                          | 清除`element`的全部禁止关系（不包含\*→`element`，仅包含`element`→\*） | $O(1)$                                     |
| `advanced_flags& operator <<=(_Tp e)`                        | 添加标志                                                     | 最好情况：$O(F+n)$<br>最坏情况：$O(C+F+n)$ |
| `std::vector<consistency_set<_Tp>> check_consistency()`      | 检测依赖-禁止环                                              | $O(C)$                                     |
| `const std::map<_Tp,flags<_Tp>>& forbiddens() const`         | 查询禁止关系                                                 | $O(1)$                                     |
| `const std::map<_Tp,flags<_Tp>>& dependencies() const`       | 查询依赖关系                                                 | $O(1)$                                     |

对于时间复杂度，V代表启用枚举值数量，F代表禁止关系数量，D代表依赖关系数量，C代表$(V+F) \times (V+D)$。

对于`check_consistency()`的返回值中的`consistency_set<_Tp>`类型，请参阅下文。

**有关`_STDEX_CONSTEXPR`的内容与前文保持一致。**

##### 使用样例

###### 以带游客的用户系统举例

```cpp
enum class UserState : uint32_t {
	LoggedOut=1,
	LoggedIn=2,
	Guest=4,
	Admin=8,
	Moderator=16,
};

advanced_flags<UserState,RP_FORCE,RP_FORCE,RP_FORCE> state;

void SetupSystem() {
	state.set_exclusion(UserState::LoggedOut,UserState::LoggedIn);
	state.add_dependency(UserState::Admin,UserState::LoggedIn);
	state.add_dependency(UserState::Moderator,UserState::LoggedIn);
	state.add_forbidden(UserState::Guest,UserState::Admin);
	state.add_forbidden(UserState::Guest,UserState::Moderator);
}

void PromoteToAdmin() {
	user_state<<=UserState::Admin; 
}
```

在上述代码中，我们创建了一个用户管理系统，并将互斥策略、依赖策略和禁止策略设置为了`RP_FORCE`。在`SetupSystem()`中，我们设置了以下关系：登出与登陆状态互斥；管理员和高级管理员需要登陆状态；游客不能成为管理员或高级管理员。`RP_FORCE`表明，当遇到违反策略的情况时，会优先强制生效当前策略，以`PromoteToAdmin()`为例，设置`UserState::Admin`的同时，将会强制取消`UserState::Guest`身份，并自动使`UserState::LoggedIn`生效。如果依赖策略为`RP_REJECT`且未登录，则`PromoteToAdmin()`会在检测到依赖不满足时自动停止进一步调整`state`，不会导致任何预期外的行为。

###### 以系统权限为例

```cpp
enum class SystemPermission : uint32_t {
	None=0,
	Login=1,
	ViewData=2,
	ExportData=4,
	ManageUsers=8,
	SystemConfig=16,
	AuditLogs=32,
	Suspended=64,
	GuestMode=128,
};

class PermissionConfigValidator {
	stdex::bitwise::advanced_flags<SystemPermission> config_;
    
public:
	void SetupConfig() {
		config_.add_dependency(SystemPermission::ManageUsers,SystemPermission::SystemConfig);
		config_.add_dependency(SystemPermission::SystemConfig,SystemPermission::AuditLogs);
		config_.add_dependency(SystemPermission::AuditLogs,SystemPermission::ManageUsers);
		config_.add_dependency(SystemPermission::ExportData,SystemPermission::Login);
		config_.add_forbidden(SystemPermission::ExportData,SystemPermission::Login);
		config_.add_dependency(SystemPermission::AuditLogs,SystemPermission::SystemConfig);
		config_.add_forbidden(SystemPermission::SystemConfig,SystemPermission::AuditLogs);
		config_.add_forbidden(SystemPermission::GuestMode,SystemPermission::GuestMode);
	}
	void Validate() {
		auto issues=config_.check_consistency();
		if (issues.empty()) {
			std::cout<<"Accepted"<<std::endl;
			return;
		}
		for (const auto& issue:issues) {
			switch (issue.type_) {
				case stdex::bitwise::CT_CYCLE: {
					std::cout<<"Dependency Cycle:"<<std::endl;
					for (auto perm:issue.value_) std::cout<<static_cast<uint32_t>(perm)<<" ";
					std::cout<<std::endl;
					break;
				}
				case stdex::bitwise::CT_FORBIDDEN_WITH_DEPENDENCY: {
					std::cout<<"Forbidden with Dependency:"<<std::endl<<static_cast<uint32_t>(issue.value_[0])<<" depends "<<static_cast<uint32_t>(issue.value_[1])<<", but also forbids it"<<std::endl;
					break;
				}
				case stdex::bitwise::CT_REVERSE_FORBIDDEN_WITH_DEPENDENCY: {
					std::cout<<"Forbidden with Dependency:"<<std::endl<<static_cast<uint32_t>(issue.value_[0])<<" is being depended by "<<static_cast<uint32_t>(issue.value_[1])<<", but also forbids it"<<std::endl;
					break;
				}
				case stdex::bitwise::CT_FORBIDDEN_SELF: {
					std::cout<<"Self forbidden:"<<std::endl<<static_cast<uint32_t>(issue.value_[0])<<" forbids itself\n";
					break;
				}
			}
		}
	}
};

int main() {
    PermissionConfigValidator validator;
    validator.SetupConfig();
    validator.Validate();
}
```

在上述代码中，我们创建了一个权限检查器，并使用`Validate()`进行了检查。在该样例代码中，我们设置了全部四种相矛盾的标志关系，会被`Validate()`函数全部检查并打印出来。如果需要检查其他的权限配置情况，只需要更改`SetupConfig()`函数，就可以完成类似的测试。

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

## math/base.h

本篇待补充

## math/matrix.h

本篇待补充

## math/geometry/graphics.h

本篇待补充

## math/geometry/trajectory.h

本篇待补充

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

本篇待补充

## other/diff_match_patch.h

本篇待补充

## structure/nary_tree.h

本篇待补充

## syntax/lexer.h-(V1.41)

### 简述

本文件的主要功能是提供一个可以自定义文法的词法分析器。

单文件使用模式：#include <syntax/lexer.h>

项目使用模式：#include <syntax/lexer.h>

本篇待补充

## syntax/parser.h

本篇待补充

## type/bitmap.h

本篇待补充

## type/json.h

本篇待补充

## vision/easing.h

本篇待补充

## vision/motion.h(Version 1.0.1.0)

### 基本信息

#### 概要

motion.h旨在提供一个用于描述与组合随时间变化的多通道运动状态的轻量级运动库，以位移、速度、加速度三元组与多通道状态与时间戳为基础逻辑表达从时间到运动状态的映射关系。

使用motion.h提供的运动功能集合，开发者可以快速生成匀速、匀加速等基础运动，也可以自定义构造任意解析式/过程式运动，并将其进行平移、缩放、叠加、拼接等操作。

同时，motion.h提供采样与检查工具，可用于验证运动在采样区间内是否不合法值或不连续跳变。

motion.h的核心类的运动类`motion`。

`motion`不是一个物理引擎，也不是一个动画播放器，而是一个**将时间映射到多通道状态**的数学抽象。它适用于任何“随时间变化的向量值”场景，无论是：

- 2D/3D 物体运动（位置、速度、加速度）
- UI 动画（透明度、缩放、颜色）
- 控制系统中的参考信号
- 游戏中的状态机（如角色行为叠加）

其优点有：

- 通道化：每个通道独立，可代表一个维度或一个属性。

- 时间行为可配置：支持自由、钳制、循环、往返四种时间映射模式。
- 函数式组合：支持平移、缩放、叠加、分段拼接等操作，便于构建复杂运动。

- 高程度封装：`motion` 封装了时间映射、多通道管理和状态派生（速度、加速度）的逻辑，你只需提供最核心的“位置-时间关系”，其余由库自动处理。

#### 使用方法

单文件使用模式：`#include <vision/motion.h>`

项目使用模式：`#include <vision/motion.h>`

#### 使用场景

本库提供的运动描述与组合能力常用于“多个连续量需要随时间变化”的情况。

以动画系统为例，开发者可以用多通道motion同时表达对象的三方向位移、旋转角、缩放等属性随时间的变化，并通过设置时间行为实现循环与往返动画，并获取离散采样数据用于渲染或调试。

以运动学轨迹为例，开发者可以用多通道motion表达多自由度系统的参考轨迹，并进行相位平移、速度变化、叠加运动分量、构造分段轨迹，完成运动计算。

以信号与控制为例，开发者可以通过自定义随时间输出的信号源，并结合偏移、缩放与时间行为映射生成更复杂的测试输入。

#### 使用限制

本库的核心类型`motion`基于`std::function`实现时间到状态的映射。该设计带来较高灵活性，但在极端性能敏感场景下可能引入一定的类型擦除开销；如需更高性能，建议在自行对motion进行采样缓存或降低采样频率。

本库对速度与加速度的约定为：`velocity_`与``acceleration_`分别为位置对时间的一阶/二阶导数。`time_scale()`会基于链式法则对速度与加速度进行缩放（速度乘k，加速度乘k²）。若用户自定义的运动函数不满足该约定，则组合后的速度与加速度含义可能与预期不一致。 对于多通道组合操作，必须保证通道数满足对应约束。

### 枚举类型

#### `enum time_behavior`

时间行为映射策略。取值包含`TB_FREE`（不映射时间）、`TB_CLAMP`（钳制时间到0的下限和`duration`的上限）、`TB_LOOP`（超出上下限的部分视为循环）、`TB_OSCILLATE`（按照2倍上限往返映射，超出部分视为循环）。 

 ### 数据结构

#### `struct motion_scalar`

##### 所属命名空间

`stdex::vision`

##### 功能简述

单通道运动学状态，包含位置、速度、加速度三元组。 

##### 成员说明

| 成员                  | 含义               | 初始化 | 注意事项 |
| --------------------- | ------------------ | ------ | -------- |
| `double position`     | 当前状态实例位置   | {0.0}  | -        |
| `double velocity`     | 当前状态实例速度   | {0.0}  | -        |
| `double acceleration` | 当前状态实例加速度 | {0.0}  | -        |

#### `struct motion_state`

##### 所属命名空间

`stdex::vision`

##### 功能简述

多通道运动状态。以`std::vector<motion_scalar>`存储各通道的运动状态，并使用`time`记录本次状态对应的时间（通常为映射后的时间）。

##### 成员说明

| 成员                         | 含义               | 初始化 | 注意事项 |
| ---------------------------- | ------------------ | ------ | -------- |
| `std::vector<motion_scalar>` | 各通道运动状态集合 | -      | -        |
| `double time`                | 采样时间           | {0.0}  | -        |

##### 成员函数

| 方法                                                         | 说明                                 | 复杂度 |
| ------------------------------------------------------------ | ------------------------------------ | ------ |
| `motion_state()`                                             | 创建空状态                           | $O(1)$ |
| `explicit motion_state(std::size_t channels)`                | 创建指定通道数的状态，并赋予状态初值 | $O(n)$ |
| `static motion_state single(motion_scalar s,double time=0.0)` | 创建单通道状态                       | $O(1)$ |

对于时间复杂度，n为通道数。

#### `class motion`

##### 所属命名空间

`stdex::vision`

##### 功能简述

本文件的核心类。本类为“随时间变化的多通道运动”的实际表达。

##### 成员说明

| 别名               | 原名                                               | 含义               | 注意事项            |
| ------------------ | -------------------------------------------------- | ------------------ | ------------------- |
| `eval_func`        | `std::function<motion_state(double)>`              | 评估函数类型       | -                   |
| `per_channel_func` | `std::function<motion_scalar(std::size_t,double)>` | 按通道评估函数类型 | 主要用于`make_nd()` |

##### 额外说明

`duration`所代表的时间设为负值时，时间行为`behavior`将**自主**退化为`TB_FREE`的自由时间行为，因此若需要`TB_CLAMP`/`TB_LOOP`/`TB_OSCILLATE`生效，应设置非负的`duration`。

##### 成员函数

| 方法                                                         | 说明                                                         | 复杂度                                                       |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| `motion()`                                                   | 创建空运动                                                   | $O(1)$                                                       |
| `motion(eval_func func,std::size_t channels,double duration=0.0,time_behavior behavior=TB_FREE)` | 创建指定通道、时间、行为、持续时间以及评估函数的的运动       | $O(1)$                                                       |
| `bool empty() const noexcept`                                | 检查评估函数是否为空                                         | $O(1)$                                                       |
| `std::size_t channels() const noexcept`                      | 获取通道数                                                   | $O(1)$                                                       |
| `double& duration()`<br>`const double& duration() const noexcept` | 获取时间引用                                                 | $O(1)$                                                       |
| `time_behavior& behavior() noexcept`<br>`time_behavior behavior() const noexcept` | 获取时间行为引用                                             | $O(1)$                                                       |
| `motion with_duration(double d) const`                       | 返回设置指定时间的运动副本，常用于分段组合功能使用           | $O(1)$                                                       |
| `motion with_time_behavior(time_behavior tb) const noexcept` | 返回设置指定时间行为的运动副本，常用于分段组合功能使用       | $O(1)$                                                       |
| `double map_time(double t) const noexcept`                   | 按照指定的时间行为映射`t`至默认合法时间区间之内              | $O(1)$                                                       |
| `double progress(double t) const noexcept`                   | 按`map_time`映射后时间计算时间`t`所处的时间进度百分比        | $O(1)$                                                       |
| `bool finished(double t) const noexcept`                     | 计算未映射的`t`是否处于结束后时间<br?对于`TB_LOOP`/`TB_OSCILLATE`，该函数值恒为`false` | $O(1)$                                                       |
| `motion_state eval(double t) const`                          | 评估映射后`t`时间的运动状态<br>若未指定评估函数，则使用内部默认评估函数 | $O(F)$                                                       |
| `std::vector<motion_state> sample_states(double t0,double t1,std::size_t n) const` | 对[t0,t1]区间内进行n等分均匀采样，返回n+1个点                | $O((n+1) \times F)$                                          |
| `std::vector<double> sample_positions(std::size_t channel,double t0,double t1,std::size_t n) const` | 对单一通道[t0,t1]区间的位置（`position`）进行n等分均匀采样，返回n+1个点 | $O((n+1) \times F)$                                          |
| `motion shift(double dt) const`                              | 对运动进行时间平移，$t_{new}=t_{old}-dt$                     | $O(1)$                                                       |
| `motion time_scale(double k) const`                          | 对运动进行时间缩放，$t_{new}=k \cdot t_{old}$                | $O(1)$                                                       |
| `motion value_offset(double& offset)`<br>`motion value_offset(const std::vector<double>& offsets) const` | 对运动逐通道地进行位置（`position`）偏移                     | $O(C)$                                                       |
| `motion value_scale(double& scale)`<br>`motion value_scale(const std::vector<double>& scales) const` | 对运动逐通道地进行缩放                                       | $O(C)$                                                       |
| `motion add(const motion& other) const`                      | 对运动逐通道地进行加法                                       | $O(C)$                                                       |
| `bool check_finite(double t0,double t1,double step) const`   | 对[t0,t1]区间内逐步采样检查是否均为有效值                    | $O(m \times C)$                                              |
| `std::vector<continuity_issue> check_continuity(const std::vector<double>& times,double eps) const` | 检测指定时刻附近的位置连续性，`eps`为容差（即差值超过此值则视为不连续）<br>采样点为$t - 1e7$和$t + 1e7$ | $O(k \times C)$                                              |
| `void set_channel(std::size_t index,const motion& channel)`  | 替换特定通道的运动为指定的单通道运动（`motion`）<br>输入`index`**必须**为合法值，且替换的运动通道数为1<br>等价于`*this=set_channel(*this,index,channel)` | 略大于`static set_channel(const motion& base,std::size_t index,const motion& channel)` |

其中，默认合法时间区间通常指[0,`duration`]；对于分段组合功能，请详见下方使用样例部分；内部默认评估函数的行为仅设置时间为指定值，不进行任何其他行为；未提及特定运动标量（如“位置（`position`）”）的函数，默认对位置（`position`）、速度（`velocity`）、加速度（`acceleration`）均生效。

对于时间复杂度，F代表所采用的`func`的时间复杂度，C代表通道数，m代表$\lceil \frac{t_{1} - t_{0}}{step} + 0.5 \rceil$​​，k为`times`元素个数。

##### 使用样例

//无关下序样例

#### `struct motion::continuity_issue`

##### 所属类

`class stdex::vision::motion`

#####  功能简述

用于表达连续检查的单个问题。

##### 成员说明

| 成员                        | 含义                                            | 初始化 | 注意事项 |
| --------------------------- | ----------------------------------------------- | ------ | -------- |
| `double t`                  | 当前问题所处时刻                                | -      | -        |
| `std::vector<double> left`  | 当前问题$t - 1e7$时刻各通道的位置（`position`） | -      | -        |
| `std::vector<double> right` | 当前问题$t + 1e7$时刻各通道的位置（`position`） | -      | -        |

##### 使用样例

//检查连续性样例

 #### `class motion::piecewise_builder` 

##### 所属类

`class stdex::vision::motion`

##### 功能简述

用于构造分段运动的组件。使用本类可将若干段运动拼接成一段运动（`motion`）。

##### 成员函数

| 方法                                                         | 说明                                                         | 复杂度                                                       |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| `piecewise_builder()`                                        | 创建空构造器                                                 | $O(1)$                                                       |
| `piecewise_builder& with_duration(double d) noexcept`        | 设置目标持续时间                                             | $O(1)$                                                       |
| `piecewise_builder& with_time_behavior(time_behavior tb) noexcept` | 设置目标时间行为                                             | $O(1)$                                                       |
| `piecewise_builder& add(const motion& m,double start_time)`  | 添加一段运动                                                 | $O(1)$                                                       |
| `motion build(bool require_sorted=true) const`               | 构造运动<br>若`required_sorted`为`true`，则检查添加的运动是否有序，否则自动排序 | `required_sorted`为`true`：$O(S)$<br>`required_sored`为`false`：$O(S \times logS)$ |

对于时间复杂度，S代表加入构造器的运动段数。

##### 使用样例

###### 以红绿灯时序为例

```cpp
int main() {
	auto light=stdex::vision::motion::piecewise()
		.add(stdex::vision::motion::make_1d([](double){ 
			return stdex::vision::motion_scalar{1,0,0};
		},30.0),0.0)//assumption: green=1 and last for 30 seconds
		.add(stdex::vision::motion::make_1d([](double){ 
			return stdex::vision::motion_scalar{2,0,0};
		},5.0),30.0)//assumption: yellow=2 and last for 5 seconds
		.add(stdex::vision::motion::make_1d([](double){ 
			return stdex::vision::motion_scalar{3,0,0};
		},30.0),35.0)//assumption: red=3 and last for 30 seconds
		.with_duration(65.0)
		.with_time_behavior(stdex::vision::TB_LOOP)
		.build();
	for (double t:{0.0,31.0,66.0}) std::cout<<"t="<<t<<": status="<<light.eval(t).scalars[0].position<<"\n";
}
```


### 原型函数

| 命名空间/类前缀 | 别名             | 原名                                   | 含义           | 注意事项             |
| --------------- | ---------------- | -------------------------------------- | -------------- | -------------------- |
| `stdex::vision` | `prototype_func` | `std::function<motion_scalar(double)>` | 单通道运动函数 | 主要用于``make_1d()` |

### 工厂方法

| 命名空间/类前缀         | 方法                                                         | 说明                                                         | 复杂度 |
| ----------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------ |
| `stdex::vision::motion` | `static prototype_func proto_constant(double c)`             | 生成常量运动：$position=c,velocity=0,acceleration=0$         | $O(1)$ |
| `stdex::vision::motion` | `static prototype_func proto_linear(double x0,double v)`     | 生成线性运动：$position=x0 + v \cdot t,velocity=v,acceleration=0$ | $O(1)$ |
| `stdex::vision::motion` | `static prototype_func proto_constant_accel(double x0,double v0,double a)` | 生成常加速度运动：$position=x0 + v0 \cdot t + 0.5 \cdot a \cdot t^{2},velocity=v0 + a \cdot t,acceleration=a$ | $O(1)$ |
| `stdex::vision::motion` | `static motion make_1d(const prototype_func& proto,double duration=0.0,time_behavior tb=TB_FREE)` | 将一维运动封装为单通道运动（`motion`）                       | $O(1)$ |
| `stdex::vision::motion` | `static motion make_nd(std::size_t channels,const per_channel_func& func,double duration=0.0,time_behavior tb=TB_FREE)` | 按通道函数将n维运动封装为多通道运动（`motion`），第i通道由func(i,t)生成 | $O(C)$ |
| `stdex::vision::motion` | `static motion pack_channels(const std::vector<motion>& channels,double duration=0.0,time_behavior=TB_FREE)` | 将n个一维单通道运动（`motion`）打包为一个n维多通道运动。输入向量**必须**为非空，且每个单通道运动通道数为1。若`duration`设置为负值，则取各通道`duration`最大值作为结果的生成结果的`duration`。 | $O(C)$ |
| `stdex::vision::motion` | `static motion set_channel(const motion& base,std::size_t index,const motion& channel)` | 替换特定通道的运动为指定的单通道运动（`motion`）。输入`index`**必须**为合法值，且替换的运动通道数为1。 | $O(F)$ |
| `stdex::vision::motion` | `inline motion::piecewise_builder piecewise()`               | 快速创建空构造器                                             | $O(1)$ |

对于时间复杂度，F代表被替换的原运动的`func`和替换的单通道运动的`func`中时间复杂度的较大值，C代表通道数。

##### 使用样例

###### 以生成心跳波形为例

```cpp
int main() {
	auto beat1=stdex::vision::motion::make_1d([](double t){
		return stdex::vision::motion_scalar{exp(-t*5)*sin(t*50),0,0};
	},0.3);
	auto beat2=stdex::vision::motion::make_1d([](double t){
		return stdex::vision::motion_scalar{exp(-t*4)*sin(t*40)*0.7,0,0};
	},0.3);
	std::vector<stdex::vision::motion> beats={beat1,beat2,beat1,beat2};
	auto builder=stdex::vision::motion::piecewise();
	double time=0.0;
	for (const auto& beat:beats) {
		builder.add(beat.shift(-0.1),time);
		time+=0.8;
	}
	auto heartbeat=builder.with_duration(3.2).build();
	for (double t=0;t<=3.2;t+=0.1) std::cout<<t<<" "<<heartbeat.eval(t).scalars[0].position<<"\n";
}
```

###### 以模拟相机抖动为例

```cpp
int main() {    
	auto shake_x=stdex::vision::motion::make_1d([](double t){
		return stdex::vision::motion_scalar{sin(t*50)*0.1,0,0};
	},0.5);
	auto shake_y=stdex::vision::motion::make_1d([](double t){
		return stdex::vision::motion_scalar{cos(t*45)*0.08,0,0};
	},0.5);
	auto shake_z=stdex::vision::motion::make_1d([](double t){
		return stdex::vision::motion_scalar{sin(t*55)*0.05,0,0};
	},0.5);
	std::vector<stdex::vision::motion> axes={shake_x,shake_y,shake_z};
	auto camera_shake=stdex::vision::motion::pack_channels(axes,0.5);
	for (double t=0;t<0.5;t+=0.05) {
		auto pos=camera_shake.eval(t).scalars;
		std::cout<<"("<<pos[0].position<<","<<pos[1].position<<","<<pos[2].position<<")\n";
	}
}
```