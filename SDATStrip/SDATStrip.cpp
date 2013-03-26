/*
 * SDAT Strip
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * NOTE: This version has been superceded by NDS to NCSF instead.
 *
 * Version history:
 *   v1.0 - Initial version
 */

#include <iostream>
#include <map>
#include <cctype>
#include "SDAT.h"
#include "optionparser.h"

static const std::string SDATSTRIP_VERSION = "1.0";

static inline option::ArgStatus RequireArgument(const option::Option &opt, bool msg)
{
	if (opt.arg && *opt.arg)
		return option::ARG_OK;

	if (msg)
		std::cerr << "Option '" << std::string(opt.name).substr(0, opt.namelen) << "' requires a non-empty argument.\n";
	return option::ARG_ILLEGAL;
}

enum { UNKNOWN, HELP, VERBOSE, FORCE, EXCLUDE, INCLUDE };
const option::Descriptor opts[] =
{
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "SDAT Strip v" + SDATSTRIP_VERSION + "\nBy Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"SDAT Strip will remove all duplicated SSEQs, SBNKs, and SWARs from the input SDAT.  It will also keep only SBNKs used by the remaining SSEQs and keep "
			"only SWARs used by the remaining SBNKs.  SSARs and STRMs are not kept.  Any gaps in the SYMB/INFO sections of the SDAT will also be removed.\n\n"
		"Usage:\n"
		"  SDATStrip [options] <Input SDAT filename> [...] <Output SDAT filename>\n"
		"  (More than one input file can be given, they will be merged into the output.)\n\n"
		"Options:"),
	option::Descriptor(HELP, 0, "h", "help", option::Arg::None, "  --help,-h \tPrint usage and exit."),
	option::Descriptor(VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose,-v \tVerbose output."),
	option::Descriptor(FORCE, 0, "f", "force", option::Arg::None, "  --force,-f \tForce overwrite of output file if it is the same as the input file."),
	option::Descriptor(EXCLUDE, 0, "x", "exclude", RequireArgument,
		"  --exclude=<filename> \v         -x <filename> \tExclude the given filename from the final SDAT.  May use * and ? wildcards."),
	option::Descriptor(INCLUDE, 0, "i", "include", RequireArgument,
		"  --include=<filename> \v         -i <filename> \tInclude the given filename in the final SDAT.  May use * and ? wildcards."),
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None,
		"\nVerbose output will tell you which SSEQs, SBNKS, and SWARs will be kept and which will be removed due to duplicates."
		"\n\nIncluded files will always override excluded files."),
	option::Descriptor()
};

typedef std::map<uint32_t, std::vector<uint32_t> > Duplicates;

// This is to find a value in a vector out of a map
struct FindInVector : std::binary_function<Duplicates::value_type, uint32_t, bool>
{
	bool operator()(const Duplicates::value_type &a, uint32_t b) const
	{
		return std::find(a.second.begin(), a.second.end(), b) != a.second.end();
	}
};

// Returns the non-duplicate number of an SBNK or SWAR
static inline uint16_t GetNonDupNumber(uint16_t orig, const Duplicates &duplicates)
{
	if (duplicates.count(orig))
		return orig;
	auto duplicate = std::find_if(duplicates.begin(), duplicates.end(), std::bind2nd(FindInVector(), orig));
	if (duplicate != duplicates.end())
		return duplicate->first;
	return orig;
}

// Output a vector with comma separation
template<typename T> static inline void OutputVector(const std::vector<T> &vec, const std::string &outputPrefix = " ", size_t columnWidth = 80)
{
	std::string output = outputPrefix;
	for (size_t i = 0, count = vec.size(); i < count; ++i)
	{
		std::string keep = stringify(vec[i]);
		if (output.size() + keep.size() > columnWidth)
		{
			std::cout << output << "\n";
			output = "   ";
		}
		output += " " + keep + ",";
	}
	if (!output.empty())
	{
		output.erase(output.end() - 1);
		std::cout << output << "\n";
	}
}

// Output a map with the vector being comma separation
template<typename T, typename U> static inline void OutputMap(const std::map<T, U> &map, size_t columnWidth = 80)
{
	for (auto curr = map.begin(), end = map.end(); curr != end; ++curr)
	{
		std::string output = "  " + stringify(curr->first) + ":";
		OutputVector(curr->second, output, columnWidth);
	}
}

static inline bool CompareAllWildcards(const std::string &tameText, const std::vector<std::string> &wildTexts)
{
	for (size_t i = 0, count = wildTexts.size(); i < count; ++i)
		if (WildcardCompare(tameText, wildTexts[i]))
			return true;

	return false;
}

static inline bool IncludeFilename(const std::string &filename, const std::vector<std::string> &excludeFilenames, const std::vector<std::string> &includeFilenames)
{
	bool include = true;
	if (!excludeFilenames.empty() && CompareAllWildcards(filename, excludeFilenames))
		include = false;
	if (!include && !includeFilenames.empty() && CompareAllWildcards(filename, includeFilenames))
		include = true;
	return include;
}

void Strip(SDAT &sdat, bool verbose, const std::vector<std::string> &excludeFilenames, const std::vector<std::string> &includeFilenames)
{
	// Search for duplicate SWARs
	Duplicates duplicateSWARs;

	for (size_t i = 0, entries = sdat.infoSection.WAVEARCrecord.entries.size(); i < entries; ++i)
	{
		if (!sdat.infoSection.WAVEARCrecord.entryOffsets[i])
			continue;
		auto alreadyFound = std::find_if(duplicateSWARs.begin(), duplicateSWARs.end(), std::bind2nd(FindInVector(), i));
		if (alreadyFound != duplicateSWARs.end()) // Already added as a duplicate of another SWAR, skip it
			continue;
		uint16_t ifileID = sdat.infoSection.WAVEARCrecord.entries[i].fileID;
		uint32_t ifileSize = sdat.fatSection.records[ifileID].size;
		const auto &ifileData = sdat.infoSection.WAVEARCrecord.entries[i].fileData;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!sdat.infoSection.WAVEARCrecord.entryOffsets[j])
				continue;
			uint16_t jfileID = sdat.infoSection.WAVEARCrecord.entries[j].fileID;
			uint32_t jfileSize = sdat.fatSection.records[jfileID].size;
			if (ifileSize != jfileSize) // Files sizes are different, not duplicates, skip it
				continue;
			if (ifileData != sdat.infoSection.WAVEARCrecord.entries[j].fileData) // File data is different, not duplicates, skip it
				continue;
			duplicates.push_back(j);
		}
		if (!duplicates.empty())
			duplicateSWARs.insert(std::make_pair(i, duplicates));
	}

	// Search for duplicate SBNKs
	Duplicates duplicateSBNKs;

	for (size_t i = 0, entries = sdat.infoSection.BANKrecord.entries.size(); i < entries; ++i)
	{
		if (!sdat.infoSection.BANKrecord.entryOffsets[i])
			continue;
		auto alreadyFound = std::find_if(duplicateSBNKs.begin(), duplicateSBNKs.end(), std::bind2nd(FindInVector(), i));
		if (alreadyFound != duplicateSBNKs.end()) // Already added as a duplicate of another SBNK, skip it
			continue;
		uint16_t ifileID = sdat.infoSection.BANKrecord.entries[i].fileID;
		uint32_t ifileSize = sdat.fatSection.records[ifileID].size;
		auto iwaveArc = std::vector<uint16_t>(4, 0xFFFF);
		for (int k = 0; k < 4; ++k)
		{
			uint16_t waveArc = sdat.infoSection.BANKrecord.entries[i].waveArc[k];
			if (waveArc != 0xFFFF)
				iwaveArc[k] = GetNonDupNumber(waveArc, duplicateSWARs);
		}
		const auto &ifileData = sdat.infoSection.BANKrecord.entries[i].fileData;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!sdat.infoSection.BANKrecord.entryOffsets[j])
				continue;
			uint16_t jfileID = sdat.infoSection.BANKrecord.entries[j].fileID;
			uint32_t jfileSize = sdat.fatSection.records[jfileID].size;
			auto jwaveArc = std::vector<uint16_t>(4, 0xFFFF);
			for (int k = 0; k < 4; ++k)
			{
				uint16_t waveArc = sdat.infoSection.BANKrecord.entries[j].waveArc[k];
				if (waveArc != 0xFFFF)
					jwaveArc[k] = GetNonDupNumber(waveArc, duplicateSWARs);
			}
			if (ifileSize != jfileSize) // File sizes are different, not duplicates, skip it
				continue;
			if (ifileData != sdat.infoSection.BANKrecord.entries[j].fileData) // File data is different, not duplicates, skip it
				continue;
			if (iwaveArc != jwaveArc) // Wave archives are different, not duplicates, skip it
				continue;
			duplicates.push_back(j);
		}
		if (!duplicates.empty())
			duplicateSBNKs.insert(std::make_pair(i, duplicates));
	}

	// Search for duplicate SSEQs, as well as ones that the user requested to exclude
	Duplicates duplicateSSEQs;
	std::vector<uint32_t> excludedSSEQs;

	for (size_t i = 0, entries = sdat.infoSection.SEQrecord.entries.size(); i < entries; ++i)
	{
		if (!sdat.infoSection.SEQrecord.entryOffsets[i])
			continue;
		auto alreadyFound = std::find_if(duplicateSSEQs.begin(), duplicateSSEQs.end(), std::bind2nd(FindInVector(), i));
		if (alreadyFound != duplicateSSEQs.end()) // Already added as a duplicate of another SSEQ, skip it
			continue;

		uint16_t ifileID = sdat.infoSection.SEQrecord.entries[i].fileID;
		std::string ifilename = "SSEQ" + NumToHexString(ifileID).substr(2);
		if (sdat.SYMBOffset)
			ifilename = sdat.symbSection.SEQrecord.entries[i];
		if (!IncludeFilename(ifilename, excludeFilenames, includeFilenames))
		{
			excludedSSEQs.push_back(i);
			continue;
		}

		uint32_t ifileSize = sdat.fatSection.records[ifileID].size;
		uint16_t inonDupBank = GetNonDupNumber(sdat.infoSection.SEQrecord.entries[i].bank, duplicateSBNKs);
		const auto &ifileData = sdat.infoSection.SEQrecord.entries[i].fileData;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!sdat.infoSection.SEQrecord.entryOffsets[j])
				continue;
			uint16_t jfileID = sdat.infoSection.SEQrecord.entries[j].fileID;
			uint32_t jfileSize = sdat.fatSection.records[jfileID].size;
			uint16_t jnonDupBank = GetNonDupNumber(sdat.infoSection.SEQrecord.entries[j].bank, duplicateSBNKs);
			if (ifileSize != jfileSize) // File sizes are different, not duplicates, skip it
				continue;
			if (ifileData != sdat.infoSection.SEQrecord.entries[j].fileData) // File data is different, not duplicates, skip it
				continue;
			if (inonDupBank != jnonDupBank) // Banks are different, not duplicates, skip it
				continue;
			duplicates.push_back(j);
		}
		if (!duplicates.empty())
			duplicateSSEQs.insert(std::make_pair(i, duplicates));
	}

	// Determine which SSEQs to keep
	std::vector<uint32_t> SSEQsToKeep;

	for (size_t i = 0, entries = sdat.infoSection.SEQrecord.entries.size(); i < entries; ++i)
	{
		if (!sdat.infoSection.SEQrecord.entryOffsets[i])
			continue;
		auto duplicate = std::find_if(duplicateSSEQs.begin(), duplicateSSEQs.end(), std::bind2nd(FindInVector(), i));
		if (duplicate != duplicateSSEQs.end())
			continue;
		if (std::find(excludedSSEQs.begin(), excludedSSEQs.end(), i) != excludedSSEQs.end())
			continue;
		SSEQsToKeep.push_back(i);
	}

	// Determine which SBNKs to keep and are being used by the SSEQs we are keeping
	std::vector<uint32_t> SBNKsToKeep;

	for (size_t i = 0, count = SSEQsToKeep.size(); i < count; ++i)
	{
		uint16_t bank = sdat.infoSection.SEQrecord.entries[SSEQsToKeep[i]].bank;
		uint16_t nonDupBank = GetNonDupNumber(bank, duplicateSBNKs);
		if (std::find(SBNKsToKeep.begin(), SBNKsToKeep.end(), nonDupBank) != SBNKsToKeep.end())
			continue;
		SBNKsToKeep.push_back(nonDupBank);
	}

	std::sort(SBNKsToKeep.begin(), SBNKsToKeep.end());

	// Determine which SWARs to keep and are being used by the SBNKs we are keeping
	std::vector<uint32_t> SWARsToKeep;

	for (size_t i = 0, count = SBNKsToKeep.size(); i < count; ++i)
		for (int j = 0; j < 4; ++j)
		{
			uint16_t waveArc = sdat.infoSection.BANKrecord.entries[SBNKsToKeep[i]].waveArc[j];
			if (waveArc == 0xFFFF)
				continue;
			uint16_t nonDupWaveArc = GetNonDupNumber(waveArc, duplicateSWARs);
			if (std::find(SWARsToKeep.begin(), SWARsToKeep.end(), nonDupWaveArc) != SWARsToKeep.end())
				continue;
			SWARsToKeep.push_back(nonDupWaveArc);
		}

	std::sort(SWARsToKeep.begin(), SWARsToKeep.end());

	// If verbosity is turned out, output which files are being kept and which are being removed for being duplicates
	if (verbose)
	{
		std::cout << "The following " << SSEQsToKeep.size() << " SSEQ" << (SSEQsToKeep.size() != 1 ? "s" : "") << " will be kept:\n";
		OutputVector(SSEQsToKeep);

		std::cout << "\nThe following " << SBNKsToKeep.size() << " SBNK" << (SBNKsToKeep.size() != 1 ? "s" : "") << " will be kept:\n";
		OutputVector(SBNKsToKeep);

		std::cout << "\nThe following " << SWARsToKeep.size() << " SWAR" << (SWARsToKeep.size() != 1 ? "s" : "") << " will be kept:\n";
		OutputVector(SWARsToKeep);

		if (!excludedSSEQs.empty())
		{
			std::cout << "\nThe following SSEQ" << (excludedSSEQs.size() != 1 ? "s" : "") << " were excluded by request:\n";
			OutputVector(excludedSSEQs);
		}

		if (!duplicateSSEQs.empty())
		{
			std::cout << "\nThe following SSEQ" << (duplicateSSEQs.size() != 1 ? "s" : "") << " had duplicates, the duplicates will be removed:\n";
			OutputMap(duplicateSSEQs);
		}

		if (!duplicateSBNKs.empty())
		{
			std::cout << "\nThe following SBNK" << (duplicateSBNKs.size() != 1 ? "s" : "") << " had duplicates, the duplicates will be removed:\n";
			OutputMap(duplicateSBNKs);
		}

		if (!duplicateSWARs.empty())
		{
			std::cout << "\nThe following SWAR" << (duplicateSWARs.size() != 1 ? "s" : "") << " had duplicates, the duplicates will be removed:\n";
			OutputMap(duplicateSWARs);
		}
	}

	// Figure out where the remaining SBNKs will be once moved
	std::map<uint32_t, uint32_t> SBNKMove;

	for (size_t i = 0, count = SBNKsToKeep.size(); i < count; ++i)
		SBNKMove[SBNKsToKeep[i]] = i;

	// Figure out where the remaining SWARs will be once moved
	std::map<uint32_t, uint32_t> SWARMove;

	for (size_t i = 0, count = SWARsToKeep.size(); i < count; ++i)
		SWARMove[SWARsToKeep[i]] = i;

	// Remove all unused items (or rather, save only used items)
	SYMBSection newSymbSection;
	if (sdat.SYMBOffset)
	{
		newSymbSection.SEQrecord.count = SSEQsToKeep.size();
		newSymbSection.SEQrecord.entryOffsets.resize(newSymbSection.SEQrecord.count);
		newSymbSection.SEQrecord.entries.resize(newSymbSection.SEQrecord.count);

		newSymbSection.BANKrecord.count = SBNKsToKeep.size();
		newSymbSection.BANKrecord.entryOffsets.resize(newSymbSection.BANKrecord.count);
		newSymbSection.BANKrecord.entries.resize(newSymbSection.BANKrecord.count);

		newSymbSection.WAVEARCrecord.count = SWARsToKeep.size();
		newSymbSection.WAVEARCrecord.entryOffsets.resize(newSymbSection.WAVEARCrecord.count);
		newSymbSection.WAVEARCrecord.entries.resize(newSymbSection.WAVEARCrecord.count);
	}

	INFOSection newInfoSection;

	newInfoSection.SEQrecord.count = SSEQsToKeep.size();
	newInfoSection.SEQrecord.entryOffsets.resize(newInfoSection.SEQrecord.count);
	newInfoSection.SEQrecord.entries.resize(newInfoSection.SEQrecord.count);

	newInfoSection.BANKrecord.count = SBNKsToKeep.size();
	newInfoSection.BANKrecord.entryOffsets.resize(newInfoSection.BANKrecord.count);
	newInfoSection.BANKrecord.entries.resize(newInfoSection.BANKrecord.count);

	newInfoSection.WAVEARCrecord.count = SWARsToKeep.size();
	newInfoSection.WAVEARCrecord.entryOffsets.resize(newInfoSection.WAVEARCrecord.count);
	newInfoSection.WAVEARCrecord.entries.resize(newInfoSection.WAVEARCrecord.count);

	uint16_t fileID = 0;
	for (size_t i = 0, count = SSEQsToKeep.size(); i < count; ++i)
	{
		if (sdat.SYMBOffset)
			newSymbSection.SEQrecord.entries[i] = sdat.symbSection.SEQrecord.entries[SSEQsToKeep[i]];

		newInfoSection.SEQrecord.entries[i] = sdat.infoSection.SEQrecord.entries[SSEQsToKeep[i]];
		newInfoSection.SEQrecord.entries[i].fileID = fileID++;
		uint16_t bank = newInfoSection.SEQrecord.entries[i].bank;
		uint16_t nonDupBank = GetNonDupNumber(bank, duplicateSBNKs);
		newInfoSection.SEQrecord.entries[i].bank = SBNKMove[nonDupBank];
	}

	for (size_t i = 0, count = SBNKsToKeep.size(); i < count; ++i)
	{
		if (sdat.SYMBOffset)
			newSymbSection.BANKrecord.entries[i] = sdat.symbSection.BANKrecord.entries[SBNKsToKeep[i]];

		newInfoSection.BANKrecord.entries[i] = sdat.infoSection.BANKrecord.entries[SBNKsToKeep[i]];
		newInfoSection.BANKrecord.entries[i].fileID = fileID++;
		for (int j = 0; j < 4; ++j)
		{
			uint16_t waveArc = newInfoSection.BANKrecord.entries[i].waveArc[j];
			if (waveArc == 0xFFFF)
				continue;
			uint16_t nonDupWaveArc = GetNonDupNumber(waveArc, duplicateSWARs);
			newInfoSection.BANKrecord.entries[i].waveArc[j] = SWARMove[nonDupWaveArc];
		}
	}

	for (size_t i = 0, count = SWARsToKeep.size(); i < count; ++i)
	{
		if (sdat.SYMBOffset)
			newSymbSection.WAVEARCrecord.entries[i] = sdat.symbSection.WAVEARCrecord.entries[SWARsToKeep[i]];

		newInfoSection.WAVEARCrecord.entries[i] = sdat.infoSection.WAVEARCrecord.entries[SWARsToKeep[i]];
		newInfoSection.WAVEARCrecord.entries[i].fileID = fileID++;
	}

	if (sdat.SYMBOffset)
		sdat.symbSection = newSymbSection;
	sdat.infoSection = newInfoSection;

	FATSection newFatSection;

	newFatSection.count = fileID;
	newFatSection.records.resize(newFatSection.count);

	sdat.fatSection = newFatSection;

	// Calculate new offsets and sizes
	sdat.INFOOffset = 0x40;
	if (sdat.SYMBOffset)
	{
		sdat.SYMBSize = sdat.symbSection.size = sdat.symbSection.Size();
		sdat.symbSection.FixOffsets();
		sdat.INFOOffset = sdat.SYMBOffset + ((sdat.SYMBSize + 3) & ~0x03);
	}
	sdat.INFOSize = sdat.infoSection.size = sdat.infoSection.Size();
	sdat.infoSection.FixOffsets();
	sdat.FATOffset = sdat.INFOOffset + sdat.INFOSize;
	sdat.FATSize = sdat.fatSection.size = sdat.fatSection.Size();
	sdat.FILEOffset = sdat.FATOffset + sdat.FATSize;
	uint32_t offset = sdat.FILEOffset + 24;
	sdat.FILESize = 0;
	fileID = 0;
	for (uint32_t i = 0, count = SSEQsToKeep.size(); i < count; ++i)
	{
		sdat.fatSection.records[fileID].offset = offset;
		uint32_t fileSize = sdat.infoSection.SEQrecord.entries[i].fileData.size();
		sdat.fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		sdat.FILESize += fileSize;
	}
	for (uint32_t i = 0, count = SBNKsToKeep.size(); i < count; ++i)
	{
		sdat.fatSection.records[fileID].offset = offset;
		uint32_t fileSize = sdat.infoSection.BANKrecord.entries[i].fileData.size();
		sdat.fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		sdat.FILESize += fileSize;
	}
	for (uint32_t i = 0, count = SWARsToKeep.size(); i < count; ++i)
	{
		sdat.fatSection.records[fileID].offset = offset;
		uint32_t fileSize = sdat.infoSection.WAVEARCrecord.entries[i].fileData.size();
		sdat.fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		sdat.FILESize += fileSize;
	}

	sdat.header.fileSize = sdat.FILEOffset + sdat.FILESize + 0x18;

	// If one of the files that was merged into this one had no SYMB section, then we need to fill in some dummy data for those entries
	if (sdat.symbSectionNeedsCleanup)
	{
		for (uint32_t i = 0, count = SSEQsToKeep.size(); i < count; ++i)
		{
			if (!sdat.symbSection.SEQrecord.entries[i].empty())
				continue;
			fileID = sdat.infoSection.SEQrecord.entries[i].fileID;
			sdat.symbSection.SEQrecord.entries[i] = "SSEQ" + NumToHexString(fileID).substr(2);
		}
		for (uint32_t i = 0, count = SBNKsToKeep.size(); i < count; ++i)
		{
			if (!sdat.symbSection.BANKrecord.entries[i].empty())
				continue;
			fileID = sdat.infoSection.BANKrecord.entries[i].fileID;
			sdat.symbSection.BANKrecord.entries[i] = "SBNK" + NumToHexString(fileID).substr(2);
		}
		for (uint32_t i = 0, count = SWARsToKeep.size(); i < count; ++i)
		{
			if (!sdat.symbSection.WAVEARCrecord.entries[i].empty())
				continue;
			fileID = sdat.infoSection.WAVEARCrecord.entries[i].fileID;
			sdat.symbSection.WAVEARCrecord.entries[i] = "SWAR" + NumToHexString(fileID).substr(2);
		}
	}
}

int main(int argc, char *argv[])
{
	argc -= argc > 0;
	argv += argc > 0;
	option::Stats stats((opts), (argc), (argv));
	std::vector<option::Option> options((stats.options_max)), buffer((stats.buffer_max));
	option::Parser parse((opts), (argc), (argv), (&options[0]), (&buffer[0]));

	if (parse.error())
		return 1;

	if (options[HELP] || !argc || parse.nonOptionsCount() < 2)
	{
		option::printUsage(std::cout, opts);
		return 0;
	}

	std::vector<std::string> inputFilenames;
	for (int i = 0, inputs = parse.nonOptionsCount() - 1; i < inputs; ++i)
		inputFilenames.push_back(parse.nonOption(i));
	std::string outputFilename = parse.nonOption(parse.nonOptionsCount() - 1);

	std::vector<std::string> excludeFilenames, includeFilenames;
	for (option::Option *opt = options[EXCLUDE]; opt; opt = opt->next())
		excludeFilenames.push_back(opt->arg);
	for (option::Option *opt = options[INCLUDE]; opt; opt = opt->next())
		includeFilenames.push_back(opt->arg);

	SDAT finalSDAT;

	size_t failed = 0;
	for (size_t i = 0, count = inputFilenames.size(); i < count; ++i)
	{
		if (inputFilenames[i] == outputFilename && !options[FORCE])
		{
			std::cerr << "One of the inputs is the same as the output!  Aborting...\n";
			std::cerr << "(If you wish to force this, use -f or --force.)\n";
			return 1;
		}

		try
		{
			if (!FileExists(inputFilenames[i]))
				throw std::runtime_error("File does not exist.");

			std::ifstream file;
			file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			file.open(inputFilenames[i].c_str(), std::ifstream::in | std::ifstream::binary);

			PseudoReadFile fileData((inputFilenames[i]));
			fileData.GetDataFromFile(file);

			file.close();

			SDAT sdat;
			sdat.Read(inputFilenames[i], fileData);
			finalSDAT += sdat;
			std::cout << "Appended " << inputFilenames[i] << " to final SDAT.\n";
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error with " << inputFilenames[i] << ": " << e.what() << "\n";
			++failed;
		}
	}

	if (failed == inputFilenames.size())
	{
		std::cerr << "Unable to create " << outputFilename << ",\n  all inputs failed to load.\n";
		return 1;
	}

	try
	{
		Strip(finalSDAT, !!options[VERBOSE], excludeFilenames, includeFilenames);

		std::ofstream file;
		file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		file.open(outputFilename.c_str(), std::ofstream::out | std::ofstream::binary);

		PseudoWrite ofile((&file));

		finalSDAT.Write(ofile);
		std::cout << "Output written to " << outputFilename << "\n";
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error with " << outputFilename << ": " << e.what() << "\n";
		return 1;
	}

	return 0;
}
