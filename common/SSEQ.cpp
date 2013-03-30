/*
 * SDAT - SSEQ (Sequence) structure
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-30
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "SSEQ.h"
#include "NDSStdHeader.h"
#include "SDAT.h"

SSEQ::SSEQ(const std::string &fn, const std::string &origFn) : filename(fn), origFilename(origFn), data(), entryNumber(-1), info()
{
}

SSEQ::SSEQ(const SSEQ &sseq) : filename(sseq.filename), origFilename(sseq.origFilename), data(sseq.data), entryNumber(sseq.entryNumber), info(sseq.info)
{
}

SSEQ &SSEQ::operator=(const SSEQ &sseq)
{
	if (this != &sseq)
	{
		this->filename = sseq.filename;
		this->origFilename = sseq.origFilename;
		this->data = sseq.data;

		this->entryNumber = sseq.entryNumber;
		this->info = sseq.info;
	}
	return *this;
}

void SSEQ::Read(PseudoReadFile &file)
{
	uint32_t startOfSSEQ = file.pos;
	NDSStdHeader header;
	header.Read(file);
	try
	{
		header.Verify("SSEQ", 0x0100FEFF);
	}
	catch (const std::exception &)
	{
		if (SDAT::failOnMissingFiles)
			throw;
		else
			return;
	}
	int8_t type[4];
	file.ReadLE(type);
	if (!VerifyHeader(type, "DATA"))
		throw std::runtime_error("SSEQ DATA structure invalid");
	uint32_t size = file.ReadLE<uint32_t>();
	uint32_t dataOffset = file.ReadLE<uint32_t>();
	this->data.resize(size - 12, 0);
	file.pos = startOfSSEQ + dataOffset;
	file.ReadLE(this->data);
}
