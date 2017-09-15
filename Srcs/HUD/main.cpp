#include "hevcimagefilereader.cpp"
#include "log.hpp"

#include <iostream>
#include <sstream>
#include <conio.h>

#include "liuzianglib/liuzianglib.h"
#include "liuzianglib/DC_STR.h"
#include "liuzianglib/DC_File.h"
#include "liuzianglib/DC_jsonBuilder.h"

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
	uint32_t rotation = 0;
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

	rv += "  rotation: ";
	rv += DC::STR::toString(info.rotation) + "\n";

	rv += "  tiles: ";
	rv += DC::STR::toString(info.tiles.size());

	return rv;
}

bool write_info(const std::string& filename, const heifdata& info,const DC::PARS_V& cmd_args)noexcept {
	try {
		DC::Web::jsonBuilder::object jsobj;

		jsobj.add("filename", DC::Web::jsonBuilder::value(cmd_args.at(3)));
		jsobj.add("format", DC::Web::jsonBuilder::value("png"));
		jsobj.add("indexBegin", DC::Web::jsonBuilder::number(0));
		jsobj.add("number", DC::Web::jsonBuilder::number((int32_t)info.tiles.size()));
		jsobj.add("outputfilename", DC::Web::jsonBuilder::value(cmd_args.at(3)));
		jsobj.add("outputformat", DC::Web::jsonBuilder::value(cmd_args.at(4)));
		jsobj.add("jpgquality", DC::Web::jsonBuilder::number(DC::STR::toType<int>(cmd_args.at(6))));

		jsobj.add("width", DC::Web::jsonBuilder::number((int32_t)info.width));
		jsobj.add("height", DC::Web::jsonBuilder::number((int32_t)info.height));
		jsobj.add("rows", DC::Web::jsonBuilder::number((int32_t)info.rows));
		jsobj.add("cols", DC::Web::jsonBuilder::number((int32_t)info.cols));
		jsobj.add("rotation", DC::Web::jsonBuilder::number((int32_t)info.rotation));
		jsobj.add("tiles", DC::Web::jsonBuilder::number((int32_t)info.tiles.size()));

		return DC::File::write(filename, jsobj.toString());
	}
	catch (...) {
		return false;
	}
}

bool read_heif_info(heifdata& readto, HevcImageFileReader& reader, const uint32_t& contextId)noexcept {
	try {
		ImageFileReaderInterface::GridItem gridItem;
		ImageFileReaderInterface::IdVector gridItemIds;

		reader.getItemListByType(contextId, "grid", gridItemIds);
		gridItem = reader.getItemGrid(contextId, gridItemIds.at(0));
		readto.width = gridItem.outputWidth;
		readto.height = gridItem.outputHeight;
		readto.rows = gridItem.rowsMinusOne + 1;
		readto.cols = gridItem.columnsMinusOne + 1;

		const uint32_t itemId = gridItemIds.at(0);
		const auto itemProperties = reader.getItemProperties(contextId, itemId);
		for (const auto& property : itemProperties) {
			if (property.type == ImageFileReaderInterface::ItemPropertyType::IROT) {
				readto.rotation = reader.getPropertyIrot(contextId, property.index).rotation;
			}
		}

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

inline bool write_heif_as_GenericFormat_AllImage(const std::string& filename, const heifdata& datatowrite, const std::string& format, const std::string& startup_path)noexcept {
	try {
		uint32_t index = 0;
		for (const auto& p : datatowrite.tiles) {
			if (!DC::File::write<DC::File::binary>(filename + DC::STR::toString(index) + ".temp", get_tile_str(p, datatowrite.paramset)))
				return false;
			system((std::string("ffmpeg -loglevel panic -y -i ") + "\"" + filename + DC::STR::toString(index) + ".temp\"" + " \"" + filename + DC::STR::toString(index) + "." + "png\"").c_str());
			DC::File::del(filename + DC::STR::toString(index) + ".temp");
			++index;
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

inline bool clear_265_temp_files(const std::string& filename, const heifdata& datatowrite, const std::string& format)noexcept {
	try {
		uint32_t index = 0;
		for (const auto& p : datatowrite.tiles) {
			DC::File::del(filename + DC::STR::toString(index) + "." + "png");
			++index;
		}
		return true;
	}
	catch (...) {
		return false;
	}
}

int main(int argc, char *argv[]) {
	//iOS-11 xxx.HEIC outputfilename outputformat jsonoutputfilename jpgquality
	//convert all image inside xxx.HEIC to Generic Format, and save to file
	//example: HUD iOS-11 IMG_4228.HEIC out jpg imageinfo.json 50

	//view xxx.HEIC
	//show HEIF file info
	//example: HUD view IMG_4228.HEIC

	auto cmd_args(DC::GetCommandLineParameters(argc, argv));
	
	auto startup_path = DC::STR::getSub(cmd_args.at(0), -1, *DC::STR::find(cmd_args.at(0), "\\").getplace_ref().rbegin() + 1);

	cmd_args.at(3) = startup_path + cmd_args.at(3);
	//cmd_args.at(5) = startup_path + cmd_args.at(5);

	heifdata data;

	try {
		Log::setLevel(Log::LogLevel::ERROR);
		printf("load file...");
		data = read_heif(cmd_args.at(2));
		printf("ok\n");
	}
	catch (std::exception& ex) {
		std::cout << "an uncatched exception has been throw: " << ex.what();
		return 0;
	}
	
	if (cmd_args[1] == "view") {
		std::cout << "\n\nImage Info\n" << heif_info_str(data) << "\n";
		return 0;
	}

	if (cmd_args[1] == "iOS-11") {
		printf("write temp files...");
		if (write_heif_as_GenericFormat_AllImage(cmd_args.at(3), data, cmd_args.at(4), startup_path))
			printf("ok\n");
		else {
			printf("err\n");
			return 0;
		}
		printf("blending files...");
		if (write_info(cmd_args.at(5), data, cmd_args))
			system((std::string("HBT ") + "\"" + startup_path + cmd_args.at(5) + "\"").c_str());
		printf("ok\n");
		printf("clear temp files...");
		clear_265_temp_files(cmd_args.at(3), data, cmd_args.at(4));
		DC::File::del(cmd_args.at(5));
		printf("ok\n");

		return 0;
	}

	return 0;
}