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

  ImageMagick private image methods.
*/
#ifndef _MAGICK_IMAGE_PRIVATE_H
#define _MAGICK_IMAGE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define DegreesToRadians(x)  (MagickPI*(x)/180.0)
#define MagickPI  3.14159265358979323846264338327950288419716939937510
#define MagickSQ2PI  2.50662827463100024161235523934010416269302368164062
#define RadiansToDegrees(x) (180.0*(x)/MagickPI)
#define QuantumScale  ((MagickRealType) 1.0/(MagickRealType) QuantumRange)
#define ScaleAnyToQuantum(x,scale)  ((Quantum) \
  (((MagickRealType) QuantumRange*(x))/(scale)+0.5))
#define ScaleColor5to8(x)  (((x) << 3) | ((x) >> 2))
#define ScaleColor6to8(x)  (((x) << 2) | ((x) >> 4))
#define ScaleQuantumToAny(x,scale)  ((unsigned long) \
  (((MagickRealType) (scale)*(x))/QuantumRange+0.5))
#define UndefinedTicksPerSecond  100UL
#define UndefinedCompressionQuality  0UL

static inline MagickBooleanType QuantumTick(const MagickOffsetType offset,
  const MagickSizeType span)
{
  if ((offset & (offset-1)) == 0)
    return(MagickTrue);
  if ((offset & 0x7f) == 0)
    return(MagickTrue);
  if (offset == (MagickOffsetType) (span-1))
    return(MagickTrue);
  return(MagickFalse);
}

static inline unsigned long RoundToMap(const MagickRealType value)
{
  if (value < 0.0)
    return(0UL);
  if (value >= (MagickRealType) MaxMap)
    return(MaxMap);
  return((unsigned long) (value+0.5));
}

static inline Quantum RoundToQuantum(const MagickRealType value)
{
  if (value < 0.0)
    return(0);
  if (value >= (MagickRealType) QuantumRange)
    return(QuantumRange);
  return((Quantum) (value+0.5));
}

extern MagickExport const char
  *BackgroundColor,
  *BorderColor,
  *DefaultTileFrame,
  *DefaultTileGeometry,
  *DefaultTileLabel,
  *ForegroundColor,
  *MatteColor,
  *LoadImageTag,
  *LoadImagesTag,
  *PSDensityGeometry,
  *PSPageGeometry,
  *SaveImageTag,
  *SaveImagesTag;

extern MagickExport const double
  DefaultResolution;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
