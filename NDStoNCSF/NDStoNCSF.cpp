/*
 * NDS to NCSF
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Version history:
 *   v1.0 - Initial version
 */

#include <iostream>
#include "NCSF.h"
#include "optionparser.h"

static const std::string NDSTONCSF_VERSION = "1.0";

static inline option::ArgStatus RequireArgument(const option::Option &opt, bool msg)
{
	if (opt.arg && *opt.arg)
		return option::ARG_OK;

	if (msg)
		std::cerr << "Option '" << std::string(opt.name).substr(0, opt.namelen) << "' requires a non-empty argument.\n";
	return option::ARG_ILLEGAL;
}

enum { UNKNOWN, HELP, VERBOSE, TIME, EXCLUDE, INCLUDE, AUTO, NOCOPY };
const option::Descriptor opts[] =
{
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "NDS to NCSF v" + NDSTONCSF_VERSION + "\nBy Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"NDS to NCSF will take the incoming NDS ROM and create a series of NCSF files.  If there is only a single SSEQ within the entire ROM, then there will be a "
			"single NCSF file.  Otherwise, there will be an NCSFLIB and multiple MININCSFs.\n\n"
		"Usage:\n"
		"  NDStoNCSF [options] <Input SDAT filename>\n\n"
		"Options:"),
	//option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, std::string("NDS").c_str()),
	option::Descriptor(HELP, 0, "h", "help", option::Arg::None, "  --help,-h \tPrint usage and exit."),
	option::Descriptor(VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose,-v \tVerbose output."),
	option::Descriptor(TIME, 0, "t", "time", option::Arg::None, "  --time,-t \tCalculate time on each track."),
	option::Descriptor(EXCLUDE, 0, "x", "exclude", RequireArgument,
		"  --exclude=<filename> \v         -x <filename> \tExclude the given filename from the final SDAT.  May use * and ? wildcards."),
	option::Descriptor(INCLUDE, 0, "i", "include", RequireArgument,
		"  --include=<filename> \v         -i <filename> \tInclude the given filename in the final SDAT.  May use * and ? wildcards."),
	option::Descriptor(AUTO, 0, "a", "auto", option::Arg::None, "  --auto,-a \tFully automatic mode (disables interactive mode)."),
	option::Descriptor(NOCOPY, 0, "n", "nocopy", option::Arg::None, "  --nocopy,-n \tDo not check for previous files in the destination directory."),
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None,
		"\nVerbose output will output the NCSFs created."
		"\n\nExcluded and included files will be processed in the order they are given on the command line, later arguments overriding earlier arguments.  If there is more "
			"than 1 SDAT contained within the NDS ROM, you can exclude or include based on the SDAT by prefixing the filename with the SDAT number (1-based) and a forward "
			"slash.  For example, if the NDS ROM has 2 SDATs and both contain a file called XYZ, but you only want XYZ from the 2nd SDAT, use 1/XYZ as an exclude.  Wildcards "
			"before the forward slash are also accepted."
		"\n\nIf --auto or -a are not given, the program will run in interactive mode, asking to confirm which sequences to keep.  (NOTE: Even in interactive mode, files "
			"which were excluded or includes on the command line will still be automatically set as such.)"
		"\n\nIf --nocopy or -n are not given, the program will use information from a previous run of NDS to NCSF, if any exists.  This will set files that were not in the "
			"previous run's SDAT as being excluded by default and it will also attempt to copy tags from the previous files.  (NOTE: This may not work if the original "
			"SDAT did not contain a symbol record, mainly because filename matching cannot be done.)"
		"\n\nTiming uses code based on FeOS Sound System by fincs, as well as code from DeSmuME for pseudo-playback."),
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

	// Get exclude and include filenames from the command line
	IncOrExc includesAndExcludes;
	for (int i = 0; i < parse.optionsCount(); ++i)
	{
		const option::Option &opt = buffer[i];
		if (opt.index() == EXCLUDE)
			includesAndExcludes.push_back(KeepInfo(opt.arg, KEEP_EXCLUDE));
		else if (opt.index() == INCLUDE)
			includesAndExcludes.push_back(KeepInfo(opt.arg, KEEP_INCLUDE));
	}

	try
	{
		// Read NDS ROM
		std::string ndsFilename = parse.nonOption(0);
		std::replace(ndsFilename.begin(), ndsFilename.end(), '\\', '/');

		if (!FileExists(ndsFilename))
			throw std::runtime_error("File " + ndsFilename + " does not exist.");

		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		file.open(ndsFilename.c_str(), std::ifstream::in | std::ifstream::binary);

		PseudoReadFile fileData((ndsFilename));
		fileData.GetDataFromFile(file);

		file.close();

		// Setup the output directory, making sure it is clear beforehand (if it
		// exists and we aren't being told not to copy the old data, then we'll
		// get all that data first)
		std::string dirName = ndsFilename;
		size_t dot = dirName.rfind('.');
		dirName = dirName.substr(0, dot) + "_NDStoNCSF";

		std::map<std::string, TagList> savedTags;
		std::map<std::string, std::string> filenames;
		Files oldSDATFiles;
		if (DirExists(dirName))
		{
			Files files = GetFilesInNCSFDirectory(dirName);

			if (!options[NOCOPY])
			{
				uint16_t sdatCount = 0;
				for (auto curr = files.begin(), end = files.end(); curr != end; ++curr)
				{
					try
					{
						std::ifstream ncsfFile;
						ncsfFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
						ncsfFile.open(curr->c_str(), std::ifstream::in | std::ifstream::binary);

						PseudoReadFile ncsfFileData((*curr));
						ncsfFileData.GetDataFromFile(ncsfFile);

						ncsfFile.close();

						if (curr->rfind(".ncsf") != std::string::npos || curr->rfind(".ncsflib") != std::string::npos)
						{
							auto sdatVector = GetProgramSectionFromNCSF(ncsfFileData);

							PseudoReadFile sdatFileData((*curr));
							sdatFileData.GetDataFromVector(sdatVector.begin(), sdatVector.end());

							SDAT sdat;
							sdat.Read(*curr, sdatFileData);
							sdatCount = sdat.count;
							if (sdat.SYMBOffset)
								for (uint32_t i = 0; i < sdat.symbSection.SEQrecord.count; ++i)
									oldSDATFiles.push_back(sdat.symbSection.SEQrecord.entries[i]);
						}
						if (curr->rfind(".ncsf") != std::string::npos || curr->rfind(".minincsf") != std::string::npos)
						{
							std::string filename = GetFilenameFromPath(*curr);
							TagList tags = GetTagsFromNCSF(ncsfFileData);
							if (tags.Exists("origFilename"))
							{
								savedTags[tags["origFilename"]] = tags;
								filenames[tags["origFilename"]] = filename;
							}
							else
								savedTags[filename] = tags;
						}
					}
					catch (const std::exception &)
					{
					}
				}
				if (sdatCount > 1)
					for (auto curr = oldSDATFiles.begin(), end = oldSDATFiles.end(); curr != end; ++curr)
					{
						if (!savedTags.count(*curr))
							continue;
						const TagList &tags = savedTags[*curr];
						if (!tags.Exists("origSDAT"))
							continue;
						*curr = tags["origSDAT"] + "/" + *curr;
					}
			}

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

		// Search for SDATs and merge them into one
		SDAT finalSDAT;
		if (options[VERBOSE])
			std::cout << "Searching for SDATs...\n";

		uint8_t sdatSignature[] = { 0x53, 0x44, 0x41, 0x54, 0xFF, 0xFE, 0x00, 0x01 };
		std::vector<uint8_t> sdatSignatureVector(sdatSignature, sdatSignature + 8);
		int32_t sdatOffset, sdatNumber = 0;
		uint32_t previousOffset = 0;
		while ((sdatOffset = GetNextOffset(fileData, previousOffset, sdatSignatureVector)) != -1)
		{
			try
			{
				PseudoReadFile sdatFile;
				sdatFile.GetDataFromVector(fileData.data->begin() + sdatOffset, fileData.data->end());
				SDAT sdat;
				sdat.Read(stringify(sdatNumber++ + 1), sdatFile);
				finalSDAT += sdat;
				if (options[VERBOSE])
					std::cout << "Found SDAT with " << sdat.infoSection.SEQrecord.actualCount << " SSEQ" << (sdat.infoSection.SEQrecord.actualCount == 1 ? "" : "s") << ".\n";
			}
			catch (const std::exception &)
			{
				--sdatNumber;
			}
			previousOffset = sdatOffset + 1;
		}

		// Fail if we do not have any SSEQs (which could also mean that there were no SDATs in the ROM or it wasn't an NDS ROM)
		if (!finalSDAT.infoSection.SEQrecord.count)
		{
			remove(dirName.c_str());
			throw std::range_error("Either there were no SSEQs within the SDATs of given NDS ROM, no SDATs in\n  the ROM, or the file was not an NDS ROM.");
		}

		// Pre-exclude/include removal
		IncOrExc tempIncludesAndExcludes = includesAndExcludes;
		if (!options[NOCOPY])
			for (size_t i = 0; i < finalSDAT.infoSection.SEQrecord.count; ++i)
			{
				if (!finalSDAT.infoSection.SEQrecord.entryOffsets[i]) // Skip empty offsets
					continue;

				std::string filename = finalSDAT.infoSection.SEQrecord.entries[i].sseq->origFilename, fullFilename = filename;
				if (sdatNumber > 1)
					fullFilename = finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber + "/" + fullFilename;

				KeepType keep = IncludeFilename(filename, finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber, includesAndExcludes);

				if (keep == KEEP_NEITHER && !oldSDATFiles.empty() && std::find(oldSDATFiles.begin(), oldSDATFiles.end(), fullFilename) == oldSDATFiles.end())
					tempIncludesAndExcludes.push_back(KeepInfo(fullFilename, KEEP_EXCLUDE));
			}
		finalSDAT.Strip(tempIncludesAndExcludes, false);

		// Output which files are included/excluded, asking if -a was not given
		for (size_t i = 0; i < finalSDAT.infoSection.SEQrecord.count; ++i)
		{
			if (!finalSDAT.infoSection.SEQrecord.entryOffsets[i]) // Skip empty offsets
				continue;

			std::string filename = finalSDAT.infoSection.SEQrecord.entries[i].sseq->origFilename, fullFilename = filename, verboseFilename = filename;
			if (sdatNumber > 1)
			{
				fullFilename = finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber + "/" + fullFilename;
				verboseFilename += " (from SDAT #" + finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber + ")";
			}

			KeepType keep = IncludeFilename(filename, finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber, includesAndExcludes);

			if (keep == KEEP_EXCLUDE)
				std::cout << verboseFilename << " was excluded on the command line.\n";
			else if (keep == KEEP_INCLUDE)
				std::cout << verboseFilename << " was included on the command line.\n";
			else
			{
				bool defaultToKeep = options[NOCOPY] || oldSDATFiles.empty() || std::find(oldSDATFiles.begin(), oldSDATFiles.end(), fullFilename) != oldSDATFiles.end();
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
		finalSDAT.Strip(includesAndExcludes);

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

			if (options[TIME])
				GetTime(ncsfFilename, &finalSDAT, finalSDAT.infoSection.SEQrecord.entries[0].sseq, tags, !!options[VERBOSE]);

			MakeNCSF(dirName + "/" + ncsfFilename, reservedData, sdatData.vector->data, tags.GetTags());
			if (options[VERBOSE])
				std::cout << "Created " << ncsfFilename << "\n";
		}
		else
		{
			// Determine filename to use for the NCSFLIB (will either be NTR-<gamecode>-<countrycode> or the name from the ROM's filename)
			std::map<char, std::string> countryCodes;
			countryCodes['J'] = "JPN";
			countryCodes['E'] = "USA";
			countryCodes['P'] = "EUR";
			countryCodes['D'] = countryCodes['F'] = "NOE";
			countryCodes['I'] = "ITA";
			countryCodes['S'] = "SPA";
			countryCodes['H'] = "HOL";
			countryCodes['K'] = "KOR";
			countryCodes['X'] = "EUU";

			std::string gameSerial = GetFilenameFromPath(ndsFilename);
			size_t gamedot = gameSerial.rfind('.');
			gameSerial = gameSerial.substr(0, gamedot);

			if (countryCodes.count(gameCode[3]))
				gameSerial = "NTR-" + gameCode + "-" + countryCodes[gameCode[3]];

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
				std::string fullFilename = finalSDAT.infoSection.SEQrecord.entries[i].sseq->origFilename;
				if (sdatNumber > 1)
					fullFilename = finalSDAT.infoSection.SEQrecord.entries[i].sdatNumber + "/" + fullFilename;

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
				if (filenames.count(thisTags["origFilename"]))
					minincsfFilename = filenames[thisTags["origFilename"]];

				if (options[TIME])
					GetTime(minincsfFilename, &finalSDAT, finalSDAT.infoSection.SEQrecord.entries[i].sseq, thisTags, !!options[VERBOSE]);

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
