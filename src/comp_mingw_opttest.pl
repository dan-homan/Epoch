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
$filename = "EXchess_v".$vers;

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
$execname = $filename."_win64.exe";
$compile = "x86_64-w64-mingw32-g++ -o $execname ../src/EXchess.cc -O3 -D VERS=$verstring -D TABLEBASES=1 -pthread -D MINGW=1 -static -s -msse4.2 -mpopcnt";
print "$compile\n";
$temp = `$compile`;
#$execname = $filename."_win32.exe";
#$compile = "i686-w64-mingw32-g++ -o $execname ../src/EXchess.cc -O3 -D VERS=$verstring -D TABLEBASES=1 -pthread -D MINGW=1 -static -s";
#print "$compile\n";
#$temp = `$compile`;

