#include <file-formats/psf.h>
#include <types.h>

#include <file-systems/file-system.h>
#include <memory/alloc.h>

PSFFont *LoadFont(const String path) {
    FILE *fp = fopen(path, "r");
    if(fp == NULL) return NULL;

    fseek(fp, 0, SEEK_END);
    u64 fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    PSFFont *font = malloc(sizeof(PSFFont));
    if(font == NULL) {
        fclose(fp);
        return NULL;
    }

    font -> header = malloc(sizeof(PSFHeader));
    if(font -> header == NULL) {
        free(font);
        fclose(fp);
        return NULL;
    }

    fread(font -> header, sizeof(PSFHeader), 1, fp);
    if(font -> header -> magic != PSF_MAGIC) {
        free(font -> header);
        free(font);
        fclose(fp);
        return NULL;
    }

    u64 glyphCount = (font -> header -> fontMode & PSF_MODE512) != 0 ? 512 : 256;
    u64 glyphBufferSize = font -> header -> charSize * glyphCount;

    font -> glyphBuffer = malloc(glyphBufferSize);
    if(font -> glyphBuffer == NULL) {
        free(font -> header);
        free(font);
        fclose(fp);
        return NULL;
    }

    fread(font -> glyphBuffer, glyphBufferSize, 1, fp);

    // no unicode table
    if(fileSize <= sizeof(PSFHeader) + glyphBufferSize) {
        fclose(fp);
        return font;
    }

    word *inFileTable = malloc(fileSize - sizeof(PSFHeader) - glyphBufferSize);
    if(inFileTable == NULL) {
        free(font -> glyphBuffer);
        free(font -> header);
        free(font);
        fclose(fp);
        return NULL;
    }

    font -> unicodeTable = malloc(2 * 0xFFFF);
    if(font -> unicodeTable == NULL) {
        free(inFileTable);
        free(font -> glyphBuffer);
        free(font -> header);
        free(font);
        fclose(fp);
        return NULL;
    }

    fread(inFileTable, fileSize - sizeof(PSFHeader) - glyphBufferSize, 1, fp);
    fclose(fp);

    u16 tblIdx = 0;
    u16 glyphIdx = 0;
    while(glyphIdx < glyphCount) {
        if(inFileTable[tblIdx] == 0xFFFF) {
            glyphIdx++;
            tblIdx++;
            continue;
        }

        font -> unicodeTable[inFileTable[tblIdx]] = glyphIdx;
        tblIdx++;
    }

    free(inFileTable);
    return font;
}
