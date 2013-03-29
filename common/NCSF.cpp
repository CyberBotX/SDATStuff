/*
 * Common NCSF functions
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-29
 */

#include <fstream>
#include <memory>
#include <iostream>
#include <zlib.h>
#include "NCSF.h"
#include "TimerPlayer.h"

// Create an NCSF file
void MakeNCSF(const std::string &filename, const std::vector<uint8_t> &reservedSectionData, const std::vector<uint8_t> &programSectionData,
	const std::vector<std::string> &tags)
{
	// Create zlib compressed version of program section, if one was given
	auto finalProgramSectionData = programSectionData;

	unsigned long programCompressedSize = finalProgramSectionData.empty() ? 0 : compressBound(finalProgramSectionData.size());
	auto programCompressedData = std::vector<uint8_t>(programCompressedSize);

	if (programCompressedSize)
	{
		compress2(&programCompressedData[0], &programCompressedSize, &finalProgramSectionData[0], finalProgramSectionData.size(), 9);
		programCompressedData.resize(programCompressedSize);
	}

	// Create file
	std::ofstream file;
	file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	file.open(filename.c_str(), std::ofstream::out | std::ofstream::binary);

	PseudoWrite ofile((&file));

	ofile.WriteLE("PSF", 3);
	ofile.WriteLE<uint8_t>(0x25);
	ofile.WriteLE<uint32_t>(reservedSectionData.empty() ? 0 : reservedSectionData.size());
	ofile.WriteLE<uint32_t>(programCompressedSize);
	if (programCompressedSize)
	{
		uint32_t crc = crc32(0, Z_NULL, 0);
		crc = crc32(crc, &programCompressedData[0], programCompressedSize);
		ofile.WriteLE(crc);
	}
	else
		ofile.WriteLE<uint32_t>(0);
	if (!reservedSectionData.empty())
		ofile.WriteLE(reservedSectionData);
	if (!programCompressedData.empty())
		ofile.WriteLE(programCompressedData);
	if (!tags.empty())
	{
		ofile.WriteLE("[TAG]", 5);
		for (size_t i = 0, len = tags.size(); i < len; ++i)
		{
			ofile.WriteLE(tags[i], tags[i].size());
			ofile.WriteLE<uint8_t>(0x0A);
		}
	}

	file.close();
}

// Check if the given file data is a valid PSF, throwing an exception if it's
// not a valid PSF
void CheckForValidPSF(PseudoReadFile &file, uint8_t versionByte)
{
	// Various checks on the file's size will be done throughout
	if (file.data->size() < 4)
		throw std::range_error("File is too small.");

	file.pos = 0;

	// Read the (hopefully) PSF header
	char PSFHeader[4];
	file.ReadLE(PSFHeader);

	// Verify it actually is a PSF file, as well as having the NCSF version byte
	if (PSFHeader[0] != 'P' || PSFHeader[1] != 'S' || PSFHeader[2] != 'F')
		throw std::runtime_error("Not a PSF file.");

	if (PSFHeader[3] != versionByte)
		throw std::runtime_error("Version byte of " + NumToHexString<uint8_t>(PSFHeader[3]) +
			" does not equal what we were looking for (" + NumToHexString(versionByte) + ").");

	if (file.data->size() < 16)
		throw std::range_error("File is too small.");

	// Get the sizes on the reserved and program sections
	uint32_t reservedSize = file.ReadLE<uint32_t>(), programCompressedSize = file.ReadLE<uint32_t>();

	// Skip the CRC
	file.pos += 4;

	// Check the reserved section
	if (reservedSize && file.data->size() < reservedSize + 16)
		throw std::range_error("File is too small.");

	file.pos += reservedSize;

	// Check the program section
	if (programCompressedSize && file.data->size() < reservedSize + programCompressedSize + 16)
		throw std::range_error("File is too small.");
}

// Extract the program section from a PSF.  Does not do any checks on the file,
// as those will be done in CheckForValidPSF anyways.
std::vector<uint8_t> GetProgramSectionFromPSF(PseudoReadFile &file, uint8_t versionByte, uint32_t programHeaderSize, uint32_t programSizeOffset)
{
	// Check to make sure the file is valid
	CheckForValidPSF(file, versionByte);

	// Get the sizes on the reserved and program sections
	file.pos = 4;
	uint32_t reservedSize = file.ReadLE<uint32_t>(), programCompressedSize = file.ReadLE<uint32_t>();

	// Skip the CRC
	file.pos += 4;

	// We need a program section to continue
	if (!programCompressedSize)
		throw std::range_error("There is no program section in this NCSF.");

	// If there is a reserved section, skip over it
	if (reservedSize)
		file.pos += reservedSize;

	// Read the compressed program section
	auto programSectionCompressed = std::vector<uint8_t>(programCompressedSize);
	file.ReadLE(programSectionCompressed);

	// Uncompress the program section, the reason uncompress is called twice:
	// first call is to get just the first 4 bytes that tell the size of the
	// entire uncompressed section, second call gets everything
	auto programSectionUncompressed = std::vector<uint8_t>(programHeaderSize);
	unsigned long programUncompressedSize = programHeaderSize;
	uncompress(&programSectionUncompressed[0], &programUncompressedSize, &programSectionCompressed[0], programCompressedSize);
	programUncompressedSize = ReadLE<uint32_t>(&programSectionUncompressed[programSizeOffset]);
	programSectionUncompressed.resize(programUncompressedSize);
	uncompress(&programSectionUncompressed[0], &programUncompressedSize, &programSectionCompressed[0], programCompressedSize);

	return std::vector<uint8_t>(programSectionUncompressed.begin(), programSectionUncompressed.end());
}

// The whitespace trimming was modified from the following answer on Stack Overflow:
// http://stackoverflow.com/a/217605

static struct IsWhitespace : std::unary_function<char, bool>
{
	bool operator()(const char &x) const
	{
		return x >= 0x01 && x <= 0x20;
	}
} isWhitespace;

static inline std::string LeftTrimWhitespace(const std::string &orig)
{
	auto first_non_space = std::find_if(orig.begin(), orig.end(), std::not1(isWhitespace));
	return std::string(first_non_space, orig.end());
}

static inline std::string RightTrimWhitespace(const std::string &orig)
{
	auto last_non_space = std::find_if(orig.rbegin(), orig.rend(), std::not1(isWhitespace)).base();
	return std::string(orig.begin(), last_non_space);
}

static inline std::string TrimWhitespace(const std::string &orig)
{
	return LeftTrimWhitespace(RightTrimWhitespace(orig));
}

// Get only the tags from the PSF
TagList GetTagsFromPSF(PseudoReadFile &file, uint8_t versionByte)
{
	// Check to make sure the file is valid
	CheckForValidPSF(file, versionByte);

	TagList tags;

	// Get the starting offset of the tags
	char TagHeader[] = "[TAG]";
	auto TagHeaderVector = std::vector<uint8_t>(TagHeader, TagHeader + 5);
	int32_t TagOffset = GetNextOffset(file, 0, TagHeaderVector);

	// Only continue on if we have tags
	if (TagOffset != -1)
	{
		file.pos = TagOffset + 5;
		std::string name, value;
		bool onName = true;
		size_t lengthOfTags = file.data->size() - file.pos;
		for (size_t x = 0; x < lengthOfTags; ++x)
		{
			char curr = file.ReadLE<uint8_t>();
			if (curr == 0x0A)
			{
				if (!name.empty() && !value.empty())
				{
					name = TrimWhitespace(name);
					value = TrimWhitespace(value);
					if (tags.Exists(name))
						tags[name] += "\n" + value;
					else
						tags[name] = value;
				}
				name = value = "";
				onName = true;
				continue;
			}
			if (curr == '=')
			{
				onName = false;
				continue;
			}
			if (onName)
				name += curr;
			else
				value += curr;
		}
	}

	return tags;
}

// Get a list of NCSF files in the given directory
Files GetFilesInNCSFDirectory(const std::string &path)
{
	DIR *dir;
	dirent *entry;
	Files files;

	if ((dir = opendir(path.c_str())))
		while ((entry = readdir(dir)))
		{
			std::string filename = std::string(entry->d_name);
			if (filename == "." || filename == "..")
				continue;
			std::string fullPath = path + "/" + filename;
			// Although the following function is for checking if a directory
			// exists, it can also be used to check if a path is a directory,
			// saving a little bit of extra code
			if (DirExists(fullPath))
				continue;
			// We want to skip any files that are not related to NCSFs
			auto dot = filename.rfind('.');
			std::string extension = dot == std::string::npos ? "" : filename.substr(dot + 1);
			if (extension != "ncsf" && extension != "minincsf" && extension != "ncsflib")
				continue;
			files.push_back(fullPath);
		}

	closedir(dir);

	return files;
}

// Remove the files in the given list of files
void RemoveFiles(const Files &files)
{
	std::for_each(files.begin(), files.end(), [](const std::string &file) { remove(file.c_str()); });
}

// Get time on SSEQ (uses a separate thread so it can be killed off if it takes longer than a few seconds)
static Time GetTime(TimerPlayer *player, uint32_t loopCount, uint32_t numberOfLoops)
{
	player->loops = numberOfLoops;
	player->StartLengthThread();
	uint32_t i = 0;
	for (; i < loopCount; ++i)
	{
		player->LockMutex();
		bool doingLength = player->doLength;
		player->UnlockMutex();
		if (doingLength)
		{
#ifdef _WIN32
			Sleep(150);
#else
			timespec req;
			req.tv_sec = 0;
			req.tv_nsec = 150000000;
			while (nanosleep(&req, &req) == -1 && errno == EINTR);
#endif
		}
		else
			break;
	}
	Time length;
	if (i == loopCount)
	{
		player->LockMutex();
		player->doLength = false;
		player->UnlockMutex();
		player->WaitForThread();
		length = Time(-1, LOOP);
	}
	else
	{
		player->WaitForThread();
		length = player->length;
	}
	return length;
}

// Get time on SSEQ, will run the player at least once (without "playing" the
// music), if the song is one-shot (and not looping), it will run the player
// a second time, "playing" the song to determine when silence has occurred.
// After which, it will store the data in the tags for the SSEQ.
void GetTime(const std::string &filename, const SDAT *sdat, const SSEQ *sseq, TagList &tags, bool verbose, uint32_t numberOfLoops, uint32_t fadeLoop, uint32_t fadeOneShot)
{
	auto player = std::auto_ptr<TimerPlayer>(new TimerPlayer());
	player->Setup(sseq);
	player->maxSeconds = 6000;
	// Get the time, without "playing" the notes
	Time length = GetTime(player.get(), 20, numberOfLoops);
	// If the length was for a one-shot song, get the time again, this time "playing" the notes
	bool gotLength = false;
	if (static_cast<int>(length.time) != -1 && length.type == END)
	{
		player.reset(new TimerPlayer());
		player->Setup(sseq);
		player->sbnk = sdat->infoSection.BANKrecord.entries[sseq->info.bank].sbnk;
		for (int i = 0; i < 4; ++i)
			if (player->sbnk->info.waveArc[i] != 0xFFFF)
				player->swar[i] = sdat->infoSection.WAVEARCrecord.entries[player->sbnk->info.waveArc[i]].swar;
		player->maxSeconds = length.time + 30;
		player->doNotes = true;
		Time oldLength = length;
		length = GetTime(player.get(), 40, numberOfLoops);
		if (static_cast<int>(length.time) != -1)
			gotLength = true;
		else
			length = oldLength;
	}
	if (static_cast<int>(length.time) != -1)
	{
		if (length.type == LOOP)
			tags["fade"] = stringify(fadeLoop);
		else
			tags["fade"] = stringify(fadeOneShot);
		if (!static_cast<int>(length.time))
			length.time = 1;
		std::string lengthString = SecondsToString(std::ceil(length.time));
		tags["length"] = lengthString;
		if (verbose)
		{
			std::cout << "Time for " << filename << ": " << lengthString << " (" << (length.type == LOOP ? "timed to 2 loops" : "one-shot") << ")\n";
			if (length.type == END && !gotLength)
				std::cout << "(NOTE: Was unable to detect silence at the end of the track, time may be inaccurate.)\n";
		}
	}
	else if (verbose)
	{
		tags.Remove("fade");
		tags.Remove("length");
		std::cout << "Unable to calculate time for " << filename << "\n";
	}
}
