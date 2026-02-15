#ifndef HH_STRING_UTILS
#define HH_STRING_UTILS

#include <types.h>

u64    strlen(const String src);
String strcpy(String dst, const String src);
String strcat(String dst, const String src);
int    strcmp(const String str1, const String str2);
String strchr(const String src, char value);
int    stricmp(const String str1, const String str2);

char toupper(char src);
char tolower(char src);
boolean isupper(char c);
boolean islower(char c);

u64     wcslen(const wString src);
wString wcscpy(wString dst, const wString src);
wString wcscat(wString dst, const wString src);
int     wcscmp(const wString str1, const wString str2);
wString wcschr(const wString src, char value);
int     wcsicmp(const wString str1, const wString str2);

wchar towupper(wchar src);
wchar towlower(wchar src);
boolean iswupper(wchar c);
boolean iswlower(wchar c);

#endif