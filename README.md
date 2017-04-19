# pspChess

Copyright 2005 by Christopher Bowron - cwbowron@gmail.com

http://www.bowron.us

Based on [Boo's Chess Engine](https://github.com/cwbowron/BCE)

## Dependancies, Credits, Thanks, Etc:

Uses code from Skippy's psp libraries, pspsdk, mikmod, libmad, libpng and PSPMediaCenter. Thanks to everyone involved in those projects.

Images were designed by Stadi Thompson.  Extra big thanks to him for contacting me and volunteering to make them.

## Pieces

You can get more images for pieces [here](http://www.dcemu.co.uk/vbulletin/showthread.php?t=8514). If you create your own sets, please share them there.

Feature Requests and Bug Reports: http://forums.ps2dev.org/viewtopic.php?t=1760

## User Interface

Select piece to move using arrows and then press x.  Select destination square using arrows then press x.

Default setup is WHITE: human, BLACK: psp

* RTRIGGER	
  * Make computer move (normal mode)
  * Perform next move in analysis stack (analysis mode)
  * Show terminal screen (client mode)
* LTRIGGER - back up a ply
* CIRCLE - cancel piece selection
* TRIANGLE - set to human vs human
* CROSS	- select piece / select target square
* SELECT - option menu
* START
  * reset game (normal/analysis)
  * show terminal screen (client mode)
* SQUARE+LTRIGGER - show text from pgn after import / return to game from pgn text

EXTRA KEYS: (can be turned off)
* SQUARE+UP - increase searchdepth
* SQUARE+DOWN - decrease searchdepth
* SQUARE+RIGHT - toggle show thoughts
* SQUARE+LEFT - screenshot

During computer searches, TRIANGLE will stop the search and reset white and black to human.

## Rating 

The original PC version of BCE (on which pspChess is based) played as "ddlchess" and "BACE" on freechess.org, but they have not been active in years. BCE achieved its highest blitz rating at 2054 and its highest standard rating at 1975. It was probably searching around 4-6 plies. It also probably lost a few games due to bugs that have been fixed in pspChess.

## Background Music

pspChess can play any any mikmod compatible file or mp3 as background music. 

## Known Errors

1. Cannot change background music mp3->mod->mp3 or mod->mp3->mod

## Installation 

If you have questions about how to run homebrew games, DO NOT EMAIL or INSTANT MESSAGE ME.  try [here](http://wiki.pspdev.org/psp:exploit_faq)

pspchess.zip contains PSPCHESS%/EBOOT.PBP and PSPCHESS/EBOOT.PBP which are the equivalent of the outputs of KXploit.  

If you have suggestions or bug reports, feel free to email me at chess@bowron.us 

pspChess should be installed under /PSP/GAME/PSPCHESS or things may break.  

FOR THE LOVE OF GOD, DO NOT CALL ME IF YOU CANNOT FIGURE OUT HOW TO INSTALL THE PROGRAM.  SEARCH THE WEB ON PSP HOMEBREW.  READ THE
README.TXT.  UNDER NO CIRCUMSTANCES SHOULD YOU THINK, HEY I SHOULD JUST CALL UP CHRIS AND ASK HIM. NO. NO. NO.

## Future Goals

1. I would eventually like to add features to allow pspChess to act as an interface to the Free Internet Chess Server (www.freechess.org) using the wi-fi capabilities of the psp.
2. I would also like to add 2-player via wifi capabilities so that two users can play each other, each having their own psp.
	
## License 

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

### pspsdk License

This program uses the pspsdk which has the following copyright restrictions

* Copyright (c) 2005  adresd
* Copyright (c) 2005  Marcus R. Brown
* Copyright (c) 2005  James Forshaw
* Copyright (c) 2005  John Kelley

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. The names of the authors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.

IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### PSPMediaCenter Info (mp3player.c/mp3player.h)

PSPMediaCenter by John_K & adresd
