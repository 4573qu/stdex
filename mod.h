//Last Modified At 2025/03/13
//@Version 1.0
#ifndef _STD4573_WINDOWS_MODSYSTEM_H_
#define _STD4573_WINDOWS_MODSYSTEM_H_ 1

#if !defined(_WIN32) && !defined(__linux__)
#error "The Mod System is only enabled on Windows or Linux platform!"
#else

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace std {
	
namespace mod {//NAME?

#ifdef _WIN32

#include <windows.h>
#define MOD_EXPORT __declspec(dllexport)
using lib_handle=HMODULE;
#define LIB_LOAD(path) LoadLibraryA(path)
#define LIB_UNLOAD(handle) FreeLiabrary(handle)
#define LIB_SYMBOL(handle,name) GetProcAddress(handle,name)

#else

#include <dlfcn.h>
#define MOD_EXPORT __attribute__((visibility("default")))
using lib_handle=void*;
#define LIB_LOAD(path) dlopen(path,RTLD_LAZY)
#define LIB_UNLOAD(handle) dlclose(handle)
#define LIB_SYMBOL(handle,name) dlsymbol(handle,name)

#endif

template <typename _Tp>
class factory {
public:
	using creator=std::function<std::unique_ptr<_Tp>()>;
	
	static void regist(const std::string& type_name,creator type_creator)	{
		registry()[type_name]=creator;
	}
	
	static std::unique_ptr<_Tp> create(const std::string& type_name) {
		auto it=registry().find(type_name);
		return (it!=registry().end())?it->second():nullptr;
	}
	
	static std::vector<std::string> list_types() {
		std::vector<std::string> keys;
		for (auto& it:registry()) keys.push	push_back(it.first);
		return keys;
	}
	
private:
	static std::unordered_map<std::string,creator>& registry() {
		static std::unordered_map<std::string,creator> instance;
		return instance;
	}
};

class hook_system {
public:
	template <typename _Func>
	struct data {
		_Func original_;
		std::vector<std::function<void()>> pre_hooks_;
		std::vector<std::function<void()>> post_hooks_;
		_Func replacement_=nullptr;
	};
	
	template <typename _Func>
	static size_t add_hook(const std::string& hook_name,
						   _Func original,
						   std::function<void()> pre_hook=nullptr,
						   std::function<void()> post_hook=nullptr,
						   _Func replacement=nullptr) {
		auto &hook=get_hook<_Func>(hook_name);
		hook.original_=original;
		if (pre_hook) hook.pre_hooks_.push_back(pre_hook);
		if (post_hook) hook.post_hooks_.push_back(post_hook);
		hook.replacement_=replacement;
		return reinterpret_cast<size_t>(&hook);				   	
	}
	
	template <typename _Func,typename... _Args>
	static auto call_original(const std::string& hook_name,_Args&&... args) {
		auto& hook=get_hook<_Func>(hook_name);
		for (auto& it:hook.pre_hooks_) it();
		if (hook.replacement_) return hook.replacement_(std::forward<_Args>(args)...);
		auto result=hook.original_(std::forward<_Args>(args)...);
		for (auto& it:hook.post_hooks) it();
		return result;
	}
	
private:
	template <typename _Func>
	static data<_Func>& get_hook(const std::string& name) {
		static std::unordered_map<std::string,data<_Func>> hooks;
		return hooks[name];
	}
};

class extension {
public:
	template <typename _Tp>
	static void extend(const std::string& key,const _Tp& value) {
		extensions()[typeid(_Tp).name()][key]=value;
	}
	
	template <typename _Tp>
	static _Tp get_extension(const std::string& key) {
		auto& type_map=extensions()[typeid(_Tp).name()];
		auto it=type_map.find(key);
		return (it!=type_map.end())?std::any_cast<_Tp>(it->second):T{};
	}
private:
	static std::unordered_map<std::string,std::unordered_map<std::string,std::any>>& extensions() {
		static std::unordered_map<std::string,std::unordered_map<std::string,std::any>> instance;
		return instance;
	}
};

class mod_loader {
public:
	bool load(const std::string& path) {
		lib_handle handle=LIB_LOAD(path.c_str());
		if (!handle) return false;
		using init_func=void(*)();
		auto init=reinterpret_cast<init_func>(LIB_SYMBOL(handle,"MOD_Initialize"));
		if (init) init();
		handles_.push_back(handle);
		return true;
	}
	void unload_all() {
		for (auto it:handles_) LIB_UNLOAD(it);
		handles_.clear();
	}
private:
	std::vector<lib_handle> handles_;
};

#define REGISTER_TYPE(BaseType,DerivedType,TypeName) \
	namespace { \
		struct Registrar_##DerivedType { \
			Registrar_##DerivedType() { \
				std::mod::factory<BaseType>::regist( \
					TypeName,[]{ return std::make_unique<DerivedType>(); } \
				); \
			} \
		}; \
		static Registrar_##DerivedType _registrar_##DerivedType; \
	}
	
#define MODULE_EXPORT \
	extern "C" MOD_EXPORT void MOD_Initialize()

}

}

#endif

#endif