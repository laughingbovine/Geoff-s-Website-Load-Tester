#!/opt/local/bin/perl

use warnings;
use strict;

use YAML;
use Data::Dumper;
use File::MimeInfo::Magic;

my %filters = (
    multipartform   => sub {
        my $fields = Load(${$_[0]});

        my @sections;

        foreach my $field (@$fields)
        {
            my $section = {head => ''};

            if ($field->{name} and $field->{value})
            {
                $section->{head}    .= qq(Content-Disposition: form-data; name="$field->{name}"\n);
                $section->{body}    .= $field->{value};

                # make sure every newline is a \r\n
                $section->{head} =~ s/\r*\n\r*/\r\n/g;
                $section->{body} =~ s/\r*\n\r*/\r\n/g;
                # append \n if not already there
                $section->{body} =~ s/[\r\n]*$/\r\n/;
            }
            elsif ($field->{name} and $field->{filename} and $field->{file})
            {
                -f $field->{file} or die "could not find file '$field->{file}'";
                #open(my $fh, '<', $field->{file}) or die "could not open file '$field->{file}'";
                open(my $fh, '<:raw', $field->{file}) or die "could not open file '$field->{file}'";
                #open(my $fh, '<:bytes', $field->{file}) or die "could not open file '$field->{file}'";
                #binmode($fh);

                $section->{head} .= qq(Content-Disposition: form-data; name="$field->{name}"; filename="$field->{filename}"\n);
                $section->{head} .= qq(Content-Type: ).mimetype($fh).qq(\n);
                #$section->{head} .= qq(Content-Transfer-Encoding: base64\n);
                #$section->{head} .= qq(Content-Transfer-Encoding: binary\n);

                my ($buffer, $read_count);
                seek($fh, 0, 0); # mimetype() (above) moves the pointer thingie
                while ($read_count = read($fh, $buffer, 4096))
                {
                    $section->{body} .= $buffer;
                    #$section->{body} .= MIME::Base64::encode_base64url($buffer);
                    #$section->{body} .= $buffer;
                }

                die "could not read from file '$field->{file}': $!" unless defined $read_count;
                close($fh) or die "could not close file '$field->{file}': $? - $!";

                # make sure every newline is a \r\n (but not in the body)
                $section->{head} =~ s/\r*\n\r*/\r\n/g;
                # append \r\n if not already there
                $section->{body} =~ s/[\r\n]*$/\r\n/;
            }

            push @sections, $section;
        }

        # find a good boundary
        my $boundary;
        my $boundary_seen = 1;
        while ($boundary_seen)
        {
            # come up with a boundary
            $boundary = '-' x (int(rand(20)) + 10) . int(rand(9999999)) . int(rand(9999999)) . int(rand(9999999)) . int(rand(9999999));

            # check to see if this boundary occurs in any of our data
            $boundary_seen = 0;
            foreach (@sections)
            {
                if ($_->{head} =~ /$boundary/ or $_->{body} =~ /$boundary/)
                {
                    $boundary_seen = 1;
                    last;
                }
            }
        }

        # come up with our form body
        my $body = '';
        foreach (@sections)
        {
            $body .= "--$boundary\r\n";
            $body .= $_->{head};
            $body .= "\r\n";
            $body .= $_->{body};
        }
        $body .= "--$boundary--\r\n";

        # good to go
        print "Content-Type: multipart/form-data; boundary=$boundary\n";
        print "Content-Length: ".length($body)."\n";
        print "\n";
        print $body;
    },
);

while (!eof and $_ = <>)
{
    if (/^#\s*(\w+)\s*$/)
    {
        my $current_filter_name = $1;
        my $current_filter_text;

        while (!eof and $_ = <>)
        {
            if (/^#\s*${current_filter_name}_end\s*$/)
            {
                if ($filters{$current_filter_name})
                {
                    $filters{$current_filter_name}->(\$current_filter_text);
                }
            }
            else
            {
                $current_filter_text .= $_;
            }
        }
    }
    else
    {
        print $_;
    }
}
