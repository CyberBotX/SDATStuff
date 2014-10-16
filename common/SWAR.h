/*
 * SDAT - SWAR (Wave Archive) structures
 * By Naram Qashat (CyberBotX)
 * Last modification on 2014-10-15
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include <map>
#include "SWAV.h"
#include "INFOEntry.h"
#include "common.h"

struct SWAR
{
	std::string filename;
	std::map<uint32_t, SWAV> swavs;

	int32_t entryNumber;
	INFOEntryWAVEARC info;

	SWAR(const std::string &fn = "");

	void Read(PseudoReadFile &file);
};
