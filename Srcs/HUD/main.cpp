#include "hevcimagefilereader.cpp"
#include <iostream>
#include <string>
#include <memory>

class file_ptr final {//管理C语言文件指针生命周期的包装器
					  //可以通过if(file_ptr)的方式判断file_ptr是否已经打开
					  //可以在使用时手动fclose(file_ptr::get())，这不会导致未定义行为
					  //仅支持移动拷贝
					  //某些时候发现在使用此类的过程中有1(VS内存查看器中的“分配(差异)”)内存没有回收，这不是内存泄漏。
public:
	file_ptr() :fp(nullptr) {}

	file_ptr(FILE* input) :fp(input) {}

	file_ptr(const file_ptr&) = delete;

	file_ptr(file_ptr&& input)noexcept {
		clear();
		this->fp = input.fp;
		input.fp = nullptr;
	}

	~file_ptr() {
		clear();
	}

	file_ptr& operator=(const file_ptr&) = delete;

	file_ptr& operator=(file_ptr&& input)noexcept {
		clear();
		this->fp = input.fp;
		input.fp = nullptr;
		return *this;
	}

	operator bool()const {
		if (fp == NULL || fp == nullptr) return false;
		return true;
	}

	inline FILE* get()const {
		return fp;
	}

	inline void reset(FILE* input) {
		clear();
		fp = input;
	}

private:
	inline void clear() {
		if (fp == NULL || fp == nullptr) return;
		fclose(fp);
	}

private:
	FILE *fp;
};

bool extract(const char* srcfile, const char* dstfile)noexcept {
	try {
		HevcImageFileReader reader;
		reader.initialize(srcfile);
		const auto& props = reader.getFileProperties();
		const uint32_t contextId = props.rootLevelMetaBoxProperties.contextId;

		HevcImageFileReader::IdVector ids;
		reader.getItemListByType(contextId, "master", ids);
		const uint32_t itemId = ids[0];

		HevcImageFileReader::ParameterSetMap paramset;
		reader.getDecoderParameterSets(contextId, itemId, paramset);

		HevcImageFileReader::DataVector bitstream;
		reader.getItemDataWithDecoderParameters(contextId, itemId, bitstream);

		//std::ofstream ofs(dstfile, std::ios::binary);
		std::string writethis;
		for (const auto& key : { "VPS", "SPS", "PPS" }) {
			const auto& nalu = paramset[key];
			//ofs.write((const char *)nalu.data(), nalu.size());
			writethis += std::string((const char *)nalu.data(), nalu.size());
		}
		//ofs.write((const char *)bitstream.data(), bitstream.size());
		writethis += std::string((const char *)bitstream.data(), bitstream.size());
		
		file_ptr ptr(fopen(dstfile, "wb"));
		if (!ptr) return false;
		if (fwrite(writethis.c_str(), 1, writethis.size(), ptr.get()) == -1) return false;
		return true;
	}
	catch (...) {
		return false;
	}
}

int main(int argc, char* argv[]) {
	if (argc < 3)
		return 1;
	if (extract(argv[1], argv[2]))
		return 0;
	return 1;
}