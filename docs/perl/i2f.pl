#!/usr/bin/env perl
use strict;
use warnings;

# SPDX-License-Identifier: MIT
# scspell-id: scspell-id: 091b1420-b6c8-11ef-b951-80ee73e9b8e7
#
# ---------------------------------------------------------------------------
#
# i2f.pl - Convert Markdown indented-style to fenced-style code blocks.
#
# ---------------------------------------------------------------------------
#
# Copyright (c) 2016 Tobias Potocek <tobiaspotocek@gmail.com>
# Copyright (c) 2024 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2025 The DPS8M Development Team
#
# ---------------------------------------------------------------------------

my $filename = $ARGV[0];
open my $fh, '<', $filename or die "Could not open file '$filename': $!";
my @input = <$fh>;
close $fh;

my $content = join '', @input;

sub isCode {
    my $line = shift;
    return $line =~ /^    /;
}

sub removeIndent {
    my $line = shift;
    $line =~ s/^    //;
    return $line;
}

sub isEmpty {
    my $line = shift;
    return $line eq '' || $line =~ /^ *$/;
}

my $inCode = 0;
my @output;

foreach my $line (split /\n/, $content) {
    if ($inCode) {
        if (isEmpty($line)) {
            push @output, $line;
        } elsif (isCode($line)) {
            push @output, removeIndent($line);
        } else {
            $output[-1] = '```';
            push @output, '', $line;
            $inCode = 0;
        }
    } else {
        if (isCode($line)) {
            push @output, '```dps8';
            push @output, removeIndent($line);
            $inCode = 1;
        } else {
            push @output, $line;
        }
    }
}

open $fh, '>', $filename or die "Could not open file '$filename': $!";
print $fh join("\n", @output);
close $fh;
