/*
  ImageMagick version and copyright.
*/
#ifndef _MAGICK_VERSION_H
#define _MAGICK_VERSION_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
  Define declarations.
*/
#define MagickPackageName "ImageMagick"
#define MagickCopyright  "Copyright (C) 1999-2005 ImageMagick Studio LLC"
#define MagickLibVersion  0x624
#define MagickLibVersionText  "6.2.4"
#define MagickLibVersionNumber  6,2,4,5
#if (QuantumDepth == 8)
#define MagickQuantumDepth  "Q8"
#define MagickQuantumRange  "255"
#elif (QuantumDepth == 16)
#define MagickQuantumDepth  "Q16"
#define MagickQuantumRange  "65535"
#elif (QuantumDepth == 32)
#define MagickQuantumDepth  "Q32"
#define MagickQuantumRange  "4294967295"
#elif (QuantumDepth == 32)
#define MagickQuantumDepth  "Q64"
#define MagickQuantumRange  "18446744073709551615"
#else
#define MagickQuantumDepth  "Q?"
#define MagickQuantumRange  "?"
#endif
#define MagickReleaseDate  "04/25/06"
#define MagickVersion MagickPackageName " " MagickLibVersionText " " \
  MagickReleaseDate " " MagickQuantumDepth " " MagickHomeURL
#define MagickHomeURL  "http://www.imagemagick.org"

extern MagickExport char
  *GetMagickHomeURL(void);

extern MagickExport const char
  *GetMagickCopyright(void),
  *GetMagickPackageName(void),
  *GetMagickQuantumDepth(unsigned long *),
  *GetMagickQuantumRange(unsigned long *),
  *GetMagickReleaseDate(void),
  *GetMagickVersion(unsigned long *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
