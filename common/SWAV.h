/*
 * SDAT - SWAV (Waveform/Sample) structure
 * By Naram Qashat (CyberBotX)
 * Last modification on 2014-10-15
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#pragma once

#include "common.h"

struct SWAV
{
	uint8_t waveType;
	uint8_t loop;
	uint16_t sampleRate;
	uint16_t time;
	uint32_t loopOffset;
	uint32_t nonLoopLength;
	std::vector<int16_t> data;

	SWAV();

	void Read(PseudoReadFile &file);
	void DecodeADPCM(const uint8_t *origData, uint32_t len);
};
