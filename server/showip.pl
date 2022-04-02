#!/usr/bin/perl

use CGI; ## load the cgi module

print "Content-type: text/plain; charset=iso-8859-1\n\n";
my $q = new CGI; ## create a CGI object
print $q->remote_host(); ## print the user ip address
 
