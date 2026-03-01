
@piece = ('p', 'n', 'b', 'r', 'q', 'k');


for($i = 0; $i < 6; $i++) {
    $name = $piece[$i] . $ARGV[0] . "s.bm";
    print "$name  -->  ";
    open(IN, $name) || die "Can't open $name";
    
    $name =~ s/s/bf/;
    print "$name\n";
    open(OUT1, ">$name") || die "Can't create $name";

    $name =~ s/bf/bb/;
    print "$name\n";
    open(OUT2, ">$name") || die "Can't create $name";

    $tag1 = "s_";
    $tag2 = "bf_";

    while(<IN>) {
	s/$tag1/$tag2/;
        print $_;
	print OUT1 $_;
        print OUT2 $_;
    }
    
    close(IN);
    close(OUT1);
    close(OUT2);

    $name = $piece[$i] . $ARGV[0] . "o.bm";
    print "$name  -->  ";
    open(IN, $name) || die "Can't open $name";
    
    $name =~ s/o/wf/;
    print "$name == ";
    open(OUT1, ">$name") || die "Can't create $name";

    $name =~ s/wf/wb/;
    print "$name\n";
    open(OUT2, ">$name") || die "Can't create $name";

    $tag1 = "o_";
    $tag2 = "wf_";

    while(<IN>) {
	s/$tag1/$tag2/;
        print $_;
	print OUT1 $_;
        print OUT2 $_;
    }
    
    close(IN);
    close(OUT1);
    close(OUT2);


}

