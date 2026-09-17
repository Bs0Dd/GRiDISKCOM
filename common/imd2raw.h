#ifndef IMD2RAW_H
#define IMD2RAW_H

/*
 * IMD -> raw image converter.
 *
 * Derived from the imd2raw utility by Tom Burnett / David Schmidt
 * (originally from the bitsavers/convergent directory). See imd2raw.c
 * for the original copyright/attribution notices.
 *
 * Exposes imd2raw_convert(): converts an ImageDisk (.IMD) file into a
 * raw sector image (.img/.dsk), byte-for-byte identical to IMDU /B.
 *
 * Return value:
 *   0  - success
 *   <0 - error (input/output file problems or malformed IMD stream)
 */

#ifdef __cplusplus
extern "C" {
#endif

int imd2raw_convert(const char* infile, const char* outfile);

#ifdef __cplusplus
}
#endif

#endif /* IMD2RAW_H */
