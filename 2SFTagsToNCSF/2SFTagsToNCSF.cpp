/*
 * 2SF Tags to NCSF
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-04-10
 *
 * Version history:
 *   v1.0 - 2013-03-30 - Initial version
 *   v1.1 - 2012-03-30 - Added option to rename the NCSFs, requested by Knurek.
 *   v1.2 - 2012-04-10 - Made it so a file is not overwritten when renaming if
 *                       a duplicate is found.
 */

#include <tuple>
#include "NCSF.h"

static const std::string TWOSFTAGSTONCSF_VERSION = "1.1";

enum { UNKNOWN, HELP, VERBOSE, EXCLUDETAG, RENAME };
const option::Descriptor opts[] =
{
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "2SF Tags to NCSF v" + TWOSFTAGSTONCSF_VERSION + "\nBy Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"2SF Tags to NCSF will copy tags from a 2SF set and place them into the tags of an NCSF set..\n\n"
		"Usage:\n"
		"  2SFTagsToNCSF [options] <Input 2SF directory> <Output NCSF directory>\n\n"
		"Options:"),
	option::Descriptor(HELP, 0, "h", "help", option::Arg::None, "  --help,-h \tPrint usage and exit."),
	option::Descriptor(VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose,-v \tVerbose output."),
	option::Descriptor(EXCLUDETAG, 0, "x", "exclude", RequireArgument, "  --exclude=<tag> \v         -x <tag> \tExclude the given tag from the tags to copy."),
	option::Descriptor(RENAME, 0, "r", "rename", option::Arg::None, "  --rename,-r \tRenames the NCSFs to match the 2SFs."),
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "\nVerbose output will output the tags that were copied."),
	option::Descriptor()
};

typedef std::map<std::string, std::tuple<uint16_t, const SSEQ *, TagList>> TwoSFs;
typedef std::map<std::string, std::pair<uint32_t, TagList>> NCSFs;

int main(int argc, char *argv[])
{
	// Options parsing
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

	// Get the tags to exclude
	std::vector<std::string> tagsToExclude;
	for (option::Option *opt = options[EXCLUDETAG]; opt; opt = opt->next())
		tagsToExclude.push_back(opt->arg);

	std::string twoSFDirectory = parse.nonOption(0);
	std::replace(twoSFDirectory.begin(), twoSFDirectory.end(), '\\', '/');
	std::string NCSFDirectory = parse.nonOption(1);
	std::replace(NCSFDirectory.begin(), NCSFDirectory.end(), '\\', '/');

	std::string twoSFExtensions[] = { ".2sf", ".mini2sf", ".2sflib" };
	auto twoSFExtensionsVector = std::vector<std::string>(twoSFExtensions, twoSFExtensions + 3);
	Files twoSFFiles = GetFilesInDirectory(twoSFDirectory, twoSFExtensionsVector);

	std::map<std::string, SDAT> twoSFSDATs;
	TwoSFs twoSFs;
	// Get the tags and sdats from the 2SFs
	std::for_each(twoSFFiles.begin(), twoSFFiles.end(), [&](const std::string &filename)
	{
		try
		{
			PseudoReadFile fileData;
			fileData.GetDataFromFile(filename);

			auto programSection = GetProgramSectionFromPSF(fileData, 0x24, 8, 4, true);
			TagList tags = GetTagsFromPSF(fileData, 0x24);
			if (tags.Exists("_lib"))
			{
				uint16_t SSEQNumber = ReadLE<uint16_t>(&programSection[8]);
				twoSFs.insert(std::make_pair(filename, std::make_tuple(SSEQNumber, nullptr, tags)));
			}
			else
			{
				PseudoReadFile romFileData((filename));
				romFileData.GetDataFromVector(programSection.begin() + 8, programSection.end());

				uint8_t sdatSignature[] = { 0x53, 0x44, 0x41, 0x54, 0xFF, 0xFE, 0x00, 0x01 };
				std::vector<uint8_t> sdatSignatureVector(sdatSignature, sdatSignature + 8);
				romFileData.pos = 0;
				romFileData.startOffset = romFileData.GetNextOffset(0, sdatSignatureVector);

				SDAT sdat;
				sdat.Read(filename, romFileData, false);
				std::string filenameMinusPath = filename.substr(twoSFDirectory.size());
				if (twoSFDirectory[twoSFDirectory.size() - 1] != '/')
					filenameMinusPath.erase(0);
				twoSFSDATs.insert(std::make_pair(filenameMinusPath, sdat));
			}
		}
		catch (const std::exception &e)
		{
			std::cerr << "ERROR: " << e.what() << "\n";
		}
	});

	if (twoSFSDATs.empty())
	{
		std::cerr << "ERROR: No SDAT found in the 2SFs, for some reason...\n";
		return 1;
	}

	// For the 2SFs that had a _lib tag, get their SSEQ from the SDAT
	std::for_each(twoSFs.begin(), twoSFs.end(), [&](TwoSFs::value_type &twoSF)
	{
		uint16_t SSEQNumber = std::get<0>(twoSF.second);
		const SSEQ *&sseq = std::get<1>(twoSF.second);
		const TagList &tags = std::get<2>(twoSF.second);
		const SDAT &sdat = twoSFSDATs.find(tags["_lib"])->second;
		sseq = sdat.infoSection.SEQrecord.entries[SSEQNumber].sseq;
	});

	std::string ncsfExtensions[] = { ".ncsf", ".minincsf", ".ncsflib" };
	auto ncsfExtensionsVector = std::vector<std::string>(ncsfExtensions, ncsfExtensions + 3);
	Files ncsfFiles = GetFilesInDirectory(NCSFDirectory, ncsfExtensionsVector);

	SDAT ncsfSDAT;
	NCSFs ncsfs;
	// Get the tags and SDAT for the NCSFs
	std::for_each(ncsfFiles.begin(), ncsfFiles.end(), [&](const std::string &filename)
	{
		try
		{
			PseudoReadFile fileData;
			fileData.GetDataFromFile(filename);

			auto programSection = GetProgramSectionFromPSF(fileData, 0x25, 12, 8);
			TagList tags = GetTagsFromPSF(fileData, 0x25);
			// If the program section is empty, this is a minincsf
			if (programSection.empty())
			{
				uint32_t SSEQNumber = ReadLE<uint32_t>(&(*fileData.data.get())[16]);
				ncsfs.insert(std::make_pair(filename, std::make_pair(SSEQNumber, tags)));
			}
			// Otherwise it is either an ncsf or an ncsflib
			else
			{
				PseudoReadFile sdatFileData((filename));
				sdatFileData.GetDataFromVector(programSection.begin(), programSection.end());

				ncsfSDAT.Read(filename, sdatFileData);
				if (!tags.Empty())
					ncsfs.insert(std::make_pair(filename, std::make_pair(0, tags)));
			}
		}
		catch (const std::exception &)
		{
		}
	});
	// Copy the tag data from the 2SFs to the NCSFs
	std::for_each(ncsfs.begin(), ncsfs.end(), [&](const NCSFs::value_type &ncsf)
	{
		uint32_t SSEQNumber = ncsf.second.first;
		const SSEQ *sseq = ncsfSDAT.infoSection.SEQrecord.entries[SSEQNumber].sseq;

		auto twoSF = std::find_if(twoSFs.begin(), twoSFs.end(), [&](const TwoSFs::value_type &item)
		{
			return sseq->data == std::get<1>(item.second)->data;
		});
		if (twoSF != twoSFs.end())
		{
			std::string filename = ncsf.first;
			if (!!options[RENAME])
			{
				auto dot = twoSF->first.rfind('.');
				filename = twoSF->first.substr(0, dot);
				filename = filename.substr(twoSFDirectory.size());
				if (twoSFDirectory[twoSFDirectory.size() - 1] != '/')
					filename.erase(0);
				if (NCSFDirectory[NCSFDirectory.size() - 1] != '/')
					filename = "/" + filename;
				filename = NCSFDirectory + filename;
				if (FileExists(filename + ".minincsf"))
					for (unsigned i = 1; ; ++i)
					{
						std::string istr = stringify(i);
						if (!FileExists(filename + "_Duplicate" + istr + ".minincsf"))
						{
							filename += "_Duplicate" + istr;
							break;
						}
					}
				filename += ".minincsf";
				remove(ncsf.first.c_str());
			}
			if (!!options[VERBOSE])
				std::cout << "Copying tags from " << twoSF->first << "\n  to " << filename << "\n";

			TagList twoSFTags = std::get<2>(twoSF->second);
			twoSFTags.Remove("_lib");
			twoSFTags.Remove("2sfby");
			twoSFTags.Remove("length");
			twoSFTags.Remove("fade");
			std::for_each(tagsToExclude.begin(), tagsToExclude.end(), [&](const std::string &tag) { twoSFTags.Remove(tag); });

			TagList ncsfTags = ncsf.second.second;
			ncsfTags.CopyOverwriteExistingOnly(twoSFTags);

			auto reservedData = IntToLEVector<uint32_t>(SSEQNumber);

			MakeNCSF(filename, reservedData, std::vector<uint8_t>(), ncsfTags.GetTags());
		}
	});

	return 0;
}
