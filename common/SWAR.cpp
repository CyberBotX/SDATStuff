/*
 * SSEQ Player - SDAT SWAR (Wave Archive) structures
 * By Naram Qashat (CyberBotX)
 * Last modification on 2013-03-30
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "SWAR.h"
#include "SDAT.h"

SWAR::SWAR(const std::string &fn) : filename(fn), swavs(), entryNumber(-1), info()
{
}

void SWAR::Read(PseudoReadFile &file)
{
	uint32_t startOfSWAR = file.pos;
	NDSStdHeader header;
	header.Read(file);
	try
	{
		header.Verify("SWAR", 0x0100FEFF);
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
		throw std::runtime_error("SWAR DATA structure invalid");
	file.ReadLE<uint32_t>(); // size
	uint32_t reserved[8];
	file.ReadLE(reserved);
	uint32_t count = file.ReadLE<uint32_t>();
	auto offsets = std::vector<uint32_t>(count);
	file.ReadLE(offsets);
	for (uint32_t i = 0; i < count; ++i)
		if (offsets[i])
		{
			file.pos = startOfSWAR + offsets[i];
			this->swavs[i] = SWAV();
			this->swavs[i].Read(file);
		}
}
