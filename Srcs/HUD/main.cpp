#include "hevcimagefilereader.cpp"
#include <iostream>

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

		std::ofstream ofs(dstfile, std::ios::binary);
		for (const auto& key : { "VPS", "SPS", "PPS" }) {
			const auto& nalu = paramset[key];
			ofs.write((const char *)nalu.data(), nalu.size());
		}
		ofs.write((const char *)bitstream.data(), bitstream.size());
	}
	catch (...) {
		return false;
	}
	return true;
}

int main(int argc, char* argv[]) {
	if (argc < 3)
		return 1;
	if (extract(argv[1], argv[2]))
		return 0;
	return 1;
}