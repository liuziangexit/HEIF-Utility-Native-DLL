#include "hevcimagefilereader.cpp"
#include "log.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <conio.h>

#define CHECK_FILE_FEATURE(X) do { \
        if(properties.fileFeature.hasFeature(ImageFileReaderInterface::FileFeature::X)){ fprintf(stdout, "  %-15s\tYES\n",#X); } else {  fprintf(stdout, "  %-15s\tNO\n",#X); } \
    } while(0)

struct Blob {
	uint32_t idx;
	std::vector<uint8_t> data;
};

struct FileData {
	uint32_t width;
	uint32_t height;
	uint8_t rows;
	uint8_t cols;
	uint16_t rotation;
	std::vector<Blob> tiles;
};

template <typename T>
std::string toString(const T& value) {
	std::stringstream sstr;
	sstr << value;
	if (sstr.fail()) {
		//不需要clear，因为sstr不会再次使用
		throw std::exception("std::stringstream::fail()==true");
	}
	return sstr.str();
}

class file_ptr final {//copy from liuzianglib(https://github.com/liuziangexit/liuzianglib
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

void print_filedata(const FileData &filedata) {
	fprintf(stdout, "\nFile information:\n");
	fprintf(stdout, " width      :%d\n", filedata.width);
	fprintf(stdout, " height     :%d\n", filedata.height);
	fprintf(stdout, " rows       :%d\n", filedata.rows);
	fprintf(stdout, " cols       :%d\n", filedata.cols);
	fprintf(stdout, " rotation   :%d\n", filedata.rotation);
	fprintf(stdout, " tiles      :%lu\n", filedata.tiles.size());
}

std::tuple<FileData, HevcImageFileReader::ParameterSetMap> LoadData(HevcImageFileReader &reader, uint32_t contextId) {
	ImageFileReaderInterface::GridItem gridItem;
	ImageFileReaderInterface::IdVector gridItemIds;
	FileData filedata;

	//get all grid items
	reader.getItemListByType(contextId, "grid", gridItemIds);
	gridItem = reader.getItemGrid(contextId, gridItemIds.at(0));
	filedata.width = gridItem.outputWidth;
	filedata.height = gridItem.outputHeight;
	filedata.rows = gridItem.rowsMinusOne + 1;
	filedata.cols = gridItem.columnsMinusOne + 1;

	const uint32_t itemId = gridItemIds.at(0);
	const auto itemProperties = reader.getItemProperties(contextId, itemId);
	for (const auto& property : itemProperties) {
		if (property.type == ImageFileReaderInterface::ItemPropertyType::IROT) {
			filedata.rotation = reader.getPropertyIrot(contextId, property.index).rotation;
		}
	}
	ImageFileReaderInterface::IdVector masterItemIds;
	reader.getItemListByType(contextId, "master", masterItemIds);
	for (auto id : masterItemIds) {
		filedata.tiles.push_back(Blob{ id });
	}

	for (auto & tile : filedata.tiles) {
		reader.getItemDataWithDecoderParameters(contextId, tile.idx, tile.data);
		//std::cout << "getTileBlob idx: " << tile.idx << " Size: " << tile.data.size() << "bytes" << std::endl;
	}

	HevcImageFileReader::ParameterSetMap paramset;
	reader.getDecoderParameterSets(contextId, filedata.tiles[0].idx, paramset);

	return std::tuple<FileData, HevcImageFileReader::ParameterSetMap>(filedata, paramset);
}

bool extract(const char* srcfile, const char* dstfile)noexcept {//copy from heic2hevc(https://github.com/yohhoy/heic2hevc
	try {		
		HevcImageFileReader reader;
		reader.initialize(srcfile);
		const auto& props = reader.getFileProperties();
		const uint32_t contextId = props.rootLevelMetaBoxProperties.contextId;

		HevcImageFileReader::IdVector ids;
		reader.getItemListByType(contextId, "master", ids);

		int index = 0;

		HevcImageFileReader::ParameterSetMap paramset;
		reader.getDecoderParameterSets(contextId, ids[1], paramset);
		std::string paramsetstr;
		for (const auto& key : { "VPS", "SPS", "PPS" }) {
			const auto& nalu = paramset[key];
			paramsetstr += std::string((const char *)nalu.data(), nalu.size());
		}		

		for (const auto& p : ids) {
			HevcImageFileReader::DataVector bitstream;
			reader.getItemDataWithDecoderParameters(contextId, p, bitstream);

			std::string writethis{ paramsetstr };
			writethis += std::string((const char *)bitstream.data(), bitstream.size());

			file_ptr ptr(fopen((std::string(dstfile) + toString(index)).c_str(), "wb"));
			index++;
			if (!ptr) continue;
			fwrite(writethis.c_str(), 1, writethis.size(), ptr.get());
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

int main(int argc, char *argv[]) {
	std::string filename;
	std::cout << "input filename:";
	std::cin >> filename;

	Log::setLevel(Log::LogLevel::INFO);

	HevcImageFileReader reader;
	fprintf(stdout, "Reading %s\n", filename);
	reader.initialize(filename);

	const auto& properties = reader.getFileProperties();
	assert(properties.fileFeature.hasFeature(ImageFileReaderInterface::FileFeature::HasRootLevelMetaBox));
	const uint32_t contextId = properties.rootLevelMetaBoxProperties.contextId;
	fprintf(stdout, "\nGot contextId:%d\n", contextId);

	fprintf(stdout, "properties of file:\n");
	CHECK_FILE_FEATURE(HasSingleImage);
	CHECK_FILE_FEATURE(HasImageCollection);
	CHECK_FILE_FEATURE(HasImageSequence);
	CHECK_FILE_FEATURE(HasCoverImage);
	CHECK_FILE_FEATURE(HasOtherTimedMedia);
	CHECK_FILE_FEATURE(HasRootLevelMetaBox);
	CHECK_FILE_FEATURE(HasMoovLevelMetaBox);
	CHECK_FILE_FEATURE(HasAlternateTracks);

	auto data = LoadData(reader, contextId);
	print_filedata(std::get<0>(data));
	bool rv = extract(filename.c_str(), "out.");
	if (rv)
		std::cout << "func returned true\n";
	else
		std::cout << "func returned false\n";

	_getch();
}