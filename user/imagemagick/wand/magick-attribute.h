/*
  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Set or Get Magick Wand Attributes.
*/

#ifndef _WAND_MAGICK_ATTRIBUTE_H
#define _WAND_MAGICK_ATTRIBUTE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern WandExport char
  *MagickGetException(MagickWand *,ExceptionType *),
  *MagickGetFilename(const MagickWand *),
  *MagickGetFormat(MagickWand *),
  *MagickGetHomeURL(void),
  *MagickGetOption(MagickWand *,const char *),
  *MagickQueryConfigureOption(const char *),
  **MagickQueryConfigureOptions(const char *,unsigned long *),
  **MagickQueryFonts(const char *,unsigned long *),
  **MagickQueryFormats(const char *,unsigned long *);

extern WandExport CompressionType
  MagickGetCompression(MagickWand *);

extern WandExport const char
  *MagickGetCopyright(void),
  *MagickGetPackageName(void),
  *MagickGetQuantumDepth(unsigned long *),
  *MagickGetQuantumRange(unsigned long *),
  *MagickGetReleaseDate(void),
  *MagickGetVersion(unsigned long *);

extern WandExport double
  *MagickGetSamplingFactors(MagickWand *,unsigned long *),
  *MagickQueryFontMetrics(MagickWand *,const DrawingWand *,const char *),
  *MagickQueryMultilineFontMetrics(MagickWand *,const DrawingWand *,
    const char *);

extern WandExport InterlaceType
  MagickGetInterlaceScheme(MagickWand *);

extern WandExport MagickBooleanType
  MagickGetPage(MagickWand *,unsigned long *,unsigned long *,long *,long *),
  MagickGetSize(const MagickWand *,unsigned long *,unsigned long *),
  MagickSetBackgroundColor(MagickWand *,const PixelWand *),
  MagickSetCompression(MagickWand *,const CompressionType),
  MagickSetCompressionQuality(MagickWand *,const unsigned long),
  MagickSetFilename(MagickWand *,const char *),
  MagickSetFormat(MagickWand *,const char *),
  MagickSetInterlaceScheme(MagickWand *,const InterlaceType),
  MagickSetOption(MagickWand *,const char *,const char *),
  MagickSetPage(MagickWand *,const unsigned long,const unsigned long,
    const long,const long),
  MagickSetPassphrase(MagickWand *,const char *),
  MagickSetResolution(MagickWand *,const double,const double),
  MagickSetResourceLimit(const ResourceType type,const unsigned long limit),
  MagickSetSamplingFactors(MagickWand *,const unsigned long,const double *),
  MagickSetSize(MagickWand *,const unsigned long,const unsigned long),
  MagickSetType(MagickWand *,const ImageType);

extern WandExport MagickProgressMonitor
  MagickSetProgressMonitor(MagickWand *,const MagickProgressMonitor,void *);

extern WandExport unsigned long
  MagickGetCompressionQuality(MagickWand *),
  MagickGetResource(const ResourceType),
  MagickGetResourceLimit(const ResourceType);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
