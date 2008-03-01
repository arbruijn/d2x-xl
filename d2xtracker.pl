# Author: Sebastian "VEX-Munk" Starosielec © 2005
# Contact: sebastian.starosielec@ruhr-uni-bochum.de
# License: GNU Public License 
#!/usr/bin/perl
# Server Program
use IO::Socket::INET;
use Data::Dumper;

# Config
my $TIMEOUT;
my $LISTENPORT;

$TIMEOUT = 30;
$LISTENPORT = 9424;

# Create a new socket
$InSocket = new IO::Socket::INET->new(LocalPort=>$LISTENPORT,Proto=>'udp');
print ">> Server Program <<\n";
%Tracker=();

# Keep receiving messages from client
while(1) {
	$InSocket->recv($Message,8);
	print "Incoming Message: ", $Message , "\n";
	if ($Message =~ /^S/) {					# incoming Message from Server: store Message in List
		print "Message from Server\n";
		$key = $InSocket->peeraddr() . chr(int($InSocket->peerport() / 256)) . chr($InSocket->peerport() % 256);
		$Tracker{$key} = time();
		print Dumper(\%Tracker);
	} elsif ($Message =~ /^R/) {	# incoming Message from Client: read Message from List
		print "Message from Client\n";

		# Updating Tracker list, delete old Tracker Entries
		while ($key = each(%Tracker)) {
			print "Differences ", $key, ": ", time() - $Tracker{$key}, "\n";
			if (time() - $Tracker{$key} > $TIMEOUT) { #Timeout ?
				delete $Tracker{$key};
			}
		}

		# Creating Response
		$Count = keys(%Tracker);
		print "Count: ", $Count, "\n";
		$Response = "S" . chr($Count);
		while ($key = each(%Tracker)) {
			$Response .= $key;
		}		
		$Response .= "\x00" x (512 - length($Response));
		
		print "Length Response: ", length($Response), "\n";
		$InSocket->send($Response);		
	}
}