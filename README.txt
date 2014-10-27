NCSF/SDAT Utilities
By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]

(If you need to email me, please include NCSF in the subject line.)

Special thanks to fincs for FeOS Sound System, as well as the DeSmuME
team for their open-source Nintendo DS emulator.

2SF Tags to NCSF Version History
--------------------------------
v1.0 - 2013-03-30 - Initial Version
v1.1 - 2012-03-30 - Added option to rename the NCSFs, requested by Knurek.
v1.2 - 2012-04-10 - Made it so a file is not overwritten when renaming if
                    a duplicate is found.

NDS to NCSF Version History
---------------------------
v1.0 - 2013-03-25 - Initial Version
v1.1 - 2013-03-26 - Merged SDAT Strip's verbosity into the SDAT class'
                    Strip function.
                  - Modified how excluded SSEQs are handled when stripping.
                  - Corrected handling of files within an existing SDAT.
v1.2 - 2013-03-28 - Made timing to be on by default, with 2 loops.
                  - Added options to change the fade times.
v1.3 - 2013-03-30 - Only remove files from the destination directory that
                    were created by this utility, instead of all files.
                  - Slightly better file checking when copying from an
                    existing SDAT, will check by data only if checking by
                    filename and data doesn't give any results.
v1.4 - 2014-10-15 - Improved timing system by implementing the random,
                    variable, and conditional SSEQ commands.
v1.5 - 2014-10-26 - Save the PLAYER blocks in the SDATs as opposed to
                    stripping them.
                  - Detect if the NDS ROM is a DSi ROM and set the prefix
                    of the NCSFLIB accordingly.
                  - Fixed removal of output directory if there are no
                    SSEQs found.

SDAT Strip Version History
--------------------------
v1.0 - 2013-03-25 - Initial Version
v1.1 - 2013-03-26 - Merged verbosity of SDAT stripping into the SDAT class
                    and removed the static Strip function from this.
                  - Copied NDS to NCSF's include/exclude handling to here.
v1.2 - 2014-10-25 - Save the PLAYER blocks in the SDATs as opposed to
                    stripping them.

SDAT to NCSF Version History
----------------------------
v1.0 - 2013-03-25 - Initial Version
v1.1 - 2013-03-28 - Made timing to be on by default, with 2 loops.
                  - Added options to change the fade times.
v1.2 - 2014-10-15 - Improved timing system by implementing the random,
                    variable, and conditional SSEQ commands.

These utilities are used to work with SDAT files from Nintendo DS ROMs.  SDATs are
created through Nitro Composor, a program in the Nintendo Nitro SDK for the DS.
NCSF is a PSF-style music format that uses the SDAT as it's "program".

Contains:
* 2SF Tags to NCSF v1.2 - A utility to copy tags from a 2SF set into an NCSF set.
*      NDS to NCSF v1.5 - A utility to take a Nintendo DS ROM and create an NCSF out of it.
*       SDAT Strip v1.2 - A utility to take an SDAT and strip it of all unneccesary items.
                          (NOTE: Superceded by NDS to NCSF.)
*     SDAT to NCSF v1.2 - A utility to take an SDAT and create an NCSF out of it.
                          (NOTE: Superceded by NDS to NCSF.)
*       zlib DLL v1.2.8 - Required by 2SF Tags to NCSF, NDS to NCSF, and SDAT to NCSF.

WINDOWS
-------
For Windows, the binaries are included (these are command-line tools).  If you do not
already have it, you will need the Microsoft Visual C++ 2010 Runtime installed.  You
can obtain it here:

http://www.microsoft.com/en-us/download/details.aspx?id=5555

(NOTE: You will usually not require this runtime if you have Visual Studio installed,
or if you plan on installing Visual Studio.  But you will need it if you do not have
Visual Studio installed.)

If you wish to compile these binaries under Windows yourself, you can do so with
Microsoft Visual Studio.  You must use version 2010 or later.  If you do not have
Microsoft Visual Studio, you can obtain the Express version of Microsoft Visual C++
from Microsoft, at:

http://www.microsoft.com/visualstudio/eng/downloads

You may use any Express version that is 2010 or later.  Once you have Visual Studio
installed, you need to open the SDATStuff.sln file.  If using a version of Visual
Studio later than 2010, you may be asked to upgrade the solution.

Of note, the utilities will not compile out of the box immediately.  The zlib library
is also required.  zlib can be obtained from here:

http://www.zlib.net/

You need to download the "compiled DLL" version.

Once you have this, you need to modify the value of zlibRootDir in common\common.props.
Just replace ZLIBFIXME with the directory to which you extracted the above file.  Once
this has been done, you can compile the solution.

UNIX-LIKE OPERATING SYSTEMS
---------------------------
For Unix-like operating systems, you will need to compile the utilities yourself.
You will need a compiler that supports C++11, at the very least it's auto keyword
and the new return value syntax.  For GCC, version 4.6 and later will work.  For
Clang, nearly any version will work.  You will also need the GNU version of Make.
This will usually be installed as either "make" or "gmake" depending.  To build
the utilities, simply run "make" or "gmake" from this directory.
