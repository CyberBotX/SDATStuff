/*
 * SDAT - Nintendo DS Standard Header structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-15
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "common.h"

struct NDSStdHeader
{
	int8_t type[4];
	uint32_t magic;
	uint32_t fileSize;
	uint16_t size;
	uint16_t blocks;

	NDSStdHeader();

	void Read(PseudoReadFile &file);
	void Verify(const std::string &typeToCheck, uint32_t magicToCheck) const;
	void Write(PseudoWrite &file) const;
};
