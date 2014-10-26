/*
 * SDAT - SDAT structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include <functional>
#include <iostream>
#include "SDAT.h"

bool SDAT::failOnMissingFiles = true;

SDAT::SDAT() : filename(""), header(), SYMBOffset(0), SYMBSize(0), INFOOffset(0), INFOSize(0), FATOffset(0), FATSize(0), FILEOffset(0), FILESize(0), symbSection(),
	infoSection(), fatSection(), symbSectionNeedsCleanup(false), count(0), SSEQs(), SBNKs(), SWARs()
{
	memcpy(this->header.type, "SDAT", sizeof(this->header.type));
	this->header.magic = 0x0100FEFF;
}

SDAT::SDAT(const SDAT &sdat) : filename(sdat.filename), header(sdat.header), SYMBOffset(sdat.SYMBOffset), SYMBSize(sdat.SYMBSize), INFOOffset(sdat.INFOOffset),
	INFOSize(sdat.INFOSize), FATOffset(sdat.FATOffset), FATSize(sdat.FATSize), FILEOffset(sdat.FILEOffset), FILESize(sdat.FILESize), symbSection(sdat.symbSection),
	infoSection(sdat.infoSection), fatSection(sdat.fatSection), symbSectionNeedsCleanup(sdat.symbSectionNeedsCleanup), count(sdat.count), SSEQs(), SBNKs(), SWARs()
{
	std::for_each(sdat.SSEQs.begin(), sdat.SSEQs.end(), [&](const std::unique_ptr<SSEQ> &sseq)
	{
		auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(*sseq.get()));
		newSSEQ->info = this->infoSection.SEQrecord.entries[sseq->entryNumber];
		this->infoSection.SEQrecord.entries[sseq->entryNumber].sseq = newSSEQ.get();
		this->SSEQs.push_back(std::move(newSSEQ));
	});
	std::for_each(sdat.SBNKs.begin(), sdat.SBNKs.end(), [&](const std::unique_ptr<SBNK> &sbnk)
	{
		auto newSBNK = std::unique_ptr<SBNK>(new SBNK(*sbnk.get()));
		newSBNK->info = this->infoSection.BANKrecord.entries[sbnk->entryNumber];
		this->infoSection.BANKrecord.entries[sbnk->entryNumber].sbnk = newSBNK.get();
		this->SBNKs.push_back(std::move(newSBNK));
	});
	std::for_each(sdat.SWARs.begin(), sdat.SWARs.end(), [&](const std::unique_ptr<SWAR> &swar)
	{
		auto newSWAR = std::unique_ptr<SWAR>(new SWAR(*swar.get()));
		newSWAR->info = this->infoSection.WAVEARCrecord.entries[swar->entryNumber];
		this->infoSection.WAVEARCrecord.entries[swar->entryNumber].swar = newSWAR.get();
		this->SWARs.push_back(std::move(newSWAR));
	});
}

SDAT &SDAT::operator=(const SDAT &sdat)
{
	if (this != &sdat)
	{
		this->filename = sdat.filename;
		this->header = sdat.header;
		this->SYMBOffset = sdat.SYMBOffset;
		this->SYMBSize = sdat.SYMBSize;
		this->INFOOffset = sdat.INFOOffset;
		this->INFOSize = sdat.INFOSize;
		this->FATOffset = sdat.FATOffset;
		this->FATSize = sdat.FATSize;
		this->FILEOffset = sdat.FILEOffset;
		this->FILESize = sdat.FILESize;

		this->symbSection = sdat.symbSection;
		this->infoSection = sdat.infoSection;
		this->fatSection = sdat.fatSection;

		this->symbSectionNeedsCleanup = sdat.symbSectionNeedsCleanup;
		this->count = sdat.count;

		std::for_each(sdat.SSEQs.begin(), sdat.SSEQs.end(), [&](const std::unique_ptr<SSEQ> &sseq)
		{
			auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(*sseq.get()));
			newSSEQ->info = this->infoSection.SEQrecord.entries[sseq->entryNumber];
			this->infoSection.SEQrecord.entries[sseq->entryNumber].sseq = newSSEQ.get();
			this->SSEQs.push_back(std::move(newSSEQ));
		});
		std::for_each(sdat.SBNKs.begin(), sdat.SBNKs.end(), [&](const std::unique_ptr<SBNK> &sbnk)
		{
			auto newSBNK = std::unique_ptr<SBNK>(new SBNK(*sbnk.get()));
			newSBNK->info = this->infoSection.BANKrecord.entries[sbnk->entryNumber];
			this->infoSection.BANKrecord.entries[sbnk->entryNumber].sbnk = newSBNK.get();
			this->SBNKs.push_back(std::move(newSBNK));
		});
		std::for_each(sdat.SWARs.begin(), sdat.SWARs.end(), [&](const std::unique_ptr<SWAR> &swar)
		{
			auto newSWAR = std::unique_ptr<SWAR>(new SWAR(*swar.get()));
			newSWAR->info = this->infoSection.WAVEARCrecord.entries[swar->entryNumber];
			this->infoSection.WAVEARCrecord.entries[swar->entryNumber].swar = newSWAR.get();
			this->SWARs.push_back(std::move(newSWAR));
		});
	}
	return *this;
}

void SDAT::Read(const std::string &fn, PseudoReadFile &file, bool shouldFailOnMissingFiles)
{
	SDAT::failOnMissingFiles = true;

	this->filename = fn;

	// Read header
	this->header.Read(file);
	this->header.Verify("SDAT", 0x0100FEFF);
	this->SYMBOffset = file.ReadLE<uint32_t>();
	this->SYMBSize = file.ReadLE<uint32_t>();
	this->INFOOffset = file.ReadLE<uint32_t>();
	this->INFOSize = file.ReadLE<uint32_t>();
	this->FATOffset = file.ReadLE<uint32_t>();
	this->FATSize = file.ReadLE<uint32_t>();
	file.pos = 0x30;
	this->count = file.ReadLE<uint16_t>();

	// Read SYMB (if it exists), INFO, and FAT sections
	if (this->SYMBOffset)
	{
		file.pos = this->SYMBOffset;
		this->symbSection.Read(file);
	}
	file.pos = this->INFOOffset;
	this->infoSection.Read(file);
	file.pos = this->FATOffset;
	this->fatSection.Read(file);

	// Throw an exception if there were no SSEQ records
	if (this->infoSection.SEQrecord.entries.empty())
		throw std::logic_error("No SSEQ records found in SDAT");

	SDAT::failOnMissingFiles = shouldFailOnMissingFiles;

	// Read files
	for (size_t i = 0, entries = this->infoSection.SEQrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.SEQrecord.entryOffsets[i])
			continue;
		uint16_t fileID = this->infoSection.SEQrecord.entries[i].fileID;
		std::string origName = "SSEQ" + NumToHexString(fileID).substr(2), name = origName;
		if (this->SYMBOffset)
		{
			origName = this->symbSection.SEQrecord.entries[i];
			name = NumToHexString<uint32_t>(i).substr(6) + " - " + origName;
		}
		this->infoSection.SEQrecord.entries[i].origFilename = origName;
		this->infoSection.SEQrecord.entries[i].sdatNumber = this->filename;
		file.pos = this->fatSection.records[fileID].offset;
		this->infoSection.SEQrecord.entries[i].fileData.resize(this->fatSection.records[fileID].size, 0);
		file.ReadLE(this->infoSection.SEQrecord.entries[i].fileData);
		file.pos = this->fatSection.records[fileID].offset;
		auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(name, origName));
		newSSEQ->info = this->infoSection.SEQrecord.entries[i];
		newSSEQ->entryNumber = i;
		newSSEQ->Read(file);
		this->infoSection.SEQrecord.entries[i].sseq = newSSEQ.get();
		this->SSEQs.push_back(std::move(newSSEQ));
	}
	for (size_t i = 0, entries = this->infoSection.BANKrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.BANKrecord.entryOffsets[i])
			continue;
		uint16_t fileID = this->infoSection.BANKrecord.entries[i].fileID;
		std::string origName = "SBNK" + NumToHexString(fileID).substr(2);
		if (this->SYMBOffset)
			origName = this->symbSection.BANKrecord.entries[i];
		this->infoSection.BANKrecord.entries[i].origFilename = origName;
		this->infoSection.BANKrecord.entries[i].sdatNumber = this->filename;
		file.pos = this->fatSection.records[fileID].offset;
		this->infoSection.BANKrecord.entries[i].fileData.resize(this->fatSection.records[fileID].size, 0);
		file.ReadLE(this->infoSection.BANKrecord.entries[i].fileData);
		file.pos = this->fatSection.records[fileID].offset;
		auto newSBNK = std::unique_ptr<SBNK>(new SBNK(origName));
		newSBNK->info = this->infoSection.BANKrecord.entries[i];
		newSBNK->entryNumber = i;
		newSBNK->Read(file);
		this->infoSection.BANKrecord.entries[i].sbnk = newSBNK.get();
		this->SBNKs.push_back(std::move(newSBNK));
	}
	for (size_t i = 0, entries = this->infoSection.WAVEARCrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.WAVEARCrecord.entryOffsets[i])
			continue;
		uint16_t fileID = this->infoSection.WAVEARCrecord.entries[i].fileID;
		std::string origName = "SWAR" + NumToHexString(fileID).substr(2);
		if (this->SYMBOffset)
			origName = this->symbSection.WAVEARCrecord.entries[i];
		this->infoSection.WAVEARCrecord.entries[i].origFilename = origName;
		this->infoSection.WAVEARCrecord.entries[i].sdatNumber = this->filename;
		file.pos = this->fatSection.records[fileID].offset;
		this->infoSection.WAVEARCrecord.entries[i].fileData.resize(this->fatSection.records[fileID].size, 0);
		file.ReadLE(this->infoSection.WAVEARCrecord.entries[i].fileData);
		file.pos = this->fatSection.records[fileID].offset;
		auto newSWAR = std::unique_ptr<SWAR>(new SWAR(origName));
		newSWAR->info = this->infoSection.WAVEARCrecord.entries[i];
		newSWAR->entryNumber = i;
		newSWAR->Read(file);
		this->infoSection.WAVEARCrecord.entries[i].swar = newSWAR.get();
		this->SWARs.push_back(std::move(newSWAR));
	}
	for (size_t i = 0, entries = this->infoSection.PLAYERrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.PLAYERrecord.entryOffsets[i])
			continue;
		std::string origName = "PLAYER" + NumToHexString<uint8_t>(i).substr(2);
		if (this->SYMBOffset)
			origName = this->symbSection.PLAYERrecord.entries[i];
		this->infoSection.PLAYERrecord.entries[i].origFilename = origName;
		this->infoSection.PLAYERrecord.entries[i].sdatNumber = this->filename;
	}
}

void SDAT::Write(PseudoWrite &file) const
{
	// Write header
	this->header.Write(file);
	file.WriteLE(this->SYMBOffset);
	file.WriteLE((this->SYMBSize + 3) & ~0x03);
	file.WriteLE(this->INFOOffset);
	file.WriteLE(this->INFOSize);
	file.WriteLE(this->FATOffset);
	file.WriteLE(this->FATSize);
	file.WriteLE(this->FILEOffset);
	file.WriteLE(this->FILESize);
	// Normally the reserved section contains 16 0s, but for the purposes of
	// detecting if this SDAT was made from more than 1 SDAT or not, the
	// first 2 bytes will store the number of SDATs that this SDAT is made of
	file.WriteLE<uint16_t>(this->count);
	std::vector<uint8_t> reserved(14, 0);
	file.WriteLE(reserved);

	// Write SYMB (if it exists), INFO, and FAT sections
	if (this->SYMBOffset)
		this->symbSection.Write(file);
	this->infoSection.Write(file);
	this->fatSection.Write(file);

	// Write FILE section header
	file.WriteLE("FILE", 4);
	file.WriteLE(this->FILESize);
	file.WriteLE(this->fatSection.count);
	uint32_t other[] = { 0, 0, 0 };
	file.WriteLE(other);

	// Write files
	for (uint32_t i = 0; i < this->infoSection.SEQrecord.count; ++i)
		file.WriteLE(this->infoSection.SEQrecord.entries[i].fileData);
	for (uint32_t i = 0; i < this->infoSection.BANKrecord.count; ++i)
		file.WriteLE(this->infoSection.BANKrecord.entries[i].fileData);
	for (uint32_t i = 0; i < this->infoSection.WAVEARCrecord.count; ++i)
		file.WriteLE(this->infoSection.WAVEARCrecord.entries[i].fileData);
}

// Appends another SDAT to this one
SDAT &SDAT::operator+=(const SDAT &other)
{
	if (this == &other)
		return *this;

	uint32_t origSEQcount = this->infoSection.SEQrecord.count, origBANKcount = this->infoSection.BANKrecord.count,
		origWAVEARCcount = this->infoSection.WAVEARCrecord.count, origPLAYERcount = this->infoSection.PLAYERrecord.count;
	if (this->SYMBOffset || other.SYMBOffset)
	{
		this->symbSection.SEQrecord.count = this->infoSection.SEQrecord.count + other.infoSection.SEQrecord.count;
		this->symbSection.SEQrecord.entryOffsets.resize(this->symbSection.SEQrecord.count, 0);
		this->symbSection.SEQrecord.entries.resize(this->symbSection.SEQrecord.count, "");
		if (other.SYMBOffset)
			std::copy(other.symbSection.SEQrecord.entries.begin(), other.symbSection.SEQrecord.entries.end(),
				this->symbSection.SEQrecord.entries.begin() + origSEQcount);

		this->symbSection.BANKrecord.count = this->infoSection.BANKrecord.count + other.infoSection.BANKrecord.count;
		this->symbSection.BANKrecord.entryOffsets.resize(this->symbSection.BANKrecord.count, 0);
		this->symbSection.BANKrecord.entries.resize(this->symbSection.BANKrecord.count, "");
		if (other.SYMBOffset)
			std::copy(other.symbSection.BANKrecord.entries.begin(), other.symbSection.BANKrecord.entries.end(),
				this->symbSection.BANKrecord.entries.begin() + origBANKcount);

		this->symbSection.WAVEARCrecord.count = this->infoSection.WAVEARCrecord.count + other.infoSection.WAVEARCrecord.count;
		this->symbSection.WAVEARCrecord.entryOffsets.resize(this->symbSection.WAVEARCrecord.count, 0);
		this->symbSection.WAVEARCrecord.entries.resize(this->symbSection.WAVEARCrecord.count, "");
		if (other.SYMBOffset)
			std::copy(other.symbSection.WAVEARCrecord.entries.begin(), other.symbSection.WAVEARCrecord.entries.end(),
				this->symbSection.WAVEARCrecord.entries.begin() + origWAVEARCcount);

		this->symbSection.PLAYERrecord.count = this->infoSection.PLAYERrecord.count + other.infoSection.PLAYERrecord.count;
		this->symbSection.PLAYERrecord.entryOffsets.resize(this->symbSection.PLAYERrecord.count, 0);
		this->symbSection.PLAYERrecord.entries.resize(this->symbSection.PLAYERrecord.count, "");
		if (other.SYMBOffset)
			std::copy(other.symbSection.PLAYERrecord.entries.begin(), other.symbSection.PLAYERrecord.entries.end(),
			this->symbSection.PLAYERrecord.entries.begin() + origPLAYERcount);

		this->symbSectionNeedsCleanup = true;

		this->SYMBOffset = 0x40;
	}

	this->infoSection.SEQrecord.count = this->infoSection.SEQrecord.count + other.infoSection.SEQrecord.count;
	this->infoSection.SEQrecord.entryOffsets.resize(this->infoSection.SEQrecord.count, 0);
	std::copy(other.infoSection.SEQrecord.entryOffsets.begin(), other.infoSection.SEQrecord.entryOffsets.end(),
		this->infoSection.SEQrecord.entryOffsets.begin() + origSEQcount);
	this->infoSection.SEQrecord.entries.resize(this->infoSection.SEQrecord.count);
	for (size_t i = origSEQcount; i < this->infoSection.SEQrecord.count; ++i)
	{
		this->infoSection.SEQrecord.entries[i] = other.infoSection.SEQrecord.entries[i - origSEQcount];
		this->infoSection.SEQrecord.entries[i].fileID += this->fatSection.count;
		this->infoSection.SEQrecord.entries[i].bank += origBANKcount;
		if (other.infoSection.SEQrecord.entries[i - origSEQcount].sseq)
		{
			auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(*other.infoSection.SEQrecord.entries[i - origSEQcount].sseq));
			newSSEQ->info = this->infoSection.SEQrecord.entries[i];
			newSSEQ->entryNumber = i;
			this->infoSection.SEQrecord.entries[i].sseq = newSSEQ.get();
			this->SSEQs.push_back(std::move(newSSEQ));
		}
	}

	this->infoSection.BANKrecord.count = this->infoSection.BANKrecord.count + other.infoSection.BANKrecord.count;
	this->infoSection.BANKrecord.entryOffsets.resize(this->infoSection.BANKrecord.count, 0);
	std::copy(other.infoSection.BANKrecord.entryOffsets.begin(), other.infoSection.BANKrecord.entryOffsets.end(),
		this->infoSection.BANKrecord.entryOffsets.begin() + origBANKcount);
	this->infoSection.BANKrecord.entries.resize(this->infoSection.BANKrecord.count);
	for (size_t i = origBANKcount; i < this->infoSection.BANKrecord.count; ++i)
	{
		this->infoSection.BANKrecord.entries[i] = other.infoSection.BANKrecord.entries[i - origBANKcount];
		this->infoSection.BANKrecord.entries[i].fileID += this->fatSection.count;
		for (size_t j = 0; j < 4; ++j)
			if (this->infoSection.BANKrecord.entries[i].waveArc[j] != 0xFFFF)
				this->infoSection.BANKrecord.entries[i].waveArc[j] += origWAVEARCcount;
		if (other.infoSection.BANKrecord.entries[i - origBANKcount].sbnk)
		{
			auto newSBNK = std::unique_ptr<SBNK>(new SBNK(*other.infoSection.BANKrecord.entries[i - origBANKcount].sbnk));
			newSBNK->info = this->infoSection.BANKrecord.entries[i];
			newSBNK->entryNumber = i;
			this->infoSection.BANKrecord.entries[i].sbnk = newSBNK.get();
			this->SBNKs.push_back(std::move(newSBNK));
		}
	}

	this->infoSection.WAVEARCrecord.count = this->infoSection.WAVEARCrecord.count + other.infoSection.WAVEARCrecord.count;
	this->infoSection.WAVEARCrecord.entryOffsets.resize(this->infoSection.WAVEARCrecord.count, 0);
	std::copy(other.infoSection.WAVEARCrecord.entryOffsets.begin(), other.infoSection.WAVEARCrecord.entryOffsets.end(),
		this->infoSection.WAVEARCrecord.entryOffsets.begin() + origWAVEARCcount);
	this->infoSection.WAVEARCrecord.entries.resize(this->infoSection.WAVEARCrecord.count);
	for (size_t i = origWAVEARCcount; i < this->infoSection.WAVEARCrecord.count; ++i)
	{
		this->infoSection.WAVEARCrecord.entries[i] = other.infoSection.WAVEARCrecord.entries[i - origWAVEARCcount];
		this->infoSection.WAVEARCrecord.entries[i].fileID += this->fatSection.count;
		if (other.infoSection.WAVEARCrecord.entries[i - origWAVEARCcount].swar)
		{
			auto newSWAR = std::unique_ptr<SWAR>(new SWAR(*other.infoSection.WAVEARCrecord.entries[i - origWAVEARCcount].swar));
			newSWAR->info = this->infoSection.WAVEARCrecord.entries[i];
			newSWAR->entryNumber = i;
			this->infoSection.WAVEARCrecord.entries[i].swar = newSWAR.get();
			this->SWARs.push_back(std::move(newSWAR));
		}
	}

	this->infoSection.PLAYERrecord.count = this->infoSection.PLAYERrecord.count + other.infoSection.PLAYERrecord.count;
	this->infoSection.PLAYERrecord.entryOffsets.resize(this->infoSection.PLAYERrecord.count, 0);
	std::copy(other.infoSection.PLAYERrecord.entryOffsets.begin(), other.infoSection.PLAYERrecord.entryOffsets.end(),
		this->infoSection.PLAYERrecord.entryOffsets.begin() + origPLAYERcount);
	this->infoSection.PLAYERrecord.entries.resize(this->infoSection.PLAYERrecord.count);
	for (size_t i = origPLAYERcount; i < this->infoSection.PLAYERrecord.count; ++i)
		this->infoSection.PLAYERrecord.entries[i] = other.infoSection.PLAYERrecord.entries[i - origPLAYERcount];

	uint32_t origFileCount = this->fatSection.count;
	this->fatSection.count += other.fatSection.count;
	this->fatSection.records.resize(this->fatSection.count);
	std::copy(other.fatSection.records.begin(), other.fatSection.records.end(), this->fatSection.records.begin() + origFileCount);

	++this->count;

	return *this;
}

typedef std::map<uint32_t, std::vector<uint32_t>> Duplicates;

// This is to find a value in a vector out of a map
static struct FindInVector : std::binary_function<Duplicates::value_type, uint32_t, bool>
{
	bool operator()(const Duplicates::value_type &a, uint32_t b) const
	{
		return std::find(a.second.begin(), a.second.end(), b) != a.second.end();
	}
} findInVector;

// Returns the non-duplicate number of an SBNK or SWAR
static inline uint16_t GetNonDupNumber(uint16_t orig, const Duplicates &duplicates)
{
	if (duplicates.count(orig))
		return orig;
	auto duplicate = std::find_if(duplicates.begin(), duplicates.end(), std::bind2nd(findInVector, orig));
	if (duplicate != duplicates.end())
		return duplicate->first;
	return orig;
}

// Output a vector with comma separation
template<typename T, typename U> static inline void OutputVector(const std::vector<T> &vec, const std::vector<U> &nameSource, bool multipleSDATs,
	const std::string &outputPrefix = " ", size_t columnWidth = 80)
{
	std::string output = outputPrefix;
	std::for_each(vec.begin(), vec.end(), [&](const T &item)
	{
		std::string keep = nameSource[item].FullFilename(multipleSDATs);
		if (output.size() + keep.size() > columnWidth)
		{
			std::cout << output << "\n";
			output = "   ";
		}
		output += " " + keep + ",";
	});
	if (!output.empty())
	{
		output.erase(output.end() - 1);
		std::cout << output << "\n";
	}
}

// Output a map with the vector being comma separation
template<typename T, typename U, typename V> static inline void OutputMap(const std::map<T, U> &map, const std::vector<V> &nameSource, bool multipleSDATs,
	size_t columnWidth = 80)
{
	std::for_each(map.begin(), map.end(), [&](const std::pair<T, U> &curr)
	{
		std::string output = "  " + nameSource[curr.first].FullFilename(multipleSDATs) + ":";
		OutputVector(curr.second, nameSource, multipleSDATs, output, columnWidth);
	});
}

// Strips data out of an SDAT.  This consists of removing duplicate SSEQs, SBNKs, and SWARs,
// as well as the gaps in some SDATs (namely when the SYMB/INFO sections have no offsets
// for items, which can be quite a waste if there are a lot of these gaps).  It also completely
// removes anything that is not an SSEQ, SBNK, or SWAR.
void SDAT::Strip(const IncOrExc &includesAndExcludes, bool verbose, bool removedExcluded)
{
	// Search for duplicate PLAYERs
	Duplicates duplicatePLAYERs;

	for (size_t i = 0, entries = this->infoSection.PLAYERrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.PLAYERrecord.entryOffsets[i]) // Skip empty offsets
			continue;
		auto alreadyFound = std::find_if(duplicatePLAYERs.begin(), duplicatePLAYERs.end(), std::bind2nd(findInVector, i));
		if (alreadyFound != duplicatePLAYERs.end()) // Already added as a duplicate of another PLAYER, skip it
			continue;
		uint16_t imaxSeqs = this->infoSection.PLAYERrecord.entries[i].maxSeqs;
		uint16_t ichannelMask = this->infoSection.PLAYERrecord.entries[i].channelMask;
		uint32_t iheapSize = this->infoSection.PLAYERrecord.entries[i].heapSize;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!this->infoSection.PLAYERrecord.entryOffsets[j]) // Skip empty offsets
				continue;
			if (imaxSeqs != this->infoSection.PLAYERrecord.entries[j].maxSeqs || ichannelMask != this->infoSection.PLAYERrecord.entries[j].channelMask ||
				iheapSize != this->infoSection.PLAYERrecord.entries[j].heapSize) // Player data is different, not duplicates, skip it
				continue;
			duplicates.push_back(j);
		}
		if (!duplicates.empty())
			duplicatePLAYERs.insert(std::make_pair(i, duplicates));
	}

	// Search for duplicate SWARs
	Duplicates duplicateSWARs;

	for (size_t i = 0, entries = this->infoSection.WAVEARCrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.WAVEARCrecord.entryOffsets[i]) // Skip empty offsets
			continue;
		auto alreadyFound = std::find_if(duplicateSWARs.begin(), duplicateSWARs.end(), std::bind2nd(findInVector, i));
		if (alreadyFound != duplicateSWARs.end()) // Already added as a duplicate of another SWAR, skip it
			continue;
		uint16_t ifileID = this->infoSection.WAVEARCrecord.entries[i].fileID;
		uint32_t ifileSize = this->fatSection.records[ifileID].size;
		const auto &ifileData = this->infoSection.WAVEARCrecord.entries[i].fileData;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!this->infoSection.WAVEARCrecord.entryOffsets[j]) // Skip empty offsets
				continue;
			uint16_t jfileID = this->infoSection.WAVEARCrecord.entries[j].fileID;
			uint32_t jfileSize = this->fatSection.records[jfileID].size;
			if (ifileSize != jfileSize) // Files sizes are different, not duplicates, skip it
				continue;
			if (ifileData != this->infoSection.WAVEARCrecord.entries[j].fileData) // File data is different, not duplicates, skip it
				continue;
			duplicates.push_back(j);
		}
		if (!duplicates.empty())
			duplicateSWARs.insert(std::make_pair(i, duplicates));
	}

	// Search for duplicate SBNKs
	Duplicates duplicateSBNKs;

	for (size_t i = 0, entries = this->infoSection.BANKrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.BANKrecord.entryOffsets[i]) // Skip empty offsets
			continue;
		auto alreadyFound = std::find_if(duplicateSBNKs.begin(), duplicateSBNKs.end(), std::bind2nd(findInVector, i));
		if (alreadyFound != duplicateSBNKs.end()) // Already added as a duplicate of another SBNK, skip it
			continue;
		uint16_t ifileID = this->infoSection.BANKrecord.entries[i].fileID;
		uint32_t ifileSize = this->fatSection.records[ifileID].size;
		auto iwaveArc = std::vector<uint16_t>(4, 0xFFFF);
		for (int k = 0; k < 4; ++k)
		{
			uint16_t waveArc = this->infoSection.BANKrecord.entries[i].waveArc[k];
			if (waveArc != 0xFFFF)
				iwaveArc[k] = GetNonDupNumber(waveArc, duplicateSWARs);
		}
		const auto &ifileData = this->infoSection.BANKrecord.entries[i].fileData;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!this->infoSection.BANKrecord.entryOffsets[j]) // Skip empty offsets
				continue;
			uint16_t jfileID = this->infoSection.BANKrecord.entries[j].fileID;
			uint32_t jfileSize = this->fatSection.records[jfileID].size;
			auto jwaveArc = std::vector<uint16_t>(4, 0xFFFF);
			for (int k = 0; k < 4; ++k)
			{
				uint16_t waveArc = this->infoSection.BANKrecord.entries[j].waveArc[k];
				if (waveArc != 0xFFFF)
					jwaveArc[k] = GetNonDupNumber(waveArc, duplicateSWARs);
			}
			if (ifileSize != jfileSize) // File sizes are different, not duplicates, skip it
				continue;
			if (ifileData != this->infoSection.BANKrecord.entries[j].fileData) // File data is different, not duplicates, skip it
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

	for (int i = 0, entries = this->infoSection.SEQrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.SEQrecord.entryOffsets[i]) // Skip empty offsets
			continue;
		if (std::find(excludedSSEQs.begin(), excludedSSEQs.end(), i) != excludedSSEQs.end()) // Skip already excluded files
			continue;
		uint16_t ifileID = this->infoSection.SEQrecord.entries[i].fileID;
		std::string ifilename = this->infoSection.SEQrecord.entries[i].sseq->origFilename;
		auto alreadyFound = std::find_if(duplicateSSEQs.begin(), duplicateSSEQs.end(), std::bind2nd(findInVector, i));
		if (IncludeFilename(ifilename, this->infoSection.SEQrecord.entries[i].sdatNumber, includesAndExcludes) == KEEP_EXCLUDE)
		{
			excludedSSEQs.push_back(i);
			if (alreadyFound != duplicateSSEQs.end())
			{
				auto duplicates = alreadyFound->second;
				duplicates.erase(std::find(duplicates.begin(), duplicates.end(), i));
			}
			continue;
		}

		if (alreadyFound != duplicateSSEQs.end()) // Already added as a duplicate of another SSEQ, skip it
			continue;
		uint32_t ifileSize = this->fatSection.records[ifileID].size;
		uint16_t inonDupBank = GetNonDupNumber(this->infoSection.SEQrecord.entries[i].bank, duplicateSBNKs);
		const auto &ifileData = this->infoSection.SEQrecord.entries[i].fileData;
		std::vector<uint32_t> duplicates;
		for (int j = i + 1; j < entries; ++j)
		{
			if (!this->infoSection.SEQrecord.entryOffsets[j]) // Skip empty offsets
				continue;
			uint16_t jfileID = this->infoSection.SEQrecord.entries[j].fileID;
			uint32_t jfileSize = this->fatSection.records[jfileID].size;
			uint16_t jnonDupBank = GetNonDupNumber(this->infoSection.SEQrecord.entries[j].bank, duplicateSBNKs);
			if (ifileSize != jfileSize) // File sizes are different, not duplicates, skip it
				continue;
			if (ifileData != this->infoSection.SEQrecord.entries[j].fileData) // File data is different, not duplicates, skip it
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

	for (size_t i = 0, entries = this->infoSection.SEQrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.SEQrecord.entryOffsets[i]) // Skip empty offsets
			continue;
		Duplicates::const_iterator duplicate = std::find_if(duplicateSSEQs.begin(), duplicateSSEQs.end(), std::bind2nd(findInVector, i));
		if (duplicate != duplicateSSEQs.end()) // Skip if it is a duplicate
			continue;
		if (removedExcluded && std::find(excludedSSEQs.begin(), excludedSSEQs.end(), i) != excludedSSEQs.end()) // Skip if the user requested it be excluded
			continue;
		SSEQsToKeep.push_back(i);
	}

	// Determine which SBNKs to keep and are being used by the SSEQs we are keeping
	std::vector<uint32_t> SBNKsToKeep;

	for (size_t i = 0, num = SSEQsToKeep.size(); i < num; ++i)
	{
		uint16_t nonDupBank = GetNonDupNumber(this->infoSection.SEQrecord.entries[SSEQsToKeep[i]].bank, duplicateSBNKs);
		if (std::find(SBNKsToKeep.begin(), SBNKsToKeep.end(), nonDupBank) != SBNKsToKeep.end()) // If the SBNK is already in the list to keep, then don't add it again
			continue;
		SBNKsToKeep.push_back(nonDupBank);
	}

	// Sort the list of SBNKs to keep
	std::sort(SBNKsToKeep.begin(), SBNKsToKeep.end());

	// Determine which SWARs to keep and are being used by the SBNKs we are keeping
	std::vector<uint32_t> SWARsToKeep;

	for (size_t i = 0, num = SBNKsToKeep.size(); i < num; ++i)
		for (int j = 0; j < 4; ++j)
		{
			uint16_t waveArc = this->infoSection.BANKrecord.entries[SBNKsToKeep[i]].waveArc[j];
			if (waveArc == 0xFFFF) // Don't bother with the wave archive if it's 0xFFFF, that is the designator for no wave archive
				continue;
			uint16_t nonDupWaveArc = GetNonDupNumber(waveArc, duplicateSWARs);
			if (std::find(SWARsToKeep.begin(), SWARsToKeep.end(), nonDupWaveArc) != SWARsToKeep.end()) // If the SWAR is already in the list to keep, then don't add it again
				continue;
			SWARsToKeep.push_back(nonDupWaveArc);
		}

	// Sort the list of SWARs to keep
	std::sort(SWARsToKeep.begin(), SWARsToKeep.end());

	// Determine which PLAYERs to keep and are being used by SSEQs we are keeping
	std::vector<uint32_t> PLAYERsToKeep;

	size_t numPlayers = this->infoSection.PLAYERrecord.entries.size();
	for (size_t i = 0, num = SSEQsToKeep.size(); i < num; ++i)
	{
		uint16_t nonDupPlayer = GetNonDupNumber(this->infoSection.SEQrecord.entries[SSEQsToKeep[i]].ply, duplicatePLAYERs);
		if (std::find(PLAYERsToKeep.begin(), PLAYERsToKeep.end(), nonDupPlayer) != PLAYERsToKeep.end()) // If the PLAYER is already in the list to keep, then don't add it again
			continue;
		if (numPlayers <= nonDupPlayer) // Somehow, some SDATs can have no players...
			continue;
		PLAYERsToKeep.push_back(nonDupPlayer);
	}

	// Sort the list of PLAYERs to keep
	std::sort(PLAYERsToKeep.begin(), PLAYERsToKeep.end());

	// If verbosity is turned on, output which files will be kept and which are being removed as duplicates
	if (verbose)
	{
		if (removedExcluded && !excludedSSEQs.empty())
		{
			std::cout << "The following SSEQ" << (excludedSSEQs.size() == 1 ? "" : "s") << " were excluded by request:\n";
			OutputVector(excludedSSEQs, this->infoSection.SEQrecord.entries, this->count > 1);
			std::cout << "\n";
		}

		if (!duplicateSSEQs.empty())
		{
			std::cout << "The following SSEQ" << (duplicateSSEQs.size() != 1 ? "s" : "") << " had duplicates, the duplicates will be removed:\n";
			OutputMap(duplicateSSEQs, this->infoSection.SEQrecord.entries, this->count > 1);
			std::cout << "\n";
		}

		if (!duplicateSBNKs.empty())
		{
			std::cout << "The following SBNK" << (duplicateSBNKs.size() != 1 ? "s" : "") << " had duplicates, the duplicates will be removed:\n";
			OutputMap(duplicateSBNKs, this->infoSection.BANKrecord.entries, this->count > 1);
			std::cout << "\n";
		}

		if (!duplicateSWARs.empty())
		{
			std::cout << "The following SWAR" << (duplicateSWARs.size() != 1 ? "s" : "") << " had duplicates, the duplicates will be removed:\n";
			OutputMap(duplicateSWARs, this->infoSection.WAVEARCrecord.entries, this->count > 1);
			std::cout << "\n";
		}

		if (!duplicatePLAYERs.empty())
		{
			std::cout << "The following PLAYER" << (duplicatePLAYERs.size() != 1 ? "s" : "") << " had duplicates, the duplicates will be removed:\n";
			OutputMap(duplicatePLAYERs, this->infoSection.PLAYERrecord.entries, this->count > 1);
			std::cout << "\n";
		}
	}

	// Figure out where the remaining SBNKs will be once moved
	std::map<uint32_t, uint32_t> SBNKMove;

	for (size_t i = 0, num = SBNKsToKeep.size(); i < num; ++i)
		SBNKMove[SBNKsToKeep[i]] = i;

	// Figure out where the remaining SWARs will be once moved
	std::map<uint32_t, uint32_t> SWARMove;

	for (size_t i = 0, num = SWARsToKeep.size(); i < num; ++i)
		SWARMove[SWARsToKeep[i]] = i;

	// Figure out where the remaining PLAYERs wil be once moved
	std::map<uint32_t, uint32_t> PLAYERMove;

	for (size_t i = 0, num = PLAYERsToKeep.size(); i < num; ++i)
		PLAYERMove[PLAYERsToKeep[i]] = i;

	// Remove all unused items (or rather, save only used items)
	SYMBSection newSymbSection;
	if (this->SYMBOffset)
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

		newSymbSection.PLAYERrecord.count = PLAYERsToKeep.size();
		newSymbSection.PLAYERrecord.entryOffsets.resize(newSymbSection.PLAYERrecord.count);
		newSymbSection.PLAYERrecord.entries.resize(newSymbSection.PLAYERrecord.count);
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

	newInfoSection.PLAYERrecord.count = PLAYERsToKeep.size();
	newInfoSection.PLAYERrecord.entryOffsets.resize(newInfoSection.PLAYERrecord.count);
	newInfoSection.PLAYERrecord.entries.resize(newInfoSection.PLAYERrecord.count);

	std::vector<std::unique_ptr<SSEQ>> newSSEQs;
	uint16_t fileID = 0;
	for (size_t i = 0, num = SSEQsToKeep.size(); i < num; ++i)
	{
		if (this->SYMBOffset)
			newSymbSection.SEQrecord.entries[i] = this->symbSection.SEQrecord.entries[SSEQsToKeep[i]];

		newInfoSection.SEQrecord.entries[i] = this->infoSection.SEQrecord.entries[SSEQsToKeep[i]];
		newInfoSection.SEQrecord.entries[i].fileID = fileID++;
		uint16_t nonDupBank = GetNonDupNumber(newInfoSection.SEQrecord.entries[i].bank, duplicateSBNKs);
		newInfoSection.SEQrecord.entries[i].bank = SBNKMove[nonDupBank];
		uint16_t nonDupPlayer = GetNonDupNumber(newInfoSection.SEQrecord.entries[i].ply, duplicatePLAYERs);
		newInfoSection.SEQrecord.entries[i].ply = PLAYERMove[nonDupPlayer];
		auto sseq = std::find_if(this->SSEQs.begin(), this->SSEQs.end(), [&](const std::unique_ptr<SSEQ> &thisSSEQ)
		{
			return thisSSEQ.get() == newInfoSection.SEQrecord.entries[i].sseq;
		});
		(*sseq)->info = newInfoSection.SEQrecord.entries[i];
		(*sseq)->entryNumber = i;
		newSSEQs.push_back(std::move(*sseq));
		this->SSEQs.erase(sseq);
	}

	std::vector<std::unique_ptr<SBNK>> newSBNKs;
	for (size_t i = 0, num = SBNKsToKeep.size(); i < num; ++i)
	{
		if (this->SYMBOffset)
			newSymbSection.BANKrecord.entries[i] = this->symbSection.BANKrecord.entries[SBNKsToKeep[i]];

		newInfoSection.BANKrecord.entries[i] = this->infoSection.BANKrecord.entries[SBNKsToKeep[i]];
		newInfoSection.BANKrecord.entries[i].fileID = fileID++;
		for (int j = 0; j < 4; ++j)
		{
			uint16_t waveArc = newInfoSection.BANKrecord.entries[i].waveArc[j];
			if (waveArc == 0xFFFF)
				continue;
			uint16_t nonDupWaveArc = GetNonDupNumber(waveArc, duplicateSWARs);
			newInfoSection.BANKrecord.entries[i].waveArc[j] = SWARMove[nonDupWaveArc];
		}
		auto sbnk = std::find_if(this->SBNKs.begin(), this->SBNKs.end(), [&](const std::unique_ptr<SBNK> &thisSBNK)
		{
			return thisSBNK.get() == newInfoSection.BANKrecord.entries[i].sbnk;
		});
		(*sbnk)->info = newInfoSection.BANKrecord.entries[i];
		(*sbnk)->entryNumber = i;
		newSBNKs.push_back(std::move(*sbnk));
		this->SBNKs.erase(sbnk);
	}

	std::vector<std::unique_ptr<SWAR>> newSWARs;
	for (size_t i = 0, num = SWARsToKeep.size(); i < num; ++i)
	{
		if (this->SYMBOffset)
			newSymbSection.WAVEARCrecord.entries[i] = this->symbSection.WAVEARCrecord.entries[SWARsToKeep[i]];

		newInfoSection.WAVEARCrecord.entries[i] = this->infoSection.WAVEARCrecord.entries[SWARsToKeep[i]];
		newInfoSection.WAVEARCrecord.entries[i].fileID = fileID++;
		auto swar = std::find_if(this->SWARs.begin(), this->SWARs.end(), [&](const std::unique_ptr<SWAR> &thisSWAR)
		{
			return thisSWAR.get() == newInfoSection.WAVEARCrecord.entries[i].swar;
		});
		(*swar)->info = newInfoSection.WAVEARCrecord.entries[i];
		(*swar)->entryNumber = i;
		newSWARs.push_back(std::move(*swar));
		this->SWARs.erase(swar);
	}

	for (size_t i = 0, num = PLAYERsToKeep.size(); i < num; ++i)
	{
		if (this->SYMBOffset)
			newSymbSection.PLAYERrecord.entries[i] = this->symbSection.PLAYERrecord.entries[PLAYERsToKeep[i]];

		newInfoSection.PLAYERrecord.entries[i] = this->infoSection.PLAYERrecord.entries[PLAYERsToKeep[i]];
	}

	if (this->SYMBOffset)
		this->symbSection = newSymbSection;
	this->infoSection = newInfoSection;

	this->SSEQs = std::move(newSSEQs);
	this->SBNKs = std::move(newSBNKs);
	this->SWARs = std::move(newSWARs);

	FATSection newFatSection;

	newFatSection.count = fileID;
	newFatSection.records.resize(newFatSection.count);

	this->fatSection = newFatSection;

	// Calculate new offsets and sizes
	this->INFOOffset = 0x40;
	if (this->SYMBOffset)
	{
		this->SYMBSize = this->symbSection.size = this->symbSection.Size();
		this->symbSection.FixOffsets();
		this->INFOOffset = this->SYMBOffset + ((this->SYMBSize + 3) & ~0x03);
	}
	this->INFOSize = this->infoSection.size = this->infoSection.Size();
	this->infoSection.FixOffsets();
	this->FATOffset = this->INFOOffset + this->INFOSize;
	this->FATSize = this->fatSection.size = this->fatSection.Size();
	this->FILEOffset = this->FATOffset + this->FATSize;
	uint32_t offset = this->FILEOffset + 24;
	this->FILESize = 0x18;
	fileID = 0;
	for (uint32_t i = 0, num = SSEQsToKeep.size(); i < num; ++i)
	{
		this->fatSection.records[fileID].offset = offset;
		uint32_t fileSize = this->infoSection.SEQrecord.entries[i].fileData.size();
		this->fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		this->FILESize += fileSize;
	}
	for (uint32_t i = 0, num = SBNKsToKeep.size(); i < num; ++i)
	{
		this->fatSection.records[fileID].offset = offset;
		uint32_t fileSize = this->infoSection.BANKrecord.entries[i].fileData.size();
		this->fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		this->FILESize += fileSize;
	}
	for (uint32_t i = 0, num = SWARsToKeep.size(); i < num; ++i)
	{
		this->fatSection.records[fileID].offset = offset;
		uint32_t fileSize = this->infoSection.WAVEARCrecord.entries[i].fileData.size();
		this->fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		this->FILESize += fileSize;
	}

	this->header.fileSize = this->FILEOffset + this->FILESize;

	// If one of the files that was merged into this one had no SYMB section, then we need to fill in some dummy data for those entries
	if (this->symbSectionNeedsCleanup)
	{
		for (uint32_t i = 0, num = SSEQsToKeep.size(); i < num; ++i)
		{
			fileID = this->infoSection.SEQrecord.entries[i].fileID;
			if (this->symbSection.SEQrecord.entries[i].empty())
				this->symbSection.SEQrecord.entries[i] = "SSEQ" + NumToHexString(fileID).substr(2);
			auto sseq = std::find_if(this->SSEQs.begin(), this->SSEQs.end(), [&](const std::unique_ptr<SSEQ> &thisSSEQ)
			{
				return thisSSEQ.get() == this->infoSection.SEQrecord.entries[i].sseq;
			});
			if (sseq != this->SSEQs.end())
			{
				(*sseq)->origFilename = this->symbSection.SEQrecord.entries[i];
				if (this->symbSection.SEQrecord.entries[i].substr(0, 4) != "SSEQ")
					(*sseq)->filename = NumToHexString(i).substr(6) + " - " + this->symbSection.SEQrecord.entries[i];
			}
		}
		for (uint32_t i = 0, num = SBNKsToKeep.size(); i < num; ++i)
		{
			if (!this->symbSection.BANKrecord.entries[i].empty())
				continue;
			fileID = this->infoSection.BANKrecord.entries[i].fileID;
			this->symbSection.BANKrecord.entries[i] = "SBNK" + NumToHexString(fileID).substr(2);
		}
		for (uint32_t i = 0, num = SWARsToKeep.size(); i < num; ++i)
		{
			if (!this->symbSection.WAVEARCrecord.entries[i].empty())
				continue;
			fileID = this->infoSection.WAVEARCrecord.entries[i].fileID;
			this->symbSection.WAVEARCrecord.entries[i] = "SWAR" + NumToHexString(fileID).substr(2);
		}
		for (uint32_t i = 0, num = PLAYERsToKeep.size(); i < num; ++i)
		{
			if (!this->symbSection.PLAYERrecord.entries[i].empty())
				continue;
			this->symbSection.PLAYERrecord.entries[i] = "PLAYER" + NumToHexString<uint8_t>(i).substr(2);
		}
	}
}
