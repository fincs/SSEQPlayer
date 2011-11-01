SSEQ Player
===========

Introduction
------------

This is a homebrew player of Nintendo's SSEQ sequence format, used in commercial DS games.

How to build
------------

1. Optionally place a sseq file, a sbnk file and swar files in the fs folder under the names
   YourFile.sseq, YourFile.sbnk, YourFile.swar and YourFile2..4.swar.
3. Compile by using make.

Running the SSEQ player
-----------------------

If the .nds file is run with no parameters, it attempts to play the hardcoded files inside its NitroFS. Alternatively you can pass the paths to the sseq file, the sbnk file and the swar files (in that order) through the ARGV mechanism. In order to do so, use HomebrewMenu .argv files.

To do
-----

- Note dropping bug appears to be gone... I hope.
- Some SSEQ commands are not implemented yet (portamento, pitch sweep): please help!

Thanks to:
----------

- kiwids, for making the SDAT spec and for providing valuable information regarding volume mixing.