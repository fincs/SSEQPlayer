SSEQ Player
===========

Introduction
------------

This is a homebrew player of Nintendo's SSEQ sequence format, used in commercial DS games.

How to build
------------

1. Place a sseq file, a sbnk file and a swar file in the fs folder
2. Open arm9/source/template.c and edit the parameters to the PlaySeq call accordingly.
3. Compile by using make.

To do
-----

- Note dropping bug appears to be gone... I hope.
- Some SSEQ commands are not implemented yet (modulation, portamento, pitch sweep): please help!

Thanks to:
----------

- kiwids, for making the SDAT spec and for providing valuable information regarding volume mixing.