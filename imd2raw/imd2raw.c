/*
 * Fixes to sort sector list so that images with skews are correct
 * Verified by actually comparing IMDU /B output byte for byte
 *
 * David Schmidt
 * December, 2022
 * July, 2026: optional padding of missing sectors
 */

/* originally from the bitsavers/convergent directory
 *
 * Used stdin/stdout that won't work in Windows.  Files
 * must be opened explicity in binary mode for both input and output
 *
 * Testing: Converts *.IMD file identical to IMDU /B as verified
 * by both size and an MD5 cryptographic hash.
 *
 * Tom Burnett
 * December, 2013
 */
#include "imd2raw.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/*
 * IMD image format
 *
 * IMD v.v: dd/mm/yyy hh:mm:ss (ascii header, optional)
 * comment terminated with 0x1a
 * for each track
 *  char mode (0-5)
 *  char cylinder
 *  char head (including additional optional bits, below)
 *  char sector count
 *  char sector size (0-6)
 *  sector numbering map
 *  optional cylinder map (head & 0x40)
 *  optional head map (head & 0x80)
 *  sector data records (type) (val, or data)
 */

char *modetbl[] = { "500K FM", "300K FM", "250K FM", "500K MFM", "300K MFM", "250K MFM" };

FILE *fpin, *fpout;
int fill_missing = 0;
unsigned int highwater[7] = {0};

int comp (const void * elem1, const void * elem2)
{
    unsigned char f = *((int*)elem1);
    unsigned char s = *((int*)elem2);
    if (f > s) return  1;
    if (f < s) return -1;
    return 0;
}

/* Integrated into GRiDISKCOM: this used to be main()/stdin/stdout that
 * called exit() on error. It is now imd2raw_convert(infile, outfile) which
 * returns a negative code on error, so it is safe to call from the GUI.
 * The conversion algorithm below is unchanged. */
int imd2raw_convert(const char *infile, const char *outfile)
{
    unsigned char sectormap[32];
    unsigned char sectormapsorted[32];
    unsigned char secdisp[32];
    unsigned char secdata[64][8192];
    unsigned int mode, cyl, hd, seccnt, headflags, sizecode;
    unsigned int secsiz = 0;
    int i, j;
    unsigned char c, value, fill;

    if ((fpin = fopen(infile, "rb")) == NULL) {
        return -1;
    }

    if ((fpout = fopen(outfile, "wb")) == NULL) {
        fclose(fpin);
        return -2;
    }

    /* Turns out this is a "later" construct, earlier examples may just have a 0x1a terminated comment
    if( (fgetc(fpin) != 'I') || (fgetc(fpin) != 'M') || (fgetc(fpin) != 'D') ) {
        fprintf(stderr,"File doesn't start with 'IMD'\n");
        leave(4);
    }
    */

    // Lop off initial comments
    while(1) {
        c = fgetc(fpin);
        if(c == 0x1a)
            break;
    }

    while(1) {
        c = fgetc(fpin);
        if(feof(fpin)) break;
        mode   = c;
        if(mode > 6) {
            fclose(fpin);
            fclose(fpout);
            return -4;
        }
        cyl    = fgetc(fpin);
        if(cyl > 80) {
            fclose(fpin);
            fclose(fpout);
            return -5;
        }
        c      = fgetc(fpin);
        hd = c & 0x0f;
        headflags = c & 0xf0;
        if(hd > 1) {
            fclose(fpin);
            fclose(fpout);
            return -6;
        }
        seccnt = fgetc(fpin);

        sizecode = fgetc(fpin);
        c = sizecode;

        switch(c) {
            case 0:
                secsiz = 128;
                break;
            case 1:
                secsiz = 256;
                break;
            case 2:
                secsiz = 512;
                break;
            case 3:
                secsiz = 1024;
                break;
            case 4:
                secsiz = 2048;
                break;
            case 5:
                secsiz = 4096;
                break;
            case 6:
                secsiz = 8192;
                break;
            default:
                break;
        }

        if (seccnt > highwater[sizecode])
            highwater[sizecode] = seccnt;

        // fprintf(stderr,"Cyl:%d Hd:%d %s %d sectors size %d\n", cyl, hd, modetbl[mode], seccnt, secsiz);

        // copy sector/interleave map
        for (i=0; i < seccnt; i++) {
            sectormap[i] = fgetc(fpin);
            sectormapsorted[i] = sectormap[i];
        }
        // Sort the sectors in case of skew
        // qsort(sectormapsorted, seccnt, 1, comp);
        // fprintf(stderr,"Tbl ");
        // for (i=0; i < seccnt; i++) fprintf(stderr,"%d ",sectormap[i]);
        // fprintf(stderr, "\n");
        // fprintf(stderr,"Srt ");
        // for (i=0; i < seccnt; i++) fprintf(stderr,"%d ",sectormapsorted[i]);
        // fprintf(stderr, "\n");

        if ((headflags & 64) == 64)
        {
          // Pull out "optional" cylinder map, discard
          for (int i = 0; i < seccnt; i++)
            c = fgetc(fpin);
        }
        if ((headflags & 128) == 128)
        {
          // Pull out "optional" head map, discard
          for (int i = 0; i < seccnt; i++)
            c = fgetc(fpin);
        }

        // copy sector information indexed by the sector number
        for (i=0; i < seccnt; i++) {
            c = fgetc(fpin);

            switch(c) {
                case 0:            // Sector data unavailable - could not be read
                case 5:            // Deleted address marks
                case 7:            // Bad sector
                    secdisp[i] = 'X';
                    fill = 0xE5;
                    for(j=0; j < secsiz; j++) {
                        if (c > 0)
                          fill = fgetc(fpin); // Grab whatever IMD wrote
                        secdata[sectormap[i]][j] = fill;
                    }
                    // fprintf(stderr,"Cyl %d Hd %d Sec %d bad, type %d\n",cyl,hd,sectormap[i],c);
                    break;

                case 1:            // normal data 'secsiz' bytes follow
                    secdisp[i] = '.';
                    for(j=0; j < secsiz; j++)
                        secdata[sectormap[i]][j] = fgetc(fpin);
                    break;

                case 3:            // data with 'deleted data' address mark
                    secdisp[i] = 'd';
                    for(j=0; j < secsiz; j++)
                        secdata[sectormap[i]][j] = fgetc(fpin);
                    break;

                case 2:            // compressed with value in next byte
                case 4:
                case 6:
                case 8:
                    secdisp[i] = 'C';
                    value = fgetc(fpin);
                    for(j=0; j < secsiz; j++)
                        secdata[sectormap[i]][j] = value;
                    // fprintf(stderr,"Cyl %d Hd %d Sec %d all %x\n",cyl,hd,sectormap[i],value);
                    break;
            }
        }

        if (fill_missing && seccnt < highwater[sizecode]) {
            unsigned char present[64] = {0};
            for (i = 0; i < seccnt; i++)
                present[sectormap[i]] = 1;
            for (i = 1; i <= highwater[sizecode]; i++) {
                if (!present[i]) {
                    for (j = 0; j < secsiz; j++)
                        secdata[i][j] = 0x00;
                    sectormap[seccnt] = i;
                    sectormapsorted[seccnt] = i;
                    secdisp[seccnt] = '0';
                    seccnt++;
                }
            }
        }
        qsort(sectormapsorted, seccnt, 1, comp);

        for(i=0; i<seccnt; i++) {
            for(j=0; j < secsiz; j++) {
                fputc(secdata[sectormapsorted[i]][j], fpout);
            }
        }
    }
    fclose(fpin);
    fclose(fpout);
    return(0);
}
