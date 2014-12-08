/*
 * SDAT - SDAT structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-12-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include <functional>
#include <iostream>
#include "SDAT.h"
#include "TimerTrack.h"

bool SDAT::failOnMissingFiles = true;

SDAT::SDAT() : filename(""), header(), SYMBOffset(0), SYMBSize(0), INFOOffset(0), INFOSize(0), FATOffset(0), FATSize(0), FILEOffset(0), FILESize(0), symbSection(),
	infoSection(), fatSection(), symbSectionNeedsCleanup(false), count(0), SSEQs(), SBNKs(), SWARs()
{
	memcpy(this->header.type, "SDAT", sizeof(this->header.type));
	this->header.magic = 0x0100FEFF;
	this->header.size = 0x40;
	this->header.blocks = 3;
}

SDAT::SDAT(const SDAT &sdat) : filename(sdat.filename), header(sdat.header), SYMBOffset(sdat.SYMBOffset), SYMBSize(sdat.SYMBSize), INFOOffset(sdat.INFOOffset),
	INFOSize(sdat.INFOSize), FATOffset(sdat.FATOffset), FATSize(sdat.FATSize), FILEOffset(sdat.FILEOffset), FILESize(sdat.FILESize), symbSection(sdat.symbSection),
	infoSection(sdat.infoSection), fatSection(sdat.fatSection), symbSectionNeedsCleanup(sdat.symbSectionNeedsCleanup), count(sdat.count), SSEQs(), SBNKs(), SWARs()
{
	std::for_each(sdat.SSEQs.begin(), sdat.SSEQs.end(), [&](const SSEQList::value_type &sseq)
	{
		auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(*sseq.get()));
		auto &entry = this->infoSection.SEQrecord.entries[sseq->entryNumber];
		entry.sseq = newSSEQ.get();
		this->SSEQs.push_back(std::move(newSSEQ));
	});
	std::for_each(sdat.SBNKs.begin(), sdat.SBNKs.end(), [&](const SBNKList::value_type &sbnk)
	{
		auto newSBNK = std::unique_ptr<SBNK>(new SBNK(*sbnk.get()));
		auto &entry = this->infoSection.BANKrecord.entries[sbnk->entryNumber];
		entry.sbnk = newSBNK.get();
		this->SBNKs.push_back(std::move(newSBNK));
	});
	std::for_each(sdat.SWARs.begin(), sdat.SWARs.end(), [&](const SWARList::value_type &swar)
	{
		auto newSWAR = std::unique_ptr<SWAR>(new SWAR(*swar.get()));
		auto &entry = this->infoSection.WAVEARCrecord.entries[swar->entryNumber];
		entry.swar = newSWAR.get();
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

		std::for_each(sdat.SSEQs.begin(), sdat.SSEQs.end(), [&](const SSEQList::value_type &sseq)
		{
			auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(*sseq.get()));
			auto &entry = this->infoSection.SEQrecord.entries[sseq->entryNumber];
			entry.sseq = newSSEQ.get();
			this->SSEQs.push_back(std::move(newSSEQ));
		});
		std::for_each(sdat.SBNKs.begin(), sdat.SBNKs.end(), [&](const SBNKList::value_type &sbnk)
		{
			auto newSBNK = std::unique_ptr<SBNK>(new SBNK(*sbnk.get()));
			auto &entry = this->infoSection.BANKrecord.entries[sbnk->entryNumber];
			entry.sbnk = newSBNK.get();
			this->SBNKs.push_back(std::move(newSBNK));
		});
		std::for_each(sdat.SWARs.begin(), sdat.SWARs.end(), [&](const SWARList::value_type &swar)
		{
			auto newSWAR = std::unique_ptr<SWAR>(new SWAR(*swar.get()));
			auto &entry = this->infoSection.WAVEARCrecord.entries[swar->entryNumber];
			entry.swar = newSWAR.get();
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
		auto &entry = this->infoSection.SEQrecord.entries[i];
		uint16_t fileID = entry.fileID;
		std::string origName = "SSEQ" + NumToHexString(fileID).substr(2), name = origName;
		if (this->SYMBOffset)
		{
			origName = this->symbSection.SEQrecord.entries[i];
			name = NumToHexString<uint32_t>(i).substr(6) + " - " + origName;
		}
		entry.origFilename = origName;
		entry.sdatNumber = this->filename;
		file.pos = this->fatSection.records[fileID].offset;
		entry.fileData.resize(this->fatSection.records[fileID].size, 0);
		file.ReadLE(entry.fileData);
		file.pos = this->fatSection.records[fileID].offset;
		auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(name, origName));
		entry.sseq = newSSEQ.get();
		newSSEQ->entryNumber = i;
		newSSEQ->Read(file);
		this->SSEQs.push_back(std::move(newSSEQ));
	}
	for (size_t i = 0, entries = this->infoSection.BANKrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.BANKrecord.entryOffsets[i])
			continue;
		auto &entry = this->infoSection.BANKrecord.entries[i];
		uint16_t fileID = entry.fileID;
		std::string origName = "SBNK" + NumToHexString(fileID).substr(2);
		if (this->SYMBOffset)
			origName = this->symbSection.BANKrecord.entries[i];
		entry.origFilename = origName;
		entry.sdatNumber = this->filename;
		file.pos = this->fatSection.records[fileID].offset;
		entry.fileData.resize(this->fatSection.records[fileID].size, 0);
		file.ReadLE(entry.fileData);
		file.pos = this->fatSection.records[fileID].offset;
		auto newSBNK = std::unique_ptr<SBNK>(new SBNK(origName));
		entry.sbnk = newSBNK.get();
		newSBNK->entryNumber = i;
		newSBNK->Read(file);
		this->SBNKs.push_back(std::move(newSBNK));
	}
	for (size_t i = 0, entries = this->infoSection.WAVEARCrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.WAVEARCrecord.entryOffsets[i])
			continue;
		auto &entry = this->infoSection.WAVEARCrecord.entries[i];
		uint16_t fileID = entry.fileID;
		std::string origName = "SWAR" + NumToHexString(fileID).substr(2);
		if (this->SYMBOffset)
			origName = this->symbSection.WAVEARCrecord.entries[i];
		entry.origFilename = origName;
		entry.sdatNumber = this->filename;
		file.pos = this->fatSection.records[fileID].offset;
		entry.fileData.resize(this->fatSection.records[fileID].size, 0);
		file.ReadLE(entry.fileData);
		file.pos = this->fatSection.records[fileID].offset;
		auto newSWAR = std::unique_ptr<SWAR>(new SWAR(origName));
		entry.swar = newSWAR.get();
		newSWAR->entryNumber = i;
		newSWAR->Read(file);
		this->SWARs.push_back(std::move(newSWAR));
	}
	for (size_t i = 0, entries = this->infoSection.PLAYERrecord.entries.size(); i < entries; ++i)
	{
		if (!this->infoSection.PLAYERrecord.entryOffsets[i])
			continue;
		auto &entry = this->infoSection.PLAYERrecord.entries[i];
		std::string origName = "PLAYER" + NumToHexString<uint8_t>(i).substr(2);
		if (this->SYMBOffset)
			origName = this->symbSection.PLAYERrecord.entries[i];
		entry.origFilename = origName;
		entry.sdatNumber = this->filename;
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

// Makes an SDAT from the current SDAT that contains only information for the SSEQ requested.
// NOTE: This purposely creates a semi-invalid SDAT, as it is expected to be fixed by the
//       stripping process.
SDAT SDAT::MakeFromSSEQ(uint16_t SSEQNumber) const
{
	SDAT newSDAT;
	auto &oldSEQEntry = this->infoSection.SEQrecord.entries[SSEQNumber];
	uint16_t BANKNumber = oldSEQEntry.bank;
	uint16_t WAVEARCNumbers[4];
	auto &oldBANKEntry = this->infoSection.BANKrecord.entries[BANKNumber];
	int i;
	for (i = 0; i < 4; ++i)
		WAVEARCNumbers[i] = oldBANKEntry.waveArc[i];
	uint8_t PLAYERNumber = oldSEQEntry.ply;

	if (this->SYMBOffset)
	{
		newSDAT.SYMBOffset = 0x40;

		newSDAT.symbSection.SEQrecord.count = 1;
		newSDAT.symbSection.SEQrecord.entryOffsets.push_back(1);
		newSDAT.symbSection.SEQrecord.entries.push_back(this->symbSection.SEQrecord.entries[SSEQNumber]);

		newSDAT.symbSection.BANKrecord.count = 1;
		newSDAT.symbSection.BANKrecord.entryOffsets.push_back(1);
		newSDAT.symbSection.BANKrecord.entries.push_back(this->symbSection.BANKrecord.entries[BANKNumber]);

		for (i = 0; i < 4; ++i)
			if (WAVEARCNumbers[i] != 0xFFFF)
			{
				++newSDAT.symbSection.WAVEARCrecord.count;
				newSDAT.symbSection.WAVEARCrecord.entryOffsets.push_back(1);
				newSDAT.symbSection.WAVEARCrecord.entries.push_back(this->symbSection.WAVEARCrecord.entries[WAVEARCNumbers[i]]);
			}

		if (PLAYERNumber < this->symbSection.PLAYERrecord.count)
		{
			newSDAT.symbSection.PLAYERrecord.count = 1;
			newSDAT.symbSection.PLAYERrecord.entryOffsets.push_back(1);
			newSDAT.symbSection.PLAYERrecord.entries.push_back(this->symbSection.PLAYERrecord.entries[PLAYERNumber]);
		}
	}

	newSDAT.infoSection.SEQrecord.count = 1;
	newSDAT.infoSection.SEQrecord.entryOffsets.push_back(1);
	newSDAT.infoSection.SEQrecord.entries.push_back(oldSEQEntry);
	auto &newSEQEntry = newSDAT.infoSection.SEQrecord.entries[0];
	newSEQEntry.fileID = newSEQEntry.bank = 0;
	newSEQEntry.ply = 0;
	if (oldSEQEntry.sseq)
	{
		auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(*oldSEQEntry.sseq));
		newSEQEntry.sseq = newSSEQ.get();
		newSSEQ->entryNumber = 0;
		newSDAT.SSEQs.push_back(std::move(newSSEQ));
	}

	newSDAT.infoSection.BANKrecord.count = 1;
	newSDAT.infoSection.BANKrecord.entryOffsets.push_back(1);
	newSDAT.infoSection.BANKrecord.entries.push_back(oldBANKEntry);
	auto &newBANKEntry = newSDAT.infoSection.BANKrecord.entries[0];
	newBANKEntry.fileID = 1;
	std::fill_n(&newBANKEntry.waveArc[0], 4, 0xFFFF);
	if (oldBANKEntry.sbnk)
	{
		auto newSBNK = std::unique_ptr<SBNK>(new SBNK(*oldBANKEntry.sbnk));
		newBANKEntry.sbnk = newSBNK.get();
		newSBNK->entryNumber = 0;
		newSDAT.SBNKs.push_back(std::move(newSBNK));
	}

	uint16_t fileID = 2;
	for (i = 0; i < 4; ++i)
		if (WAVEARCNumbers[i] != 0xFFFF)
		{
			auto &oldWAVEARCEntry = this->infoSection.WAVEARCrecord.entries[WAVEARCNumbers[i]];
			int j = fileID - 2;
			++newSDAT.infoSection.WAVEARCrecord.count;
			newSDAT.infoSection.WAVEARCrecord.entryOffsets.push_back(1);
			newSDAT.infoSection.WAVEARCrecord.entries.push_back(oldWAVEARCEntry);
			auto &newWAVEARCEntry = newSDAT.infoSection.WAVEARCrecord.entries[j];
			newWAVEARCEntry.fileID = fileID++;
			newBANKEntry.waveArc[j] = j;
			if (oldWAVEARCEntry.swar)
			{
				auto newSWAR = std::unique_ptr<SWAR>(new SWAR(*oldWAVEARCEntry.swar));
				newWAVEARCEntry.swar = newSWAR.get();
				newSWAR->entryNumber = j;
				newSDAT.SWARs.push_back(std::move(newSWAR));
			}
		}

	if (PLAYERNumber < this->infoSection.PLAYERrecord.count)
	{
		newSDAT.infoSection.PLAYERrecord.count = 1;
		newSDAT.infoSection.PLAYERrecord.entryOffsets.push_back(1);
		newSDAT.infoSection.PLAYERrecord.entries.push_back(this->infoSection.PLAYERrecord.entries[PLAYERNumber]);
	}

	newSDAT.fatSection.count = fileID;

	return newSDAT;
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
		auto &thisSEQEntry = this->infoSection.SEQrecord.entries[i];
		auto &otherSEQEntry = other.infoSection.SEQrecord.entries[i - origSEQcount];
		thisSEQEntry = otherSEQEntry;
		thisSEQEntry.fileID += this->fatSection.count;
		thisSEQEntry.bank += origBANKcount;
		thisSEQEntry.ply += origPLAYERcount;
		if (otherSEQEntry.sseq)
		{
			auto newSSEQ = std::unique_ptr<SSEQ>(new SSEQ(*otherSEQEntry.sseq));
			thisSEQEntry.sseq = newSSEQ.get();
			newSSEQ->entryNumber = i;
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
		auto &thisBANKEntry = this->infoSection.BANKrecord.entries[i];
		auto &otherBANKEntry = other.infoSection.BANKrecord.entries[i - origBANKcount];
		thisBANKEntry = otherBANKEntry;
		thisBANKEntry.fileID += this->fatSection.count;
		for (size_t j = 0; j < 4; ++j)
			if (thisBANKEntry.waveArc[j] != 0xFFFF)
				thisBANKEntry.waveArc[j] += origWAVEARCcount;
		if (otherBANKEntry.sbnk)
		{
			auto newSBNK = std::unique_ptr<SBNK>(new SBNK(*otherBANKEntry.sbnk));
			thisBANKEntry.sbnk = newSBNK.get();
			newSBNK->entryNumber = i;
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
		auto &thisWAVEARCEntry = this->infoSection.WAVEARCrecord.entries[i];
		auto &otherWAVEARCEntry = other.infoSection.WAVEARCrecord.entries[i - origWAVEARCcount];
		thisWAVEARCEntry = otherWAVEARCEntry;
		thisWAVEARCEntry.fileID += this->fatSection.count;
		if (otherWAVEARCEntry.swar)
		{
			auto newSWAR = std::unique_ptr<SWAR>(new SWAR(*otherWAVEARCEntry.swar));
			thisWAVEARCEntry.swar = newSWAR.get();
			newSWAR->entryNumber = i;
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
	std::for_each(map.begin(), map.end(), [&](const typename std::map<T, U>::value_type &curr)
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
		if (std::find_if(duplicatePLAYERs.begin(), duplicatePLAYERs.end(), std::bind2nd(findInVector, i)) != duplicatePLAYERs.end()) // Already added as a duplicate of another PLAYER, skip it
			continue;
		auto &ientry = this->infoSection.PLAYERrecord.entries[i];
		uint16_t imaxSeqs = ientry.maxSeqs;
		uint16_t ichannelMask = ientry.channelMask;
		uint32_t iheapSize = ientry.heapSize;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!this->infoSection.PLAYERrecord.entryOffsets[j]) // Skip empty offsets
				continue;
			auto &jentry = this->infoSection.PLAYERrecord.entries[j];
			if (imaxSeqs != jentry.maxSeqs || ichannelMask != jentry.channelMask || iheapSize != jentry.heapSize) // Player data is different, not duplicates, skip it
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
		if (std::find_if(duplicateSWARs.begin(), duplicateSWARs.end(), std::bind2nd(findInVector, i)) != duplicateSWARs.end()) // Already added as a duplicate of another SWAR, skip it
			continue;
		auto &ientry = this->infoSection.WAVEARCrecord.entries[i];
		uint16_t ifileID = ientry.fileID;
		uint32_t ifileSize = this->fatSection.records[ifileID].size;
		const auto &ifileData = ientry.fileData;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!this->infoSection.WAVEARCrecord.entryOffsets[j]) // Skip empty offsets
				continue;
			auto &jentry = this->infoSection.WAVEARCrecord.entries[j];
			uint16_t jfileID = jentry.fileID;
			uint32_t jfileSize = this->fatSection.records[jfileID].size;
			if (ifileSize != jfileSize) // Files sizes are different, not duplicates, skip it
				continue;
			if (ifileData != jentry.fileData) // File data is different, not duplicates, skip it
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
		if (std::find_if(duplicateSBNKs.begin(), duplicateSBNKs.end(), std::bind2nd(findInVector, i)) != duplicateSBNKs.end()) // Already added as a duplicate of another SBNK, skip it
			continue;
		auto &ientry = this->infoSection.BANKrecord.entries[i];
		uint16_t ifileID = ientry.fileID;
		uint32_t ifileSize = this->fatSection.records[ifileID].size;
		auto iwaveArc = std::vector<uint16_t>(4, 0xFFFF);
		for (int k = 0; k < 4; ++k)
		{
			uint16_t waveArc = ientry.waveArc[k];
			if (waveArc != 0xFFFF)
				iwaveArc[k] = GetNonDupNumber(waveArc, duplicateSWARs);
		}
		const auto &ifileData = ientry.fileData;
		std::vector<uint32_t> duplicates;
		for (size_t j = i + 1; j < entries; ++j)
		{
			if (!this->infoSection.BANKrecord.entryOffsets[j]) // Skip empty offsets
				continue;
			auto &jentry = this->infoSection.BANKrecord.entries[j];
			uint16_t jfileID = jentry.fileID;
			uint32_t jfileSize = this->fatSection.records[jfileID].size;
			if (ifileSize != jfileSize) // File sizes are different, not duplicates, skip it
				continue;
			if (ifileData != jentry.fileData) // File data is different, not duplicates, skip it
				continue;
			auto jwaveArc = std::vector<uint16_t>(4, 0xFFFF);
			for (int k = 0; k < 4; ++k)
			{
				uint16_t waveArc = jentry.waveArc[k];
				if (waveArc != 0xFFFF)
					jwaveArc[k] = GetNonDupNumber(waveArc, duplicateSWARs);
			}
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
		auto &ientry = this->infoSection.SEQrecord.entries[i];
		uint16_t ifileID = ientry.fileID;
		std::string ifilename = ientry.sseq->origFilename;
		auto alreadyFound = std::find_if(duplicateSSEQs.begin(), duplicateSSEQs.end(), std::bind2nd(findInVector, i));
		if (IncludeFilename(ifilename, ientry.sdatNumber, includesAndExcludes) == KEEP_EXCLUDE)
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
		uint16_t inonDupBank = GetNonDupNumber(ientry.bank, duplicateSBNKs);
		const auto &ifileData = ientry.fileData;
		std::vector<uint32_t> duplicates;
		for (int j = i + 1; j < entries; ++j)
		{
			if (!this->infoSection.SEQrecord.entryOffsets[j]) // Skip empty offsets
				continue;
			auto &jentry = this->infoSection.SEQrecord.entries[j];
			uint16_t jfileID = jentry.fileID;
			uint32_t jfileSize = this->fatSection.records[jfileID].size;
			if (ifileSize != jfileSize) // File sizes are different, not duplicates, skip it
				continue;
			if (ifileData != jentry.fileData) // File data is different, not duplicates, skip it
				continue;
			uint16_t jnonDupBank = GetNonDupNumber(jentry.bank, duplicateSBNKs);
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
		if (std::find_if(duplicateSSEQs.begin(), duplicateSSEQs.end(), std::bind2nd(findInVector, i)) != duplicateSSEQs.end()) // Skip if it is a duplicate
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

	SSEQList newSSEQs;
	uint16_t fileID = 0;
	for (size_t i = 0, num = SSEQsToKeep.size(); i < num; ++i)
	{
		if (this->SYMBOffset)
			newSymbSection.SEQrecord.entries[i] = this->symbSection.SEQrecord.entries[SSEQsToKeep[i]];

		auto &newSEQEntry = newInfoSection.SEQrecord.entries[i];
		newSEQEntry = this->infoSection.SEQrecord.entries[SSEQsToKeep[i]];
		newSEQEntry.fileID = fileID++;
		uint16_t nonDupBank = GetNonDupNumber(newSEQEntry.bank, duplicateSBNKs);
		newSEQEntry.bank = SBNKMove[nonDupBank];
		uint16_t nonDupPlayer = GetNonDupNumber(newSEQEntry.ply, duplicatePLAYERs);
		newSEQEntry.ply = PLAYERMove[nonDupPlayer];
		auto sseq = this->GetNonConstSSEQ(newSEQEntry.sseq);
		(*sseq)->entryNumber = i;
		newSSEQs.push_back(std::move(*sseq));
		this->SSEQs.erase(sseq);
	}

	SBNKList newSBNKs;
	for (size_t i = 0, num = SBNKsToKeep.size(); i < num; ++i)
	{
		if (this->SYMBOffset)
			newSymbSection.BANKrecord.entries[i] = this->symbSection.BANKrecord.entries[SBNKsToKeep[i]];

		auto &newBANKEntry = newInfoSection.BANKrecord.entries[i];
		newBANKEntry = this->infoSection.BANKrecord.entries[SBNKsToKeep[i]];
		newBANKEntry.fileID = fileID++;
		for (int j = 0; j < 4; ++j)
		{
			uint16_t waveArc = newBANKEntry.waveArc[j];
			if (waveArc == 0xFFFF)
				continue;
			uint16_t nonDupWaveArc = GetNonDupNumber(waveArc, duplicateSWARs);
			newBANKEntry.waveArc[j] = SWARMove[nonDupWaveArc];
		}
		auto sbnk = this->GetNonConstSBNK(newBANKEntry.sbnk);
		(*sbnk)->entryNumber = i;
		newSBNKs.push_back(std::move(*sbnk));
		this->SBNKs.erase(sbnk);
	}

	SWARList newSWARs;
	for (size_t i = 0, num = SWARsToKeep.size(); i < num; ++i)
	{
		if (this->SYMBOffset)
			newSymbSection.WAVEARCrecord.entries[i] = this->symbSection.WAVEARCrecord.entries[SWARsToKeep[i]];

		auto &newWAVEARCEntry = newInfoSection.WAVEARCrecord.entries[i];
		newWAVEARCEntry = this->infoSection.WAVEARCrecord.entries[SWARsToKeep[i]];
		newWAVEARCEntry.fileID = fileID++;
		auto swar = this->GetNonConstSWAR(newWAVEARCEntry.swar);
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

	// If one of the files that was merged into this one had no SYMB section, then we need to fill in some dummy data for those entries
	if (this->symbSectionNeedsCleanup)
	{
		for (uint32_t i = 0, num = SSEQsToKeep.size(); i < num; ++i)
		{
			auto &symbEntry = this->symbSection.SEQrecord.entries[i];
			auto &infoEntry = this->infoSection.SEQrecord.entries[i];
			fileID = infoEntry.fileID;
			if (symbEntry.empty())
				symbEntry = "SSEQ" + NumToHexString(fileID).substr(2);
			auto sseq = this->GetNonConstSSEQ(infoEntry.sseq);
			if (sseq != this->SSEQs.end())
			{
				(*sseq)->origFilename = symbEntry;
				if (symbEntry.substr(0, 4) != "SSEQ")
					(*sseq)->filename = NumToHexString(i).substr(6) + " - " + symbEntry;
			}
		}
		for (uint32_t i = 0, num = SBNKsToKeep.size(); i < num; ++i)
		{
			auto &symbEntry = this->symbSection.BANKrecord.entries[i];
			if (!symbEntry.empty())
				continue;
			fileID = this->infoSection.BANKrecord.entries[i].fileID;
			symbEntry = "SBNK" + NumToHexString(fileID).substr(2);
		}
		for (uint32_t i = 0, num = SWARsToKeep.size(); i < num; ++i)
		{
			auto &symbEntry = this->symbSection.WAVEARCrecord.entries[i];
			if (!symbEntry.empty())
				continue;
			fileID = this->infoSection.WAVEARCrecord.entries[i].fileID;
			symbEntry = "SWAR" + NumToHexString(fileID).substr(2);
		}
		for (uint32_t i = 0, num = PLAYERsToKeep.size(); i < num; ++i)
		{
			auto &symbEntry = this->symbSection.PLAYERrecord.entries[i];
			if (!symbEntry.empty())
				continue;
			symbEntry = "PLAYER" + NumToHexString<uint8_t>(i).substr(2);
		}

		this->symbSectionNeedsCleanup = false;
	}

	// Fix the offsets and sizes
	this->FixOffsetsAndSizes();
}

template<typename T> static inline void MergeUniqueVector(const std::vector<T> &src, std::vector<T> &dest)
{
	dest.insert(dest.end(), src.begin(), src.end());
	std::sort(dest.begin(), dest.end());
	auto last = std::unique(dest.begin(), dest.end());
	dest.erase(last, dest.end());
}

// Comes from http://techoverflow.net/blog/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/
// But modified to use a vector instead
template<typename T> static inline std::vector<uint8_t> EncodeVarLen(T value)
{
	std::vector<uint8_t> output;
	// While more than 7 bits of data are left, occupy the last output byte and set the next byte flag
	while (value > 127)
	{
		output.push_back((value & 0x7F) | 0x80);
		// Remove the seven bits we just wrote
		value >>= 7;
	}
	output.push_back(value & 0x7F);
	return output;
}

typedef std::map<uint16_t, std::vector<uint16_t>> IndexMap;
typedef std::map<uint16_t, std::map<uint16_t, uint16_t>> MoveMap;

void SDAT::StripBanksAndWaveArcs()
{
	// Get all the unique patches
	IndexMap BankPatches;
	std::map<uint32_t, std::vector<uint32_t>> PatchPositions;
	for (uint32_t i = 0; i < this->infoSection.SEQrecord.count; ++i)
	{
		if (!this->infoSection.SEQrecord.entryOffsets[i])
			continue;

		auto &entry = this->infoSection.SEQrecord.entries[i];
		auto data = TimerTrack::GetPatches(entry.sseq);
		MergeUniqueVector(data.first, BankPatches[entry.bank]);
		PatchPositions[i] = data.second;
	}

	MoveMap PatchMove;
	IndexMap WaveArcs;
	for (uint32_t i = 0; i < this->infoSection.BANKrecord.count; ++i)
	{
		auto &entry = this->infoSection.BANKrecord.entries[i];
		auto sbnk = this->GetNonConstSBNK(entry.sbnk)->get();

		// Figure out where the new patch positions are going to be
		// Also edit the SBNK so the empty spaces and unused patches are removed
		if (BankPatches.count(i))
		{
			const auto &BankPatch = BankPatches[i];
			uint16_t oldCount = sbnk->instruments.size();
			uint16_t newCount = BankPatch.size();
			std::vector<SBNKInstrument> newPatches;
			for (uint16_t j = 0, k = 0; j < newCount; ++j)
			{
				uint16_t oldPatch = BankPatch[j];
				if (oldPatch < oldCount)
				{
					PatchMove[i][oldPatch] = k++;
					newPatches.push_back(sbnk->instruments[oldPatch]);
				}
			}
			sbnk->count = std::min<uint32_t>(oldCount, newPatches.size());
			sbnk->instruments = newPatches;
			sbnk->FixOffsets();
		}

		// Get all the unique waveform for this SBNK
		IndexMap BankWaves;
		std::for_each(sbnk->instruments.begin(), sbnk->instruments.end(), [&](const SBNKInstrument &patch)
		{
			IndexMap PatchWaves;
			std::for_each(patch.ranges.begin(), patch.ranges.end(), [&](const SBNKInstrumentRange &range)
			{
				if (range.record != 2)
					PatchWaves[range.swar].push_back(range.swav);
			});
			std::for_each(PatchWaves.begin(), PatchWaves.end(), [&](const IndexMap::value_type &PatchWave)
			{
				MergeUniqueVector(PatchWave.second, BankWaves[PatchWave.first]);
			});
		});
		// Technically, this can make a bank have no wave archive references, but I believe that'll only happen if all the patches of the bank are PSGs
		for (uint16_t j = 0; j < 4; ++j)
			if (entry.waveArc[j] != 0xFFFF && BankWaves.find(j) == BankWaves.end())
				entry.waveArc[j] = 0xFFFF;
		std::for_each(BankWaves.begin(), BankWaves.end(), [&](const IndexMap::value_type &BankWave)
		{
			MergeUniqueVector(BankWave.second, WaveArcs[entry.waveArc[BankWave.first]]);
		});
	}

	// Figure out where the new waveform positions are going to be
	// Also edit the SWARs so the unused waveforms are removed
	MoveMap WaveMove;
	std::for_each(WaveArcs.begin(), WaveArcs.end(), [&](const IndexMap::value_type &WaveArc)
	{
		auto &entry = this->infoSection.WAVEARCrecord.entries[WaveArc.first];
		auto swar = this->GetNonConstSWAR(entry.swar)->get();

		SWAR::SWAVs newWaves;
		for (size_t i = 0, j = 0, waves = WaveArc.second.size(); i < waves; ++i)
		{
			uint16_t oldSwav = WaveArc.second[i];
			auto swav = swar->swavs.find(oldSwav);
			if (swav != swar->swavs.end())
			{
				WaveMove[WaveArc.first][oldSwav] = j;
				newWaves[j++] = std::move(swav->second);
			}
		}
		swar->swavs = std::move(newWaves);

		// Also replace the file data for the SWAR
		PseudoWrite newFileData;
		swar->header.fileSize = swar->Size();
		swar->Write(newFileData);
		entry.fileData = newFileData.vector->data;
	});

	// Edit the SBNKs so they point at the new waveform positions
	for (size_t i = 0; i < this->infoSection.BANKrecord.count; ++i)
	{
		auto &entry = this->infoSection.BANKrecord.entries[i];
		auto sbnk = this->GetNonConstSBNK(entry.sbnk)->get();
		std::for_each(sbnk->instruments.begin(), sbnk->instruments.end(), [&](SBNKInstrument &patch)
		{
			std::for_each(patch.ranges.begin(), patch.ranges.end(), [&](SBNKInstrumentRange &range)
			{
				if (range.record != 2)
				{
					uint16_t swar = entry.waveArc[range.swar];
					if (!WaveMove.count(swar) || !WaveMove[swar].count(range.swav))
						return;
					range.swav = WaveMove[swar][range.swav];
				}
			});
		});

		// Also replace the file data for the SBNK
		PseudoWrite newFileData;
		sbnk->header.fileSize = sbnk->Size();
		sbnk->Write(newFileData);
		entry.fileData = newFileData.vector->data;
	}

	// Edit the SSEQs so they point at the new patch positions
	for (uint32_t i = 0; i < this->infoSection.SEQrecord.count; ++i)
	{
		if (!this->infoSection.SEQrecord.entryOffsets[i])
			continue;

		auto &entry = this->infoSection.SEQrecord.entries[i];
		if (!PatchMove.count(entry.bank))
			continue;

		auto sseq = this->GetNonConstSSEQ(entry.sseq)->get();
		auto &BankPatchMove = PatchMove[entry.bank];

		PseudoReadFile file;
		file.GetDataFromVector(sseq->data.begin(), sseq->data.end());

		std::vector<uint8_t> newFileData = sseq->data;

		int offset = 0;
		const auto &positions = PatchPositions[i];
		for (size_t j = 0, num = positions.size(); j < num; ++j)
		{
			file.pos = positions[j] + offset;
			int oldPatch = file.ReadVL();
			if (!BankPatchMove.count(oldPatch))
				continue;
			int newPatch = BankPatchMove[oldPatch];
			if (oldPatch != newPatch)
			{
				auto oldPatchVector = EncodeVarLen(oldPatch);
				auto newPatchVector = EncodeVarLen(newPatch);
				newFileData.erase(newFileData.begin() + positions[j], newFileData.begin() + positions[j] + oldPatchVector.size());
				newFileData.insert(newFileData.begin() + positions[j], newPatchVector.begin(), newPatchVector.end());
				offset += newPatchVector.size() - static_cast<int>(oldPatchVector.size());
			}
		}

		sseq->data = newFileData;
		entry.fileData.erase(entry.fileData.begin() + 0x1C, entry.fileData.end());
		entry.fileData.insert(entry.fileData.end(), newFileData.begin(), newFileData.end());
	}

	// Fix the offsets and sizes
	this->FixOffsetsAndSizes();
}

void SDAT::FixOffsetsAndSizes()
{
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
	uint16_t fileID = 0;
	for (uint32_t i = 0, num = this->SSEQs.size(); i < num; ++i)
	{
		this->fatSection.records[fileID].offset = offset;
		uint32_t fileSize = this->infoSection.SEQrecord.entries[i].fileData.size();
		this->fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		this->FILESize += fileSize;
	}
	for (uint32_t i = 0, num = this->SBNKs.size(); i < num; ++i)
	{
		this->fatSection.records[fileID].offset = offset;
		uint32_t fileSize = this->infoSection.BANKrecord.entries[i].fileData.size();
		this->fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		this->FILESize += fileSize;
	}
	for (uint32_t i = 0, num = this->SWARs.size(); i < num; ++i)
	{
		this->fatSection.records[fileID].offset = offset;
		uint32_t fileSize = this->infoSection.WAVEARCrecord.entries[i].fileData.size();
		this->fatSection.records[fileID++].size = fileSize;
		offset += fileSize;
		this->FILESize += fileSize;
	}

	this->header.fileSize = this->FILEOffset + this->FILESize;
	this->header.blocks = this->SYMBOffset ? 4 : 3;
}

SDAT::SSEQList::iterator SDAT::GetNonConstSSEQ(const SSEQ *sseq)
{
	return std::find_if(this->SSEQs.begin(), this->SSEQs.end(), [&](const SSEQList::value_type &thisSSEQ)
	{
		return thisSSEQ.get() == sseq;
	});
}

SDAT::SBNKList::iterator SDAT::GetNonConstSBNK(const SBNK *sbnk)
{
	return std::find_if(this->SBNKs.begin(), this->SBNKs.end(), [&](const SBNKList::value_type &thisSBNK)
	{
		return thisSBNK.get() == sbnk;
	});
}

SDAT::SWARList::iterator SDAT::GetNonConstSWAR(const SWAR *swar)
{
	return std::find_if(this->SWARs.begin(), this->SWARs.end(), [&](const SWARList::value_type &thisSWAR)
	{
		return thisSWAR.get() == swar;
	});
}
