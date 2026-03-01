EXchess v7.92 (beta), A chess playing program 
Copyright (C) 1997-2016 by Daniel C Homan, dchoman@gmail.com

=> For updates see: https://sites.google.com/site/experimentalchessprogram/

This is "beta" software, meaning it has not been extensively tested on 
a variety of systems and may still contain bugs.  Use at your own risk.

-------------------------------------------------
License Information
-------------------------------------------------

This software comes with ABSOLUTELY NO WARRANTY of any kind.  

This program is freeware, distributed under the GNU public
license.  See file "license.txt" for details.  I ask only that you 
e-mail me your impressions/suggestions so that I may 
improve future versions.

*** NOTE: See notes on the source code version (below) for 
discussion of parts of this program that do not fall under the 
GNU public license and retain their own copyrights. 

-------------------------------------------------
Notes on the Graphical Version of EXchess
-------------------------------------------------
The simple graphical interface provided with this version is
only really suited for quick, casual games.  Much nicer, dedicated
interfaces, such as xboard/winboard or Arena, can be used with 
the console/text version of EXchess and provide much more functionality. 

 - This simple graphical user interface is based on the 
    Checkers example program by Bill Spitzak that comes with the 
    FLTK library and released under the GNU Public License (GPL).  The
    interface is modified to work with my chess program and provide
    chess specific options.   

 - The chess piece images come from the Xboard/Winboard GPL
    distribution by Tim Mann, http://www.tim-mann.org/xboard.html, 
    and remain the copyright of the original artists.

 - Pressing the space bar will generate a menu in most situations.
    Menu content may vary by context.

 - Scan through the menus to see the possibilities.

 - For multi-processing on windows computers this version uses the 
   pthread-win32 library available from http://sourceware.org/pthreads-win32/ 

-------------------------------------------------
A few General Notes
-------------------------------------------------

The file "search.par" along with the opening book, 
should be in the directory from which EXchess is being run. 

Values in the "search.par" file can be used to modify how 
EXchess thinks. The THREADS parameter controls how many
processors EXchess will use during the search; however,
this can be overridden by the 'cores' command from xboard
compatible interfaces. Do not exceed the number of 
processors on your machine. The maximum value allowed by
the program is 32 at this time.

(On a MacOS system, search.par and the opening book are
part of the application bundle.)

EXchess has a logging function which can be turned on by 
setting the relevant parameters in the "search.par" file. 
When logging is "on", a new log file is created each time
the program is started.  The "MAX_LOGS" number is the maximum number 
of log files that can be created.  No new log files will
be created beyond this number - you must delete the old log 
files first.

The opening book released with EXchess was built from an 
excellent file of PGN games created by Norm Pollock and released 
from his webpage: http://www.crafty-chess.com/down/Pollock/.
The opening book also includes learning results from thousands of 
games.  The included opening book should work on most systems, 
but if not, see the notes below on how to build your own opening 
book with the source code version.  The GUI version also includes
a 'start_bk.dat' to further randomize play against humans, and this
book is not suitable for use against computers.

-------------------------------------------------
Notes on the console version for xboard/winboard.
-------------------------------------------------
 - Once compiled (see below) "EXchess_console" will be a text-based 
   interface version of the program.

 - This version has a serviceable text interface or it can be run with
   Tim Mann's Xboard (Unix/Linux) or Winboard.  EXchess should detect
   that it is running under xboard or winboard, but to make sure, you 
   can include the 'xb' command-line option if you choose...

   winboard -fcp "c:\directory\EXchess_console xb" 

   or

   xboard -fcp "/directory/EXchess_console xb" 

 - To run EXchess in plain text mode, just type "EXchess_console" in the 
   EXchess directory

 - There is an additional command line option "hash", you can set the
   hash size in megabytes like

   ./EXchess_console hash 256      (for a 256 megabyte hash file, for example)

   If this option is used with xboard or winboard, use the line

   winboard -fcp "c:\directory\EXchess_console xb hash 256"

   or
  
   xboard -fcp "/directory/EXchess_console xb hash 256"   

   The default hash size is set to 256 megabytes in search.par and
   can be modified there as well. 

   Note: Don't set the hash table size larger than about one-half of
   the available RAM. Doing so may cause swapping to the hard disk and 
   slow the program down considerably.  

 - Some basic help is available by typing "help" at the command prompt.

 - The file "wac.epd" is a testsuite and can be run by the "testsuite"
   command inside the program or directly from the command line:

   ./EXchess_console hash 256 test wac.epd wac_results.txt 5

   Which will run the wac.epd test at 5 seconds per position and record
   the results into the file wac_results.txt...  For the command line
   "test" option, the hash command must be specified first as indicated
   above.

 - The 'build' command lets you make your own opening book out of a pgn
   text file.  It requires 1-2 times the size of the pgn file in 
   temporary storage space on the disk.  The 'build' command can currently
   handle pgn files up to 60 MBytes in size... To use a larger file, you
   will need to modify some definitions in book.cpp and recompile.

 - You can also build a special 'starting book' with the same command, but
   the format of the PGN file used is non-standard, see 'start.pgn' which
   comes with the source code version for the format.  All moves in a
   'starting book' will be played with equal probability.

---------------------------------
Notes on the source code version.  
---------------------------------
This software naturally includes some code and ideas gathered 
from other places and these are indicated in the individual 
source code files.  All of these instances remain the copyright 
of the original authors who reserve all rights and their original
licenses still apply.  A specific example is that EXchess supports 
endgame tablebases by Dr. Eugene Nalimov.  Two functions in the file 
"probe.cpp" are modified from the examples given in Dr. Nalimov's 
'tbgen' distribution.  They are for interfacing with Dr. Nalimov's 
endgame tablebase code and are copyrighted by Eugene Nalimov.  The 
functions are NOT part of the GNU Public License release of EXchess' 
source code.  They are only useful if one separately downloads the 
'tbgen' distribution, see "probe.cpp" for instructions.  Other 
examples include...

** interrupt code for winboard on MS-Windows in the function 
   "inter()" in the file main.cpp is from Crafty by Bob Hyatt.  
   Crafy's license is open source with the provision that changes 
   to the code are shared with the community

** The GUI interface in fltk_gui.cpp is based on the FLTK GUI toolkit
   Checkers example program: FLTK Checkers Copyright (C) 1997 Bill 
   Spitzak spitzak_at_d2.com.  FLTK is released under the GNU public
   license 

** Piece images come from the Xboard/Winboard distribution by Tim 
   Mann: http://www.tim-mann.org/xboard.html, and remain the 
   copyright of the original artists.  Xboard/Winboard is released
   under the GNU public license.  See the REAMDE.bitmaps file in the
   src/bitmaps directory of the source code distribution.

Compilation and other points:
 - In my experience, EXchess compiles on Mac OS X, Windows (under cygwin) 
    and Linux systems, although I only use Mac OS X and Linux on a 
    regular basis.  Earlier versions of EXchess would also compile 
    fine under MSVC (via setting #define UNIX 0, and 
    #define MSVC 1 in define.h), but I haven't tried it in several years.

 - The file "EXchess.cc" is provided which includes all the necessary 
   source to compile EXchess.  On my linux computer, I issue the following 
   command to compile the console version...

   g++ -o EXchess_console src/EXchess.cc -Ofast -flto -fwhole-program -pthread

   Your command line and compiler options may be different.

 - If you have the FLTK GUI library installed, you can include the
   -DMAKE_GUI switch to make the GUI version.  You will probably need to 
   use the fltk-config command to work out the necessary library 
   dependencies on your system...

   fltk-config -DMAKE_GUI --compile src/EXchess.cc

   which may make an un-optimized executable.  For most human play, 
   this is all that is necessary, but you can use the command given
   by this and add optimizations if you wish.

 - The included opening book (main_bk.dat) should work on 
   most systems, but if not, you will have to build your own opening book.  
   You can do this starting from a pgn game collection file using the 
   "build" command in the console version, see above.  Don't bother with 
   a 'starting book' unless you know what you are doing.  






