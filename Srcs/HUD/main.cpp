#include "hevcimagefilereader.cpp"
#include "log.hpp"

#include <iostream>
#include <sstream>
#include <conio.h>

#include "liuzianglib/liuzianglib.h"
#include "liuzianglib/DC_STR.h"
#include "liuzianglib/DC_File.h"

struct heifdata {
	heifdata() = default;

	heifdata(const heifdata&) = default;

	heifdata(heifdata&&) = default;

	heifdata& operator=(const heifdata&) = default;

	//info
	uint32_t width;
	uint32_t height;
	uint32_t rows;
	uint32_t cols;
	//data
	std::vector<HevcImageFileReader::DataVector> tiles;
	std::string paramset;
};

std::string heif_info_str(const heifdata& info)noexcept {
	std::string rv;

	rv += "  width: ";
	rv += DC::STR::toString(info.width) + "\n";

	rv += "  height: ";
	rv += DC::STR::toString(info.height) + "\n";

	rv += "  rows: ";
	rv += DC::STR::toString(info.rows) + "\n";

	rv += "  cols: ";
	rv += DC::STR::toString(info.cols) + "\n";

	rv += "  tiles: ";
	rv += DC::STR::toString(info.tiles.size()) + "\n";

	return rv;
}

bool read_heif_info(heifdata& readto, HevcImageFileReader& reader, const uint32_t& contextId)noexcept {
	try {
		ImageFileReaderInterface::GridItem gridItem;
		ImageFileReaderInterface::IdVector gridItemIds;

		reader.getItemListByType(contextId, "grid", gridItemIds);
		gridItem = reader.getItemGrid(contextId, gridItemIds.at(0));
		readto.width= gridItem.outputWidth;
		readto.height = gridItem.outputHeight;
		readto.rows = gridItem.rowsMinusOne + 1;
		readto.cols = gridItem.columnsMinusOne + 1;

		return true;
	}
	catch (...) {
		return false;
	}
}

bool read_heif_paramset(heifdata& readto, HevcImageFileReader& reader, const uint32_t& contextid, const HevcImageFileReader::IdVector& ids)noexcept {
	try {
		HevcImageFileReader::ParameterSetMap paramset;
		reader.getDecoderParameterSets(contextid, ids.at(0), paramset);
		std::string paramsetstr;
		for (const auto& key : { "VPS", "SPS", "PPS" }) {
			const auto& nalu = paramset.at(key);
			paramsetstr += std::string((const char *)nalu.data(), nalu.size());
		}

		readto.paramset = std::move(paramsetstr);

		return true;
	}
	catch (...) {
		return false;
	}
}

bool read_heif_tiles(heifdata& readto, HevcImageFileReader& reader, const uint32_t& contextid, const HevcImageFileReader::IdVector& ids)noexcept {
	try {
		readto.tiles.resize(ids.size());

		decltype(readto.tiles)::size_type index = 0;

		for (const auto& p : ids) {
			reader.getItemDataWithDecoderParameters(contextid, p, readto.tiles[index]);			
			++index;
		}

		return true;
	}
	catch (...) {
		return false;
	}
}

heifdata read_heif(const std::string& filename) {
	HevcImageFileReader reader;
	reader.initialize(filename);

	heifdata returnthis;

	//const auto& properties = reader.getFileProperties();
	const uint32_t contextid = reader.getFileProperties().rootLevelMetaBoxProperties.contextId;
	HevcImageFileReader::IdVector ids;
	reader.getItemListByType(contextid, "master", ids);

	//read info
	if (!read_heif_info(returnthis, reader, contextid))
		throw std::exception("get_heif_info returned false");

	//read paramset
	if (!read_heif_paramset(returnthis, reader, contextid, ids))
		throw std::exception("get_heif_paramset returned false");

	//read tiles
	if (!read_heif_tiles(returnthis, reader, contextid, ids))
		throw std::exception("get_heif_tiles returned false");

	return returnthis;
}

inline std::string get_tile_str(const HevcImageFileReader::DataVector& tile, const std::string& paramset) {
	return paramset + std::string(reinterpret_cast<const char*>(tile.data()), tile.size());
}

//general mode use this
inline bool write_heif_as_265_SingleImage(const std::string& filename, const heifdata& datatowrite, const uint32_t& index)noexcept {
	try {
		return DC::File::write<DC::File::binary>(filename, get_tile_str(datatowrite.tiles.at(index), datatowrite.paramset));
	}
	catch (...) {
		return false;
	}
}

//iOS-11 mode use this
inline bool write_heif_as_265_AllImage(const std::string& filename, const heifdata& datatowrite)noexcept {
	try {
		uint32_t index = 0;
		for (const auto& p : datatowrite.tiles) {
			DC::File::write<DC::File::binary>(filename + DC::STR::toString(index), get_tile_str(p, datatowrite.paramset));
			++index;
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

int main(int argc, char *argv[]) {
	//general xxx.HEIC outputfilename
	//convert the first image inside xxx.HEIC to H.265 stream, and save to file outputfilename
	//example: HUD general IMG_4228.HEIC out.265

	//iOS-11 xxx.HEIC outputfilename
	//convert all image inside xxx.HEIC to H.265 stream, and save to file outputfilename
	//example: HUD iOS-11 IMG_4228.HEIC out.265.
	auto cmd_args(DC::GetCommandLineParameters(argc, argv));

	if (argc < 4) {
		std::cout << "view https://github.com/liuziangexit/HEIF-Utility-CommandLineTool for more infomation.";
		return 0;
	}

	heifdata data;

	try {
		data = read_heif(cmd_args[2]);
	}
	catch (std::exception& ex) {
		std::cout << "an uncatched exception has been throw: " << ex.what();
		return 0;
	}

	std::cout << "\n\nImage Info\n" << heif_info_str(data) << "\n\n";
	
	if (cmd_args[1] == "general") {
		auto look = write_heif_as_265_SingleImage(cmd_args[3], data, 0);
		return 0;
	}

	if (cmd_args[1] == "iOS-11") {
		auto look = write_heif_as_265_AllImage(cmd_args[3], data);
		return 0;
	}
		
}