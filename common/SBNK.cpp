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

SBNKInstrument::SBNKInstrument() : record(0), ranges()
{
}

void SBNKInstrument::Read(PseudoReadFile &file, uint32_t startOffset)
{
	this->record = file.ReadLE<uint8_t>();
	uint16_t offset = file.ReadLE<uint16_t>();
	file.ReadLE<uint8_t>();
	uint32_t endOfInst = file.pos;
	file.pos = startOffset + offset;
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

SBNK::SBNK(const std::string &fn) : filename(fn), instruments(), entryNumber(-1), info()
{
}

void SBNK::Read(PseudoReadFile &file)
{
	uint32_t startOfSBNK = file.pos;
	NDSStdHeader header;
	header.Read(file);
	try
	{
		header.Verify("SBNK", 0x0100FEFF);
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
	uint32_t count = file.ReadLE<uint32_t>();
	this->instruments.resize(count);
	for (uint32_t i = 0; i < count; ++i)
		this->instruments[i].Read(file, startOfSBNK);
}
