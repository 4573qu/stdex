//Last Modified At 2025/04/15
//@Version 1.23
#ifndef _STD4573_META_DYNAMIC_STRUCT_H_
#define _STD4573_META_DYNAMIC_STRUCT_H_ 1

#include <any>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace std {
	
namespace meta {
	
namespace core {
	
class type_registry {
public:
	
	template <typename _Tp>
	struct is_template : std::false_type {};
	
	template <template <typename...> class _Template,typename... _Args>
	struct is_template<_Template<_Args...>> : std::true_type {};
	
	template <typename _Tp>
	struct is_vector : std::false_type {};
	
	template <typename... _Args>
	struct is_vector<std::vector<_Args...>> : std::true_type {};
	
	struct type_info {
		std::size_t size_;
		std::size_t alignment_;
		bool is_primitive_;
		bool is_string_;
		std::function<void(void*)> constructor_;
		std::function<void(void*)> destructor_;
		std::function<void(void*,void*)> copy_;
		std::size_t extra_info_;
		std::size_t template_size_;
		std::string name_;
		//std::vector<std::tuple<std::string,std::string,size_t>> fields_;
		//std::type_index type_;
	};

	template <typename _Tp>
	void register_type(const std::string& name) {
		type_map_[name]=create_type_info<_Tp>();
	}
	
	bool has_type(const std::string& name) {
		return type_map_.count(name);
	}

	type_info& get_type_info(const std::string& name) const {
		return *type_map_.at(name);
	}

	template <template <typename...> class _Template,typename... _Args>
	void register_template_type(const std::string& name) {
		register_type<_Template<_Args...>>(name);
	}

private:
	template <typename _Tp>
	std::shared_ptr<type_info> create_type_info() {
		auto info=std::make_unique<type_info>();
		info->name_=typeid(_Tp).name();
		info->extra_info_=0;
		//info->type_=std::type_index(typeid(_Tp));
		info->size_=sizeof(_Tp);
		info->alignment_=alignof(_Tp);
		info->constructor_=[](void* ptr) { new(ptr) _Tp(); };
		info->destructor_=[](void* ptr) { static_cast<_Tp*>(ptr)->~_Tp(); };
		info->copy_=[](void* dst,void* src) { new(dst) _Tp(*static_cast<_Tp*>(src)); };
		info->is_primitive_=true;
		info->is_string_=false;
		if constexpr (std::is_same_v<_Tp,std::string>) {
			info->constructor_=[](void* ptr) { new(ptr) std::string(); };
			info->destructor_=[](void* ptr) { static_cast<std::string*>(ptr)->~string(); };
			info->copy_=[](void* dst,void* src) { new(dst) std::string(*static_cast<std::string*>(src)); };
			info->is_string_=true;
		} else if constexpr (is_vector<_Tp>::value) {
			using value_type=typename _Tp::value_type;
			//info->constructor_=[](void* ptr) { new(ptr) _Tp(); };
			//info->destructor_=[](void* ptr) { static_cast<_Tp*>(ptr)->~_Tp(); };
			//info->params_.push_back(typeid(value_type).name());
			info->is_primitive_=false;
			info->template_size_=sizeof(value_type);
		}	
		return info;
	}

	std::map<std::string,std::shared_ptr<type_info>> type_map_;
};

class metadata {
private:
	void trim(std::string& str) {
		std::size_t size=str.size();
		if (!size) return;
		const char *data=str.data();
		std::size_t first=0;
		while (first<size) {
			if (data[first]==' ' || data[first]=='\t') first++;
			else break;
		}
		if (first==size) {
			str="";
			return;
		}
		std::size_t last=size-1;
		while (last>first) {
			if (data[last]==' ' || data[last]=='\t') last--;
			else break;
		}
		if (!first && last==size - 1) return;
		str=str.substr(first,last-first+1);
	}
public:
	struct field_def {
		std::string name_;
		std::string type_;
		std::size_t size_;
		std::size_t offset_;
	};

	struct struct_def {
		std::string name_;
		std::vector<field_def> fields_;
	};

	explicit metadata(type_registry& registry) : registry_(registry) , strict_(false) {}
	metadata(type_registry& registry,bool strict) : registry_(registry) , strict_(strict) {}
	
	bool operator ==(const metadata& other) const {
		if (struct_def_.fields_.size()!=other.struct_def_.fields_.size()) return false;
		for (int i=0;i<struct_def_.fields_.size();i++) {
			if (struct_def_.fields_[i].size_!=other.struct_def_.fields_[i].size_ || struct_def_.fields_[i].offset_!=struct_def_.fields_[i].offset_) return false;
			if (registry_.get_type_info(struct_def_.fields_[i].type_).name_!=other.registry_.get_type_info(other.struct_def_.fields_[i].type_).name_) return false;
			//examine typename?
		}
		return true;
	}
	bool operator !=(const metadata& other) const {
		return !((*this)==other);
	}

public:
	type_registry& registry_;
	struct_def struct_def_;
	std::size_t size_;
	bool strict_;

public:
	bool load(std::istream& is) {
		bool result=parse(is);
		if (!result) return result;
		if (strict_) {
			result=examine_exists();
			if (!result) return result;
		}
		calculate_offsets();
		return true;
	}
	bool parse(std::istream& is) {
		std::string line;
		struct_def current_struct;
		while (std::getline(is,line)) {
			line=line.substr(0,line.find('#'));
			trim(line);
			if (line.empty()) {
	        			if (current_struct.name_!="") break;
	        			else continue;
			}
			if (!line.find("struct ")) {
				auto pos=line.find(':');
				std::string name=line.substr(6,pos-6);
				trim(name);
				current_struct=struct_def({name,{}});
			} else if (current_struct.name_!="" && line[0]=='-') {
				line=line.substr(1);
				auto colon_pos=line.find(':');
				std::string name=line.substr(0,colon_pos);
				trim(name);
				std::string type_part=line.substr(colon_pos+1);
				trim(type_part);
				field_def field;
				field.name_=name;
				/*field.auto_offset_=false;
				auto auto_pos=type_part.find("auto_offset");
				if (auto_pos!=std::string::npos) {
					field.auto_offset_=true;
					type_part=type_part.substr(0,auto_pos);
					trim(type_part);
				}
			auto offset_pos=type_part.find('@');
			if (offset_pos!=std::string::npos) {
					field.offset_=std::stoull(type_part.substr(offset_pos+1));
					type_part=type_part.substr(0,offset_pos);
				}*/
				field.type_=type_part;
				current_struct.fields_.push_back(field);
			}
		}
		struct_def_=current_struct;
		if (current_struct.name_!="") return true;
		return false;
	}
	bool examine_exists() {
		std::map<std::string,bool> mp;
		for (auto it:struct_def_.fields_) {
			if (mp[it.name_]) {
				throw std::invalid_argument("Duplicated field name:"+it.name_);
				return false;
			}
			mp[it.name_]=true;
		}
		return true;
	}
	void calculate_offsets() {
		std::size_t current_offset=0;
		std::size_t align=0;
		for (auto& field:struct_def_.fields_)
			align=std::max(align,registry_.get_type_info(field.type_).alignment_); 
		for (auto& field:struct_def_.fields_) {
			if (!registry_.has_type(field.type_)) throw std::invalid_argument("Undefined type: "+field.type_);
			const auto& info=registry_.get_type_info(field.type_);
			field.size_=info.size_;
			if (field.size_>=align) {
				double temp=current_offset/align+((current_offset%align==0)?0:1);
				field.offset_=temp*align;
				current_offset+=field.size_;
			} else {
				field.offset_=current_offset;
				current_offset+=field.offset_;
			}
		}
			//if (current_offset>info.size_) throw std::invalid_argument("Calculated size exceeds type declaration for "+struct_def.name_);
		size_=current_offset;
	}
	std::string to_string() {
		std::string result="";
		result+="struct "+struct_def_.name_+":\n";
		for (auto it:struct_def_.fields_) {
			result+="- "+it.name_+": "+it.type_+"\n";
		}
		return result;
	}
};

std::vector<metadata> load_metas(type_registry& registry,const std::string& path) {
	std::ifstream file(path);
	if (!file) throw std::invalid_argument("File cannot open!");
	std::vector<metadata> result;
	while (1) {
		metadata temp(registry);
		if (temp.load(file)) result.push_back(temp);
		else break;
	}
	return result;
}


class dynamic_struct {
	//ITERATOR
public:
	explicit dynamic_struct(metadata* m)  : storage_(new char[m->size_]), meta_(m) {
		for (auto it:m->struct_def_.fields_) {
			if (m->registry_.get_type_info(it.type_).is_primitive_) meta_->registry_.get_type_info(it.type_).constructor_((void*)(storage_.get()+it.offset_));
			else new(storage_.get()+it.offset_) std::vector<char*>;
			//construct
		}
	}
	
	dynamic_struct(const dynamic_struct& other) {
		meta_=other.meta_;
		if (meta_->size_) {
			storage_=std::shared_ptr<char[]>(new char[meta_->size_]);
			std::memcpy(storage_.get(),other.storage_.get(),meta_->size_);
			for (auto it:meta_->struct_def_.fields_) {
				if (meta_->registry_.get_type_info(it.type_).is_primitive_) meta_->registry_.get_type_info(it.type_).copy_((void*)(storage_.get()+it.offset_),(void*)(other.storage_.get()+it.offset_));
				else {
					new (reinterpret_cast<std::vector<char*>*>(storage_.get()+it.offset_)) std::vector<char*>(*reinterpret_cast<std::vector<char*>*>(other.storage_.get()+it.offset_));
				}
			}
		}
	}
	
	dynamic_struct(dynamic_struct&& other) noexcept {
		meta_=other.meta_;
		storage_=std::move(other.storage_);
		other.meta_=nullptr;
	}
	
	~dynamic_struct() {
		if (!meta_) return;
		for (auto it:meta_->struct_def_.fields_) {
			if (meta_->registry_.get_type_info(it.type_).is_primitive_) meta_->registry_.get_type_info(it.type_).destructor_((void*)(storage_.get()+it.offset_));
			else {
				std::vector<char*>* v=reinterpret_cast<std::vector<char*>*>(storage_.get()+it.offset_);
				v->std::vector<char*>::~vector<char*>();
			}
			//destruct
		}
	}
	
	dynamic_struct& operator =(const dynamic_struct& other) {
		if (this!=&other) {
			this->~dynamic_struct();
			new (this) dynamic_struct(other);
		}
		return *this;
	}

	template <typename _Tp>
	_Tp& get_field(const std::string& name) {
		auto& field=find_field(name);
		return *reinterpret_cast<_Tp*>(storage_.get()+field.offset_);
	}
	template <typename _Tp>
	const _Tp& get_field(const std::string& name) const {
		return const_cast<dynamic_struct*>(this)->get_field<_Tp>(name);
	}

	class proxy {
	public:
		proxy(void* ptr,const type_registry::type_info& type) : ptr_(ptr), type_(type) {}
		template <typename _Tp>
		operator _Tp&() {
			//if (typeid(_Tp)!=type_) 
				//throw std::invalid_argument("Field type mismatch");
			return *static_cast<_Tp*>(ptr_);
		}

	public:
		void* ptr_;
		const type_registry::type_info& type_;
	};

	proxy operator[](const std::string& name) {
		auto& field=find_field(name);
		proxy result((void*)(storage_.get()+field.offset_),meta_->registry_.get_type_info(field.type_));
		return result;
	}
    
	template <typename _Tp>
	_Tp& get_at(std::size_t offset) {
		return *reinterpret_cast<_Tp*>(storage_.get()+offset);
	}
	
	void* get_at_p(std::size_t offset) {
		return (void*)(storage_.get()+offset);
	}
	
	proxy operator[](std::size_t index) {
		if (index>=meta_->struct_def_.fields_.size()) throw std::out_of_range("Field index out of range");
		const auto& field=meta_->struct_def_.fields_[index];
		return {storage_.get()+field.offset_,meta_->registry_.get_type_info(field.type_)};
	}

public:
	std::shared_ptr<char[]> storage_;
	metadata* meta_;

	const metadata::field_def& find_field(const std::string& name) const {
		for (const auto& it:meta_->struct_def_.fields_) {
    			if (it.name_==name) return it;
		}
		throw std::invalid_argument("Field not found: "+name);
	}
};

}

namespace serialization {
	
class binary_writer {
public:
	explicit binary_writer(std::ostream& os) : os_(os) {}
	void write(core::dynamic_struct& ds) {
		write_field(ds);
	}

private:
	std::ostream& os_;
	void write_field(core::dynamic_struct& ds) {
		for (auto it:ds.meta_->struct_def_.fields_) {
			if (ds.meta_->registry_.get_type_info(it.type_).is_primitive_) {
				if (ds.meta_->registry_.get_type_info(it.type_).is_string_) {
					const auto& p=ds[it.name_];
					std::string* v=reinterpret_cast<std::string*>(p.ptr_);
					write_string(*v);
				} else {
					const auto& p=ds[it.name_];
					char* temp=new char[p.type_.size_];
					for (int i=0;i<p.type_.size_;i++) temp[i]=*(((char*)p.ptr_)+i);
					write_primitive(temp,p.type_.size_);
					//delete[] temp;
				}
			} else {
				const auto& p=ds[it.name_];
				std::vector<char*>* v=reinterpret_cast<std::vector<char*>*>(p.ptr_);
				write_vector(*v,p.type_.template_size_);
			}
		}
	}

	void write_primitive(char* value,std::size_t size) {
		os_.write(reinterpret_cast<const char*>(value),size);
	}
	void write_string(std::string str) {
		char* length=new char[sizeof(std::size_t)+1];
		sprintf(length,"%0*d",sizeof(std::size_t),str.size());
		write_primitive(length,sizeof(std::size_t));
		delete[] length;
		write_primitive(const_cast<char*>(str.c_str()),str.size());
	}
	void write_vector(std::vector<char*> vec,std::size_t size) {
		char* length=new char[sizeof(std::size_t)+1];
		sprintf(length,"%0*d",sizeof(std::size_t),vec.size());
		write_primitive(length,sizeof(std::size_t));
		delete[] length;
		/*for (const auto& elem:vec) {
			std::visit([this](auto&& arg) {
				using _Tp=std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<_Tp,dynamic_struct>) write(arg);
				else write_primitive(arg,size);
			},elem);
		}*/
		for (auto it:vec) write_primitive(it,size);	
	}
};

class binary_reader {
public:
	explicit binary_reader(std::istream& is) : is_(is) {}
	core::dynamic_struct read(core::metadata& m) {
		return read_field(m);
	}
	std::size_t get_size(const char* str,std::size_t size=sizeof(std::size_t)) {
		std::size_t result=0;
 		for (int i=0;i<size;i++) {
			if (std::isdigit(str[i])) result=result*10+(str[i]-'0');
		}
		return result;
	}

private:
	std::istream& is_;
	core::dynamic_struct read_field(core::metadata& m) {
		core::dynamic_struct result(&m);
		for (auto it:result.meta_->struct_def_.fields_) {
			if (result.meta_->registry_.get_type_info(it.type_).is_primitive_) {
				if (result.meta_->registry_.get_type_info(it.type_).is_string_) {
					const auto& p=result[it.name_];
					const char* temp=read_string();
					new(p.ptr_) std::string(temp);
					//delete[] temp;
				} else {
					const auto& p=result[it.name_];
					const char* temp=read_primitive(p.type_.size_);
					strcpy((char*)p.ptr_,temp);
					//for (int i=0;i<p.type_.size_;i++)  p.ptr_=temp[i];
					//delete[] temp;	
				}
			} else {
				const auto& p=result[it.name_];
				auto res=read_vector(p.type_.template_size_);
				new(p.ptr_) std::vector<const char*>(res);
			}
		}
		return result;
	}
	const char* read_primitive(std::size_t size) {
		const char* value=new char[size+1];
		const_cast<char*>(value)[size]='\0';
		is_.read(const_cast<char*>(value),size);
		return value;
	}
	const char* read_string() {
		auto ssize=get_size(read_primitive(sizeof(std::size_t)),sizeof(std::size_t));
		return read_primitive(reinterpret_cast<size_t>(ssize));
	}
	std::vector<const char*> read_vector(std::size_t size) {
		auto vsize=get_size(read_primitive(sizeof(std::size_t)),sizeof(std::size_t));
		std::vector<const char*> result;
		result.resize(reinterpret_cast<std::size_t>(vsize));
		for (int i=0;i<result.size();i++) {
			const char* temp=read_primitive(size);
			result[i]=temp;
		}
		return result;
        /*for (auto& elem:vec) {
            // 根据模板参数类型读取元素
            if (type == "vector<int32>") {
                elem = read_primitive<int32_t>();
            }
            // 其他类型处理...
		}*/
	}
};

}
	
}

}

#endif