#!/usr/bin/perl

use CGI;

$q = CGI->new;

$filename = $q->param('upload_file');
$fh = $q->upload('upload_file');
if (defined $fh) {
	my $ioh = $fh->handle;
	open (OUTFILE, '>', $filename);
	while ($bytesread = $ioh->read($buffer, 1024)) {
		print OUTFILE $buffer;
	}
	close OUTFILE;
print $q->header,
      $q->start_html('uploaded'),
      $q->h1('The file '),
      $filename,
      $q->h1(' has been uploaded'),
      $q->end_html;
} else {

print $q->header,
      $q->start_html('uploaded'),
      $q->h1('The file '),
      $filename,
      $q->h1(' upload failed!!!'),
      $q->end_html;
}
