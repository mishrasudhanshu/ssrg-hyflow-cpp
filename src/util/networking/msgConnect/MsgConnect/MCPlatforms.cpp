//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCPlatforms.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


// ----------------------------- unicode support ------------------------------

int wstrlen(wchar_t* s)
{
	int r = 0;
	if(!s)
		return(0);
	while(*s++)
		r++;
	return(r);
}

wchar_t* wstrcat(wchar_t* dest, wchar_t* src)
{
	if(!src || !dest)
		return(dest);
  wchar_t* td = dest + wstrlen(dest);
	while(*src)
		*td++ = *src++;
	*td = '\0';
	return(dest);
}

long wstrtol(wchar_t* s, wchar_t** e, int r)
{
	char* s1 = wstr2str(s, CP_ACP);
	char* e1;
	long l = strtol(s1, &e1, r);
	if(*e1)
		*e = s + (e1 - s1);
	else
	  *e = NULL;
	return(l);
}

double wstrtod(wchar_t* s, wchar_t** e)
{
	char* s1 = wstr2str(s, CP_ACP);
	char* e1;
	double d = strtod(s1, &e1);
	if(*e1)
		*e = s + (e1 - s1);
	else
	  *e = NULL;
	return(d);
}

wchar_t* wstrdup(wchar_t* s)
{
	long i = (wstrlen(s)+1)*sizeof(wchar_t);
	wchar_t* r = (wchar_t*)malloc(i);
	memmove(r, s, i);
	return(r);
}

wchar_t* str2wstr(char* s, unsigned int cp)
{
#ifdef _WIN32
#else
	wchar_t* w = (wchar_t*)malloc((strlen(s)+1)*sizeof(wchar_t));
	wchar_t* r = w;
	while(*s)
		*(w++) = (wchar_t)*(s++);
	*w = *s;
	return(r);
#endif
}

char* wstr2str(wchar_t* w, unsigned int cp)
{
#ifdef _WIN32
#else
	char* s = (char*)malloc(wstrlen(w)+1);
	char* r = s;
	while(*w)
		*(s++) = (char)*(w++);
	*s = *w;
	return(r);
#endif
}

// ----------------------------------------------------------------------------

double TurnDouble(double D)
{
	double t = D;
	char* curP = (char*)&D;
	char* curD = (char*)&t;
	curD += sizeof(D)-1;

	for(long l=0; l < (signed long)sizeof(D); l++)
	{	
	    *curD = *curP;
	    curP++;
	    curD--;
	}	
	return t;
}

mcInt32 TurnLong(mcInt32 D)
{
  mcInt32 T = D;
  char* curP = (char*)&D;
  char* curD = (char*)&T;

  curD += sizeof(D)-1;

  for(mcInt32 l=0; (mcUInt32)l < sizeof(D); l++)
  {
    *curD = *curP;
    curP++;
    curD--;
  }
  return(T);
}

long UpperTrimCmp(char* S1, char* S2)
{
	char C1, C2, *E1, *E2;

	while(*S1 && isspace(*S1))
		S1++;
	E1 = S1;
	while(*E1)
		E1++;
	if (S1 != E1)
	{
		E1--;
		while(isspace(*E1))
			E1--;
		E1++;
	}

	while(*S2 && isspace(*S2))
		S2++;
	E2 = S2;
	while(*E2)
		E2++;
	if (S2 != E2)
	{
		E2--;
		while(isspace(*E2))
			E2--;
		E2++;
	}


	while (true)
	{
		C1 = *S1;
		if (S1 == E1)
			C1 = 0;
		else if ((C1 >= 'a') && (C1 <= 'z'))
			C1 &= (~0x20);
		S1++;

		C2 = *S2;
		if (S2 == E2)
			C2 = 0;
		else if ((C2 >= 'a') && (C2 <= 'z'))
			C2 &= (~0x20);
		S2++;

		if (C1 > C2)
			return  +1;
		else if (C1 < C2)
			return  -1;
		else if (C1 == 0)
			return 0;
	}
	return 0; 
	
/*
	unsigned char* S = (unsigned char*)S1;
	char* p = (char*)malloc(strlen(S1) + 1);
	long x = 0;
	while(*S) 
	{
		while(*S && isspace(*S)) 
			S++;
		while(*S && !isspace(*S))
		  if(*S >= 'a' || *S <= 'z')
				p[x++] = *S++ & (~0x20);
			else
				p[x++] = *S++;
	}
	p[x] = 0;
	return(p);
*/
}

LPSTR CharNstr(char C, char* S)
{
	return (CharNstr(C, S, S));
}

LPSTR CharNstr(char C, char* S, char* D)
{
	if(!S) 
		return(Char2Str(C, S));
	size_t l = strlen(S) + 2;
	char* x=(char*)malloc(l);

#ifdef _WIN32
#else
	*x = C;
	strcpy(&x[1], S);
#endif

	if(D) 
		free(D);
  return(x);
}

LPSTR StrNchar(char* S, char C)
{
	return(StrNchar(S, C, S));
}

LPSTR StrNchar(char* S, char C, char* D)
{
	if(NULL == S) 
		return(Char2Str(C, S));
	size_t l = strlen(S) + 2;
	char* x= (char*)malloc(l);
#ifdef _WIN32
#else
	strcpy(x, S);
	x[strlen(x) + 1] = 0;
	x[strlen(x)] = C;
#endif
	if(D) 
		free(D);
	return(x);
}

LPSTR Char2Str(char C, char* S)
{
  if(S) 
	  free(S);

  char* z = (char*)malloc(2);
  z[0] = C;
  z[1] = 0;
  return(z);
}

LPSTR AddStr(char* S1,char* S2)
{
  if(NULL == S1) 
	  return(strdup(S2));
  if(NULL == S2) 
	  return(S1);
  size_t zl = strlen(S1) + strlen(S2) + 1;
  char* t = (char*)malloc(zl);
  strcpy(t, S1);
  strcat(t, S2);
  free(S1);
  return(t);
}

#ifdef USE_NAMESPACE
}
#endif

