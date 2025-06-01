
namespace std::assembler {

byte_vec::byte_vec(std::vector<BYTE> vec) : vec_(vec) {
	it_=vec_.begin();
}

byte_vec::byte_vec(std::vector<BYTE> vec,Machine machine) : vec_(vec) , machine_(machine) {
	it_=vec_.begin();
}

byte_vec::byte_vec(const byte_vec& other) : vec_(other.vec_) , machine_(other.machine_) {
	std::vector<BYTE>::const_iterator const_it=other.it_;
	it_=vec_.begin()+std::distance(other.vec_.cbegin(),const_it);
}

byte_vec::byte_vec(byte_vec&& other) noexcept : vec_(other.vec_) , machine_(other.machine_) {
	it_=vec_.begin()+std::distance(other.vec_.begin(),other.it_);
	other.vec_.clear();
	//other.it_=std::nullopt;
}

//byte_vec& operator =(const byte_vec& other);
//byte_vec& operator =(byte_vec&& other) noexcept;

byte_vec byte_vec::operator ++(int) {
	byte_vec temp=*this;
	if (it_==vec_.end()) throw std::out_of_range("Reach the end of set!");
	else it_++;
	return temp;
}

byte_vec byte_vec::operator --(int) {
	byte_vec temp=*this;
	if (it_==vec_.begin()) throw std::out_of_range("Reach the begin of set!");
	else it_--;
	return temp;
}

BYTE byte_vec::read_next_byte() {
	int x=std::distance(it_,vec_.end());
	if (std::distance(it_,vec_.end())<1) throw std::out_of_range("No enough bytes!");
	BYTE result=0x00;
	for (int i=0;i<1;i++) {
		result<<=8;
		result+=*it_;
		try {
			(*this)++;	
		} catch (const std::exception& e) {
			throw;
		}
	}
	return result;
}

WORD byte_vec::read_next_word() {
	if (std::distance(it_,vec_.end())<2) throw std::out_of_range("No enough bytes!");
	WORD result=0x0000;
	for (int i=0;i<2;i++) {
		result<<=8;
		result+=*it_;
		try {
			(*this)++;	
		} catch (const std::exception& e) {
			throw;
		}
	}
	if (machine_.is_little_endian_) {
		result=(result<<8)+(result>>8);
	}
	return result;
}

DWORD byte_vec::read_next_dword() {
	if (std::distance(it_,vec_.end())<4) throw std::out_of_range("No enough bytes!");
	DWORD result=0x00000000;
	for (int i=0;i<4;i++) {
		result<<=8;
		result+=*it_;
		try {
			(*this)++;	
		} catch (const std::exception& e) {
			throw;
		}
	}
	if (machine_.is_little_endian_) {
		result=(result<<24)+(result>>24)+(((result>>8)&0xFF)<<16)+(((result>>16)&0xFF)<<8);
	}
	return result;
}

QWORD byte_vec::read_next_qword() {
	if (std::distance(it_,vec_.end())<8) throw std::out_of_range("No enough bytes!");
	QWORD result=0x0000000000000000;
	for (int i=0;i<8;i++) {
		result<<=8;
		result+=*it_;
		try {
			(*this)++;	
		} catch (const std::exception& e) {
			throw;
		}
	}
	if (machine_.is_little_endian_) {
		result=(result<<56)+(result>>56)+(((result>>8)&0xFF)<<48)+(((result>>16)&0xFF)<40)
										+(((result>>24)&0xFF)<<32)+(((result>>32)&0xFF)<24)
										+(((result>>40)&0xFF)<<16)+(((result>>48)&0xFF)<8);
	}
	return result;
}

std::string byte_vec::read_next_num() {
	return read_next_num(get_bits(machine_.type_));
}

std::string byte_vec::read_next_num(int bits) {
	std::string result;
	switch (bits) {
		case 8: {
			char* temp=new char[3];
			try {
				sprintf(temp,"%02X",read_next_byte());
			} catch (const std::exception& e) {
				delete[] temp;
				throw;	
			}
			result=temp;
			delete[] temp;
			break;
		}
		case 16: {
			char* temp=new char[5];
			try {
				sprintf(temp,"%04X",read_next_word());
			} catch (const std::exception& e) {
				delete[] temp;
				throw;	
			}
			result=temp;
			delete[] temp;
			break;
		}
		case 32: {
			char* temp=new char[9];
			try {
				sprintf(temp,"%08X",read_next_dword());
			} catch (const std::exception& e) {
				delete[] temp;
				throw;	
			}
			result=temp;
			delete[] temp;
			break;
		}
		case 64: {
			char* temp=new char[17];
			try {
				sprintf(temp,"%016X",read_next_qword());
			} catch (const std::exception& e) {
				delete[] temp;
				throw;	
			}
			result=temp;
			delete[] temp;
			break;
		}
	}
	return result;
}

std::string byte_vec::read_next_num_with_sign(int bits) {
	std::string result;
	switch (bits) {
		case 8: {
			char* temp=new char[3];
			try {
				sprintf(temp,"%02X",(int8_t)read_next_byte());
			} catch (const std::exception& e) {
				delete[] temp;
				throw;	
			}
			result=temp;
			delete[] temp;
			break;
		}
		case 16: {
			char* temp=new char[5];
			try {
				sprintf(temp,"%04X",(int16_t)read_next_word());
			} catch (const std::exception& e) {
				delete[] temp;
				throw;	
			}
			result=temp;
			delete[] temp;
			break;
		}
		case 32: {
			char* temp=new char[9];
			try {
				sprintf(temp,"%08X",(int32_t)read_next_dword());
			} catch (const std::exception& e) {
				delete[] temp;
				throw;	
			}
			result=temp;
			delete[] temp;
			break;
		}
		case 64: {
			char* temp=new char[17];
			try {
				sprintf(temp,"%016X",(int64_t)read_next_qword());
			} catch (const std::exception& e) {
				delete[] temp;
				throw;	
			}
			result=temp;
			delete[] temp;
			break;
		}
	}
	return result;
}

std::vector<std::string> byte_vec::general_registers() {
	return get_general_registers(get_bits(machine_.bits_));
}

std::vector<std::string> byte_vec::jumps() {
	switch (machine_.type_) {
		case MT_INTEL:
		case MT_ATT:
			return {"jo","jno","jb","jnb","je","jne","jna","ja","js","jns","jp","jnp","jl","jnl","jng","jg"};
	}
	return {"jo","jno","jb","jnb","je","jne","jna","ja","js","jns","jp","jnp","jl","jnl","jng","jg"};
}

ASM_INSTRUCTION get_next_asmcode(byte_vec& vec,BYTE rex=0x00) {
	return get_next_asmcode(byte_vec,false,rex);
}

ASM_INSTRUCTION get_next_asmcode(byte_vec& vec,bool is_16bits,BYTE rex=0x00) {
	BYTE byte;
	bool REX_W=ops::rex_w(rex);
	bool REX_R=ops::rex_r(rex);
	bool REX_X=ops::rex_x(rex);
	bool REX_B=ops::rex_b(rex);
	try {
		byte=vec.read_next_byte();
	} catch (const std::out_of_range& e) {
		throw std::out_of_range("No enough BYTES to get when try to get a byte at position "+
									std::to_string(std::distance(vec.vec_.begin(),vec.it_))+" with BYTE "+
									std::to_string(*(vec.it_)));
	} catch (const std::exception& e) {
		throw;
	}
	ASM_INSTRUCTION result;
	int curr_bits=get_bits(vec.machine_.bits_);
	if (use_rex(vec.machine_) && !REX_W) curr_bits=32;
	if (is_16bits) curr_bits=16;
	switch (byte) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03: {
			result.op="add";
			result=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits,REX_R,REX_B,REX_X);
			if (byte>=0x02) {
				std::string temp=result.dst;
				result.dst=result.src;
				result.src=temp;
			}
			break;
		}
		case 0x04:
		case 0x05: {
			result.op="add";
			result.dst=(byte%2?(REX_W?vec.general_registers()[0]:get_general_registers(curr_bits)[0]):"al");
			result.src=vec.read_next_num((byte%2)?curr_bits:8);
			break;
		}
		case 0x06:
		case 0x07: {
			result.op=(byte%2)?"pop":"push";
			result.dst="es";
			break;
		}
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B: {
			result.op="or";
			result=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			if (byte>=0x0A) {
				std::string temp=result.dst;
				result.dst=result.src;
				result.src=temp;
			}
			break;
		}
		case 0x0C:
		case 0x0D: {
			result.op="or";
			result.dst=(byte%2?vec.general_registers()[0]:"al");
			result.src=vec.read_next_num((byte%2)?curr_bits:8);
			break;
		}
		case 0x0E:
		case 0x0F: {
			result.op=(byte%2)?"pop":"push";
			result.dst="cs";
			break;
		}
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13: {
			result.op="adc";
			result=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits,REX_R,REX_B,REX_X);
			if (byte>=0x12) {
				std::string temp=result.dst;
				result.dst=result.src;
				result.src=temp;
			}
			break;
		}
		case 0x14:
		case 0x15: {
			result.op="adc";
			result.dst=(byte%2?(REX_W?vec.general_registers()[0]:get_general_registers(curr_bits)[0]):"al");
			result.src=vec.read_next_num((byte%2)?curr_bits:8);
			break;
		}
		case 0x16: {
			result+="push ss";
			break;
		}
		case 0x17: {
			result+="pop ss";
			break;
		}
		case 0x18:
		case 0x19:
		case 0x1A:
		case 0x1B: {
			result+=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			if (byte>=0x1A) {
				int comma_pos=result.find(',');
				result=result.substr(comma_pos+1)+","+(byte==0x1A?"byte ptr ":"")+result.substr(0,comma_pos);
			}
			result="sbb "+result;
			break;
		}
		case 0x1C:
		case 0x1D: {
			std::string eal=(byte%2?vec.general_registers()[0]:"al");
			result+="sbb "+eal+","+vec.read_next_num((byte%2)?curr_bits:8);
			break;
		}
		case 0x1E: {
			result+="push ds";
			break;
		}
		case 0x1F: {
			result+="pop ds";
			break;
		}
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23: {
			result+=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			if (byte>=0x22) {
				int comma_pos=result.find(',');
				result=result.substr(comma_pos+1)+","+result.substr(0,comma_pos);
			}
			result="and "+result;
			break;
		}
		case 0x24:
		case 0x25: {
			std::string eal=(byte%2?vec.general_registers()[0]:"al");
			result+="and "+eal+","+vec.read_next_num((byte%2)?curr_bits:8);
			break;
		}
		//26->null?
		case 0x27: {
			result+="daa";
			break;
		}
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B: {
			result+=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			if (byte>=0x2A) {
				int comma_pos=result.find(',');
				result=result.substr(comma_pos+1)+","+result.substr(0,comma_pos);
			}
			result="sub "+result;
			break;
		}
		case 0x2C:
		case 0x2D: {
			std::string eal=(byte%2?vec.general_registers()[0]:"al");
			result+="sub "+eal+","+vec.read_next_num((byte%2)?curr_bits:8);
			break;
		}
		//2E->null?
		case 0x2F: {
			result+="das";
			break;
		}
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33: {
			result+=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			if (byte>=0x32) {
				int comma_pos=result.find(',');
				result=result.substr(comma_pos+1)+","+result.substr(0,comma_pos);
			}
			result="xor "+result;
			break;
		}
		case 0x34:
		case 0x35: {
			std::string eal=(byte%2?vec.general_registers()[0]:"al");
			result+="xor "+eal+","+vec.read_next_num((byte%2)?curr_bits:8);
			break;
		}
		//36->null?
		case 0x37: {
			result.op="aaa";
			break;
		}
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B: {
			result+=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			if (byte>=0x3A) {
				int comma_pos=result.find(',');
				result=result.substr(comma_pos+1)+","+result.substr(0,comma_pos);
			}
			result="cmp "+result;
			break;
		}
		case 0x3C:
		case 0x3D: {
			std::string eal=(byte%2?vec.general_registers()[0]:"al");
			result+="cmp "+eal+","+vec.read_next_num((byte%2)?curr_bits:8);
			break;
		}
		//3E->null?
		case 0x3F: {
			result.op="aas";
			break;
		}
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47: {
			if (use_rex(vec.machine_.bits_)) {
				result=get_next_asmcode(vec,byte);
			} else {
				result.op="inc";
				result.dst=vec.general_registers()[(int)byte-0x40];
			}
			break;
		}
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F: {
			if (use_rex(vec.machine_.bits_)) {
				result=get_next_asmcode(vec,byte);
			} else {
				result.op="dec";
				result.dst=vec.general_registers()[(int)byte-0x48];
			}
			break;
		}
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57: {
			result+="push "+vec.general_registers()[(int)byte-0x50];
			break;
		}
		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F: {
			result+="pop "+vec.general_registers()[(int)byte-0x58];
			break;
		}
		case 0x60: {
			result+="pushad";
			break;
		}
		case 0x61: {
			result+="popad";
			break;
		}
		case 0x62: {
			result+=convert_by_modrm_with_sib(vec,curr_bits,curr_bits);
			int comma_pos=result.find(',');
			result=result.substr(comma_pos+1)+","+result.substr(0,comma_pos);
			result="bound "+result;
			break;
		}
		case 0x63: {
			result+="arpl "+convert_by_modrm_with_sib(vec,16,curr_bits);
			break;
		}
		//64->null? 64 A1 00 00 00 00=mov eax,fs:[0] 64 89 25 00 00 00 00=mov fs:[0],esp
		//65->null?
		case 0x66: {
			//REX.W 0F 38 F6->ADCX r32|r64,r/m32|r/m64
			//0F 58 /r->ADDPD(66->NP ADDPD->ADDPS)(66->F2|F3 ADDPD->ADDSD|ADDSS) xmm1,xmm2/m128
			//0F D0 /r->ADDSUBPD(66->F2 ADDSUBPS) xmm1,xmm2/m128
			break;
		}
		//67->null?
		case 0x68: {
			result+="push "+vec.read_next_num(curr_bits);
			break;
		}
		//69 very complex
		case 0x6A: {
			result+="push "+vec.read_next_num_with_sign(8);
			break;
		}
		//6B is similar to 69 but opcode2 is 1 byte
		case 0x6C: {
			result+="insb";
			break;
		}
		case 0x6D: {
			result+="insd";
			break;
		}
		case 0x6E: {
			result+="outsb";
			break;
		}
		case 0x6F: {
			result+="outsd";
			break;
		}
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F: {
			result+=vec.jumps()[(int)byte-0x70]+" "+vec.read_next_num_with_sign(8);
			break;
		}
		case 0x80: {
			//Mod=11?
			//Reg=000->ADD
			//Reg=010->ADC
			//RM=byte(0x81->dword)(0x83->signed byte)
			break;
		}
		//80~83
		//81 EC 90 00 00 00=sub esp,90
		case 0x84:
		case 0x85: {
			result+="test "+convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			break;
		}
		case 0x86:
		case 0x87: {
			result+="xchg "+convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			break;
		}
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B: {
			result+=convert_by_modrm_with_sib(vec,(byte%2)?curr_bits:8,curr_bits);
			if (byte>=0x8A) {
				int comma_pos=result.find(',');
				result=result.substr(comma_pos+1)+","+result.substr(0,comma_pos);
			}
			result="mov "+result;
			break;
		}
		//8C 8E ss
		case 0x8D: {
			result+=convert_by_modrm_with_sib(vec,curr_bits,curr_bits);
			int comma_pos=result.find(',');
			result=result.substr(comma_pos+1)+","+result.substr(0,comma_pos);
			result="lea "+result;
			break;
		}
		case 0x90: {
			result+="nop";
			break;
		}
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97: {
			result+="xchg "+vec.general_registers()[0]+","+vec.general_registers()[(int)byte-0x90];
			break;
		}
		case 0x98: {
			result+="cwde";
			break;
		}
		case 0x99: {
			result+="cdq";
			break;
		}
		//9A strange call
		case 0x9B: {
			result+="wait";
			break;
		}
		case 0x9C: {
			result+="pushfd";
			break;
		}
		case 0x9D: {
			result+="popfd";
			break;
		}
		case 0x9E: {
			result+="sahf";
			break;
		}
		case 0x9F: {
			result+="lahf";
			break;
		}
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3: {
			std::string eal=(byte%2?vec.general_registers()[0]:"al");
			result+="mov "+eal+",["+vec.read_next_num()+"]";
			if (byte>=0xA2) {
				int comma_pos=result.find(',');
				result=result.substr(comma_pos+1)+","+result.substr(0,comma_pos);
			}
			break;
		}
		case 0xA4: {
			result+="movsb";
			break;
		}
		case 0xA5: {
			result+="movsd";
			break;
		}
		case 0xA6: {
			result+="cmpsb";
			break;
		}
		case 0xA7: {
			result+="cmpsd";
			break;
		}
		case 0xA8: {
			result+="test al,"+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xA9: {
			result+="test "+vec.general_registers()[0]+","+vec.read_next_num(curr_bits);
			break;
		}
		case 0xAA: {
			result+="stosb";
			break;
		}
		case 0xAB: {
			result+="stosd";
			break;
		}
		case 0xAC: {
			result+="lodsb";
			break;
		}
		case 0xAD: {
			result+="lodsd";
			break;
		}
		case 0xAE: {
			result+="scasb";
			break;
		}
		case 0xAF: {
			result+="scasd";
			break;
		}
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7: {
			result+="mov "+get_general_registers(8)[(int)byte-0xB0]+","+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBE:
		case 0xBF: {
			result+="mov "+vec.general_registers()[(int)byte-0xB8]+","+vec.read_next_num(curr_bits);
			break;
		}
		//C0 C1 sar
		case 0xC2:
		case 0xCA: {
			result+="ret "+vec.read_next_num(16);
			break;
		}
		case 0xC3:
		case 0xCB: {
			result+="ret";
			break;
		}
		//C4 is qword C4000000 0F=add[r15],r15l,r9l
		//C5456000=vpunpcklbw ymm8,ymm7,[eax]
		//0F90
		//C6 4C->??? C6 44=00 44 xxxxx + 1bytesignednum
		case 0xC8: {
			result+="enter "+vec.read_next_num(16)+","+vec.read_next_num(8);
			break;
		}
		case 0xC9: {
			result+="leave";
			break;
		}
		case 0xCC: {
			result+="int3";
			break;
		}
		case 0xCD: {
			result+="int "+vec.read_next_num(8);
			break;
		}
		case 0xCE: {
			result+="into";
			break;
		}
		case 0xCF: {
			result+="iretd";
			break;
		}
		//D0~D3 rol
		case 0xD4: {
			result.op="aam";
			std::string ib=vec.read_next_num(8);
			if (ib!="0A") result.dst=ib;
			break;
		}
		case 0xD5: {
			result.op="aad";
			std::string ib=vec.read_next_num(8);
			if (ib!="0A") result.dst=ib;
			break;
		}
		case 0xD7: {
			result+="xlatb";
			break;
		}
		//D8 04 90=fadd [eax+edx*4]
		//D9 04 90=fld [eax+edx*4]
		//see floats
		//DA=D8 f->fi
		//DB=D9 f->fi but DB 08=fisttp No DB 20/30 DB 28=fld tword 38=fstp tword
		//DC~DF=D8~DB qword
		//DBC0=fcmovnb st(0),st(0)
		case 0xE0: {
			result+="loopne "+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xE1: {
			result+="loope "+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xE2: {
			result+="loop "+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xE3: {
			result+="jecxz "+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xE4: {
			result+="in al,"+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xE5: {
			result+="in "+vec.general_registers()[0]+","+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xE6: {
			result+="out "+vec.read_next_num_with_sign(8)+",al";
			break;
		}
		case 0xE7: {
			result+="out "+vec.read_next_num_with_sign(8)+","+vec.general_registers()[0];
			break;
		}
		case 0xE8: {
			result+="call "+vec.read_next_num_with_sign(curr_bits);
			break;
		}
		case 0xE9: {
			result+="jmp "+vec.read_next_num_with_sign(curr_bits);
			break;
		}
		case 0xEA: {
			std::string jmp_to=vec.read_next_num_with_sign(curr_bits);
			result+="jmp "+vec.read_next_num_with_sign(16)+":"+jmp_to;
			break;
		}
		case 0xEB: {
			result+="jmp "+vec.read_next_num_with_sign(8);
			break;
		}
		case 0xEC: {
			result+="in al,dx";//???
			break;
		}
		case 0xED: {
			result+="in "+vec.general_registers()[0]+",dx";//???
			break;
		}
		case 0xEE: {
			result+="out dx,al";//???
			break;
		}
		case 0xEF: {
			result+="out dx,"+vec.general_registers()[0];//???
			break;
		}
		//F0 0F90 11=lock seto byte ptr [ecx]
		//F2/F3 0F90 11=seto byte ptr [ecx]
		//F2/F3 3F=repne repe aas 
		//seems F2=F3=rep/nullptr F0=LOCK
		case 0xF4: {
			result+="hlt";
			break;
		}
		case 0xF5: {
			result+="cmc";
			break;
		}
		//F6 3F=idiv byte [edi]
		//F7 3F=idiv [edi]
		//imul eax,2=6B C0 02
		case 0xF8: {
			result+="clc";
			break;
		}
		case 0xF9: {
			result+="stc";
			break;
		}
		case 0xFA: {
			result+="cli";
			break;
		}
		case 0xFB: {
			result+="sti";
			break;
		}
		case 0xFC: {
			result+="cld";
			break;
		}
		case 0xFD: {
			result+="std";
			break;
		}
		//FE C0=inc al
		//FF C0=inc eax FF D0=call eax
		case 0x8F:
		case 0xD6:
		case 0xF1:
		default: {
			result="???";
			break;
		}
	}
	return result;
}

std::string convert_by_sib(byte_vec& vec,int index_bits,int base_bits,bool use_offset,bool use_scale,bool rex=0x00) {
	bool rex_x=ops::rex_x(rex);
	bool rex_b=ops::rex_b(rex);
	BYTE byte;
	try {
		byte=vec.read_next_byte();
	} catch (const std::out_of_range& e) {
		throw std::out_of_range("No enough BYTES to get when try to get the byte by SIB at position "+
									std::to_string(std::distance(vec.vec_.begin(),vec.it_))+" with BYTE "+
									std::to_string(*(vec.it_)));
	} catch (const std::exception& e) {
		throw;
	}
	BYTE scale=ops::sig_s(byte);
	switch (scale) {
		case 0b00: {
			scale=0x01;
			break;
		}
		case 0b01: {
			scale=0x02;
			break;
		}
		case 0b10: {
			scale=0x04;
			break;
		}
		case 0b11: {
			scale=0x08;
			break;
		}
	}
	std::string multiply=(scale==0b01)?"":"*"+std::to_string((int)scale);
	BYTE index=ops::sig_i(byte);
	BYTE base=ops::sig_b(byte);
	std::string result;
	if (base==0x05 && use_offset) {
		try {
			std::string offset=vec.read_next_num(base_bits);
			result="["+get_general_registers(index_bits)[(int)index+rex_x*8]+multiply+"+"+offset+"]";
			return result;
		} catch (const std::out_of_range& e) {
			throw std::out_of_range("No enough BYTES to get when try to get a num by SIB at position "+
									std::to_string(std::distance(vec.vec_.begin(),vec.it_))+" with BYTE "+
									std::to_string(*(vec.it_)));
		} catch (const std::exception& e) {
			throw;
		}
	}
	if (index==0x04 && !use_scale) {
		result="["+get_general_registers(base_bits)[(int)base+rex_b*8]+"]";
		return result;
	}
	result="["+get_general_registers(base_bits)[(int)base+rex_b*8]+"+"+get_general_registers(index_bits)[(int)index+rex_x*8]+multiply+"]";
	return result;
}

ASM_INSTRUCTION convert_by_modrm(byte_vec& vec,int reg_bits,int rm_bits,BYTE rex=0x00) {
	bool rex_x=ops::rex_x(rex);
	bool rex_r=ops::rex_r(rex);
	bool rex_b=ops::rex_b(rex);
	BYTE byte;
	try {
		byte=vec.read_next_byte();
	} catch (const std::out_of_range& e) {
		throw std::out_of_range("No enough BYTES to get when try to get the byte by ModR/M at position "+
									std::to_string(std::distance(vec.vec_.begin(),vec.it_))+" with BYTE "+
									std::to_string(*(vec.it_)));
	} catch (const std::exception& e) {
		throw;
	}
	BYTE Mod=ops::modrm_mod(byte);
	BYTE Reg=ops::modrm_reg(byte);
	BYTE RM=ops::modrm_rm(byte);
	std::string reg_str="";
	if (Mod!=0b11 && RM==0x04) {
		reg_str=convert_by_sib(vec,rm_bits,rm_bits,Mod==0b00,false,rex_b,rex_x);
		reg_str.erase(reg_str.size()-1);
	} else if (Mod==0b00 && RM==0x05) {
		try {
			reg_str="["+vec.read_next_num(rm_bits);
		} catch (const std::out_of_range& e) {
			throw std::out_of_range("No enough BYTES to get when try to get a num by ModR/M at position "+
										std::to_string(std::distance(vec.vec_.begin(),vec.it_))+" with BYTE "+
										std::to_string(*(vec.it_)));
		} catch (const std::exception& e) {
			throw;
		}
	} else {
		reg_str="["+get_general_registers(rm_bits)[(int)RM+rex_b*8];
	}	
	switch (Mod) {
		case 0b00: {
			reg_str+="]";
			break;
		}
		case 0b01: {
			try {
				std::string offset=vec.read_next_num(8);
				reg_str+="+"+offset+"]";
			} catch (const std::out_of_range& e) {
				throw std::out_of_range("No enough BYTES to get when try to get a num by ModR/M at position "+
										std::to_string(std::distance(vec.vec_.begin(),vec.it_))+" with BYTE "+
										std::to_string(*(vec.it_)));
			} catch (const std::exception& e) {
				throw;
			}
			break;
		}
		case 0b10: {
			try {
				std::string offset=vec.read_next_num(rm_bits);
				reg_str+="+"+offset+"]";
			} catch (const std::out_of_range& e) {
				throw std::out_of_range("No enough BYTES to get when try to get a num by R/M with SIB at position "+
										std::to_string(std::distance(vec.vec_.begin(),vec.it_))+" with BYTE "+
										std::to_string(*(vec.it_)));
			} catch (const std::exception& e) {
				throw;
			}
			break;
		}
		case 0b11: {
			reg_str=get_general_registers(reg_bits)[(int)RM+rex_b*8];
			break;
		}
	}
	ASM_INSTRUCTION result;
	result.dst=reg_str;
	result.src=get_general_registers(reg_bits)[(int)Reg+rex_r*8];
	return result;
}

}