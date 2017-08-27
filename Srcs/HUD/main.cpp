#include "hevcimagefilereader.cpp"
#include <iostream>

int extract(const char* srcfile, const char* dstfile)
{
	std::cout << "convert " << srcfile << " to " << dstfile << std::endl;

	HevcImageFileReader reader;
	reader.initialize(srcfile);
	const auto& props = reader.getFileProperties();
	const uint32_t contextId = props.rootLevelMetaBoxProperties.contextId;

	HevcImageFileReader::IdVector ids;
	reader.getItemListByType(contextId, "master", ids);
	const uint32_t itemId = ids[0];
	std::cout << "itemId=" << itemId << std::endl;

	HevcImageFileReader::ParameterSetMap paramset;
	reader.getDecoderParameterSets(contextId, itemId, paramset);

	HevcImageFileReader::DataVector bitstream;
	reader.getItemDataWithDecoderParameters(contextId, itemId, bitstream);

	std::ofstream ofs(dstfile, std::ios::binary);
	for (const auto& key : { "VPS", "SPS", "PPS" }) {
		const auto& nalu = paramset[key];
		std::cout << key << " len=" << nalu.size() << std::endl;
		ofs.write((const char *)nalu.data(), nalu.size());
	}
	std::cout << "bitstream=" << bitstream.size() << std::endl;
	ofs.write((const char *)bitstream.data(), bitstream.size());

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 3) {
		std::cout
			<< "Usage: heic2hevc <input.heic> <output.265>" << std::endl;
		return 0;
	}
	return extract(argv[1], argv[2]);
}