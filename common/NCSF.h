/*
 * Common NCSF functions
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
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
void CheckForValidNCSF(PseudoReadFile &file);
std::vector<uint8_t> GetProgramSectionFromNCSF(PseudoReadFile &file);
TagList GetTagsFromNCSF(PseudoReadFile &file);
Files GetFilesInNCSFDirectory(const std::string &path);
void RemoveFiles(const Files &files);
void GetTime(const std::string &filename, const SDAT *sdat, const SSEQ *sseq, TagList &tags, bool verbose);

#endif
