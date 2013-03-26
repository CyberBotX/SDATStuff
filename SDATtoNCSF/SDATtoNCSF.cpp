/*
 * SDAT to NCSF
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * NOTE: This version has been superceded by NDS to NCSF instead.  It also lacks
 *       some of the features that are in NDS to NCSF.
 *
 * Version history:
 *   v1.0 - Initial version
 */

#include <iostream>
#include "NCSF.h"
#include "optionparser.h"

static const std::string SDATTONCSF_VERSION = "1.0";

enum { UNKNOWN, HELP, VERBOSE, TIME };
const option::Descriptor opts[] =
{
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "SDAT to NCSF v" + SDATTONCSF_VERSION + "\nBy Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"SDAT to NCSF will take the incoming SDAT and create a series of NCSF files.  If there is only a single SSEQ within the SDAT, then there will be a "
			"single NCSF file.  Otherwise, there will be an NCSFLIB and multiple MININCSFs.\n\n"
		"Usage:\n"
		"  SDATtoNCSF [options] <Input SDAT filename>\n\n"
		"Options:"),
	option::Descriptor(HELP, 0, "h", "help", option::Arg::None, "  --help,-h \tPrint usage and exit."),
	option::Descriptor(VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose,-v \tVerbose output."),
	option::Descriptor(TIME, 0, "t", "time", option::Arg::None, "  --time,-t \tCalculate time on each track."),
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "\nVerbose output will output the NCSFs created.\n\nTiming uses code based on FeOS Sound System by fincs."),
	option::Descriptor()
};

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

	if (options[HELP] || !argc || parse.nonOptionsCount() < 1)
	{
		option::printUsage(std::cout, opts);
		return 0;
	}

	try
	{
		// Read SDAT
		std::string sdatFilename = parse.nonOption(0);
		std::replace(sdatFilename.begin(), sdatFilename.end(), '\\', '/');

		if (!FileExists(sdatFilename))
			throw std::runtime_error("File " + sdatFilename + " does not exist.");

		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		file.open(sdatFilename.c_str(), std::ifstream::in | std::ifstream::binary);

		PseudoReadFile fileData((sdatFilename));
		fileData.GetDataFromFile(file);

		file.close();

		// Create output directory
		std::string dirName = sdatFilename;
		size_t dot = dirName.rfind('.');
		dirName = dirName.substr(0, dot) + "_SDATtoNCSF";

		if (!DirExists(dirName))
			MakeDir(dirName);
		if (options[VERBOSE])
			std::cout << "Output will go to " << dirName << "\n";

		// Parse SDAT
		SDAT sdat;
		sdat.Read(sdatFilename, fileData);

		if (sdat.infoSection.SEQrecord.entries.size() == 1)
		{
			// Make single NCSF
			TagList tags;
			tags["ncsfby"] = "SDAT to NCSF";

			std::string ncsfFilename = sdat.infoSection.SEQrecord.entries[0].sseq->filename + ".ncsf";
			auto reservedData = IntToLEVector<uint32_t>(0);

			if (options[TIME])
				GetTime(ncsfFilename, &sdat, sdat.infoSection.SEQrecord.entries[0].sseq, tags, !!options[VERBOSE]);

			MakeNCSF(dirName + "/" + ncsfFilename, reservedData, *fileData.data.get(), tags.GetTags());
			if (options[VERBOSE])
				std::cout << "Created " << ncsfFilename << "\n";
		}
		else
		{
			// Make NCSFLIB
			std::string ncsflibFilename = GetFilenameFromPath(sdatFilename);
			size_t libdot = ncsflibFilename.rfind('.');
			ncsflibFilename = ncsflibFilename.substr(0, libdot) + ".ncsflib";
			MakeNCSF(dirName + "/" + ncsflibFilename, std::vector<uint8_t>(), *fileData.data.get());
			if (options[VERBOSE])
				std::cout << "Created " << ncsflibFilename << "\n";

			// Make multiple MININCSFs
			TagList tags;
			tags["_lib"] = ncsflibFilename;
			tags["ncsfby"] = "SDAT to NCSF";

			for (size_t i = 0; i < sdat.infoSection.SEQrecord.count; ++i)
			{
				if (!sdat.infoSection.SEQrecord.entryOffsets[i])
					continue;
				std::string minincsfFilename = sdat.infoSection.SEQrecord.entries[i].sseq->filename + ".minincsf";
				auto reservedData = IntToLEVector<uint32_t>(i);

				TagList thisTags = tags;
				thisTags["origFilename"] = sdat.infoSection.SEQrecord.entries[i].sseq->origFilename;

				if (options[TIME])
					GetTime(minincsfFilename, &sdat, sdat.infoSection.SEQrecord.entries[i].sseq, thisTags, !!options[VERBOSE]);

				MakeNCSF(dirName + "/" + minincsfFilename, reservedData, std::vector<uint8_t>(), thisTags.GetTags());
				if (options[VERBOSE])
					std::cout << "Created " << minincsfFilename << "\n";
			}
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
