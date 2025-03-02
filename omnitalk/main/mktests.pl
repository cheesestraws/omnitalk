#!/usr/bin/perl
use strict;
use Data::Dumper;
use File::Basename;

# This script automatically generates test.inc, which calls all the unit tests
# First, change into our own directory
chdir(dirname($0));

open(my $out, ">", "./test.inc");

# Misuse grep instead of recursing ourselves
my $grep_results = `grep -r TEST_FUNCTION .`;
my @res_array = split /\n/, $grep_results;

my %tests;

printf("finding unit tests:\n");

for my $line (@res_array) {
	chomp($line);
	$line =~ /^([^:]+):(.*)$/;

	my ($file, $srcline) = ($1, $2);
	
	# only look at header files
	next unless $file =~ /\.h$/;
	next unless $srcline =~ /^\s*TEST_FUNCTION\((\w+)\)\s*;/;
	
	my $testname = $1;
	$file =~ s|^\./||;
	
	if (!exists($tests{$file})) {
		$tests{$file} = [];
	}
	
	push @{$tests{$file}}, $testname;
	
	print "- $file => $testname\n";
}

my @files = sort(keys(%tests));
for my $file (@files) {
	print $out qq{#include "$file"\n\n};
	for my $test (@{$tests{$file}}) {
		print $out qq(RUN_TEST($test);\n);
	}
	print $out "\n";
}

0;
