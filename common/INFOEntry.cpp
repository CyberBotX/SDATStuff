/*
 * SDAT - INFO Entry structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "INFOEntry.h"

INFOEntry::INFOEntry() : fileData(), origFilename(""), sdatNumber("")
{
}

INFOEntry::INFOEntry(const INFOEntry& entry) : fileData(entry.fileData), origFilename(entry.origFilename), sdatNumber(entry.sdatNumber)
{
}

INFOEntry &INFOEntry::operator=(const INFOEntry &entry)
{
	if (this != &entry)
	{
		this->fileData = entry.fileData;
		this->origFilename = entry.origFilename;
		this->sdatNumber = entry.sdatNumber;
	}
	return *this;
}

std::string INFOEntry::FullFilename(bool multipleSDATs) const
{
	std::string filename = this->origFilename;
	if (multipleSDATs)
		filename = this->sdatNumber + "/" + filename;
	return filename;
}

INFOEntrySEQ::INFOEntrySEQ() : INFOEntry(), fileID(0), unknown(0), bank(0), vol(0), cpr(0), ppr(0), ply(0), sseq(nullptr)
{
	memset(this->unknown2, 0, sizeof(this->unknown2));
}

INFOEntrySEQ::INFOEntrySEQ(const INFOEntrySEQ &entry) : INFOEntry(entry), fileID(entry.fileID), unknown(entry.unknown), bank(entry.bank), vol(entry.vol), cpr(entry.cpr),
	ppr(entry.ppr), ply(entry.ply), sseq(entry.sseq)
{
	memcpy(this->unknown2, entry.unknown2, sizeof(this->unknown2));
}

INFOEntrySEQ &INFOEntrySEQ::operator=(const INFOEntrySEQ &entry)
{
	if (this != &entry)
	{
		INFOEntry::operator=(entry);
		this->fileID = entry.fileID;
		this->unknown = entry.unknown;
		this->bank = entry.bank;
		this->vol = entry.vol;
		this->cpr = entry.cpr;
		this->ppr = entry.ppr;
		this->ply = entry.ply;
		memcpy(this->unknown2, entry.unknown2, sizeof(this->unknown2));
		this->sseq = entry.sseq;
	}
	return *this;
}

void INFOEntrySEQ::Read(PseudoReadFile &file)
{
	this->fileID = file.ReadLE<uint16_t>();
	this->unknown = file.ReadLE<uint16_t>();
	this->bank = file.ReadLE<uint16_t>();
	this->vol = file.ReadLE<uint8_t>();
	this->cpr = file.ReadLE<uint8_t>();
	this->ppr = file.ReadLE<uint8_t>();
	this->ply = file.ReadLE<uint8_t>();
	file.ReadLE(this->unknown2);
}

uint32_t INFOEntrySEQ::Size() const
{
	return 12;
}

void INFOEntrySEQ::Write(PseudoWrite &file) const
{
	file.WriteLE(this->fileID);
	file.WriteLE(this->unknown);
	file.WriteLE(this->bank);
	file.WriteLE(this->vol);
	file.WriteLE(this->cpr);
	file.WriteLE(this->ppr);
	file.WriteLE(this->ply);
	file.WriteLE(this->unknown2);
}

INFOEntryBANK::INFOEntryBANK() : INFOEntry(), fileID(0), unknown(0), sbnk(nullptr)
{
	memset(this->waveArc, 0, sizeof(this->waveArc));
}

INFOEntryBANK::INFOEntryBANK(const INFOEntryBANK &entry) : INFOEntry(entry), fileID(entry.fileID), unknown(entry.unknown), sbnk(entry.sbnk)
{
	memcpy(this->waveArc, entry.waveArc, sizeof(this->waveArc));
}

INFOEntryBANK &INFOEntryBANK::operator=(const INFOEntryBANK &entry)
{
	if (this != &entry)
	{
		INFOEntry::operator=(entry);
		this->fileID = entry.fileID;
		this->unknown = entry.unknown;
		memcpy(this->waveArc, entry.waveArc, sizeof(this->waveArc));
		this->sbnk = entry.sbnk;
	}
	return *this;
}

void INFOEntryBANK::Read(PseudoReadFile &file)
{
	this->fileID = file.ReadLE<uint16_t>();
	this->unknown = file.ReadLE<uint16_t>();
	file.ReadLE(this->waveArc);
}

uint32_t INFOEntryBANK::Size() const
{
	return 12;
}

void INFOEntryBANK::Write(PseudoWrite &file) const
{
	file.WriteLE(this->fileID);
	file.WriteLE(this->unknown);
	file.WriteLE(this->waveArc);
}

INFOEntryWAVEARC::INFOEntryWAVEARC() : INFOEntry(), fileID(0), unknown(0), swar(nullptr)
{
}

INFOEntryWAVEARC::INFOEntryWAVEARC(const INFOEntryWAVEARC &entry) : INFOEntry(entry), fileID(entry.fileID), unknown(entry.unknown), swar(entry.swar)
{
}

INFOEntryWAVEARC &INFOEntryWAVEARC::operator=(const INFOEntryWAVEARC &entry)
{
	if (this != &entry)
	{
		INFOEntry::operator=(entry);
		this->fileID = entry.fileID;
		this->unknown = entry.unknown;
		this->swar = entry.swar;
	}
	return *this;
}

void INFOEntryWAVEARC::Read(PseudoReadFile &file)
{
	this->fileID = file.ReadLE<uint16_t>();
	this->unknown = file.ReadLE<uint16_t>();
}

uint32_t INFOEntryWAVEARC::Size() const
{
	return 4;
}

void INFOEntryWAVEARC::Write(PseudoWrite &file) const
{
	file.WriteLE(this->fileID);
	file.WriteLE(this->unknown);
}
