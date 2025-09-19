//Last Modified At 2025/09/19
//@Version 1.0.0.0
#ifndef _STDEX_MACHINE_ASSEMBLER_H_
#define _STDEX_MACHINE_ASSEMBLER_H_ 1

//http://shell-storm.org/online/Online-Assembler-and-Disassembler/?inst=mov+eax%2C1&arch=x86-64&as_format=hex#assembly

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "general.h"//At Least 1.0.0.1
#include "intel.h"//At Least 1.0.0.0
#include "../syntax/parser.h"//At Least 2.4

namespace stdex {

namespace machine {

namespace assembler {

class assembler {
public:
	machine_bits machine_bits_;
	machine_bits program_bits_;
	bool is_little_endian_;
};

class intel_assembler : public assembler {
#define _STDEX_ASSEMBLER_PARSER_UNIT syntax::single_parser_unit<TT,LT>
	using TT=intel::token_type;
	using LT=intel::line_type;
	using CT=intel::code_type;
	static inline std::vector<CT> digits_={CT::CT_ADD,CT::CT_OR,CT::CT_ADC,CT::CT_SBB,CT::CT_AND,CT::CT_SUB,CT::CT_XOR,CT::CT_CMP};
public:
	std::vector<BYTE> bytes_;
	std::vector<intel::instruction> instructions_;
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
	class intel_assmbler_listener : public syntax::parser_listener<TT,LT> {
		intel_assembler* assembler_;
		intel::instruction* instruction_;
		std::size_t current_id_;
		std::size_t current_length_;
		bool has_66_prefix_;
		bool has_67_prefix_;
		BYTE current_rex_;
		bool is_prefix_;
		int code_amount_;
	public:
		void reset(intel_assembler* assembler=nullptr) {
			assembler_=assembler;
			instruction_=nullptr;
			current_id_=0;
			current_length_=0;
			code_amount_=0;
			reset_prefix();
		}
	private:
		void reset_prefix() {
			has_66_prefix_=false;
			has_67_prefix_=false;
			current_rex_=0;
			is_prefix_=true;
		}
		std::vector<BYTE>& get_bytes() {
			if (!assembler_) throw std::runtime_error("Assembler is invalid while getting bytes");
			return assembler_->bytes_;
		}
		void new_instruction(intel::code_type code) {
			if (!assembler_) return;
			assembler_->instructions_.emplace_back();
			instruction_=&assembler_->instructions_.back();
			instruction_->code_=code;
			instruction_->offset_=current_id_-current_length_-1-has_66_prefix_-has_67_prefix_;
		}
		void try_set_id(int& id,int length,std::string message) {
			if (id<0) id=current_id_+id+1;
			if (id<0 || id+length>get_bytes().size()) throw std::out_of_range(message+std::string(" at id ")+std::to_string(id));
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
		machine_bits get_operand_bits_w() {
			if (!assembler_) return MB_32;
			cpu_type type=assembler_->get_cpu_type();
			if (type==CT_MACHINE_X86_64) {
				if (intel::rex_w(current_rex_,true)) return MB_64;
				if (has_66_prefix_) return MB_16;
			}
			if (type==CT_MACHINE_X86_16 || (type==CT_MACHINE_X86_32 && has_66_prefix_) || (type==CT_MACHINE_X86_32_16 && !has_66_prefix_)) return MB_16;
			return MB_32;
		}
		machine_bits get_operand_bits_wbrx() {
			if (!assembler_) return MB_32;
			cpu_type type=assembler_->get_cpu_type();
			if (type==CT_MACHINE_X86_64) {
				if (intel::rex_all(current_rex_)) return MB_64;
				if (has_66_prefix_) return MB_16;
			}
			if (type==CT_MACHINE_X86_16 || (type==CT_MACHINE_X86_32 && has_66_prefix_) || (type==CT_MACHINE_X86_32_16 && !has_66_prefix_)) return MB_16;
			return MB_32;
		}
		machine_bits get_address_bits() {
			if (!assembler_) return MB_32;
			cpu_type type=assembler_->get_cpu_type();
			if (type==CT_MACHINE_X86_16) return MB_16;
			if (type==CT_MACHINE_X86_64) return has_67_prefix_?MB_32:MB_64;
			return ((type==CT_MACHINE_X86_32)^has_67_prefix_)?MB_32:MB_16;
		}
		BYTE read_byte(std::size_t id) {
			return get_bytes()[id];
		}
		WORD read_word(std::size_t id) {
			return read_byte(id)+(get_bytes()[id+1]<<8);
		}
		DWORD read_dword(std::size_t id) {
			return read_word(id)+(get_bytes()[id+2]<<16)+(get_bytes()[id+3]<<24);
		}
		void set_operand_info(intel::operand& op) {
			op.address_bits_=get_address_bits();
			op.rex_=current_rex_;
		}
		intptr_t read_imm8(intptr_t id=-1) {
			try_set_id(id,1,"Read imm8 out of range");
			int length=0;
			if (assembler_ && instruction_) {
				intel::operand temp;
				temp.type_=intel::OT_IMMEDIATE;
				temp.bits_=MB_8;
				temp.value_=read_byte(id);
				set_operand_info(temp);
				length=temp.length_=1;
				instruction_->operands_.push_back(temp);
			}
			return id+length+1-current_id_;
		}
		intptr_t read_imm16(intptr_t id=-1) {
			try_set_id(id,2,"Read imm16 out of range");
			int length=0;
			if (assembler_ && instruction_) {
				intel::operand temp;
				temp.type_=intel::OT_IMMEDIATE;
				temp.bits_=MB_16;
				temp.value_=read_word(id);
				set_operand_info(temp);
				length=temp.length_=2;
				instruction_->operands_.push_back(temp);
			}
			return id+length+1-current_id_;
		}
		intptr_t read_imm32(intptr_t id=-1) {
			try_set_id(id,4,"Read imm32 out of range");
			int length=0;
			if (assembler_ && instruction_) {
				intel::operand temp;
				temp.type_=intel::OT_IMMEDIATE;
				temp.bits_=MB_32;
				temp.value_=read_dword(id);
				set_operand_info(temp);
				length=temp.length_=4;
				instruction_->operands_.push_back(temp);
			}
			return id+length+1-current_id_;
		}
		intptr_t read_imm64(intptr_t id=-1) {
			//read_imm32 & signed extend
			try_set_id(id,4,"Read imm64 out of range");
			//if (check_instruction()) instruction_->operands_[instruction_->operands_.size()-1].rex_=current_rex_;
			return read_imm32(id);
		}
		intptr_t read_imm_w(intptr_t id=-1) {
			machine_bits bits=get_operand_bits_w();
			if (bits==MB_8) return read_imm8(id);
			else if (bits==MB_16) return read_imm16(id);
			else if (bits==MB_32) return read_imm32(id);
			return read_imm64(id);
		}
		int read_extra32(intptr_t id,intel::operand& modrm,bool sib=false) {
			if (intel::modrm_mod(modrm.value_)==0) {
				if (intel::modrm_rm(modrm.value_)==5) {
					try_set_id(id,4,std::string("Read rm/")+(sib?std::string("sib/"):std::string(""))+std::string("disp32 out of range"));
					modrm.offset_=read_dword(id);
					return 4;
				}
				return 0;
			} else if (intel::modrm_mod(modrm.value_)==1) {
				try_set_id(id,1,std::string("Read rm/")+(sib?std::string("sib/"):std::string(""))+std::string("disp8 out of range"));
				modrm.offset_+=read_byte(id);
				return 1;
			} else if (intel::modrm_mod(modrm.value_)==2) {
				try_set_id(id,4,std::string("Read rm/")+(sib?std::string("sib/"):std::string(""))+std::string("disp32 out of range"));
				modrm.offset_+=read_dword(id);
				return 4;
			}
			return 0;
		}
		int read_extra32_sib(intptr_t id,intel::operand& modrm) {
			BYTE mod=intel::modrm_mod(modrm.value_);
			if (mod!=0) return read_extra32(id,modrm,true);
			BYTE sib=modrm.sib_;
			if (intel::modrm_reg(sib)==5) {
				try_set_id(id,4,"Read rm/sib/disp32 out of range");
				modrm.offset_=read_dword(id);
				return 4;
			}
			return 0;
		}
		int read_extra16(intptr_t id,intel::operand& modrm) {
			if (intel::modrm_mod(modrm.value_)==0) {
				if (intel::modrm_rm(modrm.value_)==6) {
					try_set_id(id,2,"Read rm/disp16 out of range");
					modrm.offset_=read_word(id);
					return 2;
				}
				return 0;
			} else if (intel::modrm_mod(modrm.value_)==1) {
				try_set_id(id,1,"Read rm/disp8 out of range");
				modrm.offset_=read_byte(id);
				return 1;
			} else if (intel::modrm_mod(modrm.value_)==2) {
				try_set_id(id,2,"Read rm/disp16 out of range");
				modrm.offset_=read_word(id);
				return 2;
			}
			return 0;
		}
		intptr_t read_rm_w(intel::operand& op,intptr_t id=-1) {
			try_set_id(id,1,"Read rm out of range");
			int length=0;
			if (assembler_ && instruction_) {
				op.type_=intel::OT_MODRM;
				op.bits_=get_operand_bits_w();
				op.address_bits_=get_address_bits();
				op.value_=read_byte(id);
				int extra_offset=0;
				BYTE mod=intel::modrm_mod(op.value_);
				if (!check_sib()) extra_offset=read_extra16(id+1,op);
				else {
					if (mod!=3 && intel::modrm_rm(op.value_)==4) {
						try_set_id(id,2,"Read rm/sib out of range");
						op.sib_=read_byte(id+1);
						extra_offset=1+read_extra32_sib(id+2,op);
					} else extra_offset=read_extra32(id+1,op);
				}
				set_operand_info(op);
				length=op.length_=1+extra_offset;
				instruction_->operands_.push_back(op);
			}
			return id+length+1-current_id_;
		}
		intptr_t read_rm_bits(intel::operand& op,intptr_t id=-1,machine_bits bits=MB_32) {
			intptr_t skip=read_rm_w(op,id);
			op.bits_=bits;
			return skip;
		}
		intptr_t read_rm(intel::operand& op,intptr_t id=-1) {
			intptr_t skip=read_rm_w(op,id);
			op.bits_=get_operand_bits_wbrx();
			return skip;
		}
		int new_single_instruction(intel::code_type type) {
			new_instruction(type);
			return 0;
		}
		intptr_t read_imm8_skips(intel::code_type type,std::vector<QWORD> skips={0x0A},intptr_t id=-3) {
			new_instruction(type);
			intptr_t skip=read_imm8(id);
			if (check_instruction() && std::find(skips.begin(),skips.end(),instruction_->operands_[0].value_)!=skips.end()) instruction_->operands_.clear();
			return skip;
		}
		intptr_t reduction_skip(intptr_t skip) {
			current_id_+=skip;
			return skip;
		}
		intptr_t on_shift(uintptr_t id,int state,TT word) override {
			code_amount_++;
			current_id_++;
			if (is_prefix_) {
				if (word==(TT)0x66) has_66_prefix_=true;
				else if (word==(TT)0x67) has_67_prefix_=true;
				else is_prefix_=false;
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,LT sentence_id,int reduction_num) override {
			intel::instruction* old_instruction=instruction_;
			code_amount_-=reduction_num-1;
			//map<LT_XXX_SENTENCE,CT_XXX>
			current_id_=id;
			current_length_=reduction_num;
			try {
				switch (sentence_id) {
					case LT::LT_SUB66_SENTENCE:
					case LT::LT_CLR66_SENTENCE: {
						reset_prefix();
						break;
					}
					case LT::LT_AAA_SENTENCE: return new_single_instruction(CT::CT_AAA);
					case LT::LT_AAD_SENTENCE: return read_imm8_skips(CT::CT_AAD,{0x0A});
					case LT::LT_AAM_SENTENCE: return read_imm8_skips(CT::CT_AAM,{0x0A});
					case LT::LT_AAS_SENTENCE: return new_single_instruction(CT::CT_AAS);
					case LT::LT_ADC14_SENTENCE: {
						new_instruction(CT::CT_ADC);
						if (check_instruction(false)) {
							intel::operand temp;
							temp.type_=intel::OT_REGISTER;
							temp.value_=0;
							temp.bits_=MB_8;
							instruction_->operands_.push_back(temp);
						}
						return reduction_skip(read_imm8(-3));
					}
					case LT::LT_ADC15_SENTENCE: 
					case LT::LT_ADC15_64_SENTENCE: {
						new_instruction(CT::CT_ADC);
						if (check_instruction(false)) {
							intel::operand temp;
							temp.type_=intel::OT_REGISTER;
							temp.value_=0;
							temp.bits_=get_operand_bits_w();
							//if (sentence_id==LT::LT_ADC15_SENTENCE && temp.bits_==MB_64) temp.bits_=MB_32;
							if (sentence_id==LT::LT_ADC15_64_SENTENCE) temp.rex_=current_rex_&0xF8;//clear REX.RBX
							//if (has_66_prefix_) temp.bits_=MB_16;
							instruction_->operands_.push_back(temp);
						}
						return reduction_skip(read_imm_w(-3));
					}
					case LT::LT_80_SENTENCE:
					case LT::LT_80_64_SENTENCE: {
						BYTE value=read_byte(current_id_-3);
						BYTE digit=intel::modrm_reg(value);
						int rm_skip=0;
						new_instruction(digits_[digit]);
						if (check_instruction(false)) {
							intel::operand temp;
							rm_skip=read_rm_bits(temp,-4,MB_8)+1;
							temp.rm_no_reg_=true;
							if (sentence_id==LT::LT_80_64_SENTENCE) temp.rex_=current_rex_;
							instruction_->operands_[instruction_->operands_.size()-1]=temp;
						}
						return reduction_skip(read_imm8(current_id_+rm_skip-2));
					}
					case LT::LT_81_SENTENCE:
					case LT::LT_81_64_SENTENCE:
					case LT::LT_83_SENTENCE:
					case LT::LT_83_64_SENTENCE: {
						
						break;
					}
					case LT::LT_SPECIALIZE_REX: {
						if (assembler_ && assembler_->get_cpu_type()==CT_MACHINE_X86_64) current_rex_=read_byte(current_id_-2);
						break;
					}
					case LT::LT_SPECIALIZE_REX_TO_SENTENCE_32: {
						BYTE value=read_byte(current_id_-2);
						new_instruction((value&8)?CT::CT_DEC:CT::CT_INC);
						if (check_instruction(false)) {
							intel::operand temp;
							temp.type_=intel::OT_REGISTER;
							temp.value_=read_byte(current_id_-2)&0x7;
							temp.bits_=get_operand_bits_w();
							instruction_->operands_.push_back(temp);
						}
						break;
					}
				}
			} catch (const std::exception& e) {
				if (assembler_) {
					if (old_instruction!=instruction_) {
						bool erase=false;
						/*for (auto it=assembler_->instructions_.begin();it!=assembler_->instructions_.end();) {
							if (old_instruction==&*it) {
								erase=true;
								it++;
							} else if (erase) it=assembler_->instructions_.erase(it);
							else it++;
						}*/
						assembler_->instructions_.pop_back();
					}
					intptr_t final_index=assembler_->bytes_.size();
					intptr_t current_index=current_id_-1-reduction_num;
					for (intptr_t i=current_index;i<final_index;i++) {
						new_instruction(CT::CT_ERROR);
						if (check_instruction(false)) instruction_->offset_=i;
					}
					code_amount_=0;
					return reduction_skip(final_index-current_id_);
				}
				throw; 
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(syntax::parser_listener<TT,LT>::error_type type,int state,TT word) override {
			current_length_=1;
			if (word==TT::TT_EOF) {
				if (assembler_) {
					uintptr_t start=std::min(0,(intptr_t)(assembler_->bytes_.size()-code_amount_));
					for (int i=0;i<code_amount_;i++) {
						new_instruction(CT::CT_ERROR);
						if (check_instruction(false)) instruction_->offset_=i+start;
					}
				}
				return 0;
			}
			if (code_amount_>0) return 3;
			new_instruction(CT::CT_ERROR);
			if (check_instruction(false)) instruction_->offset_+=2;
			return 1;
		}
	};
	static inline std::vector<syntax::parser_unit<TT,LT>> units={
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_START,{TT::TT_SENTENCES,TT::TT_EOF},LT::LT_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_SENTENCES,{TT::TT_SENTENCES,TT::TT_SENTENCE},LT::LT_SENTENCES),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_SENTENCES,{TT::TT_SENTENCE},LT::LT_SENTENCES),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_SENTENCE,{TT::TT_66_SENTENCE},LT::LT_SUB66_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_SENTENCE,{(TT)0x66,TT::TT_66_SENTENCE},LT::LT_CLR66_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_SENTENCE,{(TT)0x67,TT::TT_66_SENTENCE},LT::LT_CLR66_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_SENTENCE,{(TT)0x67,(TT)0x66,TT::TT_66_SENTENCE},LT::LT_CLR66_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_AAA_SENTENCE},LT::LT_SPECIALIZE_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_AAA_SENTENCE,{(TT)0x37},LT::LT_AAA_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_AAD_SENTENCE},LT::LT_SPECIALIZE_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_AAD_SENTENCE,{(TT)0xD5,TT::TT_BITS},LT::LT_AAD_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_AAM_SENTENCE},LT::LT_SPECIALIZE_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_AAM_SENTENCE,{(TT)0xD4,TT::TT_BITS},LT::LT_AAM_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_AAS_SENTENCE},LT::LT_SPECIALIZE_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_AAS_SENTENCE,{(TT)0x3F},LT::LT_AAS_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_ADC_SENTENCE},LT::LT_SPECIALIZE_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_ADC_SENTENCE,{(TT)0x14,TT::TT_BITS},LT::LT_ADC14_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_ADC_SENTENCE,{(TT)0x15,TT::TT_BITS},LT::LT_ADC15_SENTENCE),//2 or 4
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_80_SENTENCE},LT::LT_SUB80_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_81_SENTENCE},LT::LT_SUB81_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_83_SENTENCE},LT::LT_SUB83_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_80_SENTENCE,{(TT)0x80,TT::TT_BITS,TT::TT_BITS},LT::LT_80_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_81_SENTENCE,{(TT)0x81,TT::TT_BITS,TT::TT_BITS},LT::LT_81_SENTENCE),//2 or 4
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_83_SENTENCE,{(TT)0x83,TT::TT_BITS,TT::TT_BITS},LT::LT_83_SENTENCE),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{TT::TT_REX},LT::LT_REX_TO_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x00},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x01},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x02},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x03},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x04},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x05},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x06},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x07},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x08},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x09},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x0A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x0B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x0C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x0D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x0E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x0F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x10},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x11},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x12},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x13},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x14},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x15},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x16},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x17},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x18},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x19},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x1A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x1B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x1C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x1D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x1E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x1F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x20},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x21},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x22},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x23},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x24},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x25},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x26},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x27},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x28},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x29},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x2A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x2B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x2C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x2D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x2E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x2F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x30},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x31},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x32},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x33},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x34},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x35},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x36},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x37},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x38},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x39},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x3A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x3B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x3C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x3D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x3E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x3F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x40},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x41},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x42},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x43},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x44},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x45},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x46},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x47},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x48},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x49},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x4A},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x4B},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x4C},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x4D},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x4E},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_REX,{(TT)0x4F},LT::LT_SPECIALIZE_REX),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x50},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x51},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x52},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x53},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x54},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x55},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x56},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x57},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x58},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x59},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x5A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x5B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x5C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x5D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x5E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x5F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x60},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x61},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x62},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x63},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x64},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x65},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x66},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x67},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x68},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x69},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x6A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x6B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x6C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x6D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x6E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x6F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x70},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x71},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x72},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x73},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x74},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x75},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x76},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x77},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x78},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x79},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x7A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x7B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x7C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x7D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x7E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x7F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x80},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x81},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x82},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x83},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x84},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x85},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x86},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x87},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x88},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x89},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x8A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x8B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x8C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x8D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x8E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x8F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x90},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x91},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x92},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x93},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x94},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x95},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x96},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x97},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x98},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x99},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x9A},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x9B},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x9C},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x9D},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x9E},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0x9F},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA0},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA1},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA2},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA3},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA4},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA5},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA6},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA7},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA8},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xA9},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xAA},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xAB},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xAC},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xAD},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xAE},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xAF},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB0},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB1},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB2},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB3},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB4},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB5},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB6},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB7},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB8},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xB9},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xBA},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xBB},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xBC},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xBD},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xBE},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xBF},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC0},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC1},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC2},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC3},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC4},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC5},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC6},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC7},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC8},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xC9},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xCA},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xCB},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xCC},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xCD},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xCE},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xCF},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD0},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD1},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD2},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD3},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD4},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD5},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD6},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD7},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD8},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xD9},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xDA},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xDB},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xDC},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xDD},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xDE},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xDF},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE0},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE1},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE2},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE3},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE4},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE5},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE6},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE7},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE8},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xE9},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xEA},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xEB},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xEC},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xED},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xEE},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xEF},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF0},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF1},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF2},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF3},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF4},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF5},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF6},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF7},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF8},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xF9},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xFA},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xFB},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xFC},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xFD},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xFE},LT::LT_SPECIALIZE_BITS),
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_BITS,{(TT)0xFF},LT::LT_SPECIALIZE_BITS),
		//32 SPECIAL
		_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_66_SENTENCE,{TT::TT_REX},LT::LT_SPECIALIZE_REX_TO_SENTENCE_32),
	};
	static inline syntax::parser<TT,LT> code_parser_={
		TT::TT_START,
		TT::TT_SEPERATOR,
		TT::TT_EPSILON,
		TT::TT_EOF,
		units,
	};
	static inline syntax::parser<TT,LT> code_parser64_={
		TT::TT_START,
		TT::TT_SEPERATOR,
		TT::TT_EPSILON,
		TT::TT_EOF,
		units,
	};
	static inline intel_assmbler_listener listener_;
	static syntax::parser<TT,LT>& get_parser() {
		static std::once_flag parser_flag;
		std::call_once(parser_flag,[&](){
			code_parser_.generate_parser();
			code_parser_.listeners_.push_back(&listener_);
			listener_.enabled_=true;
		});
		return code_parser_;
	}
	static syntax::parser<TT,LT>& get_parser64() {
		static std::once_flag parser_flag64;
		std::call_once(parser_flag64,[&](){
			//auto parser32=get_parser();
			//for (auto& it:parser32.units_) code_parser64_.units_.push_back(it);
			static std::vector<syntax::parser_unit<TT,LT>> units64={
				_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_ADC_SENTENCE,{TT::TT_REX,(TT)0x15,TT::TT_BITS},LT::LT_ADC15_64_SENTENCE),
				_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_80_SENTENCE,{TT::TT_REX,(TT)0x80,TT::TT_BITS,TT::TT_BITS},LT::LT_80_64_SENTENCE),
				_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_81_SENTENCE,{TT::TT_REX,(TT)0x81,TT::TT_BITS,TT::TT_BITS},LT::LT_81_64_SENTENCE),
				_STDEX_ASSEMBLER_PARSER_UNIT(TT::TT_83_SENTENCE,{TT::TT_REX,(TT)0x83,TT::TT_BITS,TT::TT_BITS},LT::LT_83_64_SENTENCE),
			};
			for (std::vector<syntax::parser_unit<TT,LT>>::iterator it=code_parser64_.units_.begin();it!=code_parser64_.units_.end();) {
				if (it->id_==LT::LT_SPECIALIZE_REX_TO_SENTENCE_32) it=code_parser64_.units_.erase(it);
				else it++;
			}
			code_parser64_.units_.insert(code_parser64_.units_.end(),units64.begin(),units64.end());
			code_parser64_.generate_parser();
			code_parser64_.listeners_.push_back(&listener_);
			listener_.enabled_=true;
		});
		return code_parser64_;
	};

public:
	void initialize(machine_bits bits,machine_bits program_bits) {
		machine_bits_=bits;
		program_bits_=program_bits;
		switch (machine_bits_) {
			case MB_32: {
				get_parser();
				break;
			}
			case MB_64: {
				get_parser64();
				break;
			}
		}
	}
	std::vector<intel::instruction> parse() {
		//while catch error
		std::vector<syntax::parser<TT,LT>::parse_node<>> lines;
		for (auto& it:bytes_) lines.push_back(syntax::parser<TT,LT>::parse_node<>({(TT)it,nullptr,{}}));
		lines.push_back(syntax::parser<TT,LT>::parse_node<>({TT::TT_EOF,nullptr,{}}));
		switch (machine_bits_) {
			case MB_32: {
				instructions_.clear();
				listener_.reset(this);
				get_parser().parse_with_listener(lines);
				return instructions_;
			}
			case MB_64: {
				instructions_.clear();
				listener_.reset(this);
				get_parser64().parse_with_listener(lines);
				return instructions_;
			}
		}
		return {};
	}
#undef _STDEX_ASSEMBLER_PARSER_UNIT
};

}

}

}

#endif