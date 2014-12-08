/*
 * SDAT - SWAR (Wave Archive) structures
 * By Naram Qashat (CyberBotX)
 * Last modification on 2014-12-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include <map>
#include "NDSStdHeader.h"
#include "SWAV.h"
#include "INFOEntry.h"
#include "common.h"

struct SWAR
{
	typedef std::map<uint32_t, std::unique_ptr<SWAV>> SWAVs;

	std::string filename;
	NDSStdHeader header;
	SWAVs swavs;

	int32_t entryNumber;

	SWAR(const std::string &fn = "");
	SWAR(const SWAR &swar);
	SWAR &operator=(const SWAR &swar);

	void Read(PseudoReadFile &file);
	uint32_t Size() const;
	void Write(PseudoWrite &file) const;
};
