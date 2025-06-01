#ifndef _STD4573_MACHINE_GENERAL_H_
#define _STD4573_MACHINE_GENERAL_H_ 1
#include <vector>

namespace std::machine {

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef int8_t SBYTE;
typedef int16_t	SWORD;
typedef int32_t	SDWORD;
typedef int64_t	SQWORD;

enum MACHINE_TYPE {
	MT_INTEL,
	MT_ATT,
};

enum MACHINE_BITS {
	MB_8,
	MB_16,
	MB_32,
	MB_64,
};

class Machine {
public:
	MACHINE_TYPE type_;
	MACHINE_BITS bits_;
	bool is_little_endian_;

public:
	Machine() : type_(MT_INTEL) , bits_(MB_32) , is_little_endian_(false) {}
	Machine(MACHINE_TYPE type,MACHINE_BITS bits) : type_(type) , bits_(bits) , is_little_endian_(false) {}
	Machine(MACHINE_TYPE type,MACHINE_BITS bits,bool is_little_endian) : type_(type) , bits_(bits) , is_little_endian_(is_little_endian) {}
};

int get_bits(MACHINE_BITS bits) {
	switch (bits) {
		case MB_8:
			return 8;
		case MB_16:
			return 16;
		case MB_32:
			return 32;
		case MB_64:
			return 64;
	}
	return 0;
}

std::vector<std::string> get_general_registers(int bits) {
	switch (bits) {
		case 8: {
			return {"al","cl","dl","bl","ah","ch","dh","bh",
					"r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"};
			break;
		}
		case 16: {
			return {"ax","cx","dx","bx","sp","bp","si","di",
					"r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"};
			break;
		}
		case 32: {
			return {"eax","ecx","edx","ebx","esp","ebp","esi","edi",
					"r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"};
			break;
		}
		case 64: {
			return {"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
					"r8","r9","r10","r11","r12","r13","r14","r15"};
			break;
		}
	}
	return {};
}

bool use_rex(MACHINE_BITS bits) {
	switch (bits) {
		case MB_8:
		case MB_16:
		case MB_32:
			return false;
		case MB_64:
			return true;
	}
	return false;
}

}

#endif