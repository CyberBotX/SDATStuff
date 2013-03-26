/*
 * SDAT - Nintendo DS Standard Header structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "NDSStdHeader.h"

NDSStdHeader::NDSStdHeader() : magic(0), fileSize(0), size(0), blocks(0)
{
	memset(this->type, 0, sizeof(this->type));
}

void NDSStdHeader::Read(PseudoReadFile &file)
{
	file.ReadLE(this->type);
	this->magic = file.ReadLE<uint32_t>();
	this->fileSize = file.ReadLE<uint32_t>();
	this->size = file.ReadLE<uint16_t>();
	this->blocks = file.ReadLE<uint16_t>(); // # of blocks
}

void NDSStdHeader::Verify(const std::string &typeToCheck, uint32_t magicToCheck) const
{
	if (!VerifyHeader(this->type, typeToCheck) || this->magic != magicToCheck)
		throw std::runtime_error("NDS Standard Header for " + typeToCheck + " invalid");
}

void NDSStdHeader::Write(PseudoWrite &file) const
{
	file.WriteLE(this->type);
	file.WriteLE(this->magic);
	file.WriteLE(this->fileSize);
	file.WriteLE(this->size);
	file.WriteLE(this->blocks);
}
