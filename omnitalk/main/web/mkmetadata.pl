#!/usr/bin/perl
use strict;
use Data::Dumper;
use File::Basename;

my $dir = dirname($0);
my $hdrname = $dir . "/stats.h";
my $incname = $dir . "/metadata.inc";

open(my $fh, "<", $hdrname);
open(my $out, ">", $incname);

my %format_specifiers = (
	"char*" => "%s",
	"uint8_t" => q|%" PRIu8 "|,
	"uint16_t" => q|%" PRIu16 "|,
);

my %metadata_types = ();
my %format_strings = ();
my @metadata_scalar_vars = ();
my @metadata_array_vars = ();
my $accum = {};
my $in_struct = 0;

# extract field and variable definitions
while (my $line = <$fh>) {
	chomp $line;
	
	if ($line =~ /^typedef\s+PROMETHEUS_METADATA\s+struct.*{$/) {
		$in_struct = 1;
		next;
	}
	
	if ($in_struct == 1 && $line =~ /^}\W+(\w+)/) {
		$in_struct = 0;
		$metadata_types{$1} = $accum;
		$accum = {};
		next;
	}
	
	if ($in_struct == 1) {
		# remove gubbins
		$line =~ s/_Atomic//;
		$line =~ s/const//;
		$line =~ s/\(//;
		$line =~ s/\)//;
		$line =~ s/^\s+//;
		$line =~ s/;\s*$//;
		
		my ($type, $name) = split /\s+/, $line;
		
		next if ($type eq "" || $name eq "");
		next if ($type eq "bool" && $name eq "ok");
				
		$accum->{$name} = $type;
		next;
	}
	
	if ($in_struct == 0 && $line =~ /^extern\s+PROMETHEUS_METADATA\s+(\S+)\s+(\S+)/) {
		my ($type, $var) = ($1, $2);
		$var =~ s/;//;
		
		if ($var !~ /\[/) {
			push @metadata_scalar_vars, [ $var, $type ];
		} else {
			$var =~ /\[(\S+)\]/;
			my $idx = $1;
			$var =~ s/\[(\S)+\]//;
			push @metadata_array_vars, [ $var, $idx, $type ];
		}
	}
}

# generate format strings
for my $mdtype (keys %metadata_types) {
	my $fields = $metadata_types{$mdtype};
	
	my @fieldnames = keys(%$fields);
	@fieldnames = sort @fieldnames;

	my $format = "\"%%METRIC%%{";
	my $first = 1;
	for my $field (@fieldnames) {
		$format .= "," unless $first;
		$format .= "$field=\\\"";
		$format .= $format_specifiers{$fields->{$field}};
		$format .= "\\\"";
		
		$first = 0;
	}
	
	$format .= "} 1\\n\"";
	for my $field (@fieldnames) {
		$format .= ", %%VAR%%.$field";
	}
	
	$format_strings{$mdtype} = $format;
}

for my $varpair (@metadata_scalar_vars) {
	my ($var, $type) = @$varpair;

	my $fmt = $format_strings{$type};
	die "no format string or typedef found for $type" unless $fmt;
	
	my $metric = $var;
	$metric =~ s/^stats_//;
	
	$fmt =~ s/%%VAR%%/$var/g;
	$fmt =~ s/%%METRIC%%/$metric/g;
	
	print $out "if ($var.ok) {\n";
	print $out "\tsnprintf(statsbuffer, STATSBUFFER_SIZE, $fmt);\n";
	print $out "\thttpd_resp_send_chunk(req, statsbuffer, HTTPD_RESP_USE_STRLEN);\n";
	print $out "}\n\n";	
}

if (@metadata_array_vars) {
	print $out "int metadata_vars_index = 0;\n\n";
	for my $v (@metadata_array_vars) {
		my ($var, $max, $type) = @$v;
		
		my $fmt = $format_strings{$type};
		die "no format string or typedef found for $type" unless $fmt;
		
		my $metric = $var;
		$metric =~ s/^stats_//;
				
		$var .= "[metadata_vars_index]";
		
		$fmt =~ s/%%VAR%%/$var/g;
		$fmt =~ s/%%METRIC%%/$metric/g;
		
		print $out "for (metadata_vars_index = 0; metadata_vars_index < $max; metadata_vars_index++) {\n";
		print $out "\tif ($var.ok) {\n";
		print $out "\t\tsnprintf(statsbuffer, STATSBUFFER_SIZE, $fmt);\n";
		print $out "\t\thttpd_resp_send_chunk(req, statsbuffer, HTTPD_RESP_USE_STRLEN);\n";
		print $out "\t}\n";	
		print $out "}\n\n";
	}
}
