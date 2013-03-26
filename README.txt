NCSF/SDAT Utilities
By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]

These utilities are used to work with SDAT files from Nintendo DS ROMs.  SDATs are
created through Nitro Composor, a program in the Nintendo Nitro SDK for the DS.
NCSF is a PSF-style music format that uses the SDAT as it's "program".

Contains:
*  NDS to NCSF v1.0 - A utility to take a Nintendo DS ROM and create an NCSF out of it.
*   SDAT Strip v1.0 - A utility to take an SDAT and strip it of all unneccesary items.
                      (NOTE: Superceded by NDS to NCSF.)
* SDAT to NCSF v1.0 - A utility to take an SDAT and create an NCSF out of it.
                      (NOTE: Superceded by NDS to NCSF.)

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

You need to download the "compiled DLL" version.  Once you have this, you need to modify
each of the projects to point to the correct location of zlib.  In all the projects,
under Configuration Properties -> C/C++ -> General, there is a ZLIBFIXME before the zlib
path of "Additional Include Directories".  Replace the FIXME with the path to where you
extracted zlib.  The same will be done to "Additional Library Dependencies" under
Configuration Properties -> Linker -> General for the NDStoNCSF and SDATtoNCSF projects,
as well as "Command Line" under Configuration Properties -> Build Events -> Post-Build
Event under those same 2 projects.  Once this has been done, you can compile the solution.

UNIX-LIKE OPERATING SYSTEMS
---------------------------
For Unix-like operating systems, you will need to compile the utilities yourself.
You will need a compiler that supports C++11, at the very least it's auto keyword
and the new return value syntax.  For GCC, version 4.4 and later will work.  For
Clang, nearly any version will work.  You will also need the GNU version of Make.
This will usually be installed as either "make" or "gmake" depending.  To build
the utilities, simply run "make" or "gmake" from this directory.