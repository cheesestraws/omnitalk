#!/usr/bin/perl
use strict;
use Data::Dumper;
use File::Basename;

my %warnings;
my $warning_count = 0;
my %alltests;
my %pass;
my %fail;
my %missing;

my $red      = "\033[0;31m";
my $green    = "\033[0;32m";
my $yellow   = "\033[0;33m";
my $nocolour = "\033[0m";

chdir(dirname($0));

while (my $line = <>) {
	if ($line =~ /warning:/) {
		if ($line =~ /(-W[\w-]+)/) {
			$warnings{$1}++;
			$warning_count++;
		}
	}
	if ($line =~ /\[TEST (\w+)] (\w+)/) {
		if ($2 eq "OK") {
			$pass{$1} = 1;
		}
		if ($2 eq "FAIL") {
			$fail{$1} = 1;
		}
	}
}


# Now load the list of all tests
open my $fh, "<", "test.inc";
while (my $line = <$fh>) {
	if ($line =~ /RUN_TEST\((\w+)\)/) {
		$alltests{$1} = 1;
	}
}

# missing tests?
for my $m (keys %alltests) {
	if (!$pass{$m} && !$fail{$m}) {
		$missing{$m} = 1;
	}
}

# print what we find
print "$green  * " . scalar %pass . " tests passed $nocolour\n";

if (scalar %fail == 0) {
	print $green;
} else { 
	print $red;
}
print "  * " . scalar %fail . " tests failed $nocolour\n";
for my $m (keys %fail) {
	print "$red    - $m $nocolour\n";
}

if (scalar %missing == 0) {
	print $green;
} else { 
	print $red;
}
print "  * " . scalar %missing . " tests missing results $nocolour\n";
for my $m (keys %missing) {
	print "$red    - $m $nocolour\n";
}

if ($warning_count == 0) {
	print $green;
} else { 
	print $yellow;
}
print "  * " . scalar $warning_count . " compiler warnings $nocolour\n";
if ($warning_count != 0) {
	for my $w (keys %warnings) {
		print "$yellow    - $warnings{$w} x $w $nocolour\n";
	}
}

# report success/failure

if ((scalar %missing > 0) or (scalar %fail > 0)) {
	exit 1;
}

0;
