#!/usr/bin/perl
##############################################################################
# Gotongi Firewall Puncher        Version 0.01                               #
# Created 2020-07-18              Last Modified 2020-08-20                   #
# mainly based on FormMail        Version 1.93                               #
# Copyright 1995-2009 Matt Wright mattw@scriptarchive.com                    #
# Matt's Script Archive, Inc.:    http://www.scriptarchive.com/              #
##############################################################################
# COPYRIGHT NOTICE                                                           #
# Copyright 1995-2009 Matthew M. Wright  All Rights Reserved.                #
#                                                                            #
# FormMail may be used and modified free of charge by anyone so long as this #
# copyright notice and the comments above remain intact.  By using this      #
# code you agree to indemnify Matthew M. Wright from any liability that      #
# might arise from its use.                                                  #
#                                                                            #
# Selling the code for this program without prior written consent is         #
# expressly forbidden.  In other words, please ask first before you try and  #
# make money off of my program.                                              #
#                                                                            #
# Obtain permission before redistributing this software over the Internet or #
# in any other medium. In all cases copyright and header must remain intact. #
##############################################################################
# ACCESS CONTROL FIX: Peter D. Thompson Yezek                                #
# XSS + REDIRECT FIX: Francesco Ongaro, Giovanni Pellerano & Antonio Parata  #
#   v1.93             http://www.ush.it/team/ush/hack-formmail_192/adv.txt   #
##############################################################################
# Define Variables                                                           #
#      Detailed Information Found In README File.                            #

# @referers allows forms to be located only on servers which are defined     #
# in this field.  This is a security fix to prevent others from using your   #
# FormMail script on their web site.                                         #

#@referers = ('scriptarchive.com','72.52.156.109');

# @recipients defines the e-mail addresses or domain names that e-mail can   #
# be sent to.  This must be filled in correctly to prevent SPAM and allow    #
# valid addresses to receive e-mail.  Read the documentation to find out how #
# this variable works!!!  It is EXTREMELY IMPORTANT.                         #
#@recipients = &fill_recipients(@referers);

# ACCESS CONTROL FIX: Peter D. Thompson Yezek                                #
# @valid_ENV allows the sysadmin to define what environment variables can    #
# be reported via the env_report directive.  This was implemented to fix     #
# the problem reported at http://www.securityfocus.com/bid/1187              #

@valid_ENV = ('REMOTE_HOST','REMOTE_ADDR','REMOTE_USER','HTTP_USER_AGENT');

# Done                                                                       #
##############################################################################

print "Content-type: text/plain; charset=utf-8\n\n";

# Retrieve Date
#&get_date;

# Parse Form Contents
&parse_form;

# Read in group file
&parse_file;

# Check Required Fields
#&check_required;

# Send E-Mail
#&send_mail;

# Return HTML Page or Redirect User
#&return_html;


sub get_date {

    # Define arrays for the day of the week and month of the year.           #
    @days   = ('Sunday','Monday','Tuesday','Wednesday',
               'Thursday','Friday','Saturday');
    @months = ('January','February','March','April','May','June','July',
               'August','September','October','November','December');

    # Get the current time and format the hour, minutes and seconds.  Add    #
    # 1900 to the year to get the full 4 digit year.                         #
    ($sec,$min,$hour,$mday,$mon,$year,$wday) = (localtime(time))[0,1,2,3,4,5,6];
    $time = sprintf("%02d:%02d:%02d",$hour,$min,$sec);
    $year += 1900;

    # Format the date.                                                       #
    $date = "$days[$wday], $months[$mon] $mday, $year at $time";
    print $date;
}

sub parse_form {

    # Define the configuration associative array.                            #
    %Config = ('table','', 'password','', 'name','');

    #print 'Parsing Form\n';
    # Determine the form's REQUEST_METHOD (GET or POST) and split the form   #
    # fields up into their name-value pairs.  If the REQUEST_METHOD was      #
    # not GET or POST, send an error.                                        #
    if ($ENV{'REQUEST_METHOD'} eq 'GET') {
        # Split the name-value pairs
        #print 'Methode: GET';
        #print $ENV{'QUERY_STRING'};
        @pairs = split(/&/, $ENV{'QUERY_STRING'});
    }
    elsif ($ENV{'REQUEST_METHOD'} eq 'POST') {
        # Get the input
        # print 'Methode: POST';
        #print 'Content length:', $ENV{'CONTENT_LENGTH'};
        read(STDIN, $buffer, $ENV{'CONTENT_LENGTH'});
        #print 'Buffer is:', $buffer, ' and nothing more!';
 
        # Split the name-value pairs
        @pairs = split(/&/, $buffer);
    }
    else {
        &error('request_method');
    }

    # For each name-value pair:                                              #
    foreach $pair (@pairs) {
        #print 'Splitting pairs:';
        # Split the pair up into individual variables.                       #
        local($name, $value) = split(/=/, $pair);
 
        # Decode the form encoding on the name and value variables.          #
        # v1.92: remove null bytes                                           #
        $name =~ tr/+/ /;
        $name =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
        $name =~ tr/\0//d;
        #print $name, "/";
        $value =~ tr/+/ /;
        $value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
        $value =~ tr/\0//d;
        #print $value, "\r\n";
        # If the field name has been specified in the %Config array, it will #
        # return a 1 for defined($Config{$name}}) and we should associate    #
        # this value with the appropriate configuration variable.  If this   #
        # is not a configuration form field, put it into the associative     #
        # array %Form, appending the value with a ', ' if there is already a #
        # value present.  We also save the order of the form fields in the   #
        # @Field_Order array so we can use this order for the generic sort.  #
        if (defined($Config{$name})) {$Config{$name} = $value;}
    }

}

sub parse_file {
    my $content;
    $filename = $Config{'table'};
    $password = $Config{'password'};
    $name = $Config{'name'};
    #print $filename, " / ", $password, " / ", $name, "\r\n";
    if (!($filename =~ /^[a-zA-Z0-9_\+-]+$/)) {
        print "Table name must consist of letters, numbers, -, + and _ only!\n"; 
        exit;
    }
    if (length($name) < 3) {
        print "Please specefy at least the first 3 chars of the name!\n"; 
        exit;
    }
    open($file, '<',"gotongi/${filename}") or &refuse;
    read($file, my $content, -s $file );
    close($file);
    #print $content;
    my @lines = split /\n/, $content;
    
    chomp($lines[0]);
    #chomp($password);
    #print $lines[0], $password;
    
    if (!($password eq $lines[0])) {&refuse;}
    # Check whether we have exactly one match
    
    my $matches = 0;
    foreach (1 .. $#lines) {
        if ( index($lines[$_], $name) == 0) { $matches++; }
    }
    if ($matches != 1) {&refuse;}
    
    open($file, '>',"gotongi/${filename}");
    print $file $password,"\n";
    foreach (1 .. $#lines) {
        if (index($lines[$_], $name) == 0) {
            my @charname = split('/',$lines[$_]);
            print $file $charname[0], "/ ", $ENV{'REMOTE_ADDR'}, "\n";
        }
        # Simply copy the line if we don't have a match
        else {
            print $file $lines[$_],"\n";
            print $lines[$_],"\n";
        }
    }
    
    close($file);
  
    
}

sub refuse
{
    print "No match found with specified group, name and password! Sorry, thou shalt not pass!\r\n";
    exit;
}
