#include <pspkernel.h>
#include <pspctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pspiofilemgr_fcntl.h>

#include "pg.h"
#include "colors.h"
#include "bce.h"

#define MAX_ENTRY 512

#define ST_MODE_DIR 0x11ff

int is_directory(SceIoDirent *f)
{
    return (f->d_stat.st_mode & 0x1000);
    //return (f->d_stat.st_mode == ST_MODE_DIR);
}

int file_compare(const void *v1, const void *v2)
{
    SceIoDirent *f1 = (SceIoDirent*) v1;
    SceIoDirent *f2 = (SceIoDirent*) v2;

    int is_dir_1 = is_directory(f1);
    int is_dir_2 = is_directory(f2);
    
    if (is_dir_1)
    {
	if (is_dir_2)
	    return strcmp(f1->d_name,f2->d_name);
	return -1;
    }
    if (is_dir_2)
	return 1;
    return strcmp(f1->d_name,f2->d_name);
}


// return 1 if a matching extension is found
int test_extensions(char *filename, char **extensions, int n_ext)
{
    int j;

    char *file_extension = strrchr(filename,'.');

    if (file_extension)
    {
	// skip .
	file_extension++;
	
	for (j=0;j<n_ext;j++)
	{
	    if (!extensions[j])
		break;
	    
	    if (stricmp(file_extension,extensions[j]) == 0)
		return 1;
	}
    }
    
    return 0;
}


// fill files[] with files in path that end with extension
int read_directory_contents(SceIoDirent *files, int max_files,
			    char *path, char **extensions, int n_ext,
			    int include_directories)
{
    int nfiles = 0;
    int fd = sceIoDopen(path);
    //int y = 2;

    //pgFillvram(0);

    //msxPutString(0,0,cyan,"Reading");
    //msxPutString(10,0,cyan,path);
    while(nfiles<max_files)
    {
	if (sceIoDread(fd, &files[nfiles])<=0)
	    break;

	//msxPutString(4,y,red,files[nfiles].d_name);
	//pgPrintInt(0,y,red,nfiles);
	//pgPrintInt(50,y,red,files[nfiles].d_stat.st_mode);
	//y++;
	
	// skip . entry
	if (strcmp(files[nfiles].d_name,".")==0)
	{
	}
	// add / to directory entries
	else if (include_directories && is_directory(&files[nfiles]))
	{
	    strcat(files[nfiles].d_name, "/");
	    nfiles++;
	}
	// add entries with the extensions
	else 
	{
	    if (test_extensions(files[nfiles].d_name, extensions, n_ext))
	    {
		nfiles++;
	    }
	}
    }
    sceIoDclose(fd);

    qsort(files,nfiles,sizeof(files[0]),file_compare);
    
    //pgScreenFlipV();

    //while (0 == Read_Key());
    
    return nfiles;
}


void draw_directory_entries(SceIoDirent *files, int nfiles,
			    int x, int y, int display_start_index,
			    int n_display, int selection)
{
    int i;

    for (i=display_start_index;i<display_start_index+n_display;i++)
    {
	if (i>=nfiles) break;
	
	if (i == selection)
	{
	    msxPutString(x,y,yellow,"->");
	    msxPutString(x+2,y,yellow,files[i].d_name);
	}
	else
	{
	    msxPutString(x+2,y,cyan,files[i].d_name);
	}
	y++;
    }

    pgScreenFlipV();
}

// return -2 if circle pressed
// return -1 if triangle pressed
// return index if cross pressed
int navigate_directory_entries(SceIoDirent *files, int n_files, char* path)
{
    int cursor = 0;
    int display_start = 0;
    int n_display = 20;

    while (1)
    {
	if (cursor>=display_start+n_display)
	    display_start = cursor-n_display+1;
	if (cursor<display_start)
	    display_start = cursor;

	int x = 10;
	int y = 5;
	
	clear_screen();
	clear_text_area(x,y,60-2*x,n_display+4);
	msxPutString(x,y,magenta,path);
	y+=2;
	
	pgPrintCenter(29,red,"X = Select");
	pgPrintCenter(30,red,"O = Abort");
	pgPrintCenter(31,red,"TRIANGLE = up directory");
	
	draw_directory_entries(files, n_files, x, 7,
			       display_start, n_display, cursor);
	
	int key = get_buttons();

	if (key & PSP_CTRL_DOWN)
	    cursor ++;
	if (key & PSP_CTRL_UP)
	    cursor --;
	if (key & PSP_CTRL_RIGHT)
	    cursor += n_display;
	if (key & PSP_CTRL_LEFT)
	    cursor -= n_display;
	if (key & PSP_CTRL_TRIANGLE)
	    return -1;
	if (key & PSP_CTRL_CIRCLE)
	    return -2;
	if (key & PSP_CTRL_CROSS)
	    return cursor;
	
	if (cursor>=n_files)
	    cursor = n_files-1;
	if (cursor<0)
	    cursor = 0;
    }
}

// return 1 if user selects a file, 0 if they abort
int user_select_file(char *start_dir, char **extensions,
		     int n_extensions, char *out)
{
    char path[256];
    struct SceIoDirent files[MAX_ENTRY];
    strcpy(path,start_dir);

    while (1)
    {
	memset(files, 0, sizeof(files));
	int n_files =
	    read_directory_contents(files, MAX_ENTRY,
				    path, extensions, n_extensions, 1);

	int r = navigate_directory_entries(files, n_files, path);

	int upOne = 0;
    
	// abort
	if (r==-2)
	{
	    return 0;
	}
	// up a directory
	else if (r==-1)
	{
	    upOne = 1;
	}
	// into a directory
	else if (is_directory(&files[r]))
	{
	    if (strstr(files[r].d_name,"../"))
		upOne = 1;
	    else
	    {
		strcat(path,"/");
		strcat(path,files[r].d_name);

		char *ptr = strrchr(path,'/');

		if (ptr)
		{
		    *ptr = 0;
		}
	    }
	}
	// file
	else 
	{
	    strcat(path,"/");
	    strcat(path,files[r].d_name);
	    strcpy(out,path);
	    return 1;
	}

	if (upOne)
	{
	    //int len = strlen(path);
	    //pgFillvram(0);

	    char *ptr = strrchr(path,'/');

	    if (ptr)
	    {
		//pgPrintInt(0,1,red,ptr-path);
		*ptr = 0;
	    }

	    //msxPutString(0,0,red,path);
	    //pgScreenFlipV();
	    //while (0==Read_Key());
	}
    }
}
