/*
 * SDAT - SWAV (Waveform/Sample) structure
 * By Naram Qashat (CyberBotX)
 * Last modification on 2014-12-08
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
	uint16_t origLoopOffset;
	uint32_t loopOffset;
	uint32_t origNonLoopLength;
	uint32_t nonLoopLength;
	std::vector<uint8_t> origData;
	std::vector<int16_t> data;

	SWAV();

	void Read(PseudoReadFile &file);
	void DecodeADPCM(uint32_t len);
	uint32_t Size() const;
	void Write(PseudoWrite &file) const;
};
