/*
 * SDAT - SWAV (Waveform/Sample) structure
 * By Naram Qashat (CyberBotX)
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SDAT_SWAV_H
#define SDAT_SWAV_H

#include "common.h"

struct SWAV
{
	uint8_t waveType;
	uint8_t loop;
	uint16_t sampleRate;
	uint16_t time;
	uint16_t loopOffset;
	uint32_t nonLoopLength;
	std::vector<int16_t> data;

	SWAV();

	void Read(PseudoReadFile &file);
	void DecodeADPCM(const std::vector<uint8_t> &data);
};

#endif
