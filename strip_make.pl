#!/usr/bin/perl
use strict;
use warnings;
use utf8;

my $mode = 0;

while (<>) {
	if (/REMOVE_THIS_FOR_RELEASE/) {
		$mode = 1;
		next;
	}
	if ($mode == 1) {
		if (/^else/) {
			$mode = 2;
		}
		next;
	}
	if ($mode == 2) {
		if (/^endif/) {
			$mode = 0;
			next;
		}
	}
	print;
}

