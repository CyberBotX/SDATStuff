/*
 * SDAT - SBNK (Sound Bank) structures
 * By Naram Qashat (CyberBotX)
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SDAT_SBNK_H
#define SDAT_SBNK_H

#include "SWAR.h"
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
};

struct SBNKInstrument
{
	uint8_t record;
	std::vector<SBNKInstrumentRange> ranges;

	SBNKInstrument();

	void Read(PseudoReadFile &file, uint32_t startOffset);
};

struct SBNK
{
	std::string filename;
	std::vector<SBNKInstrument> instruments;

	INFOEntryBANK info;

	SBNK(const std::string &fn = "");

	void Read(PseudoReadFile &file);
};

#endif
