#include <pspkernel.h>
#include <pspiofilemgr_fcntl.h>
#include <string.h>
#include <pspaudio.h>
#include <pspaudiolib.h>

#include "mikmod.h"
#include "mp3player.h"
#include "bce.h"
#include "colors.h"

#define BACKGROUND_MUSIC_FILE	"ms0:/PSP/GAME/PSPCHESS/BACKGROUND.MOD"
#define MUSIC_DIR "ms0:/PSP/MUSIC"

#define MAX_ENTRY 512

UNIMOD *g_mf;
extern int _mm_errno;
extern BOOL _mm_critical;
extern char *_mm_errmsg[];
int __errno=0;

int g_music_on = 1;

#define O_RDONLY    0x0001

char g_background_music_file[256] = BACKGROUND_MUSIC_FILE;

enum { MUSIC_NONE, MUSIC_MP3, MUSIC_MIKMOD } g_music_type;


void shutdown_music();
void startup_music();
void shutdown_mp3(int hardcore);
void startup_mp3(int hardcore);

void get_random_song()
{
    char *extensions[] = {"mp3", 0};
    int n_extensions = 1;
    
    struct SceIoDirent files[MAX_ENTRY];   
    memset(files, 0, sizeof(files));
    int n_files =
	read_directory_contents(files, MAX_ENTRY,
				MUSIC_DIR, extensions, n_extensions, 0);

    int n = getrandomnumber() % n_files;
    strcpy(g_background_music_file, MUSIC_DIR);
    strcat(g_background_music_file, "/");
    strcat(g_background_music_file, files[n].d_name);
}

int music_type(char *file_path)
{
    if (strstr(g_background_music_file,"MP3") ||
	strstr(g_background_music_file,"mp3"))
	return MUSIC_MP3;
    else
	return MUSIC_MIKMOD;
}

int file_exists(char *file_path)
{
    FILE *f = fopen(file_path, "r");

    if (f)
    {
	fclose(f);
	return 1;
    }
    return 0;
}

void restart_music_if_necessary()
{
    static char last_song[256] = "";

    if (g_music_on == 0)
    {
	shutdown_music();
    }
    if (strcmp(last_song,"") == 0)
    {
	strcpy(last_song, g_background_music_file);
    }
    else if (strcmp(last_song, g_background_music_file) != 0)
    {
	int new_type = music_type(g_background_music_file);
	
	if (g_music_type == new_type)
	{
	    switch (new_type)
	    {
		case MUSIC_MP3:
		    shutdown_mp3(0);
		    startup_mp3(0);
		    break;
		case MUSIC_MIKMOD:
		    Player_Stop();
		    MikMod_FreeSong(g_mf);
		    g_mf = MikMod_LoadSong(g_background_music_file, 255);
		    
		    if (g_mf)
		    {
			Player_Start(g_mf);
		    }
		    break;
	    }
	}
	else
	{
	    shutdown_music();
	    user_message_wait_key("Resetting Music System");
	    startup_music();
	}
	strcpy(last_song, g_background_music_file);
    }
    else
    {
	switch (g_music_type)
	{
	    case MUSIC_NONE:
		// do nothing
		break;
	    case MUSIC_MP3:
		if (g_music_on == MUSIC_OPTION_RANDOM)
		{
		    if (MP3_EndOfStream())
		    {
			shutdown_mp3(0);
			get_random_song();
			strcpy(last_song, g_background_music_file);
			startup_mp3(0);
		    }
		}
		break;
	    case MUSIC_MIKMOD:
		if (g_mf && !Player_Active())
		{
		    Player_Stop();
		    MikMod_FreeSong(g_mf);
		    g_mf = MikMod_LoadSong(g_background_music_file, 255);
		
		    if (g_mf)
		    {
			Player_Start(g_mf);
		    }
		}
		break;
	}
    }
}

void startup_mikmod()
{
    int maxchan = 255;

    MikMod_RegisterAllLoaders();
    MikMod_RegisterAllDrivers();
    
    md_mode = DMODE_16BITS|DMODE_STEREO|DMODE_SOFT_SNDFX|DMODE_SOFT_MUSIC; 

    MikMod_Init();
    
    g_mf = MikMod_LoadSong(g_background_music_file, maxchan);
    
    if (g_mf)
    {
	Player_Start(g_mf);
    }
}


void startup_mp3(int hardcore)
{
    if (hardcore)
    {
	pspAudioInit();
    }

    MP3_Init(0);
    MP3_Load(g_background_music_file);
    MP3_Play();
}

void startup_music()
{
    if (g_music_on == MUSIC_OPTION_RANDOM)
    {
	g_music_type = MUSIC_MP3;

	get_random_song();
	startup_mp3(1);
    }
    else if (g_music_on == MUSIC_OPTION_FILE)
    {
	if (!file_exists(g_background_music_file))
	{
	    g_music_on = 0;
	    g_music_type = MUSIC_NONE;
	}
	else
	{
	    g_music_type = music_type(g_background_music_file);

	    switch (g_music_type)
	    {
		case MUSIC_NONE:
		    // do nothing
		    break;
		case MUSIC_MIKMOD:
		    startup_mikmod();
		    break;
		case MUSIC_MP3:
		    startup_mp3(1);
		    break;
	    }
	}
    }
    else
    {
	g_music_type = MUSIC_NONE;
    }
}

void shutdown_mikmod()
{
    if (g_mf)
    {
	Player_Stop();
	MikMod_FreeSong(g_mf);
    }
    MikMod_Exit();
}

void shutdown_mp3(int hardcore)
{
    MP3_Stop();
    MP3_End();

    if (hardcore)
    {
	pspAudioEndPre();
	pspAudioEnd();
    }
}

void shutdown_music()
{
    switch (g_music_type)
    {
	case MUSIC_NONE:
	    // do nothing
	    break;
	case MUSIC_MIKMOD:
	    shutdown_mikmod();
	    break;
	case MUSIC_MP3:
	    shutdown_mp3(1);
	    break;
    }
}
