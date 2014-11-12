/*
 * SSEQ Player - SDAT SBNK (Sound Bank) structures
 * By Naram Qashat (CyberBotX)
 * Last modification on 2013-03-30
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "SBNK.h"
#include "SDAT.h"

SBNKInstrumentRange::SBNKInstrumentRange(uint8_t lowerNote, uint8_t upperNote, int recordType) : lowNote(lowerNote), highNote(upperNote),
	record(recordType), swav(0), swar(0), noteNumber(0), attackRate(0), decayRate(0), sustainLevel(0), releaseRate(0), pan(0)
{
}

void SBNKInstrumentRange::Read(PseudoReadFile &file)
{
	this->swav = file.ReadLE<uint16_t>();
	this->swar = file.ReadLE<uint16_t>();
	this->noteNumber = file.ReadLE<uint8_t>();
	this->attackRate = file.ReadLE<uint8_t>();
	this->decayRate = file.ReadLE<uint8_t>();
	this->sustainLevel = file.ReadLE<uint8_t>();
	this->releaseRate = file.ReadLE<uint8_t>();
	this->pan = file.ReadLE<uint8_t>();
}

void SBNKInstrumentRange::Write(PseudoWrite &file) const
{
	file.WriteLE(this->swav);
	file.WriteLE(this->swar);
	file.WriteLE(this->noteNumber);
	file.WriteLE(this->attackRate);
	file.WriteLE(this->decayRate);
	file.WriteLE(this->sustainLevel);
	file.WriteLE(this->releaseRate);
	file.WriteLE(this->pan);
}

SBNKInstrument::SBNKInstrument() : record(0), ranges()
{
}

void SBNKInstrument::Read(PseudoReadFile &file, uint32_t startOffset)
{
	this->record = file.ReadLE<uint8_t>();
	this->offset = file.ReadLE<uint16_t>();
	this->unknown = file.ReadLE<uint8_t>();
	uint32_t endOfInst = file.pos;
	file.pos = startOffset + this->offset;
	if (this->record)
	{
		if (this->record == 16)
		{
			uint8_t lowNote = file.ReadLE<uint8_t>();
			uint8_t highNote = file.ReadLE<uint8_t>();
			uint8_t num = highNote - lowNote + 1;
			for (uint8_t i = 0; i < num; ++i)
			{
				uint16_t thisRecord = file.ReadLE<uint16_t>();
				auto range = SBNKInstrumentRange(lowNote + i, lowNote + i, thisRecord);
				range.Read(file);
				this->ranges.push_back(range);
			}
		}
		else if (this->record == 17)
		{
			uint8_t thisRanges[8];
			file.ReadLE(thisRanges);
			uint8_t i = 0;
			while (i < 8 && thisRanges[i])
			{
				uint16_t thisRecord = file.ReadLE<uint16_t>();
				uint8_t lowNote = i ? thisRanges[i - 1] + 1 : 0;
				uint8_t highNote = thisRanges[i];
				auto range = SBNKInstrumentRange(lowNote, highNote, thisRecord);
				range.Read(file);
				this->ranges.push_back(range);
				++i;
			}
		}
		else
		{
			auto range = SBNKInstrumentRange(0, 127, this->record);
			range.Read(file);
			this->ranges.push_back(range);
		}
	}
	file.pos = endOfInst;
}

uint32_t SBNKInstrument::Size() const
{
	if (this->record)
	{
		// All include 4 bytes for record type, offset, and unknown byte
		if (this->record == 16)
			return 6 + 12 * this->ranges.size(); // lowNote + highNote + instrument ranges * 12 (2 for record type, 10 for instrument range)
		else if (this->record == 17)
			return 12 + 12 * this->ranges.size(); // 8 bytes for ranges + instrument ranges * 12 (2 for record type, 10 for instrument range)
		else
			return 14; // 10 for instrument range
	}
	else
		return 4;
}

uint16_t SBNKInstrument::FixOffset(uint16_t offset)
{
	this->offset = offset;
	return this->Size() - 4;
}

void SBNKInstrument::WriteHeader(PseudoWrite &file) const
{
	file.WriteLE(this->record);
	file.WriteLE(this->offset);
	file.WriteLE(this->unknown);
}

void SBNKInstrument::WriteData(PseudoWrite &file) const
{
	if (this->record)
	{
		if (this->record == 16)
		{
			uint8_t lowNote = this->ranges.begin()->lowNote;
			uint8_t highNote = this->ranges.rbegin()->lowNote;
			uint8_t num = highNote - lowNote + 1;
			file.WriteLE(lowNote);
			file.WriteLE(highNote);
			for (uint8_t i = 0; i < num; ++i)
			{
				const auto &range = this->ranges[i];
				file.WriteLE(range.record);
				range.Write(file);
			}
		}
		else if (this->record == 17)
		{
			uint8_t actualRanges = this->ranges.size();
			uint8_t i = 0;
			for (; i < actualRanges; ++i)
				file.WriteLE(this->ranges[i].highNote);
			for (; i < 8; ++i)
				file.WriteLE<uint8_t>(0);
			for (i = 0; i < actualRanges; ++i)
			{
				const auto &range = this->ranges[i];
				file.WriteLE(range.record);
				range.Write(file);
			}
		}
		else
			this->ranges[0].Write(file);
	}
}

SBNK::SBNK(const std::string &fn) : filename(fn), instruments(), entryNumber(-1), info()
{
}

void SBNK::Read(PseudoReadFile &file)
{
	uint32_t startOfSBNK = file.pos;
	this->header.Read(file);
	try
	{
		this->header.Verify("SBNK", 0x0100FEFF);
	}
	catch (const std::exception &)
	{
		if (SDAT::failOnMissingFiles)
			throw;
		else
			return;
	}
	int8_t type[4];
	file.ReadLE(type);
	if (!VerifyHeader(type, "DATA"))
		throw std::runtime_error("SBNK DATA structure invalid");
	file.ReadLE<uint32_t>(); // size
	uint32_t reserved[8];
	file.ReadLE(reserved);
	this->count = file.ReadLE<uint32_t>();
	this->instruments.resize(this->count);
	for (uint32_t i = 0; i < this->count; ++i)
		this->instruments[i].Read(file, startOfSBNK);
}

uint32_t SBNK::Size() const
{
	return 16 + ((this->DataSize() + 3) & ~0x03);
}

// This excludes the size of the SBNK header (which is a 16 byte standard NDS header)
// This also isn't padded to the nearest multiple of 4
uint32_t SBNK::DataSize() const
{
	uint32_t fileSize = 44; // DATA + size + 8 32-bit reserved bytes + count
	for (uint32_t i = 0; i < this->count; ++i)
		fileSize += this->instruments[i].Size();
	return fileSize;
}

void SBNK::FixOffsets()
{
	uint16_t offset = 0x3C + 4 * this->count;
	for (uint32_t i = 0; i < this->count; ++i)
		offset += this->instruments[i].FixOffset(offset);
}

void SBNK::Write(PseudoWrite &file) const
{
	this->header.Write(file);
	file.WriteLE("DATA", 4);
	uint32_t size = this->DataSize();
	uint32_t sizeMulOf4 = (size + 3) & ~0x03;
	file.WriteLE(sizeMulOf4);
	uint32_t reserved[8] = { };
	file.WriteLE(reserved);
	file.WriteLE(this->count);
	for (uint32_t i = 0; i < this->count; ++i)
		this->instruments[i].WriteHeader(file);
	for (uint32_t i = 0; i < this->count; ++i)
		this->instruments[i].WriteData(file);
	if (size != sizeMulOf4)
		for (uint32_t i = 0; i < sizeMulOf4 - size; ++i)
			file.WriteLE<uint8_t>(0);
}
