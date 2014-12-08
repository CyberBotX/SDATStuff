/*
 * SSEQ Player - SDAT SWAR (Wave Archive) structures
 * By Naram Qashat (CyberBotX)
 * Last modification on 2014-12-08
 *
 * Nintendo DS Nitro Composer (SDAT) Specification document found at
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */

#include "SWAR.h"
#include "SDAT.h"

SWAR::SWAR(const std::string &fn) : filename(fn), header(), swavs(), entryNumber(-1)
{
}

SWAR::SWAR(const SWAR &swar) : filename(swar.filename), header(swar.header), swavs(), entryNumber(swar.entryNumber)
{
	std::for_each(swar.swavs.begin(), swar.swavs.end(), [&](const SWAVs::value_type &swav)
	{
		this->swavs[swav.first].reset(new SWAV(*swav.second.get()));
	});
}

SWAR &SWAR::operator=(const SWAR &swar)
{
	if (this != &swar)
	{
		this->filename = swar.filename;
		this->header = swar.header;
		std::for_each(swar.swavs.begin(), swar.swavs.end(), [&](const SWAVs::value_type &swav)
		{
			this->swavs[swav.first].reset(new SWAV(*swav.second.get()));
		});

		this->entryNumber = swar.entryNumber;
	}
	return *this;
}

void SWAR::Read(PseudoReadFile &file)
{
	uint32_t startOfSWAR = file.pos;
	this->header.Read(file);
	try
	{
		this->header.Verify("SWAR", 0x0100FEFF);
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
			this->swavs[i].reset(new SWAV());
			this->swavs[i]->Read(file);
		}
}

uint32_t SWAR::Size() const
{
	uint32_t size = 60 + 4 * this->swavs.size(); // Header + DATA + size + 8 32-bit reserved bytes + count + offsets
	std::for_each(this->swavs.begin(), this->swavs.end(), [&](const SWAVs::value_type &swav)
	{
		size += swav.second->Size();
	});
	return size;
}

void SWAR::Write(PseudoWrite &file) const
{
	this->header.Write(file);
	file.WriteLE("DATA", 4);
	file.WriteLE<uint32_t>(this->header.fileSize - 16);
	uint32_t reserved[8] = { };
	file.WriteLE(reserved);
	uint32_t count = this->swavs.size();
	file.WriteLE(count);
	uint32_t offset = 0x3C + 4 * count;
	std::for_each(this->swavs.begin(), this->swavs.end(), [&](const SWAVs::value_type &swav)
	{
		file.WriteLE(offset);
		offset += swav.second->Size();
	});
	std::for_each(this->swavs.begin(), this->swavs.end(), [&](const SWAVs::value_type &swav)
	{
		swav.second->Write(file);
	});
}
