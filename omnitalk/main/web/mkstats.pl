#!/usr/bin/perl
use strict;
use File::Basename;

my $dir = dirname($0);
my $hdrname = $dir . "/stats.h";
my $incname = $dir . "/stats.inc";

my @stats = ();

open(my $fh, "<", $hdrname);
open(my $out, ">", $incname);

while (my $line = <$fh>) {
	chomp $line;
	if ($line !~ /^\W+prometheus_([^_\W]+)_t\W+([\w]+)/) {
		next;
	}
	
	my $type = $1;
	my $var = $2;
	my $metric = $var;
	my @labels = ();
	my $help = "";
	
	# do we have labels in the metric name?
	if ($var =~ /__/) {
		@labels = split(/__/, $var);
		$metric = shift @labels;
		map { $_ =~ s/_/=\\"/; $_ .= "\\\""; } @labels;
	}
	
	# extract help from comment
	if ($line =~ m|//.*help:(.*)$|) {
		$help = $1;
		$help =~ s/^\W+//;
	}
	
	my $ltxt = join(",", @labels);
		
	my $macro = uc($type);
	print $out "${macro}_FIELD(req, $var, $metric, \"$ltxt\", \"$help\");\n";
}

close($fh);