#include "utils.h"
#include <types.h>

u64 strlen(const String src) {
    u64 i = 0;
    while(src[i] != '\0') i++;
    return i;
}

String strcpy(String dst, const String src) {
    u64 i = 0;
    while(src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
    return dst;
}

String strcat(String dst, const String src) {
    u64 i = 0;
    while(dst[i] != '\0') i++;

    u64 j = 0;
    while(src[j] != '\0') {
        dst[i] = src[j];
        i++;
        j++;
    }

    dst[i] = '\0';
    return dst;
}

int strcmp(const String str1, const String str2) {
    u64 i = 0;
    while(str1[i] != '\0' && str2[i] != '\0') {
        if(str1[i] != str2[i]) return str1[i] - str2[i];
        i++;
    }

    return str1[i] - str2[i];
}

String strchr(const String src, char value) {
    u64 i = 0;
    while(src[i] != '\0') {
        if(src[i] == value) return (String) &src[i];
        i++;
    }

    return NULL;
}

int stricmp(const String str1, const String str2) {
    u64 i = 0;
    while(str1[i] != '\0' && str2[i] != '\0') {
        if(tolower(str1[i]) != tolower(str2[i]))
            return tolower(str1[i]) - tolower(str2[i]);

        i++;
    }

    return tolower(str1[i]) - tolower(str2[i]);
}


char toupper(char c) {
    if(c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

char tolower(char c) {
    if(c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}


boolean isupper(char c) {
    return c >= 'A' && c <= 'Z';
}

boolean islower(char c) {
    return c >= 'a' && c <= 'z';
}


u64 wcslen(const wString src) {
    u64 i = 0;
    while(src[i] != '\0') i++;
    return i;
}

wString wcscpy(wString dst, const wString src) {
    u64 i = 0;
    while(src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
    return dst;
}

wString wcscat(wString dst, const wString src) {
    u64 i = 0;
    while(dst[i] != '\0') i++;

    u64 j = 0;
    while(src[j] != '\0') {
        dst[i] = src[j];
        i++;
        j++;
    }

    dst[i] = '\0';
    return dst;
}

int wcscmp(const wString str1, const wString str2) {
    u64 i = 0;
    while(str1[i] != '\0' && str2[i] != '\0') {
        if(str1[i] != str2[i]) return str1[i] - str2[i];
        i++;
    }

    return str1[i] - str2[i];
}

wString wcschr(const wString src, char value) {
    u64 i = 0;
    while(src[i] != '\0') {
        if(src[i] == value) return (wString) &src[i];
        i++;
    }

    return NULL;
}

int wcsicmp(const wString str1, const wString str2) {
    u64 i = 0;
    while(str1[i] != '\0' && str2[i] != '\0') {
        if(towlower(str1[i]) != towlower(str2[i]))
            return towlower(str1[i]) - towlower(str2[i]);

        i++;
    }

    return towlower(str1[i]) - towlower(str2[i]);
}


wchar towupper(wchar c) {
    if(c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

wchar towlower(wchar c) {
    if(c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}


boolean iswupper(wchar c) {
    return c >= 'A' && c <= 'Z';
}

boolean iswlower(wchar c) {
    return c >= 'a' && c <= 'z';
}
