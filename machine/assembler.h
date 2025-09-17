//Last Modified At 2025/09/16
//@Version 1.0.0.0
#ifndef _STDEX_MACHINE_ASSEMBLER_H_
#define _STDEX_MACHINE_ASSEMBLER_H_ 1

#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "general.h"//At Least 1.0.0.1
#include "intel.h"//At Least 1.0.0.0
#include "../syntax/parser.h"//At Least 2.3

namespace stdex {

namespace assembler {

class assembler {
public:
	machine_bits machine_bits_;
	machine_bits program_bits_;
	bool is_little_endian_;
};

class intel_assembler : public assembler {
	using namespace intel;
public:
	std::vector<BYTE> bytes_;
	std::vector<instruction> instructions_;
	enum cpu_type {
		CT_MACHINE_X86_16,
		CT_MACHINE_X86_32_16,
		CT_MACHINE_X86_32,
		CT_MACHINE_X86_64,
	};
	cpu_type get_cpu_type() const {
		switch (machine_bits_) {
			case MB_16: return CT_MACHINE_X86_16;
			case MB_64: return (program_bits_==MB_16)?CT_MACHINE_X86_32_16:CT_MACHINE_X86_64;
			default: return (program_bits_==MB_16)?CT_MACHINE_X86_32_16:CT_MACHINE_X86_32;
		}
		return CT_MACHINE_X86_32;
	}

private:
	class intel_assmbler_listener : public syntax::parser_listener<token_type,line_type> {
		intel_assembler* assmbler_;
		instruction* instruction_;
		std::size_t current_id_;
		std::size_t current_length_;
		bool has_66_prefix_;
		bool has_67_prefix_;
		BYTE current_rex_;
		bool is_prefix_;
		void reset() {
			instruction_=nullptr;
			current_id_=0;
			current_length_=0;
			reset_prefix();
		}
		void reset_prefix() {
			has_66_prefix_=false;
			has_67_prefix_=false;
			current_rex_=0;
			is_prefix_=true;
		}
		std::vector<BYTE>& get_bytes() {
			if (!assembler_) return {};
			return assembler_->bytes_;
		}
		void new_instruction(code_type code) {
			if (!assembler) return;
			assembler_->instructions_.emplace_back();
			instructions_=assembler_->instructions_.back();
			instructions_->code_=code;
			instructions_->offset_=current_id_-current_length_;
		}
		void try_set_id(int& id,int length,std::string message) {
			if (id<0) id=current_id_+id+1;
			if (id+length>get_bytes().size()) throw std::out_of_range(message);
		}
		bool check_sib() {
			//same as get_address_bits()>MB_16;
			if (assembler_->get_cpu_type()>=CT_MACHINE_X86_64) return true;
			if (assembler_->get_cpu_type()==CT_MACHINE_X86_16) return false;
			return (assembler_->get_cpu_type()==CT_MACHINE_X86_32)^has_67_prefix_;
		}
		bool check_instruction(bool with_size = true) {
			return assembler_ && instruction_ && with_size ? (instruction_->operands_.size()) : true;
		}
		machine_bit get_operand_bits() {
			if (!assembler_) return MB_32;
			cpu_type type=assembler_->get_cpu_type();
			if (type==CT_MACHINE_X86_64 && rex_w(current_rex_,true)) return MB_64;
			if (type==CT_MACHINE_X86_16 || (type==CT_MACHINE_X86_32 && has_66_prefix_) || (type==CT_MACHINE_X86_32_16 && !has_66_prefix_)) return MB_16;
			return MB_32;
		}
		machine_bit get_address_bits() {
			if (!assembler_) return MB_32;
			cpu_type type=assembler_->get_cpu_type();
			if (type==CT_MACHINE_X86_16) return MB_16;
			if (type==CT_MACHINE_X86_64) return has_67_prefix_?MB_32:MB_64;
			return ((type==CT_MACHINE_X86_32)^has_67_prefix_)?MB_32:MB_16;
		}
		BYTE read_byte(int id) {
			return get_bytes()[id];
		}
		WORD read_word(int id) {
			return read_byte(id)+(get_bytes()[id+1]<<8);
		}
		DWORD read_dword(int id) {
			return read_word(id)+(get_bytes()[id+2]<<16)+(get_bytes()[id+3]<<24);
		}
		void set_operand_info(operand& op) {
			op.address_bits_=get_address_bits();
			op.rex_=current_rex_;
		}
		int read_imm8(int id=-1) {
			try_set_id(id,1,"Read imm8 out of range");
			int length=0;
			if (assembler_ && instructions_) {
				operand temp;
				temp.type_=OT_IMMEDIATE;
				temp.bits_=MB_8;
				temp.value_=read_byte(id);
				set_operand_info(temp);
				length=temp.length_=1;
				instructions_->operands_.push_back(temp);
			}
			return id+length-current_id_;
		}
		int read_imm16(int id=-1) {
			try_set_id(id,2,"Read imm16 out of range");
			int length=0;
			if (assembler_ && instructions_) {
				operand temp;
				temp.type_=OT_IMMEDIATE;
				temp.bits_=MB_16;
				temp.value_=read_word(id);
				set_operand_info(temp);
				length=temp.length_=2;
				instructions_->operands_.push_back(temp);
			}
			return id+length-current_id_;
		}
		int read_imm32(int id=-1) {
			try_set_id(id,4,"Read imm32 out of range");
			int length=0;
			if (assembler_ && instructions_) {
				operand temp;
				temp.type_=OT_IMMEDIATE;
				temp.bits_=MB_32;
				temp.value_=read_dword(id);
				set_operand_info(temp);
				length=temp.length_=4;
				instructions_->operands_.push_back(temp);
			}
			return id+length-current_id_;
		}
		int read_imm64(int id=-1) {
			//read_imm32 & signed extend
			try_set_id(id,4,"Read imm64 out of range");
			//if (check_instruction()) instruction_->operands_[instruction_->operands_.size()-1].rex_=current_rex_;
			return read_imm32(id);
		}
		int read_imm(int id=-1) {
			machine_bits bits=get_operand_bits();
			if (bits==MB_8) return read_imm8(id);
			else if (bits==MB_16) return read_imm16(id);
			else if (bits==MB_32) return read_imm32(id);
			return read_imm64(id);
		}
		int read_extra32(int id,operand& modrm,bool sib=false) {
			if (modrm_mod(modrm.value_)==0) {
				if (modrm_reg(modrm.value_)==5) {
					try_set_id(id,4,"Read rm/"+sib?"sib/":""+"disp32 out of range");
					modrm.offset_=read_dword(id);
					return 4;
				}
				return 0;
			} else if (modrm_mod(modrm.value_)==1) {
				try_set_id(id,1,"Read rm/"+sib?"sib/":""+"disp8 out of range");
				modrm.offset_+=read_byte(id);
				return 1;
			} else if (modrm_mod(modrm.value_)==2) {
				try_set_id(id,4,"Read rm/"+sib?"sib/":""+"disp32 out of range");
				modrm.offset_+=read_dword(id);
				return 4;
			}
			return 0;
		}
		int read_extra32_sib(int id,operand& modrm) {
			BYTE mod=modrm_mod(modrm.value_);
			if (mod!=0) return read_extra32(id,modrm,true);
			BYTE sib=modrm.sib_;
			if (modrm_reg(sib)==5) {
				try_set_id(id,4,"Read rm/sib/disp32 out of range");
				modrm.offset_=read_dword(id);
				return 4;
			}
			return 0;
		}
		int read_extra16(int id,operand& modrm) {
			if (modrm_mod(modrm.value_)==0) {
				if (modrm_rm(modrm.value_)==6) {
					try_set_id(id,2,"Read rm/disp16 out of range");
					modrm.offset_=read_word(id);
					return 2;
				}
				return 0;
			} else if (modrm_mod(modrm.value_)==1) {
				try_set_id(id,1,"Read rm/disp8 out of range");
				modrm.offset_=read_byte(id);
				return 1;
			} else if (modrm_mod(modrm.value_)==2) {
				try_set_id(id,2,"Read rm/disp16 out of range");
				modrm.offset_=read_word(id);
				return 2;
			}
			return 0;
		}
		int read_rm(int id=-1) {
			try_set_id(id,1,"Read rm out of range");
			int length=0;
			if (assembler_ && instruction_) {
				operand temp;
				temp.type_=OT_MODRM;
				temp.bits_=get_operand_bits();
				temp.value_=read_byte(id);
				int extra_offset=0;
				BYTE mod=modrm_mod(temp.value_);
				if (!check_sib()) extra_offset=read_extra16(id+1,temp);
				else {
					if (mod!=3 && modrm_rm(temp.value_)==4) {
						try_set_id(id+1,1,"Read rm/sib out of range");
						temp.sib_=read_byte(id+1);
						extra_offset=1+read_extra32_sib(id+2,temp);
					} else extra_offset=read_extra32(id+1,temp);
				}
				set_operand_info(temp);
				length=temp.length_=1+extra_offset;
				instructions_->operands_.push_back(temp);
			}
			return id+length-current_id_;
		}
		int read_rm_reverse(int id=-1) {
			int result=read_rm(id);
			if (assembler_ && instruction_ && instruction_->operands_.size()) instruction_->operands_[instruction_->operands_.size()-1].reverse_rm_=true;
			return result;
		}
		int new_single_instruction(code_type type) {
			new_instruction(type);
			return 0;
		}
		int read_imm8_skip_0xA(code_type type,int id=-2) {
			new_instruction(type);
			int skip=read_imm8(id);
			if (check_instruction() && instruction_->operands_[0].value_ == 0x0A) instruction_->operands_.clear();
		}
		int on_shift(int id,int state,token_type word) override {
			if (is_prefix_) {
				if (word==(token_type)0x66) has_66_prefix_=true;
				else if (word==(token_type)0x67) has_67_prefix_=true;
				else is_prefix_=false;
			}
			return 0;
		}
		int on_reduction(int id,int state,int next,line_type sentence_id,int reduction_num) override {
			//map<LT_XXX_SENTENCE,CT_XXX>
			current_id_=id;
			current_length_=reduction_num;
			switch (sentence_id) {
				case LT_SUB66_SENTENCE:
				case LT_CLR66_SENTENCE: {
					reset_prefix();
					break;
				}
				case LT_AAA_SENTENCE: return new_single_instruction(CT_AAA);
				case LT_AAD_SENTENCE: return read_imm8_skip_0xA(CT_AAD);
				case LT_AAM_SENTENCE: return read_imm8_skip_0xA(CT_AAM);
				case LT_AAS_SENTENCE: return new_single_instruction(CT_AAS);
				case LT_ADC_14_SENTENCE: {
					new_instruction(CT_ADC);
					if (check_instruction(false)) {
						operand temp;
						temp.type_=OT_REGISTER;
						temp.value_=0;
						temp.bits_=MB_8;
						instructions_->operands_.push_back(temp);
					}
					return read_imm8(-2);
				}
				case LT_ADC_15_SENTENCE: 
				case LT_ADC_15_64_SENTENCE: {
					new_instruction(CT_ADC);
					if (check_instruction(false)) {
						operand temp;
						temp.type_=OT_REGISTER;
						temp.value_=0;
						temp.bits_=get_operand_bits();
						if (sentence_id==LT_ADC_15_SENTENCE && temp.bits_==MB_64) temp.bits_=MB_32;
						if (sencence_id==LT_ADC_15_64_SENTENCE) temp.rex_=current_rex_;
						instructions_->operands_.push_back(temp);
					}
					return read_imm(-2);
				}
			}
			return 0;
		}
		void on_accept() override { }
		bool on_error(error_type type,int state,_Tp word) override { }
	};
#define _STDEX_ASSEMBLER_PARSER_UNIT syntax::single_parser_unit<token_type,line_type>
	static syntax::parser<token_type,line_type> code_parser_={
		TT_START,
		TT_SEPERATOR,
		TT_EPSILON,
		TT_EOF,
		{
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_START,{TT_SENTENCE,TT_EOF},LT_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_SENTENCE,{TT_66_SENTENCE},LT_SUB66_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_SENTENCE,{(token_type)0x66,TT_66_SENTENCE},LT_CLR66_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_SENTENCE,{(token_type)0x67,TT_66_SENTENCE},LT_CLR66_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_SENTENCE,{(token_type)0x67,(token_type)0x66,TT_66_SENTENCE},LT_CLR66_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_AAA_SENTENCE},LT_SPECIALIZE_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_AAA_SENTENCE,{(token_type)0x37},LT_AAA_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_AAD_SENTENCE},LT_SPECIALIZE_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_AAD_SENTENCE,{(token_type)0xD5,TT_BITS},LT_AAD_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_AAM_SENTENCE},LT_SPECIALIZE_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_AAM_SENTENCE,{(token_type)0xD4,TT_BITS},LT_AAM_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_AAS_SENTENCE},LT_SPECIALIZE_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_AAS_SENTENCE,{(token_type)0x3F},LT_AAS_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_ADC_SENTENCE},LT_SPECIALIZE_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_ADC_66_SENTENCE},LT_SPECIALIZE_66_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_ADC_SENTENCE,{(token_type)0x14,TT_BITS},LT_ADC14_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_ADC_66_SENTENCE,{(token_type)0x15,TT_BITS},LT_ADC15_SENTENCE),//2 or 4

			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_80_SENTENCE},LT_SUB80_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_81_SENTENCE},LT_SUB81_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_80_SENTENCE},LT_SUB83_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_80_SENTENCE,{(token_type)0x80,TT_BITS,TT_BITS},LT_80_SENTENCE),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_81_SENTENCE,{(token_type)0x81,TT_BITS,TT_BITS},LT_81_SENTENCE),//2 or 4
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_83_SENTENCE,{(token_type)0x83,TT_BITS,TT_BITS},LT_83_SENTENCE),

			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{TT_REX},LT_REX_TO_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x00},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x01},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x02},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x03},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x04},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x05},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x06},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x07},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x08},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x09},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x0A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x0B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x0C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x0D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x0E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x0F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x10},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x11},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x12},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x13},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x14},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x15},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x16},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x17},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x18},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x19},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x1A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x1B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x1C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x1D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x1E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x1F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x20},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x21},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x22},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x23},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x24},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x25},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x26},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x27},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x28},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x29},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x2A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x2B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x2C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x2D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x2E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x2F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x30},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x31},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x32},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x33},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x34},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x35},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x36},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x37},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x38},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x39},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x3A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x3B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x3C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x3D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x3E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x3F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x40},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x41},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x42},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x43},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x44},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x45},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x46},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x47},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x48},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x49},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x4A},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x4B},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x4C},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x4D},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x4E},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_REX,{(token_type)0x4F},LT_SPECIALIZE_REX),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x50},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x51},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x52},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x53},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x54},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x55},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x56},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x57},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x58},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x59},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x5A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x5B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x5C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x5D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x5E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x5F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x60},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x61},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x62},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x63},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x64},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x65},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x66},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x67},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x68},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x69},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x6A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x6B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x6C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x6D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x6E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x6F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x70},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x71},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x72},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x73},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x74},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x75},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x76},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x77},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x78},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x79},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x7A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x7B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x7C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x7D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x7E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x7F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x80},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x81},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x82},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x83},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x84},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x85},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x86},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x87},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x88},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x89},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x8A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x8B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x8C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x8D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x8E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x8F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x90},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x91},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x92},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x93},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x94},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x95},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x96},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x97},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x98},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x99},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x9A},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x9B},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x9C},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x9D},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x9E},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0x9F},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA0},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA1},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA2},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA3},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA4},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA5},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA6},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA7},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA8},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xA9},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xAA},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xAB},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xAC},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xAD},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xAE},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xAF},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB0},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB1},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB2},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB3},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB4},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB5},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB6},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB7},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB8},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xB9},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xBA},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xBB},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xBC},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xBD},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xBE},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xBF},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC0},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC1},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC2},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC3},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC4},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC5},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC6},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC7},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC8},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xC9},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xCA},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xCB},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xCC},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xCD},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xCE},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xCF},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD0},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD1},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD2},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD3},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD4},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD5},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD6},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD7},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD8},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xD9},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xDA},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xDB},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xDC},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xDD},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xDE},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xDF},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE0},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE1},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE2},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE3},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE4},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE5},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE6},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE7},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE8},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xE9},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xEA},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xEB},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xEC},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xED},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xEE},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xEF},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF0},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF1},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF2},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF3},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF4},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF5},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF6},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF7},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF8},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xF9},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xFA},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xFB},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xFC},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xFD},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xFE},LT_SPECIALIZE_BITS),
			_STDEX_ASSEMBLER_PARSER_UNIT(TT_BITS,{(token_type)0xFF},LT_SPECIALIZE_BITS),

			//32 SPECIAL
			_STDEX_ASSMBLER_PARSER_UNIT(TT_66_SENTENCE,{TT_REX},LT_SPECIALIZE_REX_TO_SENTENCE_32),
		}
	};
	static syntax::parser<token_type,line_type> code_parser64_={
		TT_START,
		TT_SEPERATOR,
		TT_EPSILON,
		TT_EOF,
		{}
	};
	static intel_assmbler_listener listener_;
	static syntax::parser<token_type,line_type>& get_parser() {
		static std::once_flag parser_flag;
		std::call_once(parser_flag,[&](){
			code_parser_.generate_parser();
			code_parser_.listeners_.insert(&listsner_);
			listener_.enabled_=true;
		});
		return code_parser_;
	}
	static syntax::parser<token_type,line_type>& get_parser64() {
		static std::once_flag parser_flag64;
		std::call_once(parser_flag64,[&](){
			auto parser32=get_parser();
			for (auto& it:parser32.units_) code_parser64.units_.push_back(it);
			static std::vector<syntax::parser_unit<token_type,line_type>> units64={
				_STDEX_ASSEMBLER_PARSER_UNIT(TT_ADC_66_SENTENCE,{TT_REX,(token_type)0x15,TT_BITS},LT_ADC15_64_SENTENCE),
				_STDEX_ASSEMBLER_PARSER_UNIT(TT_80_SENTENCE,{TT_REX,(token_type)0x80,TT_BITS,TT_BITS},LT_80_64_SENTENCE),
				_STDEX_ASSEMBLER_PARSER_UNIT(TT_81_SENTENCE,{TT_REX,(token_type)0x81,TT_BITS,TT_BITS},LT_81_64_SENTENCE),
				_STDEX_ASSEMBLER_PARSER_UNIT(TT_83_SENTENCE,{TT_REX,(token_type)0x80,TT_BITS,TT_BITS},LT_80_64_SENTENCE),
			};
			for (std::vector<syntax::parser_unit<token_type,line_type>>::iterator it=code_parser64.units_.begin();it!=code_parser64.units_.end();) {
				if (it->id==LT_SPECIALIZE_REX_TO_SENTENCE_32) it=code_parser64.units_.erase(it);
				else it++;
			}
			code_parser64_.units_.insert(code_parser64.units_.end(),units64.begin(),units64.end());
			code_parser64_.generate_parser();
			code_parser64_.listeners_.insert(&listsner_);
			listener_.enabled_=true;
		});
		return code_parser64_;
	};

public:
	std::vector<instruction> parse() {
		switch (machine_bits_) {
			case MB_32: {
				instructions_.clear();
				listener_.reset();
				code_parser_.parse_with_listener(bytes_);
				return instructions_;
			}
			case MB_64: {
				instructions_.clear();
				listener_.reset();
				code_parser64_.parse_with_listener(bytes_);
				return instructions_;
			}
		}
		return {};
	}
#undef _STDEX_ASSEMBLER_PARSER_UNIT
};

}

}

#endif