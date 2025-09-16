//Last Modified At 2025/09/16
//@Version 1.0.0.0
#ifndef _STDEX_MACHINE_ASSEMBLER_H_
#define _STDEX_MACHINE_ASSEMBLER_H_ 1

#include <stdexcept>
#include <vector>

#include "general.h"//At Least 1.0.0.1

//#define byte_list byte_vec

namespace stdex {

namespace assembler {
	using std::machine;
	
	namespace ops {
		
		bool rex_w(BYTE rex) { return (rex>>3)&1; }
		bool rex_r(BYTE rex) { return (rex>>2)&1; }
		bool rex_x(BYTE rex) { return (rex>>1)&1; }
		bool rex_b(BYTE rex) { return rex&1; }
		
		BYTE sib_s(BYTE byte) {	return byte>>6; }
		BYTE sib_i(BYTE byte) { return (byte>>3)&7; }
		BYTE sib_b(BYTE byte) { return byte&7; }
		
		BYTE modrm_mod(BYTE byte) {	return byte>>6; }
		BYTE modrm_reg(BYTE byte) { return (byte>>3)&7; }
		BYTE modrm_rm(BYTE byte)  { return byte&7; }
		
		enum OPERAND_CODE {
			OP_NONE,
			
			OP_A,	OP_E,	OP_M,	OP_G,	OP_I,	OP_F,
			
			OP_R0,	OP_R1,	OP_R2,	OP_R3,	OP_R4,	OP_R5,	OP_R6,	OP_R7,
			
			OP_AL,	OP_CL,	OP_DL,
			OP_AX,	OP_CX,	OP_DX,
			OP_EAX,	OP_ECX,	OP_EDX,
			OP_RAX,	OP_RCX,	OP_RDX,
			
			OP_ES,	OP_CS,	OP_SS,	OP_DS,	OP_FS,	OP_GS,
			
			OP_ST0,	OP_ST1,	OP_ST2,	OP_ST3,	OP_ST4,	OP_ST5,	OP_ST6,	OP_ST7,
			
			OP_J,	OP_S,	OP_O,	OP_I1,	OP_I3,	OP_sI,
			
			OP_V,	OP_W,	OP_Q,	OP_P,	OP_U,	OP_N,	OP_MU,	OP_H,	OP_L,
			
        	OP_R,	OP_C,	OP_D,       
        	
        	OP_MR
		};
		
		enum OPERAND_SIZE {
			SZ_NA=0,
			SZ_Z,
			SZ_V,
			SZ_Y,
			SZ_X,
			SZ_RDQ=7,
			
			SZ_B=8,
			SZ_O=12,
			SZ_W=16,
			SZ_D=32,
			SZ_Q=64,
			SZ_T=80,
			SZ_DQ=128,
			SZ_QQ=256,
			
			SZ_BD=(SZ_B<<8)|SZ_D,
			SZ_BV=(SZ_B<<8)|SZ_V,
			SZ_WD=(SZ_W<<8)|SZ_D,
			SZ_WV=(SZ_W<<8)|SZ_V,
			SZ_WY=(SZ_W<<8)|SZ_Y,
			SZ_DY=(SZ_D<<8)|SZ_Y,
			SZ_WO=(SZ_W<<8)|SZ_O,
			SZ_DO=(SZ_D<<8)|SZ_O,
			SZ_QO=(SZ_Q<<8)|SZ_O,
		};
		
		enum OPERAND_TYPE {
			#define _STD4573_INCLUDE_REGISTER Register(name,str) OT_##name,
				#include "register.h"
			#undef _STD4573_INCLUDE_REGISTER
			OT_REG, OT_MEM, OT_PTR, OT_IMM,	OT_JIMM, OT_CONST
		};
		
		union OPERAND_LVAL {
			SBYTE sbyte_;
			BYTE ubyte_;
			SWORD sword_;
			WORD uword_;
			SDWORD sdword_;
			DWORD udword_;
			SQWORD sqword_;
			QWORD uqword_;
			struct {
				WORD ptr_segment_;
				DWORD ptr_offset_;
			} ptr_;
		};
		
		class operand {
			OPERAND_TYPE type_;
			MACHINE_BITS size_;
			OPERAND_TYPE base_;
			OPERAND_TYPE index_;
			BYTE scale_;
			BYTE offset_;
			OPERAND_LVAL lval_;
			OPERAND_CODE code_;
		};
	}

class byte_vec {
public:
	std::vector<BYTE> vec_;
	std::vector<BYTE>::iterator it_;
	Machine machine_;
	
public:
	byte_vec(std::vector<BYTE> vec);
	byte_vec(std::vector<BYTE> vec,Machine machine);
	~byte_vec() {}
	byte_vec(const byte_vec& other);
	byte_vec(byte_vec&& other) noexcept;
	
	//byte_vec& operator =(const byte_vec& other);
	//byte_vec& operator =(byte_vec&& other) noexcept;
	
	byte_vec operator ++(int);
	byte_vec operator --(int);
	BYTE read_next_byte();
	WORD read_next_word();
	DWORD read_next_dword();
	QWORD read_next_qword();
	std::string read_next_num();
	std::string read_next_num(int bits);
	std::string read_next_num_with_sign(int bits);
	std::vector<std::string> general_registers();
	std::vector<std::string> jumps();
	std::vector<std::string> floats() {
		switch (machine_.type_) {
			case MT_INTEL:
			case MT_ATT:
				return {"fadd","fmul","fcom","fcomp","fsub","fsubr","fdiv","fdivr",
						"fld","","fst","fstp","fldenv","fldcw","fnstenv","fnstcw"};
		}
		return {"fadd","fmul","fcom","fcomp","fsub","fsubr","fdiv","fdivr"};
	}
};

class ASM_INSTRUCTION {
public:
	std::string op;
	ops::operand dst,src;
};

ASM_INSTRUCTION get_next_asmcode(byte_vec& vec,BYTE rex=0x00);

ASM_INSTRUCTION get_next_asmcode(byte_vec& vec,bool is_16bits,BYTE rex=0x00);

std::string convert_by_sib(byte_vec& vec,int index_bits,int base_bits,bool use_offset,bool use_scale,bool rex=0x00)

ASM_INSTRUCTION convert_by_modrm(byte_vec& vec,int reg_bits,int rm_bits,BYTE rex=0x00)

}

}

#endif