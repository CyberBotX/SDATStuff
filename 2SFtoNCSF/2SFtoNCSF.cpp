/*
 * 2SF to NCSF
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-11-12
 *
 * Version history:
 *   v1.0 - 2013-03-30 - Initial version
 */

#include <tuple>
#include "NCSF.h"

static const std::string TWOSFTONCSF_VERSION = "1.0";

enum { UNKNOWN, HELP, VERBOSE, TIME, FADELOOP, FADEONESHOT, EXCLUDETAG };
const option::Descriptor opts[] =
{
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "2SF to NCSF v" + TWOSFTONCSF_VERSION + "\nBy Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"2SF to NCSF will create an NCSF set from a corresponding 2SF set.\n\n"
		"Usage:\n"
		"  2SFtoNCSF [options] <Input 2SF directory> [<Output NCSFLIB filename>]\n\n"
		"Options:"),
	option::Descriptor(HELP, 0, "h", "help", option::Arg::None, "  --help,-h \tPrint usage and exit."),
	option::Descriptor(VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose,-v \tVerbose output."),
	option::Descriptor(TIME, 0, "t", "time", RequireNumericArgument,
		"  --time,-t \tCalculate time on each track to the number of loops given. Defaults to 2 loops. 0 will disable timing."),
	option::Descriptor(FADELOOP, 0, "l", "fade-loop", RequireNumericArgument, "  --fade-loop,-l \tSet the fade time for looping tracks, in seconds, defaults to 10."),
	option::Descriptor(FADEONESHOT, 0, "o", "fade-one-shot", RequireNumericArgument, "  --fade-one-shot,-o \tSet the fade time for one-shot tracks, in seconds, defaults to 0."),
	option::Descriptor(EXCLUDETAG, 0, "x", "exclude", RequireArgument, "  --exclude=<tag> \v         -x <tag> \tExclude the given tag from the tags to copy."),
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None,
		"\nThis tool only works with 2SF sets created with Caitsith2's Legacy of Ys driver, and not older sets such as those using the Yoshi's Island DS driver."
		"\n\nIf the output NCSFLIB filename is not given, attempts to infer the filename will be made."
		"\n\nVerbose output will output the tags that were copied."
		"\n\nRegardless of which tags are excluded on the command line, the _lib, 2sfby, length and fade tags will always be excluded from the conversion process."
		"\n\nThis tool simply converts an existing 2SF set to an NCSF set. If you want more control over the process, it is recommended that you use NDStoNCSF on an NDS ROM."),
	option::Descriptor()
};

typedef std::map<std::string, std::tuple<uint16_t, TagList>> TwoSFs;

int main(int argc, char *argv[])
{
	// Options parsing
	argc -= argc > 0;
	argv += argc > 0;
	option::Stats stats(opts, argc, argv);
	std::vector<option::Option> options(stats.options_max), buffer(stats.buffer_max);
	option::Parser parse(opts, argc, argv, &options[0], &buffer[0]);

	if (parse.error())
		return 1;

	if (options[HELP] || !argc || parse.nonOptionsCount() < 1)
	{
		option::printUsage(std::cout, opts);
		return 0;
	}

	// Get the tags to exclude
	std::vector<std::string> tagsToExclude;
	for (option::Option *opt = options[EXCLUDETAG]; opt; opt = opt->next())
		tagsToExclude.push_back(opt->arg);

	uint32_t numberOfLoops = 2;
	if (options[TIME])
		numberOfLoops = convertTo<uint32_t>(options[TIME].arg);
	uint32_t fadeLoop = 10;
	if (options[FADELOOP])
		fadeLoop = convertTo<uint32_t>(options[FADELOOP].arg);
	uint32_t fadeOneShot = 1;
	if (options[FADEONESHOT])
		fadeOneShot = convertTo<uint32_t>(options[FADEONESHOT].arg);

	std::string twoSFDirectory = parse.nonOption(0);
	std::replace(twoSFDirectory.begin(), twoSFDirectory.end(), '\\', '/');
	std::string ncsflibFilename = "";
	if (parse.nonOptionsCount() >= 2)
		ncsflibFilename = parse.nonOption(1);

	std::string twoSFExtensions[] = { ".2sf", ".mini2sf", ".2sflib" };
	auto twoSFExtensionsVector = std::vector<std::string>(twoSFExtensions, twoSFExtensions + 3);
	Files twoSFFiles = GetFilesInDirectory(twoSFDirectory, twoSFExtensionsVector);
	std::sort(twoSFFiles.begin(), twoSFFiles.end());

	std::map<std::string, SDAT> twoSFSDATs;
	TwoSFs twoSFs;
	uint8_t sdatSignature[] = { 0x53, 0x44, 0x41, 0x54, 0xFF, 0xFE, 0x00, 0x01 };
	std::vector<uint8_t> sdatSignatureVector(sdatSignature, sdatSignature + 8);
	// Get the tags and sdats from the 2SFs
	std::for_each(twoSFFiles.begin(), twoSFFiles.end(), [&](const std::string &filename)
	{
		if (!!options[VERBOSE])
			std::cout << "Processing " << filename << "\n";
		try
		{
			PseudoReadFile fileData;
			fileData.GetDataFromFile(filename);

			auto programSection = GetProgramSectionFromPSF(fileData, 0x24, 8, 4, true);
			TagList tags = GetTagsFromPSF(fileData, 0x24);
			if (tags.Exists("_lib"))
			{
				if (programSection.empty())
					throw std::runtime_error("This 2SF had no program section!");
				uint16_t SSEQNumber = ReadLE<uint16_t>(&programSection[8]);
				twoSFs.insert(std::make_pair(filename, std::make_tuple(SSEQNumber, tags)));
			}
			else
			{
				PseudoReadFile romFileData(filename);
				romFileData.GetDataFromVector(programSection.begin() + 8, programSection.end());

				char gameNameArray[12];
				romFileData.ReadLE(gameNameArray);
				std::string gameName = std::string(gameNameArray, gameNameArray + 12);
				if (gameName != "LEGACY OF YS")
					throw std::runtime_error("This tool only works on the Legacy of Ys ROM, but I got '" + gameName + "' instead.");

				romFileData.pos = 0;
				romFileData.startOffset = romFileData.GetNextOffset(0, sdatSignatureVector);

				SDAT sdat;
				sdat.Read(filename, romFileData, false);
				std::string filenameMinusPath = GetFilenameFromPath(filename);
				twoSFSDATs.insert(std::make_pair(filenameMinusPath, sdat));
				if (ncsflibFilename.empty() && tags.Empty())
				{
					size_t dot = filenameMinusPath.rfind('.');
					ncsflibFilename = filenameMinusPath.substr(0, dot) + ".ncsflib";
				}
				if (!tags.Empty())
				{
					uint16_t SSEQNumber = ReadLE<uint16_t>(&programSection[0x0d0fc8]);
					twoSFs.insert(std::make_pair(filename, std::make_tuple(SSEQNumber, tags)));
				}
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

	if (twoSFs.empty())
	{
		std::cerr << "ERROR: No SSEQs were found to convert.\n";
		return 1;
	}

	if (twoSFs.size() > 1 && ncsflibFilename.empty())
	{
		std::cerr << "ERROR: Unable to infer filename to use for NCSFLIB. Specify it on the command line.\n";
		return 1;
	}

	// Create a merged SDAT from the sequences that were in the 2SF set
	if (options[VERBOSE])
		std::cout << "Building merged SDAT...\n";
	SDAT finalSDAT;
	int32_t sdatNumber = 0;
	std::for_each(twoSFs.begin(), twoSFs.end(), [&](TwoSFs::value_type &twoSF)
	{
		const TagList &tags = std::get<1>(twoSF.second);
		std::string filenameMinusPath = GetFilenameFromPath(twoSF.first);
		const SDAT &sdat = twoSFSDATs.find(tags.Exists("_lib") ? tags["_lib"] : filenameMinusPath)->second;
		SDAT newSDAT = sdat.MakeFromSSEQ(std::get<0>(twoSF.second));
		newSDAT.filename = stringify(sdatNumber++ + 1);
		newSDAT.infoSection.SEQrecord.entries[0].sdatNumber = twoSF.first;
		finalSDAT += newSDAT;
	});

	finalSDAT.count = 1;
	finalSDAT.Strip(IncOrExc(), options[VERBOSE].count() > 1);

	// Setup the output directory, making sure it is clear beforehand (if it exists)
	std::string NCSFDirectory = twoSFDirectory;
	if (twoSFDirectory[twoSFDirectory.size() - 1] == '/')
		NCSFDirectory = NCSFDirectory.substr(0, twoSFDirectory.size() - 1);
	NCSFDirectory += "_2SFtoNCSF";
	if (DirExists(NCSFDirectory))
	{
		std::string extensions[] = { ".ncsf", ".minincsf", ".ncsflib" };
		auto extensionsVector = std::vector<std::string>(extensions, extensions + 3);
		Files files = GetFilesInDirectory(NCSFDirectory, extensionsVector);
		RemoveFiles(files);
	}
	else
		MakeDir(NCSFDirectory);
	if (options[VERBOSE])
		std::cout << "Output will go to " << NCSFDirectory << "\n";

	// Create vector data for SDAT
	PseudoWrite sdatData;
	finalSDAT.Write(sdatData);

	bool singleNCSF = finalSDAT.infoSection.SEQrecord.count == 1;
	if (!singleNCSF)
	{
		// Make NCSFLIB if we are creating more than one NCSF
		MakeNCSF(NCSFDirectory + "/" + ncsflibFilename, std::vector<uint8_t>(), sdatData.vector->data);
		if (options[VERBOSE])
			std::cout << "Created " << ncsflibFilename << "\n";
	}
	for (size_t i = 0, sseqs = finalSDAT.infoSection.SEQrecord.count; i < sseqs; ++i)
	{
		std::string origFilename = finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber;
		TagList tags = std::get<1>(twoSFs.find(origFilename)->second);
		tags.Remove("_lib");
		tags.Remove("2sfby");
		tags.Remove("length");
		tags.Remove("fade");
		std::for_each(tagsToExclude.begin(), tagsToExclude.end(), [&](const std::string &tag) { tags.Remove(tag); });
		tags["ncsfby"] = "2SF to NCSF";
		tags["origFilename"] = finalSDAT.infoSection.SEQrecord.entries[i].sseq->origFilename;
		if (!singleNCSF)
			tags["_lib"] = ncsflibFilename;

		std::string filename = GetFilenameFromPath(origFilename);
		size_t dot = filename.rfind('.');
		filename = filename.substr(0, dot) + (singleNCSF ? ".ncsf" : ".minincsf");
		std::vector<uint8_t> programData;
		if (singleNCSF)
			programData = sdatData.vector->data;

		auto reservedData = IntToLEVector<uint32_t>(i);

		if (numberOfLoops)
			GetTime(filename, &finalSDAT, finalSDAT.infoSection.SEQrecord.entries[i].sseq, tags, !!options[VERBOSE], numberOfLoops, fadeLoop, fadeOneShot);

		MakeNCSF(NCSFDirectory + "/" + filename, reservedData, programData, tags.GetTags());
		if (options[VERBOSE])
			std::cout << "Created " << filename << "\n";
	}

	return 0;
}
