/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                 M   M   AAA    GGGG  IIIII   CCCC  K   K                    %
%                 MM MM  A   A  G        I    C      K  K                     %
%                 M M M  AAAAA  G GGG    I    C      KKK                      %
%                 M   M  A   A  G   G    I    C      K  K                     %
%                 M   M  A   A   GGGG  IIIII   CCCC  K   K                    %
%                                                                             %
%        AAA   TTTTT  TTTTT  RRRR   IIIII  BBBB   U   U  TTTTT  EEEEE         %
%       A   A    T      T    R   R    I    B   B  U   U    T    E             %
%       AAAAA    T      T    RRRR     I    BBBB   U   U    T    EEE           %
%       A   A    T      T    R R      I    B   B  U   U    T    E             %
%       A   A    T      T    R  R   IIIII  BBBB    UUU     T    EEEEE         %
%                                                                             %
%                                                                             %
%                       Set or Get MagickWand Attributes                      %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                 August 2003                                 %
%                                                                             %
%                                                                             %
%  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "wand/studio.h"
#include "wand/magick-wand.h"
#include "wand/magick-wand-private.h"
#include "wand/wand.h"
#include "magick/ImageMagick.h"

/*
  Define declarations.
*/
#define ThrowWandException(severity,tag,context) \
{ \
  (void) ThrowMagickException(&wand->exception,GetMagickModule(),severity, \
    tag,"`%s'",context); \
  return(MagickFalse); \
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t C o m p r e s s i o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetCompression() gets the wand compression.
%
%  The format of the MagickGetCompression method is:
%
%      CompressionType MagickGetCompression(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport CompressionType MagickGetCompression(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(wand->image_info->compression);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t C o m p r e s s i o n Q u a l i t y                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetCompression() gets the wand compression quality.
%
%  The format of the MagickGetCompression method is:
%
%      unsigned long MagickGetCompression(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetCompressionQuality(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(wand->image_info->quality);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t C o p y r i g h t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetCopyright() returns the ImageMagick API copyright as a string
%  constant.
%
%  The format of the MagickGetCopyright method is:
%
%      const char *MagickGetCopyright(void)
%
*/
WandExport const char *MagickGetCopyright(void)
{
  return(GetMagickCopyright());
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t E x c e p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetException() returns the severity, reason, and description of any
%  error that occurs when using other methods in this API.
%
%  The format of the MagickGetException method is:
%
%      char *MagickGetException(MagickWand *wand,ExceptionType *severity)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o severity: The severity of the error is returned here.
%
*/
WandExport char *MagickGetException(MagickWand *wand,ExceptionType *severity)
{
  char
    *description;

  assert(wand != (const MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(severity != (ExceptionType *) NULL);
  *severity=wand->exception.severity;
  description=(char *) AcquireMagickMemory(2*MaxTextExtent);
  if (description == (char *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "MemoryAllocationFailed","`%s'",wand->name);
      return((char *) NULL);
    }
  *description='\0';
  if (wand->exception.reason != (char *) NULL)
    (void) CopyMagickString(description,GetLocaleExceptionMessage(
      wand->exception.severity,wand->exception.reason),MaxTextExtent);
  if (wand->exception.description != (char *) NULL)
    {
      (void) ConcatenateMagickString(description," (",MaxTextExtent);
      (void) ConcatenateMagickString(description,GetLocaleExceptionMessage(
        wand->exception.severity,wand->exception.description),MaxTextExtent);
      (void) ConcatenateMagickString(description,")",MaxTextExtent);
    }
  return(description);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t F i l e n a m e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetFilename() returns the filename associated with an image sequence.
%
%  The format of the MagickGetFilename method is:
%
%      const char *MagickGetFilename(const MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%
*/
WandExport char *MagickGetFilename(const MagickWand *wand)
{
  assert(wand != (const MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(AcquireString(wand->image_info->filename));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t F o r m a t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetFormat() returns the format of the magick wand.
%
%  The format of the MagickGetFormat method is:
%
%      const char MagickGetFormat(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport char *MagickGetFormat(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(AcquireString(wand->image_info->magick));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t H o m e U R L                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetHomeURL() returns the ImageMagick home URL.
%
%  The format of the MagickGetHomeURL method is:
%
%      char *MagickGetHomeURL(void)
%
*/
WandExport char *MagickGetHomeURL(void)
{
  return(GetMagickHomeURL());
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I n t e r l a c e S c h e m e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetInterlaceScheme() gets the wand interlace scheme.
%
%  The format of the MagickGetInterlaceScheme method is:
%
%      InterlaceType MagickGetInterlaceScheme(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport InterlaceType MagickGetInterlaceScheme(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(wand->image_info->interlace);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t O p t i o n                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetOption() returns a value associated with a wand and the specified
%  key.  Use MagickRelinquishMemory() to free the value when you are finished
%  with it.
%
%  The format of the MagickGetOption method is:
%
%      char *MagickGetOption(MagickWand *wand,const char *key)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o key: The key.
%
*/
WandExport char *MagickGetOption(MagickWand *wand,const char *key)
{
  const char
    *option;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  option=GetImageOption(wand->image_info,key);
  return(ConstantString(AcquireString(option)));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t P a c k a g e N a m e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetName() returns the ImageMagick package name as a string constant.
%
%  The format of the MagickGetName method is:
%
%      const char *MagickGetName(void)
%
%
*/
WandExport const char *MagickGetPackageName(void)
{
  return(GetMagickPackageName());
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t P a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetPage() returns the page geometry associated with the magick wand.
%
%  The format of the MagickGetPage method is:
%
%      MagickBooleanType MagickGetPage(MagickWand *wand,unsigned long *width,
%        unsigned long *height,long *x,long *y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The page width.
%
%    o height: page height.
%
%    o x: The page x-offset.
%
%    o y: The page y-offset.
%
*/
WandExport MagickBooleanType MagickGetPage(MagickWand *wand,
  unsigned long *width,unsigned long *height,long *x,long *y)
{
  RectangleInfo
    geometry;

  assert(wand != (const MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) ResetMagickMemory(&geometry,0,sizeof(geometry));
  (void) ParseAbsoluteGeometry(wand->image_info->page,&geometry);
  *width=geometry.width;
  *height=geometry.height;
  *x=geometry.x;
  *y=geometry.y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t Q u a n t u m D e p t h                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetQuantumDepth() returns the ImageMagick quantum depth as a string
%  constant.
%
%  The format of the MagickGetQuantumDepth method is:
%
%      const char *MagickGetQuantumDepth(unsigned long *depth)
%
%  A description of each parameter follows:
%
%    o depth: The quantum depth is returned as a number.
%
%
*/
WandExport const char *MagickGetQuantumDepth(unsigned long *depth)
{
  return(GetMagickQuantumDepth(depth));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t Q u a n t u m R a n g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetQuantumRange() returns the ImageMagick quantum range as a string
%  constant.
%
%  The format of the MagickGetQuantumRange method is:
%
%      const char *MagickGetQuantumRange(unsigned long *range)
%
%  A description of each parameter follows:
%
%    o range: The quantum range is returned as a number.
%
%
*/
WandExport const char *MagickGetQuantumRange(unsigned long *range)
{
  return(GetMagickQuantumRange(range));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t R e l e a s e D a t e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetReleaseDate() returns the ImageMagick release date as a string
%  constant.
%
%  The format of the MagickGetReleaseDate method is:
%
%      const char *MagickGetReleaseDate(void)
%
*/
WandExport const char *MagickGetReleaseDate(void)
{
  return(GetMagickReleaseDate());
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t R e s o u r c e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetResourceLimit() returns the specified resource in megabytes.
%
%  The format of the MagickGetResource method is:
%
%      unsigned long MagickGetResource(const ResourceType type)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetResource(const ResourceType type)
{
  return(GetMagickResource(type));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t R e s o u r c e L i m i t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetResourceLimit() returns the specified resource limit in megabytes.
%
%  The format of the MagickGetResourceLimit method is:
%
%      unsigned long MagickGetResourceLimit(const ResourceType type)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetResourceLimit(const ResourceType type)
{
  return(GetMagickResourceLimit(type));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t S a m p l i n g F a c t o r s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetSamplingFactors() gets the horizontal and vertical sampling factor.
%
%  The format of the MagickGetSamplingFactors method is:
%
%      double *MagickGetSamplingFactor(MagickWand *wand,
%        unsigned long *number_factors)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o number_factors: The number of factors in the returned array.
%
*/
WandExport double *MagickGetSamplingFactors(MagickWand *wand,
  unsigned long *number_factors)
{
  double
    *sampling_factors;

  register const char
    *p;

  register long
    i;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  *number_factors=0;
  sampling_factors=(double *) NULL;
  if (wand->image_info->sampling_factor == (char *) NULL)
    return(sampling_factors);
  i=0;
  for (p=wand->image_info->sampling_factor; p != (char *) NULL; p=strchr(p,','))
  {
    while (((int) *p != 0) && ((isspace((int) ((unsigned char) *p)) != 0) ||
           (*p == ',')))
      p++;
    i++;
  }
  sampling_factors=(double *) AcquireMagickMemory((size_t)
    i*sizeof(*sampling_factors));
  if (sampling_factors == (double *) NULL)
    ThrowWandFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      wand->image_info->filename);
  i=0;
  for (p=wand->image_info->sampling_factor; p != (char *) NULL; p=strchr(p,','))
  {
    while (((int) *p != 0) && ((isspace((int) ((unsigned char) *p)) != 0) ||
           (*p == ',')))
      p++;
    sampling_factors[i]=atof(p);
    i++;
  }
  *number_factors=(unsigned long) i;
  return(sampling_factors);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t S i z e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetSize() returns the size associated with the magick wand.
%
%  The format of the MagickGetSize method is:
%
%      MagickBooleanType MagickGetSize(const MagickWand *wand,
%        unsigned long *columns,unsigned long *rows)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns: The width in pixels.
%
%    o height: The height in pixels.
%
*/
WandExport MagickBooleanType MagickGetSize(const MagickWand *wand,
  unsigned long *columns,unsigned long *rows)
{
  RectangleInfo
    geometry;

  assert(wand != (const MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) ResetMagickMemory(&geometry,0,sizeof(geometry));
  (void) ParseAbsoluteGeometry(wand->image_info->size,&geometry);
  *columns=geometry.width;
  *rows=geometry.height;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t V e r s i o n                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetVersion() returns the ImageMagick API version as a string constant
%  and as a number.
%
%  The format of the MagickGetVersion method is:
%
%      const char *MagickGetVersion(unsigned long *version)
%
%  A description of each parameter follows:
%
%    o version: The ImageMagick version is returned as a number.
%
*/
WandExport const char *MagickGetVersion(unsigned long *version)
{
  return(GetMagickVersion(version));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t B a c k g r o u n d C o l o r                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetBackgroundColor() sets the wand background color.
%
%  The format of the MagickSetBackgroundColor method is:
%
%      MagickBooleanType MagickSetBackgroundColor(MagickWand *wand,
%        const PixelWand *background)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o background: The background pixel wand.
%
*/
WandExport MagickBooleanType MagickSetBackgroundColor(MagickWand *wand,
  const PixelWand *background)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  PixelGetQuantumColor(background,&wand->image_info->background_color);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t C o m p r e s s i o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetCompression() sets the wand compression type.
%
%  The format of the MagickSetCompression method is:
%
%      MagickBooleanType MagickSetCompression(MagickWand *wand,
%        const CompressionType compression)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o compression: The wand compression.
%
*/
WandExport MagickBooleanType MagickSetCompression(MagickWand *wand,
  const CompressionType compression)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  wand->image_info->compression=compression;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t C o m p r e s s i o n Q u a l i t y                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetCompressionQuality() sets the wand compression quality.
%
%  The format of the MagickSetCompressionQuality method is:
%
%      MagickBooleanType MagickSetCompressionQuality(MagickWand *wand,
%        const unsigned long quality)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o quality: The wand compression quality.
%
*/
WandExport MagickBooleanType MagickSetCompressionQuality(MagickWand *wand,
  const unsigned long quality)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  wand->image_info->quality=quality;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t F i l e n a m e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetFilename() sets the filename before you read or write an image file.
%
%  The format of the MagickSetFilename method is:
%
%      MagickBooleanType MagickSetFilename(MagickWand *wand,
%        const char *filename)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o filename: The image filename.
%
*/
WandExport MagickBooleanType MagickSetFilename(MagickWand *wand,
  const char *filename)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (filename != (const char *) NULL)
    (void) CopyMagickString(wand->image_info->filename,filename,MaxTextExtent);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t F o r m a t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetFormat() sets the format of the magick wand.
%
%  The format of the MagickSetFormat method is:
%
%      MagickBooleanType MagickSetFormat(MagickWand *wand,const char *format)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o format: The image format.
%
*/
WandExport MagickBooleanType MagickSetFormat(MagickWand *wand,
  const char *format)
{
  const MagickInfo
    *magick_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((format == (char *) NULL) || (*format == '\0'))
    {
      *wand->image_info->magick='\0';
      return(MagickTrue);
    }
  magick_info=GetMagickInfo(format,&wand->exception);
  if (magick_info == (const MagickInfo *) NULL)
    return(MagickFalse);
  (void) SetExceptionInfo(&wand->exception,UndefinedException);
  (void) CopyMagickString(wand->image_info->magick,format,MaxTextExtent);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I n t e r l a c e S c h e m e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetInterlaceScheme() sets the image compression.
%
%  The format of the MagickSetInterlaceScheme method is:
%
%      MagickBooleanType MagickSetInterlaceScheme(MagickWand *wand,
%        const InterlaceType interlace_scheme)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o interlace_scheme: The image interlace scheme: NoInterlace, LineInterlace,
%      PlaneInterlace, PartitionInterlace.
%
*/
WandExport MagickBooleanType MagickSetInterlaceScheme(MagickWand *wand,
  const InterlaceType interlace_scheme)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  wand->image_info->interlace=interlace_scheme;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t O p t i o n                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetOption() associates one or options with the wand (.e.g
%  MagickSetOption(wand,"jpeg:perserve","yes")).
%
%  The format of the MagickSetOption method is:
%
%      MagickBooleanType MagickSetOption(MagickWand *wand,const char *key,
%        const char *value)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o key:  The key.
%
%    o value:  The value.
%
*/
WandExport MagickBooleanType MagickSetOption(MagickWand *wand,const char *key,
  const char *value)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(SetImageOption(wand->image_info,key,value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t P a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetPageSize() sets the page geometry of the magick wand.
%
%  The format of the MagickSetPage method is:
%
%      MagickBooleanType MagickSetPage(MagickWand *wand,
%        const unsigned long width,const unsigned long height,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The page width.
%
%    o height: The page height.
%
%    o x: The page x-offset.
%
%    o y: The page y-offset.
%
*/
WandExport MagickBooleanType MagickSetPage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y)
{
  char
    geometry[MaxTextExtent];

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) FormatMagickString(geometry,MaxTextExtent,"%lux%lu%+ld%+ld",
    width,height,x,y);
  (void) CloneString(&wand->image_info->page,geometry);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t P a s s p h r a s e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetPassphrase() sets the passphrase.
%
%  The format of the MagickSetPassphrase method is:
%
%      MagickBooleanType MagickSetPassphrase(MagickWand *wand,
%        const char *passphrase)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o passphrase: The passphrase.
%
*/
WandExport MagickBooleanType MagickSetPassphrase(MagickWand *wand,
  const char *passphrase)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) CloneString(&wand->image_info->authenticate,passphrase);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t P r o g r e s s M o n i t o r                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetProgressMonitor() sets the wand progress monitor to the specified
%  method and returns the previous progress monitor if any.  The progress
%  monitor method looks like this:
%
%    MagickBooleanType MagickProgressMonitor(const char *text,
%      const MagickOffsetType offset,const MagickSizeType span,
%      void *client_data)
%
%  If the progress monitor returns MagickFalse, the current operation is
%  interrupted.
%
%  The format of the MagickSetProgressMonitor method is:
%
%      MagickProgressMonitor MagickSetProgressMonitor(MagickWand *wand
%        const MagickProgressMonitor progress_monitor,void *client_data)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o progress_monitor: Specifies a pointer to a method to monitor progress
%      of an image operation.
%
%    o client_data: Specifies a pointer to any client data.
%
*/
WandExport MagickProgressMonitor MagickSetProgressMonitor(MagickWand *wand,
  const MagickProgressMonitor progress_monitor,void *client_data)
{
  MagickProgressMonitor
    previous_monitor;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  previous_monitor=SetImageInfoProgressMonitor(wand->image_info,
    progress_monitor,client_data);
  return(previous_monitor);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t R e s o u r c e L i m i t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetResourceLimit() sets the limit for a particular resource in
%  megabytes.
%
%  The format of the MagickSetResourceLimit method is:
%
%      MagickBooleanType MagickSetResourceLimit(const ResourceType type,
%        const unsigned long *limit)
%
%  A description of each parameter follows:
%
%    o type: The type of resource: AreaResource, MemoryResource, MapResource,
%      DiskResource, FileResource.
%
%    o The maximum limit for the resource.
%
*/
WandExport MagickBooleanType MagickSetResourceLimit(const ResourceType type,
  const unsigned long limit)
{
  return(SetMagickResourceLimit(type,limit));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t R e s o l u t i o n                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetResolution() sets the image resolution.
%
%  The format of the MagickSetResolution method is:
%
%      MagickBooleanType MagickSetResolution(MagickWand *wand,
%        const double x_resolution,const doubtl y_resolution)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x_resolution: The image x resolution.
%
%    o y_resolution: The image y resolution.
%
*/
WandExport MagickBooleanType MagickSetResolution(MagickWand *wand,
  const double x_resolution,const double y_resolution)
{
  char
    density[MaxTextExtent];

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) FormatMagickString(density,MaxTextExtent,"%gx%g",x_resolution,
    y_resolution);
  (void) CloneString(&wand->image_info->density,density);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t S a m p l i n g F a c t o r s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetSamplingFactors() sets the image sampling factors.
%
%  The format of the MagickSetSamplingFactors method is:
%
%      MagickBooleanType MagickSetSamplingFactors(MagickWand *wand,
%        const unsigned long number_factors,const double *sampling_factors)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o number_factoes: The number of factors.
%
%    o sampling_factors: An array of doubles representing the sampling factor
%      for each color component (in RGB order).
%
*/
WandExport MagickBooleanType MagickSetSamplingFactors(MagickWand *wand,
  const unsigned long number_factors,const double *sampling_factors)
{
  char
    sampling_factor[MaxTextExtent];

  register long
    i;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->image_info->sampling_factor != (char *) NULL)
    wand->image_info->sampling_factor=(char *)
      RelinquishMagickMemory(wand->image_info->sampling_factor);
  if (number_factors == 0)
    return(MagickTrue);
  for (i=0; i < (long) (number_factors-1); i++)
  {
    (void) FormatMagickString(sampling_factor,MaxTextExtent,"%g,",
      sampling_factors[i]);
    (void) ConcatenateString(&wand->image_info->sampling_factor,
      sampling_factor);
  }
  (void) FormatMagickString(sampling_factor,MaxTextExtent,"%g",
    sampling_factors[i]);
  (void) ConcatenateString(&wand->image_info->sampling_factor,sampling_factor);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t S i z e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetSize() sets the size of the magick wand.  Set it before you
%  read a raw image format such as RGB, GRAY, or CMYK.
%
%  The format of the MagickSetSize method is:
%
%      MagickBooleanType MagickSetSize(MagickWand *wand,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns: The width in pixels.
%
%    o height: The height in pixels.
%
%
*/
WandExport MagickBooleanType MagickSetSize(MagickWand *wand,
  const unsigned long columns,const unsigned long rows)
{
  char
    geometry[MaxTextExtent];

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) FormatMagickString(geometry,MaxTextExtent,"%lux%lu",columns,rows);
  (void) CloneString(&wand->image_info->size,geometry);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t T y p e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetType() sets the image type attribute.
%
%  The format of the MagickSetType method is:
%
%      MagickBooleanType MagickSetType(MagickWand *wand,
%        const ImageType image_type)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o image_type: The image type:   UndefinedType, BilevelType, GrayscaleType,
%      GrayscaleMatteType, PaletteType, PaletteMatteType, TrueColorType,
%      TrueColorMatteType, ColorSeparationType, ColorSeparationMatteType,
%      or OptimizeType.
%
*/
WandExport MagickBooleanType MagickSetType(MagickWand *wand,
  const ImageType image_type)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->image_info->type=image_type;
  return(MagickTrue);
}
