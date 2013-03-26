/*
 * SDAT - FAT (File Allocation Table) Section structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "FATSection.h"

FATRecord::FATRecord() : offset(0), size(0)
{
}

void FATRecord::Read(PseudoReadFile &file)
{
	this->offset = file.ReadLE<uint32_t>();
	this->size = file.ReadLE<uint32_t>();
	uint32_t reserved[2];
	file.ReadLE(reserved);
}

void FATRecord::Write(PseudoWrite &file) const
{
	file.WriteLE(this->offset);
	file.WriteLE(this->size);
	uint32_t reserved[2] = { };
	file.WriteLE(reserved);
}

FATSection::FATSection() : size(0), count(0), records()
{
	memcpy(this->type, "FAT ", sizeof(this->type));
}

void FATSection::Read(PseudoReadFile &file)
{
	file.ReadLE(this->type);
	if (!VerifyHeader(this->type, "FAT "))
		throw std::runtime_error("SDAT FAT Section invalid");
	this->size = file.ReadLE<uint32_t>();
	this->count = file.ReadLE<uint32_t>();
	this->records.resize(this->count);
	for (uint32_t i = 0; i < this->count; ++i)
		this->records[i].Read(file);
}

uint32_t FATSection::Size() const
{
	return 12 + 16 * this->count; // type + size + count + records * 16 (size of each record)
}

void FATSection::Write(PseudoWrite &file) const
{
	file.WriteLE(this->type);
	file.WriteLE(this->size);
	file.WriteLE(this->count);
	for (uint32_t i = 0; i < this->count; ++i)
		this->records[i].Write(file);
}
