/*
 * SDAT Strip
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-15
 *
 * NOTE: This version has been superceded by NDS to NCSF instead.
 *
 * Version history:
 *   v1.0 - 2013-03-25 - Initial version
 *   v1.1 - 2013-03-26 - Merged verbosity of SDAT stripping into the SDAT class
 *                       and removed the static Strip function from this.
 *                     - Copied NDS to NCSF's include/exclude handling to here.
 */

#include <map>
#include "SDAT.h"

static const std::string SDATSTRIP_VERSION = "1.1";

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
		"\n\nExcluded and included files will be processed in the order they are given on the command line, later arguments overriding earlier arguments.  If there is more "
			"than 1 SDAT contained within the NDS ROM, you can exclude or include based on the SDAT by prefixing the filename with the SDAT number (1-based) and a forward "
			"slash.  For example, if the NDS ROM has 2 SDATs and both contain a file called XYZ, but you only want XYZ from the 2nd SDAT, use 1/XYZ as an exclude.  Wildcards "
			"before the forward slash are also accepted."),
	option::Descriptor()
};

int main(int argc, char *argv[])
{
	argc -= argc > 0;
	argv += argc > 0;
	option::Stats stats(opts, argc, argv);
	std::vector<option::Option> options(stats.options_max), buffer(stats.buffer_max);
	option::Parser parse(opts, argc, argv, &options[0], &buffer[0]);

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

	// Get exclude and include filenames from the command line
	IncOrExc includesAndExcludes;
	for (int i = 0, optionCount = parse.optionsCount(); i < optionCount; ++i)
	{
		const option::Option &opt = buffer[i];
		if (opt.index() == EXCLUDE)
			includesAndExcludes.push_back(KeepInfo(opt.arg, KEEP_EXCLUDE));
		else if (opt.index() == INCLUDE)
			includesAndExcludes.push_back(KeepInfo(opt.arg, KEEP_INCLUDE));
	}

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

			PseudoReadFile fileData;
			fileData.GetDataFromFile(inputFilenames[i]);

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
		finalSDAT.Strip(includesAndExcludes, !!options[VERBOSE]);

		std::ofstream file;
		file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		file.open(outputFilename.c_str(), std::ofstream::out | std::ofstream::binary);

		PseudoWrite ofile(&file);

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
