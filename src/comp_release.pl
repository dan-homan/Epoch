# 
# Perl script to compile and name console versions of EXchess
# during development
#

$date = `date "+%Y_%m_%d"`;
chomp($date);

if($ARGV[0]) {
    $vers = $ARGV[0];
} else {
    $vers = $date;
}
$filename = "EXchess_v" . $vers . "_win32.exe";

if(-e "./$filename") { 
    print "File $filename already exists!  Overwrite (y/n)? ";
    $resp = <STDIN>;
    chomp($resp);
    if($resp =~ /n/) {
	print "Quitting without compile...\n";
	print "Try again with a different name specified on command line.\n";
	exit;
    }
}
$verstring = "\\" . "\"" . $vers . "\\" . "\"";

print "Compiling $filename...\n";
$compile = "i686-w64-mingw32-g++ -m32 -o $filename ../src/EXchess.cc -Ofast -flto -fwhole-program -D VERS=$verstring -D TABLEBASES=1 -D MINGW=1 -Wl,--stack,8388608 -lpthreadGC2 -I. -L. -static -DPTW32_STATIC_LIB";
print "$compile\n";
$temp = `$compile`;

$filename = "EXchess_v" . $vers . "_win64.exe";

if(-e "./$filename") { 
    print "File $filename already exists!  Overwrite (y/n)? ";
    $resp = <STDIN>;
    chomp($resp);
    if($resp =~ /n/) {
	print "Quitting without compile...\n";
	print "Try again with a different name specified on command line.\n";
	exit;
    }
}

$verstring = "\\" . "\"" . $vers . "\\" . "\"";

print "Compiling $filename...\n";
$compile = "x86_64-w64-mingw32-g++ -m64 -o $filename ../src/EXchess.cc -Ofast -flto -fwhole-program -D VERS=$verstring -D TABLEBASES=1 -D MINGW=1 -Wl,--stack,8388608 -static -DPTW32_STATIC_LIB -lpthreadGC2_x86 -I. -L. -fpermissive";
print "$compile\n";
$temp = `$compile`;


$filename = "EXchess_v" . $vers . "_linux32";

if(-e "./$filename") { 
    print "File $filename already exists!  Overwrite (y/n)? ";
    $resp = <STDIN>;
    chomp($resp);
    if($resp =~ /n/) {
	print "Quitting without compile...\n";
	print "Try again with a different name specified on command line.\n";
	exit;
    }
}
$verstring = "\\" . "\"" . $vers . "\\" . "\"";

print "Compiling $filename...\n";
$compile = "g++ -m32 -o $filename ../src/EXchess.cc -Ofast -flto -fwhole-program -D VERS=$verstring -D TABLEBASES=1 -static -pthread";
print "$compile\n";
$temp = `$compile`;

$filename = "EXchess_v" . $vers . "_linux64";

if(-e "./$filename") { 
    print "File $filename already exists!  Overwrite (y/n)? ";
    $resp = <STDIN>;
    chomp($resp);
    if($resp =~ /n/) {
	print "Quitting without compile...\n";
	print "Try again with a different name specified on command line.\n";
	exit;
    }
}

$verstring = "\\" . "\"" . $vers . "\\" . "\"";

print "Compiling $filename...\n";
$compile = "g++ -m64 -o $filename ../src/EXchess.cc -Ofast -flto -fwhole-program -D VERS=$verstring -D TABLEBASES=1 -static -pthread";
print "$compile\n";
$temp = `$compile`;


