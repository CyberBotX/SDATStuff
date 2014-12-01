/*
 * NDS to NCSF
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-11-12
 *
 * Version history:
 *   v1.0 - 2013-03-25 - Initial version
 *   v1.1 - 2013-03-26 - Merged SDAT Strip's verbosity into the SDAT class'
 *                       Strip function.
 *                     - Modified how excluded SSEQs are handled when stripping.
 *                     - Corrected handling of files within an existing SDAT.
 *   v1.2 - 2013-03-28 - Made timing to be on by default, with 2 loops.
 *                     - Added options to change the fade times.
 *   v1.3 - 2013-03-30 - Only remove files from the destination directory that
 *                       were created by this utility, instead of all files.
 *                     - Slightly better file checking when copying from an
 *                       existing SDAT, will check by data only if checking by
 *                       filename and data doesn't give any results.
 *   v1.4 - 2014-10-15 - Improved timing system by implementing the random,
 *                       variable, and conditional SSEQ commands.
 *   v1.5 - 2014-10-26 - Save the PLAYER blocks in the SDATs as opposed to
 *                       stripping them.
 *                     - Detect if the NDS ROM is a DSi ROM and set the prefix
 *                       of the NCSFLIB accordingly.
 *                     - Fixed removal of output directory if there are no
 *                       SSEQs found.
 *   v1.6 - 2014-11-07 - Added functionality for an SMAP-like file to be used
 *                       to include/exclude SSEQs.
 */

#include <iomanip>
#include "NCSF.h"

static const std::string NDSTONCSF_VERSION = "1.6";

enum { UNKNOWN, HELP, VERBOSE, TIME, FADELOOP, FADEONESHOT, EXCLUDE, INCLUDE, AUTO, CREATE_SMAP, USE_SMAP, NOCOPY };
const option::Descriptor opts[] =
{
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "NDS to NCSF v" + NDSTONCSF_VERSION + "\nBy Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"NDS to NCSF will take the incoming NDS ROM and create a series of NCSF files. If there is only a single SSEQ within the entire ROM, then there will be a "
			"single NCSF file. Otherwise, there will be an NCSFLIB and multiple MININCSFs.\n\n"
		"Usage:\n"
		"  NDStoNCSF [options] <Input SDAT filename>\n\n"
		"Options:"),
	option::Descriptor(HELP, 0, "h", "help", option::Arg::None, "  --help,-h \tPrint usage and exit."),
	option::Descriptor(VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose,-v \tVerbose output."),
	option::Descriptor(TIME, 0, "t", "time", RequireNumericArgument,
		"  --time,-t \tCalculate time on each track to the number of loops given. Defaults to 2 loops. 0 will disable timing."),
	option::Descriptor(FADELOOP, 0, "l", "fade-loop", RequireNumericArgument, "  --fade-loop,-l \tSet the fade time for looping tracks, in seconds, defaults to 10."),
	option::Descriptor(FADEONESHOT, 0, "o", "fade-one-shot", RequireNumericArgument, "  --fade-one-shot,-o \tSet the fade time for one-shot tracks, in seconds, defaults to 0."),
	option::Descriptor(EXCLUDE, 0, "x", "exclude", RequireArgument,
		"  --exclude=<filename> \v         -x <filename> \tExclude the given filename from the final SDAT. May use * and ? wildcards."),
	option::Descriptor(INCLUDE, 0, "i", "include", RequireArgument,
		"  --include=<filename> \v         -i <filename> \tInclude the given filename in the final SDAT. May use * and ? wildcards."),
	option::Descriptor(AUTO, 0, "a", "auto", option::Arg::None, "  --auto,-a \tFully automatic mode (disables interactive mode)."),
	option::Descriptor(CREATE_SMAP, 0, "s", "create-smap", RequireArgument,
		"  --create-smap=<filename> \v             -s <filename> \tCreates an SMAP-like file that can later be used with the --use-smap/-S option."),
	option::Descriptor(USE_SMAP, 0, "S", "use-smap", RequireArgument,
		"  --use-smap=<filename> \v          -S <filename> \tUses the given SMAP-like file to determine what files to include/exclude."),
	option::Descriptor(NOCOPY, 0, "n", "nocopy", option::Arg::None, "  --nocopy,-n \tDo not check for previous files in the destination directory."),
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None,
		"\nVerbose output will output the NCSFs created. If given more than once, verbose output will also output duplicates found during the SDAT stripping step."
		"\n\nExcluded and included files will be processed in the order they are given on the command line, later arguments overriding earlier arguments. If there is more "
			"than 1 SDAT contained within the NDS ROM, you can exclude or include based on the SDAT by prefixing the filename with the SDAT number (1-based) and a forward "
			"slash. For example, if the NDS ROM has 2 SDATs and both contain a file called XYZ, but you only want XYZ from the 2nd SDAT, use 1/XYZ as an exclude. Wildcards "
			"before the forward slash are also accepted."
		"\n\nIf --auto or -a are not given, the program will run in interactive mode, asking to confirm which sequences to keep. (NOTE: Even in interactive mode, files "
			"which were excluded or included on the command line will still be automatically set as such.)"
		"\n\nThe --create-smap or -s and --use-smap or -S options are meant to be used in tandum. Using --create-smap or -s will generate a specialized SMAP file. Lines in "
			"this file starting with a '#' symbol are considered comments. Files can be excluded by commenting the line out. Once that has been done, the SMAP file can be "
			"used with --use-smap or -S. The SMAP file is created relative to the output directory. NOTE: The short forms of the options are case-sensitive, so -s is not the "
			"same as -S. Command line exclusions and inclusions, as well as automatic mode, are not allowed when these commands are used."
		"\n\nIf --nocopy or -n are not given, the program will use information from a previous run of NDS to NCSF, if any exists. This will set files that were not in the "
			"previous run's SDAT as being excluded by default and it will also attempt to copy tags from the previous files. (NOTE: This may not work if the original "
			"SDAT did not contain a symbol record, mainly because filename matching cannot be done.)"
		"\n\nTiming uses code based on FeOS Sound System by fincs, as well as code from DeSmuME for pseudo-playback."),
	option::Descriptor()
};

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

	if (options[CREATE_SMAP] || options[USE_SMAP])
	{
		if (!includesAndExcludes.empty())
		{
			std::cerr << "Error: Command line exclusions and inclusions are not allowed when working with SMAP files.\n";
			return 1;
		}
		if (options[AUTO])
		{
			std::cerr << "Error: Fully automatic mode is not allowed when working with SMAP files.\n";
			return 1;
		}
	}

	uint32_t numberOfLoops = 2;
	if (options[TIME])
		numberOfLoops = convertTo<uint32_t>(options[TIME].arg);
	uint32_t fadeLoop = 10;
	if (options[FADELOOP])
		fadeLoop = convertTo<uint32_t>(options[FADELOOP].arg);
	uint32_t fadeOneShot = 1;
	if (options[FADEONESHOT])
		fadeOneShot = convertTo<uint32_t>(options[FADEONESHOT].arg);

	try
	{
		// Read NDS ROM
		std::string ndsFilename = parse.nonOption(0);
		std::replace(ndsFilename.begin(), ndsFilename.end(), '\\', '/');

		if (!FileExists(ndsFilename))
			throw std::runtime_error("File " + ndsFilename + " does not exist.");

		PseudoReadFile fileData;
		fileData.GetDataFromFile(ndsFilename);

		// Setup the output directory, making sure it is clear beforehand (if it
		// exists and we aren't being told not to copy the old data, then we'll
		// get all that data first)
		std::string dirName = ndsFilename;
		size_t dot = dirName.rfind('.');
		dirName = dirName.substr(0, dot) + "_NDStoNCSF";

		std::map<std::string, TagList> savedTags;
		std::map<std::string, std::string> filenames;
		std::multimap<std::string, SSEQ> oldSDATFiles;
		if (DirExists(dirName))
		{
			std::string extensions[] = { ".ncsf", ".minincsf", ".ncsflib" };
			auto extensionsVector = std::vector<std::string>(extensions, extensions + 3);
			Files files = GetFilesInDirectory(dirName, extensionsVector);

			if (!options[NOCOPY])
				for (auto curr = files.begin(), end = files.end(); curr != end; ++curr)
				{
					try
					{
						PseudoReadFile ncsfFileData;
						ncsfFileData.GetDataFromFile(*curr);

						if (curr->rfind(".ncsf") != std::string::npos || curr->rfind(".ncsflib") != std::string::npos)
						{
							auto sdatVector = GetProgramSectionFromPSF(ncsfFileData, 0x25, 12, 8);
							if (sdatVector.empty())
								throw std::runtime_error("Program section for " + *curr + " was empty.");

							PseudoReadFile sdatFileData(*curr);
							sdatFileData.GetDataFromVector(sdatVector.begin(), sdatVector.end());

							SDAT sdat;
							sdat.Read(*curr, sdatFileData);
							if (sdat.SYMBOffset)
								for (uint32_t i = 0; i < sdat.symbSection.SEQrecord.count; ++i)
									oldSDATFiles.insert(std::make_pair(sdat.symbSection.SEQrecord.entries[i], *sdat.infoSection.SEQrecord.entries[i].sseq));
						}
						if (curr->rfind(".ncsf") != std::string::npos || curr->rfind(".minincsf") != std::string::npos)
						{
							std::string filename = GetFilenameFromPath(*curr);
							TagList tags = GetTagsFromPSF(ncsfFileData, 0x25);
							// If 2SF to NCSF was used, don't use the tags for this file at all,
							// they might not be valid for use with NDS to NCSF's purposes.
							if (tags.Exists("ncsfby") && tags["ncsfby"] != "2SF to NCSF")
							{
								if (tags.Exists("origFilename"))
								{
									std::string fullOrigFilename = tags["origFilename"];
									if (tags.Exists("origSDAT"))
										fullOrigFilename = tags["origSDAT"] + "/" + fullOrigFilename;
									savedTags[fullOrigFilename] = tags;
									filenames[fullOrigFilename] = filename;
								}
								else
									savedTags[filename] = tags;
							}
						}
					}
					catch (const std::exception &)
					{
					}
				}

			// Only remove the files if we are not creating an SMAP
			if (!options[CREATE_SMAP])
				RemoveFiles(files);
		}
		else
			MakeDir(dirName);
		if (options[VERBOSE])
			std::cout << "Output will go to " << dirName << "\n";

		// Get game code
		fileData.pos = 0x0C;
		char gameCodeArray[4];
		fileData.ReadLE(gameCodeArray);
		std::string gameCode = std::string(gameCodeArray, gameCodeArray + 4);

		// Get if the ROM is a DSi ROM or not
		fileData.pos = 0x180;
		bool DSi = fileData.ReadLE<uint32_t>() == 0x8D898581u;
		DSi = DSi || fileData.ReadLE<uint32_t>() == 0x8C888480u;

		// Search for SDATs and merge them into one
		SDAT finalSDAT;
		if (options[VERBOSE])
			std::cout << "Searching for SDATs...\n";

		uint8_t sdatSignature[] = { 0x53, 0x44, 0x41, 0x54, 0xFF, 0xFE, 0x00, 0x01 };
		std::vector<uint8_t> sdatSignatureVector(sdatSignature, sdatSignature + 8);
		int32_t sdatOffset, sdatNumber = 0;
		uint32_t previousOffset = 0;
		while ((sdatOffset = fileData.GetNextOffset(previousOffset, sdatSignatureVector)) != -1)
		{
			try
			{
				fileData.pos = 0;
				fileData.startOffset = sdatOffset;
				SDAT sdat;
				sdat.Read(stringify(sdatNumber++ + 1), fileData);
				finalSDAT += sdat;
				if (options[VERBOSE])
					std::cout << "Found SDAT with " << sdat.infoSection.SEQrecord.actualCount << " SSEQ" << (sdat.infoSection.SEQrecord.actualCount == 1 ? "" : "s") << ".\n";
			}
			catch (const std::exception &)
			{
				--sdatNumber;
			}
			fileData.startOffset = 0;
			previousOffset = sdatOffset + 1;
		}

		// Fail if we do not have any SSEQs (which could also mean that there were no SDATs in the ROM or it wasn't an NDS ROM)
		if (!finalSDAT.infoSection.SEQrecord.count)
		{
			rmdir(dirName.c_str());
			throw std::range_error("Either there were no SSEQs within the SDATs of given NDS ROM, no SDATs in\n  the ROM, or the file was not an NDS ROM.");
		}

		if (options[CREATE_SMAP])
		{
			// Create an SMAP-like file
			std::string smapFilename = dirName + "/" + options[CREATE_SMAP].arg;

			std::ofstream smapFile(smapFilename.c_str());
			smapFile << "# NOTE: This SMAP is not identical to SMAPs generated by other tools.\n";
			smapFile << "#       It is meant for use with NDStoNCSF.\n\n";
			smapFile << "# To exclude an SSEQ from the final NCSF set, you can put a '#' symbol\n";
			smapFile << "# in front of the SSEQ's label.\n\n";
			if (!finalSDAT.SYMBOffset)
				smapFile << "# NOTE: This SDAT did not have a SYMB section. Labels were generated.\n\n";
			smapFile << "# SEQ:\n";
			size_t max_length = std::max_element(finalSDAT.infoSection.SEQrecord.entries.begin(), finalSDAT.infoSection.SEQrecord.entries.end(),
				[](const INFOEntrySEQ &a, const INFOEntrySEQ &b) { return a.origFilename.size() < b.origFilename.size(); })->origFilename.size();
			if (max_length < 26)
				max_length = 26;
			smapFile << "# " << std::left << std::setw(max_length) << "label" << "number ";
			if (sdatNumber > 1)
				smapFile << "SDAT# ";
			smapFile << "fileID bnk vol cpr ppr ply       size name\n";
			for (size_t i = 0; i < finalSDAT.infoSection.SEQrecord.count; ++i)
			{
				if (finalSDAT.infoSection.SEQrecord.entryOffsets[i])
				{
					const auto &entry = finalSDAT.infoSection.SEQrecord.entries[i];
					smapFile << "  " << std::left << std::setw(max_length) << entry.origFilename;
					smapFile << std::right << std::setw(6) << i;
					if (sdatNumber > 1)
						smapFile << std::setw(6) << entry.sdatNumber;
					smapFile << std::setw(7) << entry.fileID;
					smapFile << std::setw(4) << entry.bank;
					smapFile << std::setw(4) << static_cast<int>(entry.vol);
					smapFile << std::setw(4) << static_cast<int>(entry.cpr);
					smapFile << std::setw(4) << static_cast<int>(entry.ppr);
					smapFile << std::setw(4) << static_cast<int>(entry.ply);
					smapFile << std::setw(11) << finalSDAT.fatSection.records[entry.fileID].size;
					smapFile << " \\Seq\\" << entry.origFilename << ".sseq\n";
				}
				else
					smapFile << std::right << std::setw(max_length + 8) << i << "\n";
			}

			std::cout << "Created SMAP: " << smapFilename << "\n";
			return 0;
		}

		if (options[USE_SMAP])
		{
			// First, process the SMAP-like file
			std::string smapFilename = dirName + "/" + options[USE_SMAP].arg;

			std::ifstream smapFile(smapFilename.c_str());
			do
			{
				std::string line;
				std::getline(smapFile, line);

				// Skip blank lines, commented lines (those starting with #) or lines that don't contain an SSEQ on them
				if (line.empty() || line[0] == '#' || line[2] == ' ')
					continue;

				// We only need to extract the label and the SDAT# (if it exists)
				std::istringstream stream(line);
				std::string label;
				size_t number;
				stream >> label >> number;
				std::string sdatNum = "";
				if (sdatNumber > 1)
				{
					stream >> sdatNum;
					sdatNum += "/";
				}
				includesAndExcludes.push_back(KeepInfo(sdatNum + label, KEEP_INCLUDE));
			} while (!smapFile.eof());

			// Second, mark all entries not included from the SMAP as being excluded
			for (size_t i = 0; i < finalSDAT.infoSection.SEQrecord.count; ++i)
			{
				if (!finalSDAT.infoSection.SEQrecord.entryOffsets[i]) // Skip empty offsets
					continue;

				KeepType keep = IncludeFilename(finalSDAT.infoSection.SEQrecord.entries[i].sseq->origFilename,
					finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber, includesAndExcludes);

				if (keep == KEEP_NEITHER)
					includesAndExcludes.push_back(KeepInfo(finalSDAT.infoSection.SEQrecord.entries[i].FullFilename(sdatNumber > 1), KEEP_EXCLUDE));
			}
		}

		// Pre-exclude/include removal
		IncOrExc tempIncludesAndExcludes = includesAndExcludes;
		Files oldSDATFilesList;
		if (!options[NOCOPY])
			for (size_t i = 0; i < finalSDAT.infoSection.SEQrecord.count; ++i)
			{
				if (!finalSDAT.infoSection.SEQrecord.entryOffsets[i]) // Skip empty offsets
					continue;

				std::string filename = finalSDAT.infoSection.SEQrecord.entries[i].sseq->origFilename,
					fullFilename = finalSDAT.infoSection.SEQrecord.entries[i].FullFilename(sdatNumber > 1);

				KeepType keep = IncludeFilename(filename, finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber, includesAndExcludes);

				// This file was neither included or excluded on the command line, we need to check if it already existed in the old SDAT
				if (keep == KEEP_NEITHER)
				{
					// First check by filename as well as data
					size_t count = oldSDATFiles.count(filename);
					bool exclude = true;
					const auto &thisData = finalSDAT.infoSection.SEQrecord.entries[i].sseq->data;
					// Data comparison lambda
					auto dataCompare = [&](const std::pair<std::string, SSEQ> &curr)
					{
						if (exclude)
						{
							auto &currData = curr.second.data;
							if (thisData == currData)
								exclude = false;
						}
					};
					if (count > 0)
					{
						auto range = oldSDATFiles.equal_range(filename);
						std::for_each(range.first, range.second, dataCompare);
					}
					// If we are still excluding the file, then we will check by binary comparing the data only
					if (exclude)
						std::for_each(oldSDATFiles.begin(), oldSDATFiles.end(), dataCompare);
					// Now, if we are still excluding the file, we add it to the temp list, otherwise we put it into a list to keep
					if (exclude)
						tempIncludesAndExcludes.push_back(KeepInfo(fullFilename, KEEP_EXCLUDE));
					else
						oldSDATFilesList.push_back(fullFilename);
				}
			}
		finalSDAT.Strip(tempIncludesAndExcludes, options[VERBOSE].count() > 1, false);

		// Only do the following for includes/excludes if we are not using an SMAP (when we are, this has already been done)
		if (!options[USE_SMAP])
			// Output which files are included/excluded, asking if -a was not given
			for (size_t i = 0; i < finalSDAT.infoSection.SEQrecord.count; ++i)
			{
				if (!finalSDAT.infoSection.SEQrecord.entryOffsets[i]) // Skip empty offsets
					continue;

				std::string filename = finalSDAT.infoSection.SEQrecord.entries[i].sseq->origFilename,
					fullFilename = finalSDAT.infoSection.SEQrecord.entries[i].FullFilename(sdatNumber > 1), verboseFilename = filename;
				if (sdatNumber > 1)
					verboseFilename += " (from SDAT #" + finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber + ")";

				KeepType keep = IncludeFilename(filename, finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber, includesAndExcludes);

				if (keep == KEEP_EXCLUDE)
					std::cout << verboseFilename << " was excluded on the command line.\n";
				else if (keep == KEEP_INCLUDE)
					std::cout << verboseFilename << " was included on the command line.\n";
				else
				{
					bool defaultToKeep = options[NOCOPY] || oldSDATFiles.empty() ||
						std::find(oldSDATFilesList.begin(), oldSDATFilesList.end(), fullFilename) != oldSDATFilesList.end();
					if (options[AUTO])
					{
						if (defaultToKeep)
							std::cout << verboseFilename << " was included automatically.\n";
						else
						{
							std::cout << verboseFilename << " was excluded automatically.\n";
							includesAndExcludes.push_back(KeepInfo(fullFilename, KEEP_EXCLUDE));
						}
					}
					else
					{
						std::cout << "Would you like to keep " << verboseFilename << "? [" << (defaultToKeep ? "Y" : "y") << "/" << (defaultToKeep ? "n" : "N") << "] ";
						std::string input;
						do
						{
							getline(std::cin, input);
							if (!input.empty())
								input[0] = std::tolower(input[0]);
						} while (!input.empty() && input[0] != 'y' && input[0] != 'n');
						if ((input.empty() && !defaultToKeep) || (!input.empty() && input[0] == 'n'))
							includesAndExcludes.push_back(KeepInfo(fullFilename, KEEP_EXCLUDE));
					}
				}
			}

		// Post-exclude/input removal
		finalSDAT.Strip(includesAndExcludes, options[VERBOSE].count() > 1);
		finalSDAT.StripBanks();

		// Create vector data for SDAT
		PseudoWrite sdatData;
		finalSDAT.Write(sdatData);

		if (finalSDAT.infoSection.SEQrecord.entries.size() == 1)
		{
			// Make single NCSF
			TagList tags;

			// If we had saved tags from a previous run and the nocopy option isn't being used,
			// copy the old tags before creating the file
			if (!options[NOCOPY] && savedTags.count(finalSDAT.infoSection.SEQrecord.entries[0].sseq->origFilename))
				tags = savedTags[finalSDAT.infoSection.SEQrecord.entries[0].sseq->origFilename];

			tags["ncsfby"] = "NDS to NCSF";

			std::string ncsfFilename = finalSDAT.infoSection.SEQrecord.entries[0].sseq->filename + ".ncsf";
			auto reservedData = IntToLEVector<uint32_t>(0);

			if (numberOfLoops)
				GetTime(ncsfFilename, &finalSDAT, finalSDAT.infoSection.SEQrecord.entries[0].sseq, tags, !!options[VERBOSE], numberOfLoops, fadeLoop, fadeOneShot);

			MakeNCSF(dirName + "/" + ncsfFilename, reservedData, sdatData.vector->data, tags.GetTags());
			if (options[VERBOSE])
				std::cout << "Created " << ncsfFilename << "\n";
		}
		else
		{
			// Determine filename to use for the NCSFLIB (will either be NTR/TWL-<gamecode>-<countrycode> or the name from the ROM's filename)
			std::map<char, std::string> countryCodes;
			countryCodes['J'] = "JPN";
			countryCodes['E'] = countryCodes['O'] = countryCodes['T'] = "USA"; // Apparently AUS can sometimes use the E country code | The O and T appear to be seen on DSi carts
			countryCodes['P'] = "EUR"; // Also seen as UKV sometimes
			countryCodes['D'] = "GER"; // Seen as NOE more often, though
			countryCodes['F'] = "FRA";
			countryCodes['I'] = "ITA";
			countryCodes['S'] = "SPA"; // Also seen as ESP sometimes
			countryCodes['H'] = "HOL";
			countryCodes['K'] = "KOR";
			countryCodes['C'] = "CHN";
			countryCodes['U'] = "AUS";
			countryCodes['N'] = "NOR";
			countryCodes['Q'] = "DEN";
			countryCodes['R'] = "RUS";
			countryCodes['M'] = "SWE";
			countryCodes['V'] = countryCodes['W'] = countryCodes['X'] = countryCodes['Y'] = countryCodes['Z'] = "EUU";

			std::string gameSerial = GetFilenameFromPath(ndsFilename);
			size_t gamedot = gameSerial.rfind('.');
			gameSerial = gameSerial.substr(0, gamedot);

			if (countryCodes.count(gameCode[3]))
				gameSerial = std::string(DSi ? "TWL" : "NTR") + "-" + gameCode + "-" + countryCodes[gameCode[3]];

			// Make NCSFLIB
			std::string ncsflibFilename = gameSerial + ".ncsflib";
			MakeNCSF(dirName + "/" + ncsflibFilename, std::vector<uint8_t>(), sdatData.vector->data);
			if (options[VERBOSE])
				std::cout << "Created " << ncsflibFilename << "\n";

			// Make multiple MININCSFs
			TagList tags;
			tags["_lib"] = ncsflibFilename;
			tags["ncsfby"] = "NDS to NCSF";

			for (size_t i = 0; i < finalSDAT.infoSection.SEQrecord.count; ++i)
			{
				if (!finalSDAT.infoSection.SEQrecord.entryOffsets[i])
					continue;
				std::string minincsfFilename = finalSDAT.infoSection.SEQrecord.entries[i].sseq->filename + ".minincsf";
				auto reservedData = IntToLEVector<uint32_t>(i);

				TagList thisTags = tags;
				std::string fullFilename = finalSDAT.infoSection.SEQrecord.entries[i].FullFilename(sdatNumber > 1);

				// If we had saved tags from a previous run and the nocopy option isn't being used,
				// copy the old tags before creating the file (NOTE: we also have to redo setting
				// the _lib and ncsfby tags, just in case)
				if (!options[NOCOPY] && savedTags.count(fullFilename))
				{
					thisTags = savedTags[fullFilename];
					thisTags["_lib"] = ncsflibFilename;
					thisTags["ncsfby"] = "NDS to NCSF";
				}

				thisTags["origFilename"] = finalSDAT.infoSection.SEQrecord.entries[i].sseq->origFilename;
				if (sdatNumber > 1)
					thisTags["origSDAT"] = finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber;

				// If this file was renamed from the generated name, then use the new filename instead
				// (This will only work if there was a SYMB section in the SDAT)
				if (filenames.count(fullFilename))
					minincsfFilename = filenames[fullFilename];

				if (numberOfLoops)
					GetTime(minincsfFilename, &finalSDAT, finalSDAT.infoSection.SEQrecord.entries[i].sseq, thisTags, !!options[VERBOSE], numberOfLoops, fadeLoop, fadeOneShot);

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
