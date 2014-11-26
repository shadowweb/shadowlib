#!/usr/bin/perl
use strict;
use warnings;
use integer;

my %options = ();

sub usage
{
    print "-e <executable file> -i <base file name> -t <tool path>\n";
    exit;
}

sub init
{
    use Getopt::Std;
    getopts("i:e:t:", \%options) or usage();
    usage() if not ($options{i} and $options{e} and $options{t});
}

my %func_addr = ();

sub load_input
{
    my ($input_file) = @_;
    print "Loading '$input_file.addr'...\n";
    open(INPUTFILE, "<$input_file.addr") or die "Failed to open input file: '$input_file.addr'\n";
    while (<INPUTFILE>)
    {
        my $line = $_;
        chomp $line;
        $func_addr{$line} = 1;
    }
    close (INPUTFILE);
    return;
}

my %symbols = ();

sub load_symbols
{
    my ($exe_file) = @_;
    print "Loading symbols from '$exe_file'...\n";
    if (-x $exe_file)
    {
        open FILE, "nm -C $exe_file |" or die "Failed to open pipe on \'nm -C $exe_file\'\n";
        while (<FILE>)
        {
            my $line = $_;
            chomp $line;
            if ($line =~ /([0-9a-fA-F]+)\s+\w\s+(.*)$/)
            {
                my $addr = $1;
                my $symbol = $2;
                $addr =~ s/^0+//;
                $symbols{$addr} = $symbol;
            }
        }
    }
}

my %addr_to_symbols = ();

sub dump_output
{
    my ($output_file) = @_;
    print "Writing to  '$output_file.symbols'...\n";
    open(OUTPUTFILE, ">$output_file.symbols") or die "Failed to open output file: '$output_file.symbols'\n";
    foreach my $addr (sort keys %func_addr)
    {
        my $stripped_addr = $addr;
        $stripped_addr =~ s/^0x//;
        my $symbol = (exists $symbols{$stripped_addr})? $symbols{$stripped_addr} : $stripped_addr;
        $addr_to_symbols{$stripped_addr} = $symbol;
        print OUTPUTFILE "$stripped_addr $symbol\n";
    }
    close(OUTPUTFILE);
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
            if (exists $addr_to_symbols{$addr})
            {
                print OUTFILE "$addr_to_symbols{$addr}";
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

my @input_files = glob("$options{i}.*");
my @threads = ();

foreach my $file (@input_files)
{
    if ($file =~ /TRACE\.(\d+)$/)
    {
        my $thread_id = $1;
        push @threads, $1;
    }
}
my $threads_cs_list = join( ',', @threads);
my $cmd = "$options{t}/build/src/tools/trace/build-call-tree  -input-file $options{i} --input-threads $threads_cs_list --log-stdout";
print "$cmd\n";
my $result = `$cmd`;
print $result;

load_symbols($options{e});
load_input($options{i});
dump_output($options{i});

my @files = glob("$options{i}.*.out");
foreach my $file  (@files)
{
    print_tree("$file", "$file.tree");
}
