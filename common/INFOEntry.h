/*
 * SDAT - INFO Entry structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#ifndef SDAT_INFOENTRY_H
#define SDAT_INFOENTRY_H

#include "common.h"

struct SSEQ;
struct SBNK;
struct SWAR;

struct INFOEntry
{
	std::vector<uint8_t> fileData;
	std::string origFilename;

	INFOEntry();
	INFOEntry(const INFOEntry &entry);
	INFOEntry &operator=(const INFOEntry &entry);

	virtual ~INFOEntry()
	{
	}

	virtual void Read(PseudoReadFile &file) = 0;
	virtual uint32_t Size() const = 0;
	virtual void Write(PseudoWrite &file) const = 0;
};

struct INFOEntrySEQ : INFOEntry
{
	uint16_t fileID;
	uint16_t unknown;
	uint16_t bank;
	uint8_t vol;
	uint8_t cpr;
	uint8_t ppr;
	uint8_t ply;
	uint8_t unknown2[2];
	const SSEQ *sseq;
	std::string sdatNumber;

	INFOEntrySEQ();
	INFOEntrySEQ(const INFOEntrySEQ &entry);
	INFOEntrySEQ &operator=(const INFOEntrySEQ &entry);

	void Read(PseudoReadFile &file);
	uint32_t Size() const;
	void Write(PseudoWrite &file) const;
};

struct INFOEntryBANK : INFOEntry
{
	uint16_t fileID;
	uint16_t unknown;
	uint16_t waveArc[4];
	const SBNK *sbnk;

	INFOEntryBANK();
	INFOEntryBANK(const INFOEntryBANK &entry);
	INFOEntryBANK &operator=(const INFOEntryBANK &entry);

	void Read(PseudoReadFile &file);
	uint32_t Size() const;
	void Write(PseudoWrite &file) const;
};

struct INFOEntryWAVEARC : INFOEntry
{
	uint16_t fileID;
	uint16_t unknown;
	const SWAR *swar;

	INFOEntryWAVEARC();
	INFOEntryWAVEARC(const INFOEntryWAVEARC &entry);
	INFOEntryWAVEARC &operator=(const INFOEntryWAVEARC &entry);

	void Read(PseudoReadFile &file);
	uint32_t Size() const;
	void Write(PseudoWrite &file) const;
};

#endif
