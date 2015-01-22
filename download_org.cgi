#!/usr/bin/perl -wT

use CGI;

my $html= new CGI;
#get the file name from URL ex. http://<server_IP>/cgi-bin/download.cgi?a=testing.txt
my $file= $html->param('file'); 
# $file is nothing but testing.txt
my $filepath= "/home/vj/Desktop/tinyhttpd-0.1.0/htdocs/$file";

print ("Content-Type:application/x-download\n");
print ("Content-Disposition: attachment; filename=$file\n\n");

open FILE, "< $filepath" or die "can't open : $!";
binmode FILE;
local $/ = \10240;
while (<FILE>){
    print $_;
}
    close FILE;


