//Last Modified At 2026/06/01
//@Version 1.0.2.0
#ifndef _STDEX_UTILITY_PLUGIN_H_
#define _STDEX_UTILITY_PLUGIN_H_ 1

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "version.h"//At Least 1.0

#if __has_include("../macros/cpp_platform.h")
#include "../macros/cpp_platform.h"//At Least 1.0
#endif

#ifndef _STDEX_WINDOWS_PLATFORM
#if defined(_WIN32)
#define _STDEX_WINDOWS_PLATFORM 1
#else
#define _STDEX_WINDOWS_PLATFORM 0
#endif
#endif
#ifndef _STDEX_LINUX_PLATFORM
#if defined(__linux__)
#define _STDEX_LINUX_PLATFORM 1
#else
#define _STDEX_LINUX_PLATFORM 0
#endif
#endif
#ifndef _STDEX_ANDROID_PLATFORM
#if defined(__ANDROID__)
#define _STDEX_ANDROID_PLATFORM 1
#else
#define _STDEX_ANDROID_PLATFORM 0
#endif
#endif
#ifndef _STDEX_APPLE_PLATFORM
#if defined(__APPLE__)
#define _STDEX_APPLE_PLATFORM 1
#else
#define _STDEX_APPLE_PLATFORM 0
#endif
#endif
#ifndef _STDEX_IOS_PLATFORM
#if _STDEX_APPLE_PLATFORM
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define _STDEX_IOS_PLATFORM 1
#else
#define _STDEX_IOS_PLATFORM 0
#endif
#else
#define _STDEX_IOS_PLATFORM 0
#endif
#endif
#ifndef _STDEX_MACOS_PLATFORM
#if _STDEX_APPLE_PLATFORM
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#define _STDEX_MACOS_PLATFORM 1
#else
#define _STDEX_MACOS_PLATFORM 0
#endif
#else
#define _STDEX_MACOS_PLATFORM 0
#endif
#endif

#if _STDEX_IOS_PLATFORM
#define _STDEX_PLUGIN_DYNAMIC_ENABLED 0
#else
#define _STDEX_PLUGIN_DYNAMIC_ENABLED 1
#endif

#if _STDEX_PLUGIN_DYNAMIC_ENABLED
#if _STDEX_WINDOWS_PLATFORM
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#endif

namespace stdex {

namespace utility {

namespace plugin {

class plugin_context;

class plugin {
public:
	virtual ~plugin()=default;
	virtual const plugin_info& info() const noexcept=0;
	virtual void on_load(plugin_context& ctx)=0;
	virtual void on_unload(plugin_context& ctx) noexcept {
		(void)ctx;
	}
};

using create_plugin_func=plugin*(*)();
using destroy_plugin_func=void(*)(plugin*);

inline constexpr const char* default_create_symbol_name="plugin_create_plugin";
inline constexpr const char* default_destroy_symbol_name="plugin_destroy_plugin";

class event_bus {
	using erased_listener=std::function<void(const void*)>;
	std::unordered_map<std::type_index,std::vector<std::pair<std::uint64_t,erased_listener>>> listeners_;
	std::uint64_t last_id_=0;

public:
	event_bus()=default;

	template <class _Tp>
	using listener=std::function<void(const _Tp&)>;

	template <class _Tp>
	std::uint64_t subscribe(listener<_Tp> fn) {
		auto& it=listeners_[std::type_index(typeid(_Tp))];
		std::uint64_t id=++last_id_;
		it.emplace_back(id,[f= std::move(fn)](const void* e){
			f(*static_cast<const _Tp*>(e));
		});
		return id;
	}
	template <class _Tp>
	void unsubscribe(std::uint64_t id) {
		auto it=listeners_.find(std::type_index(typeid(_Tp)));
		if (it==listeners_.end()) return;
		auto& jt=it->second;
		jt.erase(std::remove_if(jt.begin(),jt.end(),[id](const auto& pair){
			return pair.first==id;
		}),jt.end());
	}
	template <class _Tp>
	void publish(const _Tp& ev) const {
		auto it=listeners_.find(std::type_index(typeid(_Tp)));
		if (it==listeners_.end()) return;
		for (const auto& jt:it->second) jt.second(&ev);
	}
};

class plugin_context {
	event_bus* bus_;
	std::unordered_map<std::type_index,void*> services_;

public:
	explicit plugin_context(event_bus& bus) : bus_(&bus) { }
	event_bus& events() noexcept { return *bus_; }
	const event_bus& events() const noexcept { return *bus_; }

	template <class _Tp>
	void register_service(_Tp* s) {
		services_[std::type_index(typeid(_Tp))]=s;
	}

	template <class _Tp>
	_Tp* get_service() const noexcept {
		auto it=services_.find(std::type_index(typeid(_Tp)));
		if (it==services_.end()) return nullptr;
		return static_cast<_Tp*>(it->second);
	}
};


#if _STDEX_PLUGIN_DYNAMIC_ENABLED

class shared_library {
	void* handle_=nullptr;
public:
	shared_library() noexcept=default;
	explicit shared_library(const std::string& path) {
		load(path);
	}
	~shared_library() {
		unload();
	}
	shared_library(const shared_library&)=delete;
	shared_library& operator =(const shared_library&)=delete;
	shared_library(shared_library&& other) noexcept : handle_(other.handle_) {
		other.handle_=nullptr;
	}
	shared_library& operator =(shared_library&& other) noexcept {
		if (this!=&other) {
			unload();
			handle_=other.handle_;
			other.handle_=nullptr;
		}
		return *this;
	}
	bool is_loaded() const noexcept {
		return handle_!=nullptr;
	}
	void load(const std::string& path) {
		unload();
#if _STDEX_WINDOWS_PLATFORM
		handle_=reinterpret_cast<void*>(::LoadLibraryA(path.c_str()));
		if (!handle_) throw runtime_error("Failed to load module: "+path);
#else
		handle_=::dlopen(path.c_str(),RTLD_NOW);
		if (!handle_) {
			const char* err=::dlerror();
			throw runtime_error(std::string("Failed to load module: ")+path+" : "+(err?err:""));
		}
#endif
	}
	void unload() noexcept {
		if (!handle_) return;
#if _STDEX_WINDOWS_PLATFORM
		::FreeLibrary(reinterpret_cast<HMODULE>(handle_));
#else
		::dlclose(handle_);
#endif
		handle_=nullptr;
	}
	template <class _Func>
	_Func get_symbol(const std::string& name) {
#if _STDEX_WINDOWS_PLATFORM
		auto sym=::GetProcAddress(reinterpret_cast<HMODULE>(handle_),name.c_str());
		if (!sym) throw runtime_error("Symbol not found: "+name);
		return reinterpret_cast<_Func>(sym);
#else
		auto sym=::dlsym(handle_,name.c_str());
		if (!sym) throw runtime_error("Symbol not found: "+name);
		return reinterpret_cast<_Func>(sym);
#endif
	}
};

#endif

class plugin_instance {
public:
	using static_factory=std::function<std::unique_ptr<plugin>()>;

private:
	enum kind {
		KD_STATIC,
#if _STDEX_PLUGIN_DYNAMIC_ENABLED
		KD_DYNAMIC,
#endif
	};
	kind kind_;

#if _STDEX_PLUGIN_DYNAMIC_ENABLED
	void load_symbols() {
		create_=lib_.get_symbol<create_plugin_func>(create_symbol_name_);
		destroy_=lib_.get_symbol<destroy_plugin_func>(destroy_symbol_name_);
	}
	shared_library lib_;
	create_plugin_func create_=nullptr;
	destroy_plugin_func destroy_=nullptr;
	std::string create_symbol_name_;
	std::string destroy_symbol_name_;
#endif
	static_factory static_factory_;
	std::unique_ptr<plugin> static_storage_;
	plugin* plugin_ptr_=nullptr;

public:
#if _STDEX_PLUGIN_DYNAMIC_ENABLED
	plugin_instance(std::string path,std::string create_symbol=default_create_symbol_name,std::string destroy_symbol=default_destroy_symbol_name) : kind_(KD_DYNAMIC) , lib_(std::move(path)) , create_symbol_name_(std::move(create_symbol)) , destroy_symbol_name_(std::move(destroy_symbol)) {
		load_symbols();
		plugin_ptr_=create_();
		if (!plugin_ptr_) throw runtime_error("Create_plugin returned null for dynamic module");
	}
#endif
	explicit plugin_instance(static_factory factory) : kind_(KD_STATIC) , static_factory_(std::move(factory)) {
		if (!static_factory_) throw runtime_error("Static plugin factory is null");
		static_storage_=static_factory_();
		plugin_ptr_=static_storage_.get();
		if (!plugin_ptr_) throw runtime_error("Static plugin factory returned null");
	}
	~plugin_instance() {
#if _STDEX_PLUGIN_DYNAMIC_ENABLED
		if (kind_==KD_DYNAMIC) {
			if (plugin_ptr_ && destroy_) {
				destroy_(plugin_ptr_);
				plugin_ptr_=nullptr;
			}
		}
#endif
	}

	plugin_instance(const plugin_instance&)=delete;
	plugin_instance& operator =(const plugin_instance&)=delete;
	plugin_instance(plugin_instance&& other) noexcept : kind_(other.kind_) ,
#if _STDEX_PLUGIN_DYNAMIC_ENABLED
		  lib_(std::move(other.lib_)) , create_(other.create_) , destroy_(other.destroy_) , create_symbol_name_(std::move(other.create_symbol_name_)) , destroy_symbol_name_(std::move(other.destroy_symbol_name_)) ,
#endif
		  static_factory_(std::move(other.static_factory_)) , static_storage_(std::move(other.static_storage_)) , plugin_ptr_(other.plugin_ptr_) {
		other.plugin_ptr_=nullptr;
#if _STDEX_PLUGIN_DYNAMIC_ENABLED
		other.create_=nullptr;
		other.destroy_=nullptr;
#endif
	}
	plugin_instance& operator =(plugin_instance&& other) noexcept {
		if (this!=&other) {
			this->~plugin_instance();
			kind_=other.kind_;
#if _STDEX_PLUGIN_DYNAMIC_ENABLED
			lib_=std::move(other.lib_);
			create_=other.create_;
			destroy_=other.destroy_;
			create_symbol_name_=std::move(other.create_symbol_name_);
			destroy_symbol_name_=std::move(other.destroy_symbol_name_);
			other.create_=nullptr;
			other.destroy_=nullptr;
#endif
			static_factory_=std::move(other.static_factory_);
			static_storage_=std::move(other.static_storage_);
			plugin_ptr_=other.plugin_ptr_;
			other.plugin_ptr_=nullptr;
		}
		return *this;
	}

	plugin& get() noexcept { return *plugin_ptr_; }
	const plugin& get() const noexcept { return *plugin_ptr_; }
	plugin* operator ->() noexcept { return plugin_ptr_; }
	const plugin* operator ->() const noexcept { return plugin_ptr_; }
};

class plugin_manager {
	plugin_context* ctx_;
	std::map<std::string,plugin_instance> plugins_;

	std::string add_instance(plugin_instance inst) {
		plugin& p=inst.get();
		const plugin_info& info=p.info();
		if (plugins_.count(info.id_)!=0) throw runtime_error("Plugin with id already loaded: "+info.id_);
		for (const auto& it:info.required_plugins_) {
			if (plugins_.count(it)==0) throw runtime_error("Missing required plugin: "+it+" for plugin "+info.id_);
		}
		auto id=info.id_;
		plugins_.emplace(id,std::move(inst));
		plugins_.at(id).get().on_load(*ctx_);
		return id;
	}

public:
	explicit plugin_manager(plugin_context& ctx) : ctx_(&ctx) { }
#if _STDEX_PLUGIN_DYNAMIC_ENABLED
	std::string load_plugin_from_file(const std::string& path,const std::string& create_symbol=default_create_symbol_name,const std::string& destroy_symbol=default_destroy_symbol_name) {
		plugin_instance inst(path, create_symbol, destroy_symbol);
		return add_instance(std::move(inst));
	}
#endif

	std::string register_static_plugin(plugin_instance::static_factory factory) {
		plugin_instance inst(std::move(factory));
		return add_instance(std::move(inst));
	}

	void unload_plugin(const std::string& id) noexcept {
		auto it=plugins_.find(id);
		if (it==plugins_.end()) return;
		plugin& p=it->second.get();
		try {
			p.on_unload(*ctx_);
		} catch (...) {
		}
		plugins_.erase(it);
	}
	bool has_plugin(const std::string& id) const noexcept {
		return plugins_.count(id)!=0;
	}
	plugin* get_plugin(const std::string& id) noexcept {
		auto it=plugins_.find(id);
		if (it==plugins_.end()) return nullptr;
		return &it->second.get();
	}
	const plugin* get_plugin(const std::string& id) const noexcept {
		auto it=plugins_.find(id);
		if (it==plugins_.end()) return nullptr;
		return &it->second.get();
	}

	const std::map<std::string,plugin_instance>& plugins() const noexcept {
		return plugins_;
	}
};

}

}

}

#endif