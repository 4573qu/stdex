//Last Modified At 2025/09/18
//@Version 1.0.0.0
#ifndef _STDEX_MACHINE_INTEL_H_
#define _STDEX_MACHINE_INTEL_H_ 1

#include <map>

#include "general.h"//At Least 1.0.0.1

namespace stdex {

namespace machine {

namespace intel {

enum code_type {
#define _STDEX_INTEL_ASSEMBLER_CODE_TYPE(name) CT_##name,
#include "intel_assembler.inc"
#undef _STDEX_INTEL_ASSEMBLER_CODE_TYPE
};

enum line_type {
	LT_NONE,
	LT_SENTENCE,
	LT_SENTENCES,
	LT_SPECIALIZE_SENTENCE,
	LT_SPECIALIZE_66_SENTENCE,
	LT_AAA_SENTENCE,
	LT_AAD_SENTENCE,
	LT_AAM_SENTENCE,
	LT_AAS_SENTENCE,
	LT_ADC14_SENTENCE,
	LT_ADC15_SENTENCE,
	LT_ADC15_64_SENTENCE,
	LT_SUB66_SENTENCE,
	LT_CLR66_SENTENCE,
	LT_SUB80_SENTENCE,
	LT_SUB81_SENTENCE,
	LT_SUB83_SENTENCE,
	LT_80_SENTENCE,
	LT_80_64_SENTENCE,
	LT_81_SENTENCE,
	LT_81_64_SENTENCE,
	LT_83_SENTENCE,
	LT_83_64_SENTENCE,
	LT_REX_TO_BITS,
	LT_SPECIALIZE_BITS,
	LT_SPECIALIZE_REX,
	LT_SPECIALIZE_REX_TO_SENTENCE_32,
};

enum token_type {
	TT_START=0x100,
	TT_SEPERATOR,
	TT_EPSILON,
	TT_EOF,

	TT_SENTENCE=0x1000,
	TT_SENTENCES,
	TT_AAA_SENTENCE,
	TT_AAD_SENTENCE,
	TT_AAM_SENTENCE,
	TT_AAS_SENTENCE,
	TT_ADC_SENTENCE,
	TT_ADC_66_SENTENCE,
	TT_66_SENTENCE,
	TT_80_SENTENCE,
	TT_81_SENTENCE,
	TT_83_SENTENCE,

	TT_BITS=0x10000,
	TT_REX,
};

bool rex_w(BYTE rex,bool strict=false) {
	if (strict && (rex>>4)!=4) return false;
	return (rex>>3)&1;
}

bool rex_r(BYTE rex,bool strict=false) {
	if (strict && (rex>>4)!=4) return false;
	return (rex>>2)&1;
}

bool rex_x(BYTE rex,bool strict=false) {
	if (strict && (rex>>4)!=4) return false;
	return (rex>>1)&1;
}

bool rex_b(BYTE rex,bool strict=false) {
	if (strict && (rex>>4)!=4) return false;
	return rex&1;
}

BYTE modrm_mod(BYTE modrm) {
	return modrm>>6;
}

BYTE modrm_reg(BYTE modrm) {
	return (modrm>>3)&7;
}

BYTE modrm_rm(BYTE modrm) {
	return modrm&7;
}

enum operand_type {
	OT_IMMEDIATE,
	OT_REGISTER,
	OT_MODRM
};

struct operand {
	operand_type type_;
	machine_bits bits_;
	machine_bits address_bits_;
	QWORD value_;
	QWORD sib_;
	QWORD offset_;
	QWORD length_;
	BYTE rex_;//only when address_bits_==MB_64
	bool reverse_rm_;

private:
	static inline const char* reg8_64_[16]={"AL","CL","DL","BL","SPL","BPL","SIL","DIL","R8B","R9B","R10B","R11B","R12B","R13B","R14B","R15B"};
	static inline const char* reg8_32_[16]={"AL","CL","DL","BL","AH","CH","DH","BH","R8B","R9B","R10B","R11B","R12B","R13B","R14B","R15B"};
	static inline const char* reg16_[16]={"AX","CX","DX","BX","SP","BP","SI","DI","R8W","R9W","R10W","R11W","R12W","R13W","R14W","R15W"};
	static inline const char* reg32_[16]={"EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI","R8D","R9D","R10D","R11D","R12D","R13D","R14D","R15D"};
	static inline const char* reg64_[16]={"RAX","RCX","RDX","RBX","RSP","RBP","RSI","RDI","R8","R9","R10","R11","R12","R13","R14","R15"};
	static inline const char* modrm16_[8]={"[BX+SI]","[BX+DI]","[BP+SI]","[BP+DI]","[SI]","[DI]","[BP]","[BX]"};
	static inline const char* modrm32_[8]={"[EAX]","[ECX]","[EDX]","[EBX]","[ESP]","[EBP]","[ESI]","[EDI]"};
	static inline const char* mm_[8]={"MM0","MM1","MM2","MM3","MM4","MM5","MM6","MM7"};
	static inline const char* xmm_[8]={"XMM0","XMM1","XMM2","XMM3","XMM4","XMM5","XMM6","XMM7"};
	const char** get_reg_map() {
		switch (bits_) {
			case MB_16: return reg16_;
			case MB_32: return reg32_;
			case MB_64: rex_w(rex_)?reg64_:reg32_;
		}
		return reg32_;
	}

	std::string get_imm_string(QWORD value,int length) {
		value&=((1ULL<<(length*8))-1);
		char* temp=new char[18];
		sprintf(temp,"0x%0*X",length,value);
		std::string result=temp;
		delete[] temp;
		return result;
	}
public:
	operand() {
		value_=sib_=offset_=length_=0;
		rex_=0;
		reverse_rm_=false;
	}
	std::string to_intel_string() {
		int length=4;
		if (bits_==MB_8) length=1;
		if (bits_==MB_16) length=2;
		else if (bits_==MB_64) length=8;
		switch (type_) {
			case OT_IMMEDIATE: {
				return get_imm_string(value_,length);
			}
			case OT_REGISTER: {
				BYTE value=value_&0x7;
				bool rex=!!(rex_&0x40) && bits_==MB_64;
				if (bits_==MB_64) value=rex_r(rex_,true)<<3+value;
				switch (bits_) {
					case MB_8: return rex?reg8_64_[value]:reg8_32_[value&7];
					case MB_16: return reg16_[rex?value:(value&7)];
					case MB_32: return reg32_[rex?value:(value&7)];
					case MB_64: return reg64_[rex?value:(value&7)];
				}
			}
			case OT_MODRM: {
				switch (address_bits_) {
					case MB_16: {
						BYTE mod=modrm_mod(value_);
						BYTE reg=modrm_reg(value_);
						BYTE rm=modrm_rm(value_);
						if (bits_==MB_64) {
							rm+=rex_b(rex_,true)<<3;
							reg+=rex_r(rex_,true)<<3;
						}
						std::string dst,src;
						dst=modrm16_[rm&7];
						if (mod==0 && rm&7==6) dst=get_imm_string(offset_,2);
						if (mod==1) dst+="+"+get_imm_string(offset_,1);
						if (mod==2) dst+="+"+get_imm_string(offset_,2);
						if (mod==3) dst=get_reg_map()[rm];
						src=get_reg_map()[reg];
						return reverse_rm_?(src+","+dst):(dst+","+src);
					}
					default: {
						BYTE mod=modrm_mod(value_);
						BYTE reg=modrm_reg(value_);
						BYTE rm=modrm_rm(value_);
						if (bits_==MB_64) {
							rm+=rex_b(rex_,true)<<3;
							reg+=rex_r(rex_,true)<<3;
						}
						std::string dst,src,dst_extra;
						dst=modrm32_[rm&7];
						if (mod==0 && rm&7==5) {
							if (address_bits_!=MB_64) dst=get_imm_string(offset_,4);
							else dst="[RIP+"+get_imm_string(offset_,4)+"]";
						}
						if (mod==1) dst_extra="+"+get_imm_string(offset_,1);
						if (mod==2) dst_extra="+"+get_imm_string(offset_,4);
						dst+=dst_extra;
						if (mod!=3 && rm&7==4) {
							//SIB
							std::string sib,sib_base,sib_index,sib_extra;
							BYTE scale=modrm_mod(sib_);
							BYTE index=modrm_reg(sib_);
							BYTE base=modrm_rm(sib_);
							if (bits_==MB_64) {
								base+=rex_b(rex_,true)<<3;
								index+=rex_x(rex_,true)<<3;
							}
							sib_base=base==5?"":get_reg_map()[base];
							sib_index=index==4?"":get_reg_map()[index];
							if (scale!=0 && sib_index!="") sib_index+="*"+std::to_string(1<<scale);
							//sib_base+sib_index+dst_extra
							if (sib_base=="" && sib_index=="") dst="["+(dst_extra==""?get_imm_string(0,1):dst_extra)+"]";
							else if (sib_base=="") dst="["+sib_index+(dst_extra==""?"":("+"+dst_extra))+"]";
							else if (sib_index=="") dst="["+sib_base+(dst_extra==""?"":("+"+dst_extra))+"]";
							else dst="["+sib_base+"+"+sib_index+(dst_extra==""?"":("+"+dst_extra))+"]";
						}
						if (mod==3) dst=get_reg_map()[rm];
						src=get_reg_map()[reg];
						return reverse_rm_?(src+","+dst):(dst+","+src);
					}
				}
			}
		}
		return "";
	}
	/*void flip_operand_bits() {
		if (bits_==MB_16) bits_=MB_32;
		else if (bits_==MB_32) bits_=MB_16;
	}
	void flip_address_bits() {
		if (address_bits_ == MB_16) bits_ = MB_32;
		else if (address_bits_ == MB_32) bits_ = MB_16;
	}*/
};

struct instruction {
	code_type code_;
	std::vector<operand> operands_;
	std::size_t offset_;
	static inline std::map<code_type,std::string> code_name_={
#define _STDEX_INTEL_ASSEMBLER_CODE_TYPE(name) {CT_##name,#name},
#include "intel_assembler.inc"
#undef _STDEX_INTEL_ASSEMBLER_CODE_TYPE
	};
	std::string to_intel_string() {
		std::string result=code_name_[code_];
		/*if (operands_.size()>0) {
			result+=" ";
			for (std::size_t i=0;i<operands_.size();i++) {
				result+=operands_[i].to_intel_string();
				if (i!=operands_.size()-1) result+=",";
			}
		}*/
		result+=" ";
		for (int i=0;i<operands_.size();i++) result+=operands_[i].to_intel_string()+",";
		result.pop_back();
		return result;
	}
};

}

}

}

#endif