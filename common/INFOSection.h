/*
 * SDAT - INFO Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "INFOEntry.h"

template<typename T> struct INFORecord
{
	uint32_t count, actualCount;
	std::vector<uint32_t> entryOffsets;
	std::vector<T> entries;

	INFORecord();

	void Read(PseudoReadFile &file, uint32_t startOffset);
	uint32_t Size() const;
	void FixOffsets(uint32_t startOffset);
	void WriteHeader(PseudoWrite &file) const;
	void WriteData(PseudoWrite &file) const;
};

struct INFOSection
{
	int8_t type[4];
	uint32_t size;
	uint32_t recordOffsets[8];
	INFORecord<INFOEntrySEQ> SEQrecord;
	INFORecord<INFOEntryBANK> BANKrecord;
	INFORecord<INFOEntryWAVEARC> WAVEARCrecord;
	INFORecord<INFOEntryPLAYER> PLAYERrecord;

	INFOSection();

	void Read(PseudoReadFile &file);
	uint32_t Size() const;
	void FixOffsets();
	void Write(PseudoWrite &file) const;
};
