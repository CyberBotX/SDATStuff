/*
 * SDAT - FAT (File Allocation Table) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SDAT_FATSECTION_H
#define SDAT_FATSECTION_H

#include "common.h"

struct FATRecord
{
	uint32_t offset;
	uint32_t size;

	FATRecord();

	void Read(PseudoReadFile &file);
	void Write(PseudoWrite &file) const;
};

struct FATSection
{
	int8_t type[4];
	uint32_t size;
	uint32_t count;
	std::vector<FATRecord> records;

	FATSection();

	void Read(PseudoReadFile &file);
	uint32_t Size() const;
	void Write(PseudoWrite &file) const;
};

#endif
