/*
 * SDAT - Common functions
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-30
 */

#ifndef SDAT_COMMON_H
#define SDAT_COMMON_H

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <typeinfo>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <cctype>
#include "pstdint.h"
#include <sys/stat.h>
#ifdef _WIN32
# include <direct.h>
# define mkdir(dir, mode) _mkdir((dir))
# include "win_dirent.h"
#else
# include <dirent.h>
#endif
#include "optionparser.h"

/*
 * Pseudo-file data structures
 *
 * The first structure is mainly so and entire can be loaded at once
 * and then "read" from the vector in this.
 *
 * The second set of structures are wrappers around either an std::ofstream
 * or an std::vector of uint8_t to make it easier to write data to it.
 */

struct PseudoReadFile
{
	std::string filename;
	std::unique_ptr<std::vector<uint8_t>> data;
	uint32_t pos;

	PseudoReadFile(const std::string &fn = "") : filename(fn), data(), pos(0)
	{
	}

	PseudoReadFile(const PseudoReadFile &file) : filename(file.filename), data(new std::vector<uint8_t>(file.data->begin(), file.data->end())), pos(file.pos)
	{
	}

	PseudoReadFile &operator=(const PseudoReadFile &file)
	{
		if (this != &file)
		{
			this->filename = file.filename;
			this->data.reset(new std::vector<uint8_t>(file.data->begin(), file.data->end()));
			this->pos = file.pos;
		}
		return *this;
	}

	void GetDataFromFile(const std::string &fn)
	{
		this->filename = fn;
		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		file.open(fn.c_str(), std::ifstream::in | std::ifstream::binary);
		this->GetDataFromFile(file);
		file.close();
	}

	void GetDataFromFile(std::ifstream &file)
	{
		auto origPos = file.tellg();
		file.seekg(0, std::ifstream::end);
		this->pos = file.tellg();
		file.seekg(0, std::ifstream::beg);
		this->data.reset(new std::vector<uint8_t>(this->pos));
		file.read(reinterpret_cast<char *>(&(*this->data.get())[0]), this->pos);
		this->pos = 0;
		file.seekg(origPos, std::ifstream::beg);
	}

	template<typename InputIterator> void GetDataFromVector(InputIterator start, InputIterator end)
	{
		this->data.reset(new std::vector<uint8_t>(start, end));
		this->pos = 0;
	}

	template<typename T> T ReadLE()
	{
		if (!this->data.get())
			return 0;
		if (this->pos >= this->data->size() || this->pos + sizeof(T) > this->data->size())
			throw std::range_error("PseudoReadFile position was set past the end of the data.");
		T finalVal = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
			finalVal |= (*this->data.get())[this->pos++] << (i * 8);
		return finalVal;
	}

	template<typename T, size_t N> void ReadLE(T (&arr)[N])
	{
		if (!this->data.get())
			return;
		for (size_t i = 0; i < N; ++i)
			arr[i] = this->ReadLE<T>();
	}

	template<size_t N> void ReadLE(uint8_t (&arr)[N])
	{
		if (!this->data.get())
			return;
		if (this->pos >= this->data->size() || this->pos + N > this->data->size())
			throw std::range_error("PseudoReadFile position was set past the end of the data.");
		memcpy(&arr[0], &(*this->data.get())[this->pos], N);
		this->pos += N;
	}

	template<typename T> void ReadLE(std::vector<T> &arr)
	{
		if (!this->data.get())
			return;
		for (size_t i = 0, len = arr.size(); i < len; ++i)
			arr[i] = this->ReadLE<T>();
	}

	void ReadLE(std::vector<uint8_t> &arr)
	{
		if (!this->data.get())
			return;
		if (this->pos >= this->data->size() || this->pos + arr.size() > this->data->size())
			throw std::range_error("PseudoReadFile position was set past the end of the data.");
		memcpy(&arr[0], &(*this->data.get())[this->pos], arr.size());
		this->pos += arr.size();
	}

	std::string ReadNullTerminatedString()
	{
		if (!this->data.get())
			return "";
		char chr;
		std::string str;
		do
		{
			chr = static_cast<char>(this->ReadLE<uint8_t>());
			if (chr)
				str += chr;
		} while (chr);
		return str;
	}

	// The following 2 functions are only utilized by the SSEQ player.
	int Read24()
	{
		int finalVal = 0;
		for (size_t i = 0; i < 3; ++i)
			finalVal |= this->ReadLE<uint8_t>() << (i * 8);
		return finalVal;
	}

	int ReadVL()
	{
		int x = 0;
		for (;;)
		{
			int vl = this->ReadLE<uint8_t>();
			x = (x << 7) | (vl & 0x7F);
			if (!(vl & 0x80))
				break;
		}
		return x;
	}
};

struct PseudoWriteFile
{
	std::ofstream *file;

	PseudoWriteFile(std::ofstream *ofile) : file(ofile)
	{
	}

	template<typename T> void WriteLE(const T &val)
	{
		for (size_t i = 0; i < sizeof(T); ++i)
			this->file->put((val >> (i * 8)) & 0xFF);
	}

	template<typename T, size_t N> void WriteLE(const T (&arr)[N])
	{
		for (size_t i = 0; i < N; ++i)
			this->WriteLE(arr[i]);
	}

	template<size_t N> void WriteLE(const uint8_t (&arr)[N])
	{
		this->file->write(reinterpret_cast<const char *>(&arr[0]), N);
	}

	template<typename T> void WriteLE(const std::vector<T> &arr)
	{
		for (size_t i = 0, len = arr.size(); i < len; ++i)
			this->WriteLE(arr[i]);
	}

	void WriteLE(const std::vector<uint8_t> &arr)
	{
		this->file->write(reinterpret_cast<const char *>(&arr[0]), arr.size());
	}

	void WriteLE(const std::string &str, int32_t size = -1)
	{
		this->file->write(str.c_str(), size == -1 ? str.size() + 1 : size);
	}
};

struct PseudoWriteVector
{
	std::vector<uint8_t> data;

	PseudoWriteVector() : data()
	{
	}

	template<typename T> void WriteLE(const T &val)
	{
		for (size_t i = 0; i < sizeof(T); ++i)
			this->data.push_back((val >> (i * 8)) & 0xFF);
	}

	template<typename T, size_t N> void WriteLE(const T (&arr)[N])
	{
		for (size_t i = 0; i < N; ++i)
			this->WriteLE(arr[i]);
	}

	template<size_t N> void WriteLE(const uint8_t (&arr)[N])
	{
		this->data.insert(this->data.end(), arr, arr + N);
	}

	template<typename T> void WriteLE(const std::vector<T> &arr)
	{
		for (size_t i = 0, len = arr.size(); i < len; ++i)
			this->WriteLE(arr[i]);
	}

	void WriteLE(const std::vector<uint8_t> &arr)
	{
		this->data.insert(this->data.end(), arr.begin(), arr.end());
	}

	void WriteLE(const std::string &str, int32_t size = -1)
	{
		size_t finalSize = size == -1 ? str.size() + 1 : size;
		auto strData = std::vector<uint8_t>(finalSize, 0);
		memcpy(&strData[0], str.c_str(), finalSize);
		this->data.insert(this->data.end(), strData.begin(), strData.end());
	}
};

enum PseudoWriteType
{
	PSEUDOWRITE_FILE,
	PSEUDOWRITE_VECTOR
};

struct PseudoWrite
{
	std::unique_ptr<PseudoWriteFile> file;
	std::unique_ptr<PseudoWriteVector> vector;
	PseudoWriteType type;

	PseudoWrite() : file(), vector(new PseudoWriteVector()), type(PSEUDOWRITE_VECTOR)
	{
	}

	PseudoWrite(std::ofstream *newFile) : file(new PseudoWriteFile(newFile)), vector(), type(PSEUDOWRITE_FILE)
	{
	}

	template<typename T> void WriteLE(const T &val)
	{
		if (type == PSEUDOWRITE_FILE)
			this->file->WriteLE(val);
		else
			this->vector->WriteLE(val);
	}

	template<typename T, size_t N> void WriteLE(const T (&arr)[N])
	{
		if (type == PSEUDOWRITE_FILE)
			this->file->WriteLE(arr);
		else
			this->vector->WriteLE(arr);
	}

	template<size_t N> void WriteLE(const uint8_t (&arr)[N])
	{
		if (type == PSEUDOWRITE_FILE)
			this->file->WriteLE(arr);
		else
			this->vector->WriteLE(arr);
	}

	template<typename T> void WriteLE(const std::vector<T> &arr)
	{
		if (type == PSEUDOWRITE_FILE)
			this->file->WriteLE(arr);
		else
			this->vector->WriteLE(arr);
	}

	void WriteLE(const std::vector<uint8_t> &arr)
	{
		if (type == PSEUDOWRITE_FILE)
			this->file->WriteLE(arr);
		else
			this->vector->WriteLE(arr);
	}

	void WriteLE(const std::string &str, int32_t size = -1)
	{
		if (type == PSEUDOWRITE_FILE)
			this->file->WriteLE(str, size);
		else
			this->vector->WriteLE(str, size);
	}
};

/*
 * String Conversion
 *
 * The following class and 3 functions come from the C++ FAQ, Section 39, FAQ 39.3
 * http://www.parashift.com/c++-faq/convert-string-to-any.html
 */

class BadConversion : public std::runtime_error
{
public:
	BadConversion(const std::string &s) : std::runtime_error(s)
	{
	}
};

template<typename T> inline std::string stringify(const T &x)
{
	std::ostringstream o;
	if (!(o << x))
		throw BadConversion(std::string("stringify(") + typeid(x).name() + ")");
	return o.str();
}

template<typename T> inline void convert(const std::string &s, T &x, bool failIfLeftoverChars = true)
{
	std::istringstream i(s);
	char c;
	if (!(i >> x) || (failIfLeftoverChars && i.get(c)))
		throw BadConversion(s);
}

template<typename T> inline T convertTo(const std::string &s, bool failIfLeftoverChars = true)
{
	T x;
	convert(s, x, failIfLeftoverChars);
	return x;
}

// Read a little endian value from an array/pointer
template<typename T> inline T ReadLE(const uint8_t *arr)
{
	T finalVal = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
		finalVal |= arr[i] << (i * 8);
	return finalVal;
}

// Get a vector of 8-bit values in little endian format from any other integer
template<typename T> static inline std::vector<uint8_t> IntToLEVector(const T &val)
{
	auto vec = std::vector<uint8_t>(sizeof(T));
	for (size_t i = 0; i < sizeof(T); ++i)
		vec[i] = (val >> (i * 8)) & 0xFF;
	return vec;
}

/*
 * The following function is used to convert an integer into a hexadecimal
 * string, the length being determined by the size of the integer.  8-bit
 * integers are in the format of 0x00, 16-bit integers are in the format of
 * 0x0000, and so on.
 */
template<typename T> inline std::string NumToHexString(const T &num)
{
	std::string hex;
	uint8_t len = sizeof(T) * 2;
	for (uint8_t i = 0; i < len; ++i)
	{
		uint8_t tmp = (num >> (i * 4)) & 0xF;
		hex = static_cast<char>(tmp < 10 ? tmp + '0' : tmp - 10 + 'a') + hex;
	}
	return "0x" + hex;
}

/*
 * SDAT Record types
 * List of types taken from the Nitro Composer Specification
 * http://www.feshrine.net/hacking/doc/nds-sdat.html
 */
enum RecordName
{
	REC_SEQ,
	REC_SEQARC,
	REC_BANK,
	REC_WAVEARC,
	REC_PLAYER,
	REC_GROUP,
	REC_PLAYER2,
	REC_STRM
};

template<size_t N> inline bool VerifyHeader(const int8_t (&arr)[N], const std::string &header)
{
	std::string arrHeader = std::string(&arr[0], &arr[N]);
	return arrHeader == header;
}

inline bool FileExists(const std::string &filename)
{
	std::ifstream file((filename.c_str()));
	return !!file;
}

/*
 * Wildcard matching code by M Shahid Shafiq, from:
 * http://www.codeproject.com/Articles/19694/String-Wildcard-Matching-and
 */
inline bool WildcardCompare(const std::string &tameText, const std::string &wildText)
{
	enum State
	{
		Exact, // exact match
		Any, // ?
		AnyRepeat // *
	};

	const char *str = tameText.c_str();
	const char *pattern = wildText.c_str();
	const char *q = NULL;
	State state = Exact;

	bool match = true;
	while (match && *pattern)
	{
		if (*pattern == '*')
		{
			state = AnyRepeat;
			q = pattern + 1;
		}
		else if (*pattern == '?')
			state = Any;
		else
			state = Exact;

		if (!*str)
			break;

		switch (state)
		{
			case Exact:
				match = std::toupper(*str) == std::toupper(*pattern);
				++str;
				++pattern;
				break;

			case Any:
				match = true;
				++str;
				++pattern;
				break;

			case AnyRepeat:
				match = true;
				++str;

				if (std::toupper(*str) == std::toupper(*q))
					++pattern;
		}
	}

	if (state == AnyRepeat)
		return std::toupper(*str) == std::toupper(*q);
	else if (state == Any)
		return std::toupper(*str) == std::toupper(*pattern);
	else
		return match && std::toupper(*str) == std::toupper(*pattern);
}

// The following are for handling a vector of included or excluded files
enum KeepType
{
	KEEP_EXCLUDE,
	KEEP_INCLUDE,
	KEEP_NEITHER
};

struct KeepInfo
{
	std::string filename;
	KeepType keep;

	KeepInfo(const std::string &fn = "", KeepType toKeep = KEEP_NEITHER) : filename(fn), keep(toKeep)
	{
	}
};

typedef std::vector<KeepInfo> IncOrExc;

inline KeepType IncludeFilename(const std::string &filename, const std::string &sdatNumber, const IncOrExc &includesAndExcludes)
{
	KeepType keep = KEEP_NEITHER;
	std::for_each(includesAndExcludes.begin(), includesAndExcludes.end(), [&](const KeepInfo &info)
	{
		size_t slash = info.filename.find('/');
		if (slash != std::string::npos)
		{
			std::string currSDATNumber = info.filename.substr(0, slash);
			std::string currFilename = info.filename.substr(slash + 1);
			if (WildcardCompare(sdatNumber, currSDATNumber) && WildcardCompare(filename, currFilename))
				keep = info.keep;
		}
		else if (WildcardCompare(filename, info.filename))
			keep = info.keep;
	});
	return keep;
}

// Check if the directory exists
inline bool DirExists(const std::string &dirName)
{
	struct stat st;
	if (!stat(dirName.c_str(), &st))
		return S_ISDIR(st.st_mode);
	return false;
}

// Make the directory given
inline void MakeDir(const std::string &dirName)
{
	mkdir(dirName.c_str(), 0755);
}

// Get just the filename from a path
inline std::string GetFilenameFromPath(const std::string &path)
{
	size_t lastSlash = path.rfind('/');
	if (lastSlash == std::string::npos)
		return path;
	return path.substr(lastSlash + 1);
}

// The idea behind this function comes from VGMToolbox, however the actual functionality
// of it is much smoother than the one in VGMToolbox, as this leverages the std::search
// function to get the offset in the data vector
inline int32_t GetNextOffset(PseudoReadFile &file, uint32_t startingOffset, const std::vector<uint8_t> &searchBytes)
{
	int32_t ret = -1;

	auto offset = std::search(file.data->begin() + startingOffset, file.data->end(), searchBytes.begin(), searchBytes.end());
	if (offset != file.data->end())
		ret = offset - file.data->begin();

	return ret;
}

// Convert a time from seconds into a human readable string
inline std::string SecondsToString(double seconds)
{
	uint32_t minutes = static_cast<uint32_t>(seconds / 60);
	seconds -= minutes * 60;

	std::string time;
	if (minutes < 10)
		time += "0";
	time += stringify(minutes) + ":";
	if (seconds < 10)
		time += "0";
	time += stringify(seconds);
	return time;
}

// The following functions are for the options parser

inline option::ArgStatus RequireArgument(const option::Option &opt, bool msg)
{
	if (opt.arg && *opt.arg)
		return option::ARG_OK;

	if (msg)
		std::cerr << "Option '" << std::string(opt.name).substr(0, opt.namelen) << "' requires a non-empty argument.\n";
	return option::ARG_ILLEGAL;
}

inline bool IsDigitsOnly(const std::string &input, const std::locale &loc = std::locale::classic())
{
	auto inputChars = std::vector<char>(input.begin(), input.end());
	size_t length = inputChars.size();
	auto masks = std::vector<std::ctype<char>::mask>(length);
	std::use_facet<std::ctype<char>>(loc).is(&inputChars[0], &inputChars[length], &masks[0]);
	for (size_t x = 0; x < length; ++x)
		if (inputChars[x] != '.' && !(masks[x] & std::ctype<char>::digit))
			return false;
	return true;
}

inline option::ArgStatus RequireNumericArgument(const option::Option &opt, bool msg)
{
	if (opt.arg && *opt.arg && IsDigitsOnly(opt.arg))
		return option::ARG_OK;

	if (msg)
		std::cerr << "Option '" << std::string(opt.name).substr(0, opt.namelen) << "' requires a non-empty numeric argument.\n";
	return option::ARG_ILLEGAL;
}

#endif
