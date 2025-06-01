//Last Modified At 2024/10/19
//@Version 1.0
#ifndef _STD4573_TYPE_BITMAP_H_
#define _STD4573_TYPE_BITMAP_H_ 1

namespace std::type {

class bitmap {
public:
	struct bitmap_header {
		unsigned short type_;
		unsigned int size_;
		unsigned short reserved1_;//=0
		unsigned short reserved2_;//=0
		unsigned int off_bits_;
		bitmap_header () : type_() { }
		bitmap_header (unsiged int size) : type_() , size_(size) { }
	} header_;
	struct bitmap_info_header {
		unsigned int size_;
		int width_;
		int height_;
		unsigned short planes_;//=1
		unsigned short bit_count_;//1=double 4=16color 8=256color 24/32=truetype
		unsigned int compression_;//0=nozip 1=BI_RLE8 2=BI_RLE4
		unsigned int size_image_;
		int x_pels_per_meter_;
		int y_pels_per_meter_;
		unsigned int clr_used_;
		unsigned int clr_important_;
	} info_header_;
	struct rgb_quad {
		unsigned char blue_;
		unsigned char green_;
		unsigned char red_;
		unsigned char reversed_;//=0
	};
	std::vector<rgb_quad> quads_;
	std::vector<BYTE> pixels_;
	enum BMP_EXAMINE {
		BE_CONSTS,
		BE_NUMS
	};
	void examine(BMP_EXAMINE mode=BE_CONSTS) {
		if (mode&1) {
			if (header_.reserved1_ || header_.reversed2_) throw std::invalid_argument("header.reversed must be 0"); 
			if (header_.type_!=0x424D) throw std::invalid_argument("type must be BITMAP(424DH)");
			if (info_header_.planes_!=1) throw std::invalid_argument("planes must be 1");
			unsigned short bc=info_header_.bit_count_;
			if (bc!=1 && bc!=4 && bc!=8 && bc!=16 && bc!=24 && bc!=32) throw std::invalid_argument("bit_count must be 1,4,8,16,24 or 32");
			if (info_header_.compression_>5) throw std::invalid_argument("compression must be less than 6");
			if (info_header_.clr_important_>info_header_.clr_used_ && info_header_.clr_important_!=0) throw std::invalid_argument("color important cannot be bigger than color used");
			//clr_used==0 || clr_used<=2^bc(bc!=24/32)//when bc=24/32 quads=0
			//quads_.size()==2^clr_used
			//compression=1->bc=8 compression=2->bc=4 3->16/32
		}
		if (mode&2) {
			int width=info_header_.width_;
			if (width<0) width=-width;
			int height=info_header_.height_;
			if (height<0) height=-height;
			//if (quads_.size()!=width*height) throw std::out_of_range("width or height is invalid");
			for (int i=0;i<quads_.size();i++) {
				if (quads_[i].reversed_!=0) throw std::invalid_argument("wrong param \"reversed\" at pixel "+std::to_string(i)+"(row="+std::to_string(i/width)+",column="+std::to_string(i%width)+")");
			}
			if (info_header_.size_!=0x28) throw std::invalid_argument("info_header.size must be 40(28H)"); 
			if (0xE+0x28+quads_.size()*4!=header_.off_bits_) throw std::length_error("header.off_bits_ is invalid");
		//size=4*ceil(BPP*width/32),其中BPP（Bits Per 为每像素的比特数。
		//int iLineByteCnt = (((m_iImageWidth * m_iBitsPerPixel) + 31) >> 5) << 2
		//m_iImageDataSize = iLineByteCnt * m_iImageHeight
		//skip = 4 - ((m_iImageWidth * m_iBitsPerPixel)>>3) & 3 because of scan by line
		//pixels_.size
		}
	}
	bitmap() {
		//sth
	}
	bitmap(std::vector<BYTE> bits){
		//read and examine
	}
	~bitmap() {
		//something
	}
	std::vector<BYTE> to_byte_lists() {
		std::vector<unsigned char> result;
		BYTE* bytes=reinterpret_cast<BYTE*>(&header_);
		result.insert(result.end(),bytes,bytes+sizeof(bitmap_header));
		bytes=reinterpret_cast<BYTE*>(&info_header_);
		result.insert(result.end(),bytes,bytes+sizeof(bitmap_info_header));
		for (int i=0;i<quads_.size();i++) {
			bytes=reinterpret_cast<BYTE*>(&quads_[i]);
			result.insert(result.end(),bytes,bytes+sizeof(rgb_quad));
		}
		for (int i=0;i<pixels_.size();i++) {
			bytes=reinterpret_cast<BYTE*>(&pixels_[i]);
			result.insert(result.end(),bytes,bytes+sizeof(BYTE));
		}
		return result;
	}
	friend std::ostream& operator <<(std::ostream& o,const bitmap& b) {
		std::vector<BYTE> bytes=b.to_byte_lists();
		o.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
		return o;
    }
    bool convert_to(int new_bitcount) {
    	//examine new_bitcount
    	//switch curr_bc
    	//how to produce convert to less bc?
	}
	int get_rgb(int pixel_x,int pixel_y) {
		examine(BE_CONSTS);
		int width=info_header_.width_;
		if (width<0) width=-width;
		int height=info_header_.height_;
		if (height<0) height=-height;
		if (pixel_x>=width || pixel_y>=height) {
			throw std::out_of_range("pixel is not in the bitmap:the width is "+std::to_string(width)+" and the height is "+std::to_string(height));
		}
		rgb_quad result_rgb;
		if (info_header_.bit_count_>=24) {
			
		} else {
			int pixel=pixel_y*width+pixel_x;
			result_rgb=quads_[pixels_[pixel]];
		}
		int result=result_rgb.red_<<24+result_rgb.green_<<16+result_rgb.blue_<<8+result_rgb.reversed_;
		return result;
	}
};

}

#endif