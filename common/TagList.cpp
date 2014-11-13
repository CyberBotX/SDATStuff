/*
 * xSF Tag List
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-11-12
 *
 * Storage of tags from PSF-style files, specifications found at
 * http://wiki.neillcorlett.com/PSFTagFormat
 */

#include <algorithm>
#include "TagList.h"

eq_str TagList::eqstr;

auto TagList::GetKeys() const -> const TagsList &
{
	return this->tagsOrder;
}

auto TagList::GetTags() const -> TagsList
{
	TagsList allTags;
	std::for_each(this->tagsOrder.begin(), this->tagsOrder.end(), [&](const std::string &tag)
	{
		allTags.push_back(tag + "=" + this->tags.find(tag)->second);
	});
	return allTags;
}

bool TagList::Empty() const
{
	return this->tags.empty();
}

bool TagList::Exists(const std::string &name) const
{
	return std::find_if(this->tagsOrder.begin(), this->tagsOrder.end(), std::bind2nd(TagList::eqstr, name)) != this->tagsOrder.end();
}

std::string TagList::operator[](const std::string &name) const
{
	auto tag = this->tags.find(name);
	if (tag == this->tags.end())
		return "";
	return tag->second;
}

std::string &TagList::operator[](const std::string &name)
{
	auto tag = std::find_if(this->tagsOrder.begin(), this->tagsOrder.end(), std::bind2nd(TagList::eqstr, name));
	if (tag == this->tagsOrder.end())
	{
		this->tagsOrder.push_back(name);
		this->tags[name] = "";
	}
	return this->tags[name];
}

void TagList::CopyOverwriteExistingOnly(const TagList &copy)
{
	std::for_each(copy.tagsOrder.begin(), copy.tagsOrder.end(), [&](const std::string &tag) { (*this)[tag] = copy[tag]; });
}

void TagList::Remove(const std::string &name)
{
	auto tagOrder = std::find_if(this->tagsOrder.begin(), this->tagsOrder.end(), std::bind2nd(TagList::eqstr, name));
	if (tagOrder != this->tagsOrder.end())
		this->tagsOrder.erase(tagOrder);
	if (this->tags.count(name))
		this->tags.erase(name);
}

void TagList::Clear()
{
	this->tagsOrder.clear();
	this->tags.clear();
}
