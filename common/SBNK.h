/*
 * SDAT - SBNK (Sound Bank) structures
 * By Naram Qashat (CyberBotX)
 * Last modification on 2014-11-12
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "NDSStdHeader.h"
#include "INFOEntry.h"
#include "common.h"

struct SBNKInstrumentRange
{
	uint8_t lowNote;
	uint8_t highNote;
	uint16_t record;
	uint16_t swav;
	uint16_t swar;
	uint8_t noteNumber;
	uint8_t attackRate;
	uint8_t decayRate;
	uint8_t sustainLevel;
	uint8_t releaseRate;
	uint8_t pan;

	SBNKInstrumentRange(uint8_t lowerNote, uint8_t upperNote, int recordType);

	void Read(PseudoReadFile &file);
	void Write(PseudoWrite &file) const;
};

struct SBNKInstrument
{
	uint8_t record;
	uint16_t offset;
	uint8_t unknown;
	std::vector<SBNKInstrumentRange> ranges;

	SBNKInstrument();

	void Read(PseudoReadFile &file, uint32_t startOffset);
	uint32_t Size() const;
	uint16_t FixOffset(uint16_t newOffset);
	void WriteHeader(PseudoWrite &file) const;
	void WriteData(PseudoWrite &file) const;
};

struct SBNK
{
	std::string filename;
	NDSStdHeader header;
	uint32_t count;
	std::vector<SBNKInstrument> instruments;

	int32_t entryNumber;
	INFOEntryBANK info;

	SBNK(const std::string &fn = "");

	void Read(PseudoReadFile &file);
	uint32_t Size() const;
	uint32_t DataSize() const;
	void FixOffsets();
	void Write(PseudoWrite &file) const;
};
