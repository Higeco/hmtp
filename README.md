# hmtp
MTP file transfer command line utility

# Description
This project started as a way to send and receive files between Android/Microsoft smartphones and a linux embedded device through USB cables.
I used [libmtp] (http://libmtp.sourceforge.net/).
We found this was not what we needed so we decided to opensource our work, it could be a helpful example of what can be done.
Actually this program let you send/receive files and list directory content.
Quite simple at the moment.

## Status of the art
As active development on this project has been suspended, this is far away a "production" ready state of the art.
The program suffer some small memory-leaks that should be investigated. However it works and is a fun example on libmtp usage.

## Trees, why?
I used a quite complex tree structure to maintain MTP filesystem status, you should ask:
> Why did you do this painful thing?

Well, even if actually there's no need to maintain information about MTP fs, the original design was to use it like a daemon that should cache directories content in order to speed-up file searching and downloading.
