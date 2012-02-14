#include "stdlib.h"

void * memcpy(void *dst, const void *src, size_t count)
{
    int i;
    char *d = (char*)dst;
    char *s = (char*)src;
    
    for (i=0;i<count;i++)
	d[i] = s[i];

    return (void*)src;
}

void * memset(void *p, int value, size_t count)
{
    int i;
    
    char *ptr = (char*)p;
    
    for (i=0;i<count;i++)
	ptr[i] = value;

    return p;
}

int tolower(int a)
{
    if (a>='A' && a<='Z')
	return a-'A'+'a';
    return a;
}

int strlen(const char *s)
{
    int i;

    for (i=0;;i++)
    {
	if (s[i] == 0)
	    return i;
    }
}

int strcmp(const char *a, const char *b)
{
    int i;

    for (i=0;;i++)
    {
	if (a[i] == 0 && b[i] == 0)
	    return 0;
	if (a[i] == 0)
	    return -1;
	if (b[i] == 0)
	    return 1;
	if (a[i]>b[i])
	    return 1;
	if (a[i]<b[i])
	    return -1;
    }
}

int strncasecmp(char *a, char *b, int n)
{
    int i;
    
    for (i=0;;i++)
    {
	char x = tolower(a[i]);
	char y = tolower(b[i]);
	
	if (x == 0 && y == 0)
	    return 0;
	if (x == 0)
	    return -1;
	if (y == 0)
	    return 1;
	if (x>y)
	    return 1;
	if (x<y)
	    return -1;
    }
}

char * strchr(const char *a, int b)
{
    int i = 0;
    for (i=0;;i++)
    {
	if (a[i]==0)
	    return NULL;
	if (a[i] == b)
	    return (char*)&a[i];
    }
}

