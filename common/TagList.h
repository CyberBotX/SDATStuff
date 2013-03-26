/*
 * xSF Tag List
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-25
 *
 * Storage of tags from PSF-style files, specifications found at
 * http://wiki.neillcorlett.com/PSFTagFormat
 */

#ifndef TAGLIST_H
#define TAGLIST_H

#include <map>
#include <vector>
#include "eqstr.h"
#include "ltstr.h"

class TagList
{
	static eq_str eqstr;

	typedef std::map<std::string, std::string, lt_str> Tags;
	typedef std::vector<std::string> TagsList;

	Tags tags;
	TagsList tagsOrder;
public:
	TagList() : tags(), tagsOrder() { }
	const TagsList &GetKeys() const;
	TagsList GetTags() const;
	bool Exists(const std::string &name) const;
	std::string operator[](const std::string &name) const;
	std::string &operator[](const std::string &name);
	void Remove(const std::string &name);
	void Clear();
};

#endif
