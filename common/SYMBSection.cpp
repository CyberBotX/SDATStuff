/*
 * SDAT - SYMB (Symbol/Filename) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "SYMBSection.h"

SYMBRecord::SYMBRecord() : count(0), entryOffsets(), entries()
{
}

void SYMBRecord::Read(PseudoReadFile &file, uint32_t startOffset)
{
	this->count = file.ReadLE<uint32_t>();
	this->entryOffsets.resize(this->count);
	file.ReadLE(entryOffsets);
	this->entries.resize(this->count);
	for (uint32_t i = 0; i < this->count; ++i)
		if (this->entryOffsets[i])
		{
			file.pos = startOffset + this->entryOffsets[i];
			this->entries[i] = file.ReadNullTerminatedString();
		}
}

uint32_t SYMBRecord::Size() const
{
	uint32_t recordSize = 4 + 4 * this->count; // count + entryOffsets
	for (uint32_t i = 0; i < this->count; ++i)
		recordSize += this->entries[i].size() + 1;
	return recordSize;
}

void SYMBRecord::FixOffsets(uint32_t startOffset)
{
	uint32_t offset = startOffset;
	for (uint32_t i = 0; i < this->count; ++i)
	{
		this->entryOffsets[i] = offset;
		offset += this->entries[i].size() + 1;
	}
}

void SYMBRecord::WriteHeader(PseudoWrite &file) const
{
	file.WriteLE(this->count);
	file.WriteLE(this->entryOffsets);
}

void SYMBRecord::WriteData(PseudoWrite &file) const
{
	for (uint32_t i = 0; i < this->count; ++i)
		file.WriteLE(this->entries[i]);
}

SYMBSection::SYMBSection() : size(0), SEQrecord(), BANKrecord(), WAVEARCrecord()
{
	memcpy(this->type, "SYMB", sizeof(this->type));
	memset(this->recordOffsets, 0, sizeof(this->recordOffsets));
}

void SYMBSection::Read(PseudoReadFile &file)
{
	uint32_t startOfSYMB = file.pos;
	file.ReadLE(this->type);
	if (!VerifyHeader(this->type, "SYMB"))
		throw std::runtime_error("SDAT SYMB Section invalid");
	this->size = file.ReadLE<uint32_t>();
	file.ReadLE(this->recordOffsets);
	if (this->recordOffsets[REC_SEQ])
	{
		file.pos = startOfSYMB + this->recordOffsets[REC_SEQ];
		this->SEQrecord.Read(file, startOfSYMB);
	}
	if (this->recordOffsets[REC_BANK])
	{
		file.pos = startOfSYMB + this->recordOffsets[REC_BANK];
		this->BANKrecord.Read(file, startOfSYMB);
	}
	if (this->recordOffsets[REC_WAVEARC])
	{
		file.pos = startOfSYMB + this->recordOffsets[REC_WAVEARC];
		this->WAVEARCrecord.Read(file, startOfSYMB);
	}
}

uint32_t SYMBSection::Size() const
{
	uint32_t sectionSize = 84; // type + size + record offsets + reserved + 5 * unused records
	sectionSize += this->SEQrecord.Size();
	sectionSize += this->BANKrecord.Size();
	sectionSize += this->WAVEARCrecord.Size();
	return sectionSize;
}

void SYMBSection::FixOffsets()
{
	this->recordOffsets[REC_SEQ] = 0x40;
	this->recordOffsets[REC_SEQARC] = this->recordOffsets[REC_SEQ] + 4 + 4 * this->SEQrecord.count;
	this->recordOffsets[REC_BANK] = this->recordOffsets[REC_SEQARC] + 4;
	this->recordOffsets[REC_WAVEARC] = this->recordOffsets[REC_BANK] + 4 + 4 * this->BANKrecord.count;
	this->recordOffsets[REC_PLAYER] = this->recordOffsets[REC_WAVEARC] + 4 + 4 * this->WAVEARCrecord.count;
	this->recordOffsets[REC_GROUP] = this->recordOffsets[REC_PLAYER] + 4;
	this->recordOffsets[REC_PLAYER2] = this->recordOffsets[REC_GROUP] + 4;
	this->recordOffsets[REC_STRM] = this->recordOffsets[REC_PLAYER2] + 4;
	uint32_t offset = this->recordOffsets[REC_STRM] + 4;
	this->SEQrecord.FixOffsets(offset);
	offset += this->SEQrecord.Size() - 4 - 4 * this->SEQrecord.count;
	this->BANKrecord.FixOffsets(offset);
	offset += this->BANKrecord.Size() - 4 - 4 * this->BANKrecord.count;
	this->WAVEARCrecord.FixOffsets(offset);
}

void SYMBSection::Write(PseudoWrite &file) const
{
	file.WriteLE(this->type);
	file.WriteLE(this->size);
	file.WriteLE(this->recordOffsets);
	uint8_t reserved[24] = { };
	file.WriteLE(reserved);
	this->SEQrecord.WriteHeader(file);
	file.WriteLE<uint32_t>(0);
	this->BANKrecord.WriteHeader(file);
	this->WAVEARCrecord.WriteHeader(file);
	uint32_t other[4] = { };
	file.WriteLE(other);
	this->SEQrecord.WriteData(file);
	this->BANKrecord.WriteData(file);
	this->WAVEARCrecord.WriteData(file);
	uint32_t sizeMulOf4 = (this->size + 3) & ~0x03;
	if (this->size != sizeMulOf4)
		for (uint32_t i = 0; i < sizeMulOf4 - this->size; ++i)
			file.WriteLE<uint8_t>(0);
}
