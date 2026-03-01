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
$filename = "EXchess_v" . $vers . "_MacOSX_10.12.1";

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
$compile = "g++-6 -o $filename ../src/EXchess.cc -O3 -D VERS=$verstring -D TABLEBASES=1 -pthread";
print "$compile\n";
$temp = `$compile`;

