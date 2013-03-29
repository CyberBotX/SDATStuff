/*
 * Common NCSF functions
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-29
 */

#ifndef NCSF_H
#define NCSF_H

#include <string>
#include <vector>
#include "TagList.h"
#include "SDAT.h"
#include "common.h"

typedef std::vector<std::string> Files;

void MakeNCSF(const std::string &filename, const std::vector<uint8_t> &reservedSectionData, const std::vector<uint8_t> &programSectionData,
	const std::vector<std::string> &tags = std::vector<std::string>());
void CheckForValidPSF(PseudoReadFile &file, uint8_t versionByte);
std::vector<uint8_t> GetProgramSectionFromPSF(PseudoReadFile &file, uint8_t versionByte, uint32_t programHeaderSize, uint32_t programSizeOffset);
TagList GetTagsFromPSF(PseudoReadFile &file, uint8_t versionByte);
Files GetFilesInDirectory(const std::string &path, const std::vector<std::string> &extensions = std::vector<std::string>());
void RemoveFiles(const Files &files);
void GetTime(const std::string &filename, const SDAT *sdat, const SSEQ *sseq, TagList &tags, bool verbose, uint32_t numberOfLoops, uint32_t fadeLoop, uint32_t fadeOneShot);

#endif
