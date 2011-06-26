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

- There are weird problems with ADSR: the decay and release rates have to be multiplied by four for an unknown reason.
- For some reason, sometimes there aren't enough free channels and notes are dropped, whereas in the official SSEQ player this doesn't happen for the files I've tested.
- Some SSEQ commands are not implemented yet (modulation, portamento, pitch sweep): please help!