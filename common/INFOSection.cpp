/*
 * SDAT - INFO Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "INFOSection.h"

template<typename T> INFORecord<T>::INFORecord() : count(0), actualCount(0), entryOffsets(), entries()
{
}

template<typename T> void INFORecord<T>::Read(PseudoReadFile &file, uint32_t startOffset)
{
	this->count = file.ReadLE<uint32_t>();
	this->entryOffsets.resize(this->count);
	file.ReadLE(this->entryOffsets);
	this->entries.resize(this->count);
	for (uint32_t i = 0; i < this->count; ++i)
		if (this->entryOffsets[i])
		{
			file.pos = startOffset + this->entryOffsets[i];
			this->entries[i].Read(file);
			++this->actualCount;
		}
}

template<typename T> uint32_t INFORecord<T>::Size() const
{
	uint32_t recordSize = 4 + 4 * this->count; // count + entry offsets
	for (uint32_t i = 0; i < this->count; ++i)
		recordSize += this->entries[i].Size();
	return recordSize;
}

template<typename T> void INFORecord<T>::FixOffsets(uint32_t startOffset)
{
	uint32_t offset = startOffset;
	for (uint32_t i = 0; i < this->count; ++i)
	{
		this->entryOffsets[i] = offset;
		offset += this->entries[i].Size();
	}
}

template<typename T> void INFORecord<T>::WriteHeader(PseudoWrite &file) const
{
	file.WriteLE(this->count);
	file.WriteLE(this->entryOffsets);
}

template<typename T> void INFORecord<T>::WriteData(PseudoWrite &file) const
{
	for (uint32_t i = 0; i < this->count; ++i)
		this->entries[i].Write(file);
}

INFOSection::INFOSection() : size(0), SEQrecord(), BANKrecord(), WAVEARCrecord(), PLAYERrecord()
{
	memcpy(this->type, "INFO", sizeof(this->type));
	memset(this->recordOffsets, 0, sizeof(this->recordOffsets));
}

void INFOSection::Read(PseudoReadFile &file)
{
	uint32_t startOfINFO = file.pos;
	file.ReadLE(this->type);
	if (!VerifyHeader(this->type, "INFO"))
		throw std::runtime_error("SDAT INFO Section invalid");
	this->size = file.ReadLE<uint32_t>();
	file.ReadLE(this->recordOffsets);
	if (this->recordOffsets[REC_SEQ])
	{
		file.pos = startOfINFO + this->recordOffsets[REC_SEQ];
		this->SEQrecord.Read(file, startOfINFO);
	}
	if (this->recordOffsets[REC_BANK])
	{
		file.pos = startOfINFO + this->recordOffsets[REC_BANK];
		this->BANKrecord.Read(file, startOfINFO);
	}
	if (this->recordOffsets[REC_WAVEARC])
	{
		file.pos = startOfINFO + this->recordOffsets[REC_WAVEARC];
		this->WAVEARCrecord.Read(file, startOfINFO);
	}
	if (this->recordOffsets[REC_PLAYER])
	{
		file.pos = startOfINFO + this->recordOffsets[REC_PLAYER];
		this->PLAYERrecord.Read(file, startOfINFO);
	}
}

uint32_t INFOSection::Size() const
{
	uint32_t sectionSize = 80; // type + size + record offsets + reserved + 4 * unused records
	sectionSize += this->SEQrecord.Size();
	sectionSize += this->BANKrecord.Size();
	sectionSize += this->WAVEARCrecord.Size();
	sectionSize += this->PLAYERrecord.Size();
	return sectionSize;
}

void INFOSection::FixOffsets()
{
	this->recordOffsets[REC_SEQ] = 0x40;
	this->recordOffsets[REC_SEQARC] = this->recordOffsets[REC_SEQ] + 4 + 4 * this->SEQrecord.count;
	this->recordOffsets[REC_BANK] = this->recordOffsets[REC_SEQARC] + 4;
	this->recordOffsets[REC_WAVEARC] = this->recordOffsets[REC_BANK] + 4 + 4 * this->BANKrecord.count;
	this->recordOffsets[REC_PLAYER] = this->recordOffsets[REC_WAVEARC] + 4 + 4 * this->WAVEARCrecord.count;
	this->recordOffsets[REC_GROUP] = this->recordOffsets[REC_PLAYER] + 4 + 4 * this->PLAYERrecord.count;
	this->recordOffsets[REC_PLAYER2] = this->recordOffsets[REC_GROUP] + 4;
	this->recordOffsets[REC_STRM] = this->recordOffsets[REC_PLAYER2] + 4;
	uint32_t offset = this->recordOffsets[REC_STRM] + 4;
	this->SEQrecord.FixOffsets(offset);
	offset += this->SEQrecord.Size() - 4 - 4 * this->SEQrecord.count;
	this->BANKrecord.FixOffsets(offset);
	offset += this->BANKrecord.Size() - 4 - 4 * this->BANKrecord.count;
	this->WAVEARCrecord.FixOffsets(offset);
	offset += this->WAVEARCrecord.Size() - 4 - 4 * this->WAVEARCrecord.count;
	this->PLAYERrecord.FixOffsets(offset);
}

void INFOSection::Write(PseudoWrite &file) const
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
	this->PLAYERrecord.WriteHeader(file);
	uint32_t other[3] = { };
	file.WriteLE(other);
	this->SEQrecord.WriteData(file);
	this->BANKrecord.WriteData(file);
	this->WAVEARCrecord.WriteData(file);
	this->PLAYERrecord.WriteData(file);
}
