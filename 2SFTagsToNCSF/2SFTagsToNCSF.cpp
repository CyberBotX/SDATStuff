/*
 * 2SF Tags to NCSF
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2013-03-29
 *
 * Version history:
 *   v1.0 - 2013-03-29 - Initial version
 */

/*
 * COURSE OF ACTION:
 * Read all the 2SFs and their corresponding 2SFLIB.
 * Read all the NCSFs and their corresponding NCSFLIB.
 * For all of them, store the SSEQ that corresponds to each as well as tags for
 *   the 2SFs.
 * Search the NCSFs for matching SSEQs from the 2SFs and copy tag information
 *   over, EXCLUDING the time ones (length and fade).  We are going to always
 *   used whatever was already in the NCSFs since they will be more accurate.
 */

#include "NCSF.h"

static const std::string TWOSFTAGSTONCSF_VERSION = "1.2";

enum { UNKNOWN, HELP, VERBOSE };
const option::Descriptor opts[] =
{
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "2SF Tags to NCSF v" + TWOSFTAGSTONCSF_VERSION + "\nBy Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]\n\n"
		"2SF Tags to NCSF will copy tags from a 2SF set and place them into the tags of an NCSF set..\n\n"
		"Usage:\n"
		"  2SFTagsToNCSF [options] <Input 2SF directory> <Output NCSF directory>\n\n"
		"Options:"),
	option::Descriptor(HELP, 0, "h", "help", option::Arg::None, "  --help,-h \tPrint usage and exit."),
	option::Descriptor(VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose,-v \tVerbose output."),
	option::Descriptor(UNKNOWN, 0, "", "", option::Arg::None, "\nVerbose output will output the tags that were copied."),
	option::Descriptor()
};

int main(int argc, char *argv[])
{
	// Options parsing
	argc -= argc > 0;
	argv += argc > 0;
	option::Stats stats((opts), (argc), (argv));
	std::vector<option::Option> options((stats.options_max)), buffer((stats.buffer_max));
	option::Parser parse((opts), (argc), (argv), (&options[0]), (&buffer[0]));

	if (parse.error())
		return 1;

	if (options[HELP] || !argc || parse.nonOptionsCount() < 2)
	{
		option::printUsage(std::cout, opts);
		return 0;
	}

	std::string twoSFDirectory = parse.nonOption(0);
	std::string NCSFDirectory = parse.nonOption(1);

	return 0;
}
