//Last Modified At 2025/09/19
//@Version 1.0.0.0
#ifndef _STDEX_MACHINE_INTEL_H_
#define _STDEX_MACHINE_INTEL_H_ 1

#include <algorithm>
#include <cctype>
#include <cstddef>
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
	
	LT_DIGITS_START=0xFF,
	LT_ADD00_SENTENCE,LT_ADD00_64_SENTENCE,LT_ADD01_SENTENCE,LT_ADD01_64_SENTENCE,
	LT_ADD02_SENTENCE,LT_ADD02_64_SENTENCE,LT_ADD03_SENTENCE,LT_ADD03_64_SENTENCE,
	LT_ADD04_SENTENCE,LT_ADD04_64_SENTENCE,/*No Use*/LT_ADD05_SENTENCE,LT_ADD05_64_SENTENCE,
	LT_OR08_SENTENCE, LT_OR08_64_SENTENCE, LT_OR09_SENTENCE, LT_OR09_64_SENTENCE,
	LT_OR0A_SENTENCE, LT_OR0A_64_SENTENCE, LT_OR0B_SENTENCE, LT_OR0B_64_SENTENCE,
	LT_OR0C_SENTENCE, LT_OR0C_64_SENTENCE, /*No Use*/LT_OR0D_SENTENCE, LT_OR0D_64_SENTENCE,
	LT_ADC10_SENTENCE,LT_ADC10_64_SENTENCE,LT_ADC11_SENTENCE,LT_ADC11_64_SENTENCE,
	LT_ADC12_SENTENCE,LT_ADC12_64_SENTENCE,LT_ADC13_SENTENCE,LT_ADC13_64_SENTENCE,
	LT_ADC14_SENTENCE,LT_ADC14_64_SENTENCE,/*No Use*/LT_ADC15_SENTENCE,LT_ADC15_64_SENTENCE,
	LT_SBB18_SENTENCE,LT_SBB18_64_SENTENCE,LT_SBB19_SENTENCE,LT_SBB19_64_SENTENCE,
	LT_SBB1A_SENTENCE,LT_SBB1A_64_SENTENCE,LT_SBB1B_SENTENCE,LT_SBB1B_64_SENTENCE,
	LT_SBB1C_SENTENCE,LT_SBB1C_64_SENTENCE,/*No Use*/LT_SBB1D_SENTENCE,LT_SBB1D_64_SENTENCE,
	LT_AND20_SENTENCE,LT_AND20_64_SENTENCE,LT_AND21_SENTENCE,LT_AND21_64_SENTENCE,
	LT_AND22_SENTENCE,LT_AND22_64_SENTENCE,LT_AND23_SENTENCE,LT_AND23_64_SENTENCE,
	LT_AND24_SENTENCE,LT_AND24_64_SENTENCE,/*No Use*/LT_AND25_SENTENCE,LT_AND25_64_SENTENCE,
	LT_SUB28_SENTENCE,LT_SUB28_64_SENTENCE,LT_SUB29_SENTENCE,LT_SUB29_64_SENTENCE,
	LT_SUB2A_SENTENCE,LT_SUB2A_64_SENTENCE,LT_SUB2B_SENTENCE,LT_SUB2B_64_SENTENCE,
	LT_SUB2C_SENTENCE,LT_SUB2C_64_SENTENCE,/*No Use*/LT_SUB2D_SENTENCE,LT_SUB2D_64_SENTENCE,
	LT_XOR30_SENTENCE,LT_XOR30_64_SENTENCE,LT_XOR31_SENTENCE,LT_XOR31_64_SENTENCE,
	LT_XOR32_SENTENCE,LT_XOR32_64_SENTENCE,LT_XOR33_SENTENCE,LT_XOR33_64_SENTENCE,
	LT_XOR34_SENTENCE,LT_XOR34_64_SENTENCE,/*No Use*/LT_XOR35_SENTENCE,LT_XOR35_64_SENTENCE,
	LT_CMP38_SENTENCE,LT_CMP38_64_SENTENCE,LT_CMP39_SENTENCE,LT_CMP39_64_SENTENCE,
	LT_CMP3A_SENTENCE,LT_CMP3A_64_SENTENCE,LT_CMP3B_SENTENCE,LT_CMP3B_64_SENTENCE,
	LT_CMP3C_SENTENCE,LT_CMP3C_64_SENTENCE,/*No Use*/LT_CMP3D_SENTENCE,LT_CMP3D_64_SENTENCE,
	
	LT_BOUND62_SENTENCE=0x200,
	LT_ARPL63_SENTENCE,
	
	LT_SUB66_SENTENCE=0x1000,
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
	
	TT_ADD_SENTENCE,
	TT_OR_SENTENCE,
	TT_ADC_SENTENCE,
	TT_SBB_SENTENCE,
	TT_AND_SENTENCE,
	TT_SUB_SENTENCE,
	TT_XOR_SENTENCE,
	TT_CMP_SENTENCE,
	
	TT_BOUND_SENTENCE,
	TT_ARPL_SENTENCE,

	TT_66_SENTENCE,
	TT_80_SENTENCE,
	TT_81_SENTENCE,
	TT_83_SENTENCE,

	TT_BITS=0x10000,
	TT_REX,
};

bool rex_w(BYTE rex,bool strict=true) {
	if (strict && (rex>>4)!=4) return false;
	return (rex>>3)&1;
}

bool rex_r(BYTE rex,bool strict=true) {
	if (strict && (rex>>4)!=4) return false;
	return (rex>>2)&1;
}

bool rex_x(BYTE rex,bool strict=true) {
	if (strict && (rex>>4)!=4) return false;
	return (rex>>1)&1;
}

bool rex_b(BYTE rex,bool strict=true) {
	if (strict && (rex>>4)!=4) return false;
	return rex&1;
}

bool rex_all(BYTE rex) {
	return rex&0x40;
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
	bool rm_no_reg_;
	bool bound_;

private:
	static inline const char* reg8_64_[16]={"AL","CL","DL","BL","SPL","BPL","SIL","DIL","R8B","R9B","R10B","R11B","R12B","R13B","R14B","R15B"};
	static inline const char* reg8_32_[16]={"AL","CL","DL","BL","AH","CH","DH","BH","R8B","R9B","R10B","R11B","R12B","R13B","R14B","R15B"};
	static inline const char* reg16_[16]={"AX","CX","DX","BX","SP","BP","SI","DI","R8W","R9W","R10W","R11W","R12W","R13W","R14W","R15W"};
	static inline const char* reg32_[16]={"EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI","R8D","R9D","R10D","R11D","R12D","R13D","R14D","R15D"};
	static inline const char* reg64_[16]={"RAX","RCX","RDX","RBX","RSP","RBP","RSI","RDI","R8","R9","R10","R11","R12","R13","R14","R15"};
	static inline const char* modrm16_[8]={"[BX+SI]","[BX+DI]","[BP+SI]","[BP+DI]","[SI]","[DI]","[BP]","[BX]"};
	static inline const char* mm_[8]={"MM0","MM1","MM2","MM3","MM4","MM5","MM6","MM7"};
	static inline const char* xmm_[8]={"XMM0","XMM1","XMM2","XMM3","XMM4","XMM5","XMM6","XMM7"};
	static inline const char* line_[4]={"byte ptr","word ptr","dword ptr","qword ptr"};
	const char** get_reg_map() {
		switch (bits_) {
			case MB_8: return rex_all(rex_)?reg8_64_:reg8_32_;
			case MB_16: return reg16_;
			case MB_32: return reg32_;
			case MB_64: return rex_w(rex_)?reg64_:reg32_;
		}
		return reg32_;
	}
	std::string get_imm_string(QWORD value,int length,bool upper,bool omit_leading_zero) {
		value&=((1ULL<<(length*8))-1);
		char* temp=new char[18];
		sprintf(temp,"0x%0*X",omit_leading_zero?0:(length*2),value);
		std::string result=temp;
		delete[] temp;
		if (!upper) std::transform(result.begin(),result.end(),result.begin(),::tolower);
		return result;
	}
	void add_length_sign(std::string& str,machine_bits bits,bool with_full_length) {
		if (str[0]=='[') {
			machine_bits op_bits=(machine_bits)((int)bits_+bound_);
			if (op_bits!=bits || with_full_length) str=std::string(line_[op_bits-MB_8])+" "+str;
		}
	}
public:
	operand() {
		value_=sib_=offset_=length_=0;
		rex_=0;
		reverse_rm_=false;
		rm_no_reg_=false;
	}
	std::string to_intel_string(bool upper_operand,bool upper_operator,bool omit_leading_zero,machine_bits bits,std::size_t offset,bool with_full_length) {
		int length=4;
		if (bits_==MB_8) length=1;
		if (bits_==MB_16) length=2;
		else if (bits_==MB_64) length=8;
		switch (type_) {
			case OT_IMMEDIATE: {
				return get_imm_string(value_,length,upper_operator,omit_leading_zero);
			}
			case OT_REGISTER: {
				BYTE value=value_&0x7;
				bool rex=rex_all(rex_);// && bits_==MB_64;
				/*if (bits_==MB_64)*/ value=rex_r(rex_)<<3+value;
				std::string result;
				switch (bits_) {
					case MB_8: {
						result=rex?reg8_64_[value]:reg8_32_[value&7];
						break;
					}
					case MB_16: {
						result=reg16_[rex?value:(value&7)];
						break;
					}
					case MB_32: {
						result=reg32_[rex?value:(value&7)];
						break;
					}
					case MB_64: {
						result=reg64_[rex?value:(value&7)];
						break;
					}
				}
				if (!upper_operand) std::transform(result.begin(),result.end(),result.begin(),::tolower);
				return result;
			}
			case OT_MODRM: {
				switch (address_bits_) {
					case MB_16: {
						BYTE mod=modrm_mod(value_);
						BYTE reg=modrm_reg(value_);
						BYTE rm=modrm_rm(value_);
						//if (bits_==MB_64) {
						rm+=rex_b(rex_)<<3;
						reg+=rex_r(rex_)<<3;
						//}
						std::string dst,src;
						dst=modrm16_[rm&7];
						if (!upper_operand) std::transform(dst.begin(),dst.end(),dst.begin(),::tolower);
						if (mod==0 && rm&7==6) dst=get_imm_string(offset_,2,upper_operator,omit_leading_zero);
						if (mod==1) dst+="+"+get_imm_string(offset_,1,upper_operator,omit_leading_zero);
						if (mod==2) dst+="+"+get_imm_string(offset_,2,upper_operator,omit_leading_zero);
						if (mod==3) {
							dst=get_reg_map()[rm];
							if (!upper_operand) std::transform(dst.begin(),dst.end(),dst.begin(),::tolower);
						}
						add_length_sign(dst,bits,with_full_length);
						src=get_reg_map()[reg];
						if (!upper_operand) std::transform(src.begin(),src.end(),src.begin(),::tolower);
						return rm_no_reg_?dst:(reverse_rm_?(src+","+dst):(dst+","+src));
					}
					default: {
						BYTE mod=modrm_mod(value_);
						BYTE reg=modrm_reg(value_);
						BYTE rm=modrm_rm(value_);
						//if (bits_==MB_64) {
						rm+=rex_b(rex_)<<3;
						reg+=rex_r(rex_)<<3;
						//}
						std::string dst="[",src,dst_extra;
						dst+=std::string(address_bits_==MB_32?reg32_[rm]:reg64_[rm])+"]";
						if (!upper_operand) std::transform(dst.begin(),dst.end(),dst.begin(),::tolower);
						if (mod==0 && (rm&7)==5) {
							if (bits!=MB_64) dst=std::string("[")+std::string(get_imm_string(offset_,4,upper_operator,omit_leading_zero))+"]";
							else {
								dst=address_bits_==MB_64?"[RIP+":"[EIP+";
								if (!upper_operand) std::transform(dst.begin(),dst.end(),dst.begin(),::tolower);
								dst+=get_imm_string(offset_,4,upper_operator,omit_leading_zero)+"]";
							}
						}
						if (mod==1) dst_extra="+"+get_imm_string(offset_,1,upper_operator,omit_leading_zero);
						if (mod==2) dst_extra="+"+get_imm_string(offset_,4,upper_operator,omit_leading_zero);
						dst+=dst_extra;
						if (mod!=3 && (rm&7)==4) {
							//SIB
							std::string sib,sib_base,sib_index,sib_extra;
							BYTE scale=modrm_mod(sib_);
							BYTE index=modrm_reg(sib_);
							BYTE base=modrm_rm(sib_);
							//if (bits_==MB_64) {
							base+=rex_b(rex_)<<3;
							index+=rex_x(rex_)<<3;
							//}
							sib_base=base==5?"":get_reg_map()[base];
							sib_index=index==4?"":get_reg_map()[index];
							if (!upper_operand) {
								std::transform(sib_base.begin(),sib_base.end(),sib_base.begin(),::tolower);
								std::transform(sib_index.begin(),sib_index.end(),sib_index.begin(),::tolower);
							}
							if (scale!=0 && sib_index!="") sib_index+="*"+std::to_string(1<<scale);
							//sib_base+sib_index+dst_extra
							if (sib_base=="" && sib_index=="") dst="["+(dst_extra==""?get_imm_string(0,1,upper_operator,omit_leading_zero):dst_extra)+"]";
							else if (sib_base=="") dst="["+sib_index+(dst_extra==""?"":("+"+dst_extra))+"]";
							else if (sib_index=="") dst="["+sib_base+(dst_extra==""?"":("+"+dst_extra))+"]";
							else dst="["+sib_base+"+"+sib_index+(dst_extra==""?"":("+"+dst_extra))+"]";
						}
						if (mod==3) {
							dst=get_reg_map()[rm];
							if (!upper_operand) std::transform(dst.begin(),dst.end(),dst.begin(),::tolower);
						}
						add_length_sign(dst,bits,with_full_length);
						src=get_reg_map()[reg];
						if (!upper_operand) std::transform(src.begin(),src.end(),src.begin(),::tolower);
						return rm_no_reg_?dst:(reverse_rm_?(src+","+dst):(dst+","+src));
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
	machine_bits bits_;
	static inline std::map<code_type,std::string> code_name_={
#define _STDEX_INTEL_ASSEMBLER_CODE_TYPE(name) {CT_##name,#name},
#include "intel_assembler.inc"
#undef _STDEX_INTEL_ASSEMBLER_CODE_TYPE
	};
	std::string to_intel_string(bool upper_operand=true,bool upper_operator=true,bool omit_leading_zero=true,bool with_full_length=true) {
		std::string result=code_name_[code_];
		if (!upper_operand) std::transform(result.begin(),result.end(),result.begin(),::tolower);
		/*if (operands_.size()>0) {
			result+=" ";
			for (std::size_t i=0;i<operands_.size();i++) {
				result+=operands_[i].to_intel_string();
				if (i!=operands_.size()-1) result+=",";
			}
		}*/
		result+=" ";
		for (int i=0;i<operands_.size();i++) result+=operands_[i].to_intel_string(upper_operand,upper_operator,omit_leading_zero,bits_,offset_,with_full_length)+",";
		result.pop_back();
		return result;
	}
};

}

}

}

#endif