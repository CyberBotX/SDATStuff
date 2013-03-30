/*
 * SDAT - SDAT structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-30
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SDAT_SDAT_H
#define SDAT_SDAT_H

#include "NDSStdHeader.h"
#include "SYMBSection.h"
#include "INFOSection.h"
#include "FATSection.h"
#include "SSEQ.h"
#include "SBNK.h"
#include "SWAR.h"
#include "common.h"

struct SDAT
{
	static bool failOnMissingFiles;

	std::string filename;
	NDSStdHeader header;
	uint32_t SYMBOffset;
	uint32_t SYMBSize;
	uint32_t INFOOffset;
	uint32_t INFOSize;
	uint32_t FATOffset;
	uint32_t FATSize;
	uint32_t FILEOffset;
	uint32_t FILESize;

	SYMBSection symbSection;
	INFOSection infoSection;
	FATSection fatSection;

	bool symbSectionNeedsCleanup;
	uint16_t count;

	std::vector<std::unique_ptr<SSEQ>> SSEQs;
	std::vector<std::unique_ptr<SBNK>> SBNKs;
	std::vector<std::unique_ptr<SWAR>> SWARs;

	SDAT();
	SDAT(const SDAT &sdat);
	SDAT &operator=(const SDAT &sdat);

	void Read(const std::string &fn, PseudoReadFile &file, bool shouldFailOnMissingFiles = true);
	void Write(PseudoWrite &file) const;

	SDAT &operator+=(const SDAT &other);
	void Strip(const IncOrExc &includesAndExcludes, bool verbose, bool removeExcluded = true);
};

#endif
