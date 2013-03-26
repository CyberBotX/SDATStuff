/*
 * SDAT - SYMB (Symbol/Filename) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SDAT_SYMBSECTION_H
#define SDAT_SYMBSECTION_H

#include "common.h"

struct SYMBRecord
{
	uint32_t count;
	std::vector<uint32_t> entryOffsets;
	std::vector<std::string> entries;

	SYMBRecord();

	void Read(PseudoReadFile &file, uint32_t startOffset);
	uint32_t Size() const;
	void FixOffsets(uint32_t startOffset);
	void WriteHeader(PseudoWrite &file) const;
	void WriteData(PseudoWrite &file) const;
};

struct SYMBSection
{
	int8_t type[4];
	uint32_t size;
	uint32_t recordOffsets[8];
	SYMBRecord SEQrecord;
	SYMBRecord BANKrecord;
	SYMBRecord WAVEARCrecord;

	SYMBSection();

	void Read(PseudoReadFile &file);
	uint32_t Size() const;
	void FixOffsets();
	void Write(PseudoWrite &file) const;
};

#endif
