//nokiatech-heif
#include "hevcimagefilereader.cpp"
#include "log.hpp"

//opencv
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/opencv.hpp>

//std
#include <sstream>
#include <exception>

//liuzianglib
#include "liuzianglib/liuzianglib.h"
#include "liuzianglib/DC_STR.h"
#include "liuzianglib/DC_File.h"

struct heifdata {
	heifdata() = default;

	heifdata(const heifdata&) = default;

	heifdata(heifdata&&) = default;

	heifdata& operator=(const heifdata&) = default;

	heifdata& operator=(heifdata&&) = default;

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

std::string heif_info_str(const heifdata& info) {
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
				break;
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
		decltype(readto.tiles)::size_type index = 0;
		readto.tiles.resize(ids.size());
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

heifdata read_heif(const std::string& heic_bin) {
	std::istringstream iss(heic_bin);

	HevcImageFileReader reader;
	reader.initialize(iss);

	heifdata returnthis;

	const uint32_t& contextid = reader.getFileProperties().rootLevelMetaBoxProperties.contextId;
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

bool write_hevc_bitstream(const std::string& filename, const heifdata& data)noexcept {
	try {
		std::string str;
		uint32_t reserve_to = data.paramset.size()*data.tiles.size();
		for (const auto& p : data.tiles)
			reserve_to += p.size();
		str.reserve(reserve_to + 128);//直接reserve到reserve_to的时候发现循环的时候还是经历了一次reserve，所以不如多分配一点避免循环中reserve

		for (const auto& p : data.tiles)
			str += get_tile_str(p, data.paramset);

		return DC::File::write<DC::File::binary>(filename, str);
	}
	catch (...) {
		return false;
	}
}

std::vector<cv::Mat> read_hevc_bitstream_to_mat_vector(const std::string& filename)noexcept {
	try {
		cv::VideoCapture vcap(filename);
		vcap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', 'E', 'V', 'C'));
		if (!vcap.isOpened())
			throw std::exception("cv::VideoCapture::isOpened returned false");

		std::vector<cv::Mat> returnvalue;
		cv::Mat frame;
		returnvalue.reserve(48);//48是4032*3024分辨率时候的图块数

		while (true) {
			if (!vcap.read(frame))
				break;
			returnvalue.push_back(frame.clone());
		}

		return returnvalue;
	}
	catch (...) {
		return std::vector<cv::Mat>();
	}
}

cv::Mat hconcatLine(const std::vector<cv::Mat>& images) {
	cv::Mat rv;
	cv::hconcat(images.at(0), images.at(1), rv);
	for (uint32_t i = 2; i < images.size(); i++)
		cv::hconcat(rv, images.at(i), rv);
	return rv;
}

cv::Mat vconcatLine(const std::vector<cv::Mat>& images) {
	cv::Mat rv;
	cv::vconcat(images.at(0), images.at(1), rv);
	for (uint32_t i = 2; i < images.size(); i++)
		cv::vconcat(rv, images.at(i), rv);
	return rv;
}

cv::Mat resize(const cv::Mat& image, const heifdata& info) {
	IplImage img(image);
	cvSetImageROI(&img, CvRect(0, 0, info.width, info.height));
	return cv::Mat(cv::cvarrToMat(&img, true));
}

void rotate(cv::Mat& image, const heifdata& info) {
	if (info.rotation % 90 != 0 || info.rotation == 0)
		return;

	auto rotate90 = [&image] {
		cv::transpose(image, image);
		cv::flip(image, image, 0);
	};

	for (int i = 0; i < info.rotation / 90; i++)
		rotate90();
}

extern "C" __declspec(dllexport) void heif2jpg(const char heif_bin[], int input_buffer_size, const int jpg_quality, char output_buffer[], int output_buffer_size, const char* input_temp_filename) {
	const char* temp_filename = input_temp_filename;
	auto copy_to_output_buffer = [&output_buffer, &output_buffer_size](const std::string& source) {
		if (output_buffer_size < source.size())
			return false;

		memset(output_buffer, 0, output_buffer_size);
		memcpy(output_buffer, source.data(), source.size());
		return true;
	};

	//拿到heic文件二进制数据
	//get the binary data of heic file
	std::string heif_bin_str(heif_bin, input_buffer_size);

	Log::setLevel(Log::LogLevel::ERROR);
	heifdata data;

	//解析到data里
	//read that
	try {
		data = read_heif(heif_bin_str);
	}
	catch (...) {
		copy_to_output_buffer("error");
		return;
	}

	//把heic文件里所有的图块转成hevc裸流写到临时文件里（文件名由temp_filename指定）
	//convert all tiles inside heic image to a hevc bitstream,and write bitstream to a temp file(the name of temp file is specified by temp_filename)
	if (!write_hevc_bitstream(temp_filename, data)) {
		copy_to_output_buffer("error");
		return;
	}

	//读取临时文件并提取中其中所有的帧(图块)
	//read the temp file and get all tiles inside
	auto mat_vec = read_hevc_bitstream_to_mat_vector(temp_filename);
	//DC::File::del(temp_filename);
	if (mat_vec.empty()) {
		copy_to_output_buffer("error");
		return;
	}

	std::string encoded_jpg;
	try {
		//拼合
		//piece
		auto getLineIndex = [&data](const uint32_t& line) {
			return line * data.cols;
		};
		std::vector<cv::Mat> lines, temp;
		temp.reserve(data.cols);
		lines.reserve(data.rows);

		for (uint32_t i = 0; i < data.rows; i++) {
			for (uint32_t u = getLineIndex(i); u < getLineIndex(i) + data.cols; u++)
				temp.push_back(mat_vec.at(u));
			lines.push_back(hconcatLine(temp));
			temp.clear();
		}

		//剪裁
		//clip
		auto fullImage(resize(vconcatLine(lines), data));

		//旋转
		//rotate
		rotate(fullImage, data);

		//编码为jpg
		//encoded as jpg
		std::vector<uint8_t> encoded_buf;
		if (!cv::imencode(".jpg", fullImage, encoded_buf, { CV_IMWRITE_JPEG_QUALITY, jpg_quality })) {
			copy_to_output_buffer("error");
			return;
		}
		encoded_jpg = std::string(encoded_buf.begin(), encoded_buf.end());
	}
	catch (...) {
		copy_to_output_buffer("error");
		return;
	}

	if (!copy_to_output_buffer(encoded_jpg))
		copy_to_output_buffer("buffer to small");
}