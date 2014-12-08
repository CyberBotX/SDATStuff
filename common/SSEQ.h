/*
 * SDAT - SSEQ (Sequence) structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-12-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "INFOEntry.h"
#include "common.h"

struct SSEQ
{
	std::string filename, origFilename;
	std::vector<uint8_t> data;

	int32_t entryNumber;

	SSEQ(const std::string &fn = "", const std::string &origFn = "");

	void Read(PseudoReadFile &file);
};
