How to use new tablebase generator
==================================

1. Compile it. If you are using Microsoft C++ at x86 platform, use the
   following command line:
      cl -Ox -Oa -Gr -G6 tbgen.cpp
   If you are using Microsoft C++ at Alpha, use
      cl -Ox -Oa -QAgq -Gt32 -QA21164A tbgen.cpp
   (maybe you'll have to specify different processor type).
   If you are using GNU C, use
      gcc -O2 -o tbgen tbgen.cpp
   If you are using other C++ compiler, consult the manual. Please be sure
   that:
      - you specified the highest optimization level (usually -O2),
      - compiler will inline as many functions that are marked 'inline'
        as possible (sometimes that is controlled by other flag).
   If you want to build 4+1 tablebases (e.g. KPPPK), you'll have to declare
   symbol T41_INCLUDE; usually that's done by specifying -DT41_INCLUDE in
   the command line. Generator is several percents faster without 4+1
   support.

2. Run it. You have to have A LOT OF memory if you want to generate 5-man
   tables that contains pawns (I used machine with 512Mb during debugging,
   and recommend to have at least 256Mb). 4-man tables can be easily
   generated at a machine with 32Mb of RAM.

   To generate all 3- and 4-man tables you have to type
      tbgen -m 10 -p kpkp kppk
   (program will use 10Mb of RAM for generated tables and will either map
   or load necessary minor tables into memory). It takes 2 hours to generate
   all 3- and 4-man tables at PII/400.

   To generate 5-man pawnless table (and all necessary minor tables) you
   have to type
      tbgen -m 200 -p <table>
   It usually takes from several hours to one day to generate such a table
   at PII/400 with 128Mb of RAM.

   To generate 5-man table, where only one side has pawn, you have to type
      tbgen -m 580 -p <table>
   It can take several days to generate such a table at Alpha/533 or PII/400
   with 512Mb of RAM.

   Unfortunately, 32-bit operating systems have not enough address space to
   map or load all necessary minor tables into memory when generating 5-man
   tables with 2 or 3 pawns. You have to use either 64-bit OS, or TB caching
   code that usually is less efficient but will not use gigabytes of address
   space, e.g.
      tbgen -m 572 -c 100 <table>
   (program will use 572Mb of RAM for generated tables and 100Mb of RAM
   for minor tables caching).

3. Verify it, if necessary (if nobody earlier generated that table).
   There are two ways to do that:
   (1) You can have pair of Edward's tables. If so, you can run
         tbcmp <table> <directory of the old TBs>
       (of course you'll have to compile tbcmp.cpp first). Please note that
       there must be differencies in tables where both sides have pawns,
       because of en-passant captures, so it's not much sense to run tbcmp
       for those tables. If there will be any other differencies, please send
       output of tbcmp to me, so that I can fix tbgen.
   (2) Run
         tbgen -m <memory size> -c <cache size> -e <table>
       Generator will check internal consistency of the table - e.g. it will
       check that one of the ancestors of (mate in N) is (loss in N-1), etc.

4. Compress resulting TBs, if necessary. To do so you'll need DATACOMP
   utility. Compile and link DATACOMP.C and COMPLIB.C. After that compress
   each TB using command line
      DATACOMP e:8192 file_name
   Do not specify several file names in one call to DATACOMP - always compress
   one TB at a time.

   If you want to copy compressed TB to any system or disk that supports only
   CP/M-MSDOS style names (8+3), you can rename compressed file from
   kxykz.nb?.emd to kxykznb?.emd (i.e. concatenate first extension with file
   name).

Have fun!

Eugene Nalimov
eugenen@microsoft.com
