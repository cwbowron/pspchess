TARGET = pspChess
PSPSDK=$(shell psp-config --pspsdk-path)
MIKMOD = ../mikmod-3.0.3
CC = psp-gcc
LD = psp-gcc
STRIP = psp-strip
PACK_PBP = pack-pbp
CFLAGS = -O2 -Wall -I$(PSPSDK)/include -I$(MIKMOD)/include 

LINK_FLAGS = -L$(MIKMOD)/lib -L$(PSPSDK)/lib  \
	-lpng -lz -lm \
	-lmikmod \
	-lmmio \
	-lmad \
	-lpspaudiolib \
	-lpspaudio \
	-lpspdebug \
	-lpspdisplay \
	-lpspge \
	-lpspwlan \
	-lpspctrl \
	-lpspsdk \
	-lc \
	-lpspuser \
	-lpspkernel  \

PSP_DIR = G:/PSP/GAME/PSPCHESS

FTP = C:/WINDOWS/SYSTEM32/FTP.EXE
SFTP = "C:/PROGRAM FILES/PUTTY/PSFTP.EXE"

PKGNAME = PSPCHESS
HFILES = bce.h
CFILES = board.c main.c movegen.c domove.c user.c search.c \
	vars.c book.c checktest.c eval.c hash.c \
	osk.c \
	network.c \
	png_handler.c \
	pgn.c \
	file_select.c \
	string_select.c \
	random.c pg.c music.c \
	book_moves.c option_menu.c \
	title_image.c \
	mp3player.c \
	background_image.c \
	images/image_captured_pieces_bevel.c \
	images/image_lastmove.c \
	images/image_thinking.c \
	images/image_tomove.c \
	images/black_empty.c \
	images/white_empty.c \
	images/black_b-bishop.png.c \
	images/black_b-knight.png.c \
	images/black_b-rook.png.c \
	images/black_b-king.png.c \
	images/black_b-queen.png.c \
	images/black_b-pawn.png.c \
	images/black_w-bishop.png.c \
	images/black_w-knight.png.c \
	images/black_w-rook.png.c \
	images/black_w-king.png.c \
	images/black_w-queen.png.c \
	images/black_w-pawn.png.c \
	images/white_b-bishop.png.c \
	images/white_b-knight.png.c \
	images/white_b-rook.png.c \
	images/white_b-king.png.c \
	images/white_b-queen.png.c \
	images/white_b-pawn.png.c \
	images/white_w-bishop.png.c \
	images/white_w-knight.png.c \
	images/white_w-rook.png.c \
	images/white_w-king.png.c \
	images/white_w-queen.png.c \
	images/white_w-pawn.png.c \

OFILES = ${CFILES:.c=.o}
OBJS = $(OFILES)

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = pspChess
# include $(PSPSDK)/lib/build.mak

BIN_DIST_FILES = EBOOT.PBP PSPCHESS/EBOOT.PBP PSPCHESS%/EBOOT.PBP \
		 readme.txt changelog.txt pieces.png pieces2.png

default:
	@make eboot.pbp
	@make PSPCHESS/EBOOT.PBP
	@make PSPCHESS%/EBOOT.PBP

clean:
	rm *.o
	rm images/*.o
	rm eboot.pbp
	rm out

loadpsp: PSPCHESS/EBOOT.PBP
	cp PSPCHESS/EBOOT.PBP $(PSP_DIR)/EBOOT.PBP

pspchess_src.zip: $(CFILES)
	zip pspchess_src.zip * images/* -x \*.o freechess.txt -x out -x cvs 

pspchess.zip: $(BIN_DIST_FILES)
	zip pspchess.zip EBOOT.PBP $(BIN_DIST_FILES)

out:	$(OFILES)
	$(LD) $(CFLAGS) $(OFILES) $(LINK_FLAGS)  -o out 
	$(STRIP) out

PARAM.SFO:
	mksfo "pspChess" PARAM.SFO

eboot.pbp: out PARAM.SFO
	$(PACK_PBP) EBOOT.PBP PARAM.SFO ICON0.PNG NULL NULL PICT1.png NULL out NULL

PSPCHESS%/EBOOT.PBP: PARAM.SFO 
	$(PACK_PBP) PSPCHESS%/EBOOT.PBP PARAM.SFO ICON0.PNG NULL NULL PICT1.png NULL NULL NULL

PSPCHESS/EBOOT.PBP: out
	cp out PSPCHESS/EBOOT.PBP

.o:	$(HFILES)
	$(CC) $(CFLAGS) -c $<
