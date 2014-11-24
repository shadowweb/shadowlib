#!/usr/bin/perl
use strict;
use warnings;
use integer;

my %options = ();

sub usage
{
    print "-i <tree file> -s <symbols file> -o <output file>\n";
    exit;
}

sub init
{
    use Getopt::Std;
    getopts("i:s:o:", \%options) or usage();
    usage() if not ($options{i} and $options{s} and $options{o});
}

my %symbols = ();

sub load_symbols
{
    my ($symbol_file) = @_; 
    print "Loading symbols from '$symbol_file'...\n";
    open FILE, "<$symbol_file" or die "Failed to open symbols file: $symbol_file\n";
    while (<FILE>)
    {   
        my $line = $_; 
        chomp $line;
        if ($line =~ /([0-9a-fA-F]+)\s+(.*)$/)
        {   
            my $addr = $1; 
            my $symbol = $2; 
            # $addr =~ s/^0+//;
            $symbols{$addr} = $symbol;
        }
    }
}

sub print_tree
{
    my ($input_file, $output_file) = @_;
    print "Reading symbols from '$input_file' and writing try into '$output_file'\n";
    open INFILE, "<$input_file" or die "Failed to open input file: $input_file\n";
    open OUTFILE, ">$output_file" or die "led to open output file: $output_file\n";
    while (<INFILE>)
    {
        my $line = $_; 
        chomp $line;
        if ($line =~ /(\d+)\s+0x([0-9a-fA-F]+)\s+(\d+)$/)
        {
            my $offset = $1;
            my $addr = $2;
            my $repeat = $3;
            # print "$offset $addr $repeat\n";
            print OUTFILE "--"x$offset;
            print OUTFILE "\\ ";
            if ($repeat > 1)
            {
              print OUTFILE "($repeat) ";
            }
            if (exists $symbols{$addr})
            {
                print OUTFILE "$symbols{$addr}";
            }
            else
            {
                print OUTFILE "$addr";
            }
            print OUTFILE "\n";
        }
    }
    close (INFILE);
    close (OUTFILE);
}


sub dump_symbols
{
    foreach my $addr (sort keys %symbols)
    {
      print "$addr $symbols{$addr}\n";
    }
}

init();
load_symbols($options{s});
print_tree($options{i}, $options{o});

# dump_symbols();

