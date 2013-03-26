/*
 * SDAT - SSEQ (Sequence) structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SDAT_SSEQ_H
#define SDAT_SSEQ_H

#include "INFOEntry.h"
#include "common.h"

struct SSEQ
{
	std::string filename, origFilename;
	std::vector<uint8_t> data;

	INFOEntrySEQ info;

	SSEQ(const std::string &fn = "", const std::string &origFn = "");
	SSEQ(const SSEQ &sseq);
	SSEQ &operator=(const SSEQ &sseq);

	void Read(PseudoReadFile &file);
};

#endif
