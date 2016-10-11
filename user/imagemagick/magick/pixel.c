/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      PPPP   IIIII  X   X  EEEEE  L                          %
%                      P   P    I     X X   E      L                          %
%                      PPPP     I      X    EEE    L                          %
%                      P        I     X X   E      L                          %
%                      P      IIIII  X   X  EEEEE  LLLLL                      %
%                                                                             %
%                       Methods to Import/Export Pixels                       %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                               October 1998                                  %
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
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/cache.h"
#include "magick/constitute.h"
#include "magick/delegate.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/pixel.h"
#include "magick/pixel-private.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/statistic.h"
#include "magick/stream.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p o r t I m a g e P i x e l s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExportImagePixels() extracts pixel data from an image and returns it to you.
%  The method returns MagickTrue on success otherwise MagickFalse if an error is
%  encountered.  The data is returned as char, short int, int, long, float,
%  or double in the order specified by map.
%
%  Suppose you want to extract the first scanline of a 640x480 image as
%  character data in red-green-blue order:
%
%      ExportImagePixels(image,0,0,640,1,"RGB",CharPixel,pixels,exception);
%
%  The format of the ExportImagePixels method is:
%
%      MagickBooleanType ExportImagePixels(const Image *image,
%        const long x_offset,const long y_offset,const unsigned long columns,
%        const unsigned long rows,const char *map,const StorageType type,
%        void *pixels,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x_offset,y_offset,columns,rows:  These values define the perimeter
%      of a region of pixels you want to extract.
%
%    o map:  This string reflects the expected ordering of the pixel array.
%      It can be any combination or order of R = red, G = green, B = blue,
%      A = alpha (0 is transparent), O = opacity (0 is opaque), C = cyan,
%      Y = yellow, M = magenta, K = black, I = intensity (for grayscale),
%      P = pad.
%
%    o type: Define the data type of the pixels.  Float and double types are
%      normalized to [0..1] otherwise [0..QuantumRange].  Choose from these
%      types: CharPixel, DoublePixel, FloatPixel, IntegerPixel, LongPixel,
%      QuantumPixel, or ShortPixel.
%
%    o pixels: This array of values contain the pixel components as defined by
%      map and type.  You must preallocate this array where the expected
%      length varies depending on the values of width, height, map, and type.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ExportImagePixels(const Image *image,
  const long x_offset,const long y_offset,const unsigned long columns,
  const unsigned long rows,const char *map,const StorageType type,void *pixels,
  ExceptionInfo *exception)
{
  long
    y;

  QuantumType
    *quantum_map;

  register long
    i,
    x;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  size_t
    length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  length=strlen(map);
  quantum_map=(QuantumType *) AcquireMagickMemory(length*sizeof(*quantum_map));
  if (quantum_map == (QuantumType *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  for (i=0; i < (long) length; i++)
  {
    switch (map[i])
    {
      case 'A':
      case 'a':
      {
        quantum_map[i]=AlphaQuantum;
        break;
      }
      case 'B':
      case 'b':
      {
        quantum_map[i]=BlueQuantum;
        break;
      }
      case 'C':
      case 'c':
      {
        quantum_map[i]=CyanQuantum;
        if (image->colorspace == CMYKColorspace)
          break;
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
          "ColorSeparatedImageRequired","`%s'",map);
        return(MagickFalse);
      }
      case 'g':
      case 'G':
      {
        quantum_map[i]=GreenQuantum;
        break;
      }
      case 'I':
      case 'i':
      {
        quantum_map[i]=IndexQuantum;
        break;
      }
      case 'K':
      case 'k':
      {
        quantum_map[i]=BlackQuantum;
        if (image->colorspace == CMYKColorspace)
          break;
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
          "ColorSeparatedImageRequired","`%s'",map);
        return(MagickFalse);
      }
      case 'M':
      case 'm':
      {
        quantum_map[i]=MagentaQuantum;
        if (image->colorspace == CMYKColorspace)
          break;
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
          "ColorSeparatedImageRequired","`%s'",map);
        return(MagickFalse);
      }
      case 'o':
      case 'O':
      {
        quantum_map[i]=OpacityQuantum;
        break;
      }
      case 'P':
      case 'p':
      {
        quantum_map[i]=UndefinedQuantum;
        break;
      }
      case 'R':
      case 'r':
      {
        quantum_map[i]=RedQuantum;
        break;
      }
      case 'Y':
      case 'y':
      {
        quantum_map[i]=YellowQuantum;
        if (image->colorspace == CMYKColorspace)
          break;
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
          "ColorSeparatedImageRequired","`%s'",map);
        return(MagickFalse);
      }
      default:
      {
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
          "UnrecognizedPixelMap","`%s'",map);
        return(MagickFalse);
      }
    }
  }
  switch (type)
  {
    case CharPixel:
    {
      register unsigned char
        *q;

      q=(unsigned char *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToChar(p->blue);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->red);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=ScaleQuantumToChar(p->blue);
                  *q++=ScaleQuantumToChar(p->green);
                  *q++=ScaleQuantumToChar(p->red);
                  *q++=ScaleQuantumToChar(TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToChar(p->blue);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->red);
              *q++=ScaleQuantumToChar(QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToChar(p->blue);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->red);
              *q++=ScaleQuantumToChar(0);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToChar(PixelIntensityToQuantum(p));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToChar(p->red);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->blue);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=ScaleQuantumToChar(p->red);
                  *q++=ScaleQuantumToChar(p->green);
                  *q++=ScaleQuantumToChar(p->blue);
                  *q++=ScaleQuantumToChar(TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToChar(p->red);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->blue);
              *q++=ScaleQuantumToChar(QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToChar(p->red);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->blue);
              *q++=ScaleQuantumToChar(0);
              p++;
            }
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            *q=0;
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                *q=ScaleQuantumToChar(p->red);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                *q=ScaleQuantumToChar(p->green);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                *q=ScaleQuantumToChar(p->blue);
                break;
              }
              case AlphaQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=ScaleQuantumToChar(QuantumRange-p->opacity);
                break;
              }
              case OpacityQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=ScaleQuantumToChar(p->opacity);
                break;
              }
              case BlackQuantum:
              {
                if (image->colorspace == CMYKColorspace)
                  *q=ScaleQuantumToChar(indexes[x]);
                break;
              }
              case IndexQuantum:
              {
                *q=ScaleQuantumToChar(PixelIntensityToQuantum(p));
                break;
              }
              default:
                break;
            }
            q++;
          }
          p++;
        }
      }
      break;
    }
    case DoublePixel:
    {
      register double
        *q;

      q=(double *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(double) (QuantumScale*p->blue);
              *q++=(double) (QuantumScale*p->green);
              *q++=(double) (QuantumScale*p->red);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=(double) (QuantumScale*p->blue);
                  *q++=(double) (QuantumScale*p->green);
                  *q++=(double) (QuantumScale*p->red);
                  *q++=(double) (QuantumScale*TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=(double) (QuantumScale*p->blue);
              *q++=(double) (QuantumScale*p->green);
              *q++=(double) (QuantumScale*p->red);
              *q++=(double) (QuantumScale*(QuantumRange-p->opacity));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(double) (QuantumScale*p->blue);
              *q++=(double) (QuantumScale*p->green);
              *q++=(double) (QuantumScale*p->red);
              *q++=0.0;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(double) (QuantumScale*PixelIntensityToQuantum(p));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(double) (QuantumScale*p->red);
              *q++=(double) (QuantumScale*p->green);
              *q++=(double) (QuantumScale*p->blue);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=(double) (QuantumScale*p->red);
                  *q++=(double) (QuantumScale*p->green);
                  *q++=(double) (QuantumScale*p->blue);
                  *q++=(double) (QuantumScale*TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=(double) (QuantumScale*p->red);
              *q++=(double) (QuantumScale*p->green);
              *q++=(double) (QuantumScale*p->blue);
              *q++=(double) (QuantumScale*(QuantumRange-p->opacity));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(double) (QuantumScale*p->red);
              *q++=(double) (QuantumScale*p->green);
              *q++=(double) (QuantumScale*p->blue);
              *q++=0.0;
              p++;
            }
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            *q=0;
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                *q=(double) (QuantumScale*p->red);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                *q=(double) (QuantumScale*p->green);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                *q=(double) (QuantumScale*p->blue);
                break;
              }
              case AlphaQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=(double) (QuantumScale*(QuantumRange-p->opacity));
                break;
              }
              case OpacityQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=(double) (QuantumScale*p->opacity);
                break;
              }
              case BlackQuantum:
              {
                if (image->colorspace == CMYKColorspace)
                  *q=(double) (QuantumScale*indexes[x]);
                break;
              }
              case IndexQuantum:
              {
                *q=(double) (QuantumScale*PixelIntensityToQuantum(p));
                break;
              }
              default:
                *q=0;
            }
            q++;
          }
          p++;
        }
      }
      break;
    }
    case FloatPixel:
    {
      register float
        *q;

      q=(float *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(float) (QuantumScale*p->blue);
              *q++=(float) (QuantumScale*p->green);
              *q++=(float) (QuantumScale*p->red);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=(float) (QuantumScale*p->blue);
                  *q++=(float) (QuantumScale*p->green);
                  *q++=(float) (QuantumScale*p->red);
                  *q++=(float) (QuantumScale*TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=(float) (QuantumScale*p->blue);
              *q++=(float) (QuantumScale*p->green);
              *q++=(float) (QuantumScale*p->red);
              *q++=(float) (QuantumScale*QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(float) (QuantumScale*p->blue);
              *q++=(float) (QuantumScale*p->green);
              *q++=(float) (QuantumScale*p->red);
              *q++=0.0;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(float) (QuantumScale*PixelIntensityToQuantum(p));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(float) (QuantumScale*p->red);
              *q++=(float) (QuantumScale*p->green);
              *q++=(float) (QuantumScale*p->blue);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=(float) (QuantumScale*p->red);
                  *q++=(float) (QuantumScale*p->green);
                  *q++=(float) (QuantumScale*p->blue);
                  *q++=(float) (QuantumScale*TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=(float) (QuantumScale*p->red);
              *q++=(float) (QuantumScale*p->green);
              *q++=(float) (QuantumScale*p->blue);
              *q++=(float) (QuantumScale*(QuantumRange-p->opacity));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(float) (QuantumScale*p->red);
              *q++=(float) (QuantumScale*p->green);
              *q++=(float) (QuantumScale*p->blue);
              *q++=0.0;
              p++;
            }
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            *q=0;
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                *q=(float) (QuantumScale*p->red);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                *q=(float) (QuantumScale*p->green);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                *q=(float) (QuantumScale*p->blue);
                break;
              }
              case AlphaQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=(float) (QuantumScale*(QuantumRange-p->opacity));
                break;
              }
              case OpacityQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=(float) (QuantumScale*p->opacity);
                break;
              }
              case BlackQuantum:
              {
                if (image->colorspace == CMYKColorspace)
                  *q=(float) (QuantumScale*indexes[x]);
                break;
              }
              case IndexQuantum:
              {
                *q=(float) (QuantumScale*PixelIntensityToQuantum(p));
                break;
              }
              default:
                *q=0;
            }
            q++;
          }
          p++;
        }
      }
      break;
    }
    case IntegerPixel:
    {
      register unsigned int
        *q;

      q=(unsigned int *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(unsigned int) ScaleQuantumToLong(p->blue);
              *q++=(unsigned int) ScaleQuantumToLong(p->green);
              *q++=(unsigned int) ScaleQuantumToLong(p->red);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=(unsigned int) ScaleQuantumToLong(p->blue);
                  *q++=(unsigned int) ScaleQuantumToLong(p->green);
                  *q++=(unsigned int) ScaleQuantumToLong(p->red);
                  *q++=(unsigned int) ScaleQuantumToLong(TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=(unsigned int) ScaleQuantumToLong(p->blue);
              *q++=(unsigned int) ScaleQuantumToLong(p->green);
              *q++=(unsigned int) ScaleQuantumToLong(p->red);
              *q++=(unsigned int) ScaleQuantumToLong(QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(unsigned int) ScaleQuantumToLong(p->blue);
              *q++=(unsigned int) ScaleQuantumToLong(p->green);
              *q++=(unsigned int) ScaleQuantumToLong(p->red);
              *q++=0U;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(unsigned int)
                ScaleQuantumToLong(PixelIntensityToQuantum(p));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(unsigned int) ScaleQuantumToLong(p->red);
              *q++=(unsigned int) ScaleQuantumToLong(p->green);
              *q++=(unsigned int) ScaleQuantumToLong(p->blue);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=(unsigned int) ScaleQuantumToLong(p->red);
                  *q++=(unsigned int) ScaleQuantumToLong(p->green);
                  *q++=(unsigned int) ScaleQuantumToLong(p->blue);
                  *q++=(unsigned int) ScaleQuantumToLong(TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=(unsigned int) ScaleQuantumToLong(p->red);
              *q++=(unsigned int) ScaleQuantumToLong(p->green);
              *q++=(unsigned int) ScaleQuantumToLong(p->blue);
              *q++=(unsigned int) ScaleQuantumToLong(QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=(unsigned int) ScaleQuantumToLong(p->red);
              *q++=(unsigned int) ScaleQuantumToLong(p->green);
              *q++=(unsigned int) ScaleQuantumToLong(p->blue);
              *q++=0U;
              p++;
            }
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            *q=0;
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                *q=(unsigned int) ScaleQuantumToLong(p->red);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                *q=(unsigned int) ScaleQuantumToLong(p->green);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                *q=(unsigned int) ScaleQuantumToLong(p->blue);
                break;
              }
              case AlphaQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=(unsigned int) ScaleQuantumToLong(QuantumRange-p->opacity);
                break;
              }
              case OpacityQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=(unsigned int) ScaleQuantumToLong(p->opacity);
                break;
              }
              case BlackQuantum:
              {
                if (image->colorspace == CMYKColorspace)
                  *q=(unsigned int) ScaleQuantumToLong(indexes[x]);
                break;
              }
              case IndexQuantum:
              {
                *q=(unsigned int)
                  ScaleQuantumToLong(PixelIntensityToQuantum(p));
                break;
              }
              default:
                *q=0;
            }
            q++;
          }
          p++;
        }
      }
      break;
    }
    case LongPixel:
    {
      register unsigned long
        *q;

      q=(unsigned long *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToLong(p->blue);
              *q++=ScaleQuantumToLong(p->green);
              *q++=ScaleQuantumToLong(p->red);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=ScaleQuantumToLong(p->blue);
                  *q++=ScaleQuantumToLong(p->green);
                  *q++=ScaleQuantumToLong(p->red);
                  *q++=ScaleQuantumToLong(TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToLong(p->blue);
              *q++=ScaleQuantumToLong(p->green);
              *q++=ScaleQuantumToLong(p->red);
              *q++=ScaleQuantumToLong(QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToLong(p->blue);
              *q++=ScaleQuantumToLong(p->green);
              *q++=ScaleQuantumToLong(p->red);
              *q++=0;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToLong(PixelIntensityToQuantum(p));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToLong(p->red);
              *q++=ScaleQuantumToLong(p->green);
              *q++=ScaleQuantumToLong(p->blue);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=ScaleQuantumToLong(p->red);
                  *q++=ScaleQuantumToLong(p->green);
                  *q++=ScaleQuantumToLong(p->blue);
                  *q++=ScaleQuantumToLong(TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToLong(p->red);
              *q++=ScaleQuantumToLong(p->green);
              *q++=ScaleQuantumToLong(p->blue);
              *q++=ScaleQuantumToLong(QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToLong(p->red);
              *q++=ScaleQuantumToLong(p->green);
              *q++=ScaleQuantumToLong(p->blue);
              *q++=0;
              p++;
            }
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            *q=0;
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                *q=ScaleQuantumToLong(p->red);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                *q=ScaleQuantumToLong(p->green);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                *q=ScaleQuantumToLong(p->blue);
                break;
              }
              case AlphaQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=ScaleQuantumToLong(QuantumRange-p->opacity);
                break;
              }
              case OpacityQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=ScaleQuantumToLong(p->opacity);
                break;
              }
              case BlackQuantum:
              {
                if (image->colorspace == CMYKColorspace)
                  *q=ScaleQuantumToLong(indexes[x]);
                break;
              }
              case IndexQuantum:
              {
                *q=ScaleQuantumToLong(PixelIntensityToQuantum(p));
                break;
              }
              default:
                break;
            }
            q++;
          }
          p++;
        }
      }
      break;
    }
    case QuantumPixel:
    {
      register Quantum
        *q;

      q=(Quantum *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=p->blue;
              *q++=p->green;
              *q++=p->red;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=p->blue;
                  *q++=p->green;
                  *q++=p->red;
                  *q++=TransparentOpacity;
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=p->blue;
              *q++=p->green;
              *q++=p->red;
              *q++=QuantumRange-p->opacity;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=p->blue;
              *q++=p->green;
              *q++=p->red;
              *q++=0U;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=PixelIntensityToQuantum(p);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=p->red;
              *q++=p->green;
              *q++=p->blue;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=p->red;
                  *q++=p->green;
                  *q++=p->blue;
                  *q++=TransparentOpacity;
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=p->red;
              *q++=p->green;
              *q++=p->blue;
              *q++=QuantumRange-p->opacity;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=p->red;
              *q++=p->green;
              *q++=p->blue;
              *q++=0U;
              p++;
            }
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            *q=0;
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                *q=p->red;
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                *q=p->green;
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                *q=p->blue;
                break;
              }
              case AlphaQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=QuantumRange-p->opacity;
                break;
              }
              case OpacityQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=p->opacity;
                break;
              }
              case BlackQuantum:
              {
                if (image->colorspace == CMYKColorspace)
                  *q=indexes[x];
                break;
              }
              case IndexQuantum:
              {
                *q=(PixelIntensityToQuantum(p));
                break;
              }
              default:
                *q=0;
            }
            q++;
          }
          p++;
        }
      }
      break;
    }
    case ShortPixel:
    {
      register unsigned short
        *q;

      q=(unsigned short *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToShort(p->blue);
              *q++=ScaleQuantumToShort(p->green);
              *q++=ScaleQuantumToShort(p->red);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=ScaleQuantumToShort(p->blue);
                  *q++=ScaleQuantumToShort(p->green);
                  *q++=ScaleQuantumToShort(p->red);
                  *q++=ScaleQuantumToShort(TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToShort(p->blue);
              *q++=ScaleQuantumToShort(p->green);
              *q++=ScaleQuantumToShort(p->red);
              *q++=ScaleQuantumToShort(QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToShort(p->blue);
              *q++=ScaleQuantumToShort(p->green);
              *q++=ScaleQuantumToShort(p->red);
              *q++=0;
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToShort(PixelIntensityToQuantum(p));
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToShort(p->red);
              *q++=ScaleQuantumToShort(p->green);
              *q++=ScaleQuantumToShort(p->blue);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            if (image->matte == MagickFalse)
              {
                for (x=0; x < (long) columns; x++)
                {
                  *q++=ScaleQuantumToShort(p->red);
                  *q++=ScaleQuantumToShort(p->green);
                  *q++=ScaleQuantumToShort(p->blue);
                  *q++=ScaleQuantumToShort(TransparentOpacity);
                  p++;
                }
                continue;
              }
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToShort(p->red);
              *q++=ScaleQuantumToShort(p->green);
              *q++=ScaleQuantumToShort(p->blue);
              *q++=ScaleQuantumToShort(QuantumRange-p->opacity);
              p++;
            }
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              *q++=ScaleQuantumToShort(p->red);
              *q++=ScaleQuantumToShort(p->green);
              *q++=ScaleQuantumToShort(p->blue);
              *q++=0;
              p++;
            }
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        p=AcquireImagePixels(image,x_offset,y_offset+y,columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            *q=0;
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                *q=ScaleQuantumToShort(p->red);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                *q=ScaleQuantumToShort(p->green);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                *q=ScaleQuantumToShort(p->blue);
                break;
              }
              case AlphaQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=ScaleQuantumToShort(QuantumRange-p->opacity);
                break;
              }
              case OpacityQuantum:
              {
                if (image->matte != MagickFalse)
                  *q=ScaleQuantumToShort(p->opacity);
                break;
              }
              case BlackQuantum:
              {
                if (image->colorspace == CMYKColorspace)
                  *q=ScaleQuantumToShort(indexes[x]);
                break;
              }
              case IndexQuantum:
              {
                *q=ScaleQuantumToShort(PixelIntensityToQuantum(p));
                break;
              }
              default:
                break;
            }
            q++;
          }
          p++;
        }
      }
      break;
    }
    default:
    {
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
        "UnrecognizedPixelMap","`%s'",map);
      break;
    }
  }
  quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p o r t Q u a n t u m P i x e l s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExportQuantumPixels() transfers one or more pixel components from a user
%  supplied buffer into the image pixel cache of an image.  The pixels are
%  expected in network byte order.  It returns MagickTrue if the pixels are
%  successfully transferred, otherwise MagickFalse.
%
%  The format of the ExportQuantumPixels method is:
%
%      MagickBooleanType ExportQuantumPixels(Image *image,
%        const QuantumType quantum_type,const size_t pad,
%        const unsigned char *source)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o quantum_type: Declare which pixel components to transfer (red, green,
%      blue, opacity, RGB, or RGBA).
%
%    o pad: skip components.
%
%    o source:  The pixel components are transferred from this buffer.
%
%
*/

static inline IndexPacket PushColormapIndex(Image *image,
  const unsigned long index)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (index < image->colors)
    return((IndexPacket) index);
  (void) ThrowMagickException(&image->exception,GetMagickModule(),
    CorruptImageError,"InvalidColormapIndex","`%s'",image->filename);
  return(0);
}

static inline unsigned long PushPixelQuantum(const unsigned char *pixels,
  const unsigned long depth)
{
  register long
    i;

  register unsigned long
    quantum_bits,
    quantum;

  static const unsigned char
    *p;

	static unsigned long
    data_bits;

  if (depth == 0UL)
    {
      p=pixels;
      data_bits=8UL;
    }
  quantum=0UL;
  for (i=(long) depth; i > 0L; )
  {
    quantum_bits=(unsigned long) i;
    if (quantum_bits > data_bits)
      quantum_bits=data_bits;
    i-=quantum_bits;
    data_bits-=quantum_bits;
    quantum=(quantum << quantum_bits) |
      ((*p >> data_bits) &~ ((~0UL) << quantum_bits));
    if (data_bits == 0UL)
      {
        p++;
        data_bits=8UL;
      }
  }
  return(quantum);
}

MagickExport MagickBooleanType ExportQuantumPixels(Image *image,
  const QuantumType quantum_type,const size_t pad,const unsigned char *source)
{
#define PushCharPixel(p,pixel) \
{ \
  pixel=(unsigned char) (*(p)); \
  (p)++; \
}
#define PushLongPixel(p,pixel) \
{ \
  if (swab == MagickFalse) \
    { \
      pixel=(unsigned long) (*(p) << 24); \
      pixel|=(unsigned long) (*((p)+1) << 16); \
      pixel|=(unsigned long) (*((p)+2) << 8); \
      pixel|=(unsigned long) *((p)+3); \
    } \
  else \
    { \
      pixel=(unsigned long) *(p); \
      pixel|=(unsigned long) (*((p)+1) << 8); \
      pixel|=(unsigned long) (*((p)+2) << 16); \
      pixel|=(unsigned long) (*((p)+3) << 24); \
    } \
  (p)+=4; \
}
#define PushShortPixel(p,pixel) \
{ \
  if (swab == MagickFalse) \
    { \
      pixel=(unsigned short) (*(p) << 8); \
      pixel|=(unsigned short) *((p)+1); \
    } \
  else \
    { \
      pixel=(unsigned short) *(p); \
      pixel|=(unsigned short) (*((p)+1) << 8); \
    } \
  (p)+=2; \
}

  long
    bit;

  MagickBooleanType
    swab;

  MagickSizeType
    number_pixels;

  register const unsigned char
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  unsigned long
    lsb_first;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(source != (const unsigned char *) NULL);
  number_pixels=GetPixelCacheArea(image);
  swab=MagickFalse;
  lsb_first=1;
  if ((int) (*(char *) &lsb_first) != 0)
    {
      if (image->endian == LSBEndian)
        swab=MagickTrue;
    }
  else
    if (image->endian == LSBEndian)
      swab=MagickTrue;
  x=0;
  p=source;
  q=GetPixels(image);
  indexes=GetIndexes(image);
  switch (quantum_type)
  {
    case IndexQuantum:
    {
      if (image->storage_class != PseudoClass)
        ThrowBinaryException(ImageError,"ColormappedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-7); x+=8)
          {
            for (bit=0; bit < 8; bit++)
            {
              pixel=(unsigned char)
                (((*p) & (1 << (7-bit))) != 0 ? 0x01 : 0x00);
              indexes[x+bit]=PushColormapIndex(image,pixel);
              *q=image->colormap[indexes[x+bit]];
              q++;
            }
            p++;
          }
          for (bit=0; bit < (long) (number_pixels % 8); bit++)
          {
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x01 : 0x00);
            indexes[x+bit]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+bit]];
            q++;
          }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            pixel=(unsigned char) ((*p >> 6) & 0x03);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            q++;
            pixel=(unsigned char) ((*p >> 4) & 0x03);
            indexes[x+1]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+1]];
            q++;
            pixel=(unsigned char) ((*p >> 2) & 0x03);
            indexes[x+2]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+2]];
            q++;
            pixel=(unsigned char) ((*p) & 0x03);
            indexes[x+3]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+3]];
            p++;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 4); bit++)
          {
            pixel=(unsigned char) ((*p >> (2*(3-bit))) & 0x03);
            indexes[x+bit]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+bit]];
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-1); x+=2)
          {
            pixel=(unsigned char) ((*p >> 4) & 0xf);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            q++;
            pixel=(unsigned char) ((*p) & 0xf);
            indexes[x+1]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+1]];
            p++;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 2); bit++)
          {
            pixel=(unsigned char) ((*p++ >> 4) & 0xf);
            indexes[x+bit]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+bit]];
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            p+=pad;
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) (number_pixels-1); x+=2)
          {
            pixel=(unsigned short) ((((*(p+1) >> 4) & 0xf) << 8) | (*p));
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            q++;
            pixel=(unsigned short) (((*(p+1) & 0xf) << 8) | (*(p+2)));
            indexes[x+1]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+1]];
            p+=3;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 2); bit++)
          {
            pixel=(unsigned short) (((*(p+1) >> 4) & 0xf) | (*p));
            indexes[x+bit]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+bit]];
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case IndexAlphaQuantum:
    {
      if (image->storage_class != PseudoClass)
        ThrowBinaryException(ImageError,"ColormappedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            for (bit=0; bit < 8; bit+=2)
            {
              pixel=(unsigned char)
                (((*p) & (1 << (7-bit))) != 0 ? 0x01 : 0x00);
              indexes[x+bit]=PushColormapIndex(image,pixel);
              *q=image->colormap[indexes[x+bit]];
              pixel=(unsigned char)
                (((*p) & (1 << (8-bit))) != 0 ? 0x01 : 0x00);
              q->opacity=pixel == 0x00 ? TransparentOpacity : OpaqueOpacity;
              q++;
            }
            p++;
          }
          for (bit=0; bit < (long) (number_pixels % 4); bit+=2)
          {
            pixel=(unsigned char)
              (((*p) & (1UL << (unsigned char) (7-bit))) != 0 ? 0x01 : 0x00);
            indexes[x+bit]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+bit]];
            pixel=(unsigned char) (((*p) & (1 << (8-bit))) != 0 ? 0x01 : 0x00);
            q->opacity=pixel == 0x00 ? TransparentOpacity : OpaqueOpacity;
            q++;
          }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-1); x+=2)
          {
            pixel=(unsigned char) ((*p >> 6) & 0x03);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            q->opacity=(Quantum) (QuantumRange*((int) (*p >> 4) & 0x03)/4);
            q++;
            pixel=(unsigned char) ((*p >> 2) & 0x03);
            indexes[x+2]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x+2]];
            q->opacity=(Quantum) (QuantumRange*((int) (*p) & 0x03)/4);
            p++;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned char) ((*p >> 4) & 0xf);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            q->opacity=(Quantum) (QuantumRange-
              (QuantumRange*((int) (*p) & 0xf)/15));
            p++;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            PushCharPixel(p,pixel);
            q->opacity=(Quantum) (QuantumRange-ScaleCharToQuantum(pixel));
            p+=pad;
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned short) ((((*(p+1) >> 4) & 0xf) << 8) | (*p));
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            pixel=(unsigned short) (((*(p+1) & 0xf) << 8) | (*(p+2)));
            q->opacity=(Quantum) ((unsigned long) QuantumRange*pixel/1024);
            p+=3;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            PushShortPixel(p,pixel);
            q->opacity=QuantumRange-ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            indexes[x]=PushColormapIndex(image,pixel);
            *q=image->colormap[indexes[x]];
            PushLongPixel(p,pixel);
            q->opacity=QuantumRange-ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case GrayQuantum:
    case GrayPadQuantum:
    {
      switch (image->depth)
      {
        case 1:
        {
          for (x=0; x < ((long) number_pixels-7); x+=8)
          {
            for (bit=0; bit < 8; bit++)
            {
              q->red=(Quantum) (((*p) & (1 << (7-bit))) != 0 ? 0 : MaxRGB);
              q->green=q->red;
              q->blue=q->red;
              q++;
            }
            p++;
          }
          for (bit=0; bit < (long) (number_pixels % 8); bit++)
          {
            q->red=(Quantum) (((*p) & (1 << (7-bit))) != 0 ? 0 : MaxRGB);
            q->green=q->red;
            q->blue=q->red;
            q++;
          }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            pixel=(unsigned char) ((*p >> 6) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,1);
            q->green=q->red;
            q->blue=q->red;
            q++;
            pixel=(unsigned char) ((*p >> 4) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,1);
            q->green=q->red;
            q->blue=q->red;
            q++;
            pixel=(unsigned char) ((*p >> 2) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,1);
            q->green=q->red;
            q->blue=q->red;
            q++;
            pixel=(unsigned char) ((*p) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,1);
            q->green=q->red;
            q->blue=q->red;
            p++;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 4); bit++)
          {
            pixel=(unsigned char) ((*p >> (2*(3-bit))) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,1);
            q->green=q->red;
            q->blue=q->red;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-1); x+=2)
          {
            pixel=(unsigned char) ((*p >> 4) & 0xf);
            q->red=ScaleAnyToQuantum(pixel,15);
            q->green=q->red;
            q->blue=q->red;
            q++;
            pixel=(unsigned char) ((*p) & 0xf);
            q->red=ScaleAnyToQuantum(pixel,15);
            q->green=q->red;
            q->blue=q->red;
            p++;
            q++;
          }
          for (bit=0; bit < (long) (number_pixels % 2); bit++)
          {
            pixel=(unsigned char) ((*p++ >> 4) & 0xf);
            q->red=ScaleAnyToQuantum(pixel,15);
            q->green=q->red;
            q->blue=q->red;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->red=ScaleCharToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            p+=pad;
            q++;
          }
          break;
        }
        case 10:
        {
          register unsigned short
            pixel;

          if (quantum_type == GrayPadQuantum)
            {
              register unsigned long
                pixel;

              for (x=0; x < (long) number_pixels/3; x++)
              {
                PushLongPixel(p,pixel);
                q->red=ScaleAnyToQuantum((pixel >> 0) & 0x3ff,1023);
                q->green=q->red;
                q->blue=q->red;
                q++;
                q->red=ScaleAnyToQuantum((pixel >> 10) & 0x3ff,1023);
                q->green=q->red;
                q->blue=q->red;
                q++;
                q->red=ScaleAnyToQuantum((pixel >> 20) & 0x3ff,1023);
                q->green=q->red;
                q->blue=q->red;
                p+=pad;
                q++;
              }
              break;
            }
          (void) PushPixelQuantum(p,0);
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->red=ScaleAnyToQuantum(pixel,(1UL << image->depth)-1);
            q->green=q->red;
            q->blue=q->red;
            p+=pad;
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned short
            pixel;

          if (quantum_type == GrayPadQuantum)
            {
              for (x=0; x < (long) (number_pixels-1); x+=2)
              {
                PushShortPixel(p,pixel);
                q->red=ScaleAnyToQuantum(pixel >> 4,4095);
                q->green=q->red;
                q->blue=q->red;
                q++;
                PushShortPixel(p,pixel);
                q->red=ScaleAnyToQuantum(pixel >> 4,4095);
                q->green=q->red;
                q->blue=q->red;
                p+=pad;
                q++;
              }
              for (bit=0; bit < (long) (number_pixels % 2); bit++)
              {
                PushShortPixel(p,pixel);
                q->red=ScaleAnyToQuantum(pixel >> 4,4095);
                q->green=q->red;
                q->blue=q->red;
                p+=pad;
                q++;
              }
              break;
            }
          (void) PushPixelQuantum(p,0);
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->red=ScaleAnyToQuantum(pixel,(1UL << image->depth)-1);
            q->green=q->red;
            q->blue=q->red;
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->red=ScaleShortToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->red=ScaleLongToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            p+=pad;
            q++;
          }
          break;
        }
        case 64:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned long) (QuantumRange*(*((const double *) p))+0.5);
            p+=sizeof(double);
            q->red=ScaleLongToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case GrayAlphaQuantum:
    {
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            for (bit=0; bit < 8; bit+=2)
            {
              pixel=(unsigned char)
                (((*p) & (1 << (7-bit))) != 0 ? 0x01 : 0x00);
              q->red=pixel == 0 ? 0 : MaxRGB;
              q->green=q->red;
              q->blue=q->red;
              q->opacity=((*p) & (1UL << (unsigned char) (6-bit))) != 0 ?
                TransparentOpacity : OpaqueOpacity;
              q++;
            }
            p++;
          }
          for (bit=0; bit < (long) (number_pixels % 4); bit+=2)
          {
            pixel=(unsigned char) (((*p) & (1 << (7-bit))) != 0 ? 0x01 : 0x00);
            q->red=pixel == 0 ? 0 : MaxRGB;
            q->green=q->red;
            q->blue=q->red;
            q->opacity=((*p) & (1UL << (unsigned char) (6-bit))) != 0 ?
              TransparentOpacity : OpaqueOpacity;
            q++;
          }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-1); x+=2)
          {
            pixel=(unsigned char) ((*p >> 6) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,1);
            q->green=q->red;
            q->blue=q->red;
            q->opacity=(Quantum) (QuantumRange*((int) (*p >> 4) & 0x03)/4);
            q++;
            pixel=(unsigned char) ((*p >> 2) & 0x03);
            q->red=ScaleAnyToQuantum(pixel,1);
            q->green=q->red;
            q->blue=q->red;
            q->opacity=(Quantum) (QuantumRange*((int) (*p) & 0x03)/4);
            p++;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned char) ((*p >> 4) & 0xf);
            q->red=ScaleAnyToQuantum(pixel,15);
            q->green=q->red;
            q->blue=q->red;
            q->opacity=(Quantum) (QuantumRange-(QuantumRange*((*p) & 0xf)/15));
            p++;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->red=ScaleCharToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            PushCharPixel(p,pixel);
            q->opacity=(Quantum) (QuantumRange-ScaleCharToQuantum(pixel));
            p+=pad;
            q++;
          }
          break;
        }
        case 10:
        {
          register unsigned short
            pixel;

          (void) PushPixelQuantum(p,0);
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->red=ScaleAnyToQuantum(pixel,(1UL << image->depth)-1);
            q->green=q->red;
            q->blue=q->red;
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->opacity=ScaleAnyToQuantum(pixel,(1UL << image->depth)-1);
            p+=pad;
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned short
            pixel;

          (void) PushPixelQuantum(p,0);
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->red=ScaleAnyToQuantum(pixel,(1UL << image->depth)-1);
            q->green=q->red;
            q->blue=q->red;
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->opacity=ScaleAnyToQuantum(pixel,(1UL << image->depth)-1);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->red=ScaleShortToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            PushShortPixel(p,pixel);
            q->opacity=QuantumRange-ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->red=ScaleLongToQuantum(pixel);
            q->green=q->red;
            q->blue=q->red;
            PushLongPixel(p,pixel);
            q->opacity=QuantumRange-ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case RedQuantum:
    case CyanQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->red=ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->red=ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->red=ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->green=ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->green=ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->green=ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->blue=ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->blue=ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->blue=ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case AlphaQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->opacity=QuantumRange-ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->opacity=QuantumRange-ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->opacity=QuantumRange-ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            indexes[x]=ScaleCharToQuantum(pixel);
            p+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            indexes[x]=ScaleShortToQuantum(pixel);
            p+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            indexes[x]=ScaleLongToQuantum(pixel);
            p+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case RGBQuantum:
    case RGBPadQuantum:
    default:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->red=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->green=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->blue=ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 10:
        {
          register unsigned long
            pixel;

          if (quantum_type == RGBPadQuantum)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                PushLongPixel(p,pixel);
                q->red=ScaleShortToQuantum(((pixel >> 22) & 0x3ff) << 6);
                q->green=ScaleShortToQuantum(((pixel >> 12) & 0x3ff) << 6);
                q->blue=ScaleShortToQuantum(((pixel >> 2) & 0x3ff) << 6);
                p+=pad;
                q++;
              }
              break;
            }
          (void) PushPixelQuantum(p,0);
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=PushPixelQuantum(p,image->depth);
            q->red=ScaleShortToQuantum(pixel << 6);
            pixel=PushPixelQuantum(p,image->depth);
            q->green=ScaleShortToQuantum(pixel << 6);
            pixel=PushPixelQuantum(p,image->depth);
            q->blue=ScaleShortToQuantum(pixel << 6);
            q++;
          }
          break;
        }
        case 12:
        {
          register unsigned short
            pixel;

          if (quantum_type == RGBPadQuantum)
            {
              for (x=0; x < (long) (3*number_pixels-1); x+=2)
              {
                PushShortPixel(p,pixel);
                switch (x % 3)
                {
                  default:
                  case 0: q->red=ScaleShortToQuantum(pixel &~ 0xf); break;
                  case 1: q->green=ScaleShortToQuantum(pixel &~ 0xf); break;
                  case 2: q->blue=ScaleShortToQuantum(pixel &~ 0xf); q++; break;
                }
                PushShortPixel(p,pixel);
                switch ((x+1) % 3)
                {
                  default:
                  case 0: q->red=ScaleShortToQuantum(pixel &~ 0xf); break;
                  case 1: q->green=ScaleShortToQuantum(pixel &~ 0xf); break;
                  case 2: q->blue=ScaleShortToQuantum(pixel &~ 0xf); q++; break;
                }
                p+=pad;
              }
              for (bit=0; bit < (long) (3*number_pixels % 2); bit++)
              {
                PushShortPixel(p,pixel);
                switch ((x+bit) % 3)
                {
                  default:
                  case 0: q->red=ScaleShortToQuantum(pixel &~ 0xf); break;
                  case 1: q->green=ScaleShortToQuantum(pixel &~ 0xf); break;
                  case 2: q->blue=ScaleShortToQuantum(pixel &~ 0xf); q++; break;
                }
                p+=pad;
              }
              break;
            }
          (void) PushPixelQuantum(p,0);
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->red=ScaleShortToQuantum(pixel << 4);
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->green=ScaleShortToQuantum(pixel << 4);
            pixel=(unsigned short) PushPixelQuantum(p,image->depth);
            q->blue=ScaleShortToQuantum(pixel << 4);
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->red=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->green=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->blue=ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->red=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->green=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->blue=ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case RGBAQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->red=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->green=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->blue=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->opacity=QuantumRange-ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->red=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->green=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->blue=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->opacity=QuantumRange-ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->red=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->green=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->blue=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->opacity=QuantumRange-ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case RGBOQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->red=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->green=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->blue=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->opacity=ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->red=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->green=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->blue=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->opacity=ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->red=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->green=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->blue=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->opacity=ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case CMYKQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->red=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->green=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->blue=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            indexes[x]=ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->red=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->green=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->blue=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            indexes[x]=ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->red=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->green=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->blue=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            indexes[x]=ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case CMYKAQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushCharPixel(p,pixel);
            q->red=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->green=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->blue=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            indexes[x]=ScaleCharToQuantum(pixel);
            PushCharPixel(p,pixel);
            q->opacity=QuantumRange-ScaleCharToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushShortPixel(p,pixel);
            q->red=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->green=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->blue=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            indexes[x]=ScaleShortToQuantum(pixel);
            PushShortPixel(p,pixel);
            q->opacity=QuantumRange-ScaleShortToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PushLongPixel(p,pixel);
            q->red=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->green=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->blue=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            indexes[x]=ScaleLongToQuantum(pixel);
            PushLongPixel(p,pixel);
            q->opacity=QuantumRange-ScaleLongToQuantum(pixel);
            p+=pad;
            q++;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m p o r t I m a g e P i x e l s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImportImagePixels() accepts pixel data and stores in the image at the
%  location you specify.  The method returns MagickTrue on success otherwise
%  MagickFalse if an error is encountered.  The pixel data can be either char,
%  short int, int, long, float, or double in the order specified by map.
%
%  Suppose your want want to upload the first scanline of a 640x480 image from
%  character data in red-green-blue order:
%
%      ImportImagePixels(image,0,0,640,1,"RGB",CharPixel,pixels);
%
%  The format of the ImportImagePixels method is:
%
%      MagickBooleanType ImportImagePixels(Image *image,const long x_offset,
%        const long y_offset,const unsigned long columns,
%        const unsigned long rows,const char *map,const StorageType type,
%        const void *pixels)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x_offset,y_offset,columns,rows:  These values define the perimeter
%      of a region of pixels you want to define.
%
%    o map:  This string reflects the expected ordering of the pixel array.
%      It can be any combination or order of R = red, G = green, B = blue,
%      A = alpha (0 is transparent), O = opacity (0 is opaque), C = cyan,
%      Y = yellow, M = magenta, K = black, I = intensity (for grayscale),
%      P = pad.
%
%    o type: Define the data type of the pixels.  Float and double types are
%      normalized to [0..1] otherwise [0..QuantumRange].  Choose from these types:
%      CharPixel, ShortPixel, IntegerPixel, LongPixel, FloatPixel, or
%      DoublePixel.
%
%    o pixels: This array of values contain the pixel components as defined by
%      map and type.  You must preallocate this array where the expected
%      length varies depending on the values of width, height, map, and type.
%
%
*/
MagickExport MagickBooleanType ImportImagePixels(Image *image,
  const long x_offset,const long y_offset,const unsigned long columns,
  const unsigned long rows,const char *map,const StorageType type,
  const void *pixels)
{
  long
    y;

  PixelPacket
    *q;

  QuantumType
    *quantum_map;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  size_t
    length;

  /*
    Allocate image structure.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  length=strlen(map);
  quantum_map=(QuantumType *) AcquireMagickMemory(length*sizeof(*quantum_map));
  if (quantum_map == (QuantumType *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  image->storage_class=DirectClass;
  for (i=0; i < (long) length; i++)
  {
    switch (map[i])
    {
      case 'a':
      case 'A':
      {
        quantum_map[i]=AlphaQuantum;
        image->matte=MagickTrue;
        break;
      }
      case 'B':
      case 'b':
      {
        quantum_map[i]=BlueQuantum;
        break;
      }
      case 'C':
      case 'c':
      {
        quantum_map[i]=CyanQuantum;
        image->colorspace=CMYKColorspace;
        break;
      }
      case 'g':
      case 'G':
      {
        quantum_map[i]=GreenQuantum;
        break;
      }
      case 'K':
      case 'k':
      {
        quantum_map[i]=BlackQuantum;
        image->colorspace=CMYKColorspace;
        break;
      }
      case 'I':
      case 'i':
      {
        quantum_map[i]=IndexQuantum;
        break;
      }
      case 'm':
      case 'M':
      {
        quantum_map[i]=MagentaQuantum;
        image->colorspace=CMYKColorspace;
        break;
      }
      case 'O':
      case 'o':
      {
        quantum_map[i]=OpacityQuantum;
        image->matte=MagickTrue;
        break;
      }
      case 'P':
      case 'p':
      {
        quantum_map[i]=UndefinedQuantum;
        break;
      }
      case 'R':
      case 'r':
      {
        quantum_map[i]=RedQuantum;
        break;
      }
      case 'Y':
      case 'y':
      {
        quantum_map[i]=YellowQuantum;
        image->colorspace=CMYKColorspace;
        break;
      }
      default:
      {
        quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
        (void) ThrowMagickException(&image->exception,GetMagickModule(),
          OptionError,"UnrecognizedPixelMap","`%s'",map);
        return(MagickFalse);
      }
    }
  }
  /*
    Transfer the pixels from the pixel data array to the image.
  */
  switch (type)
  {
    case CharPixel:
    {
      register const unsigned char
        *p;

      p=(const unsigned char *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleCharToQuantum(*p++);
              q->green=ScaleCharToQuantum(*p++);
              q->red=ScaleCharToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleCharToQuantum(*p++);
              q->green=ScaleCharToQuantum(*p++);
              q->red=ScaleCharToQuantum(*p++);
              q->opacity=QuantumRange-ScaleCharToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRO") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleCharToQuantum(*p++);
              q->green=ScaleCharToQuantum(*p++);
              q->red=ScaleCharToQuantum(*p++);
              q->opacity=ScaleCharToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleCharToQuantum(*p++);
              q->green=ScaleCharToQuantum(*p++);
              q->red=ScaleCharToQuantum(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleCharToQuantum(*p++);
              q->green=q->red;
              q->blue=q->red;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleCharToQuantum(*p++);
              q->green=ScaleCharToQuantum(*p++);
              q->blue=ScaleCharToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleCharToQuantum(*p++);
              q->green=ScaleCharToQuantum(*p++);
              q->blue=ScaleCharToQuantum(*p++);
              q->opacity=QuantumRange-ScaleCharToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBO") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleCharToQuantum(*p++);
              q->green=ScaleCharToQuantum(*p++);
              q->blue=ScaleCharToQuantum(*p++);
              q->opacity=ScaleCharToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleCharToQuantum(*p++);
              q->green=ScaleCharToQuantum(*p++);
              q->blue=ScaleCharToQuantum(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                q->red=ScaleCharToQuantum(*p);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                q->green=ScaleCharToQuantum(*p);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                q->blue=ScaleCharToQuantum(*p);
                break;
              }
              case AlphaQuantum:
              {
                q->opacity=QuantumRange-ScaleCharToQuantum(*p);
                break;
              }
              case OpacityQuantum:
              {
                q->opacity=ScaleCharToQuantum(*p);
                break;
              }
              case BlackQuantum:
              {
                indexes[x]=ScaleCharToQuantum(*p);
                break;
              }
              case IndexQuantum:
              {
                q->red=ScaleCharToQuantum(*p);
                q->green=q->red;
                q->blue=q->red;
                break;
              }
              default:
                break;
            }
            p++;
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      break;
    }
    case FloatPixel:
    {
      register const float
        *p;

      p=(const float *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->opacity=QuantumRange-RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              q->green=q->red;
              q->blue=q->red;
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->opacity=QuantumRange-RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case AlphaQuantum:
              {
                q->opacity=QuantumRange-RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case OpacityQuantum:
              {
                q->opacity=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case BlackQuantum:
              {
                indexes[x]=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case IndexQuantum:
              {
                q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                q->green=q->red;
                q->blue=q->red;
                break;
              }
              default:
                break;
            }
            p++;
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      break;
    }
    case DoublePixel:
    {
      register const double
        *p;

      p=(const double *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->opacity=QuantumRange-RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              q->green=q->red;
              q->blue=q->red;
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->opacity=QuantumRange-RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                q->green=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                q->blue=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case AlphaQuantum:
              {
                q->opacity=QuantumRange-RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case OpacityQuantum:
              {
                q->opacity=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case BlackQuantum:
              {
                indexes[x]=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                break;
              }
              case IndexQuantum:
              {
                q->red=RoundToQuantum((MagickRealType) QuantumRange*(*p));
                q->green=q->red;
                q->blue=q->red;
                break;
              }
              default:
                break;
            }
            p++;
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      break;
    }
    case IntegerPixel:
    {
      register const unsigned int
        *p;

      p=(const unsigned int *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->red=ScaleLongToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->red=ScaleLongToQuantum(*p++);
              q->opacity=QuantumRange-ScaleLongToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->red=ScaleLongToQuantum(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleLongToQuantum(*p++);
              q->green=q->red;
              q->blue=q->red;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->blue=ScaleLongToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->blue=ScaleLongToQuantum(*p++);
              q->opacity=QuantumRange-ScaleLongToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->blue=ScaleLongToQuantum(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                q->red=ScaleLongToQuantum(*p);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                q->green=ScaleLongToQuantum(*p);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                q->blue=ScaleLongToQuantum(*p);
                break;
              }
              case AlphaQuantum:
              {
                q->opacity=QuantumRange-ScaleLongToQuantum(*p);
                break;
              }
              case OpacityQuantum:
              {
                q->opacity=ScaleLongToQuantum(*p);
                break;
              }
              case BlackQuantum:
              {
                indexes[x]=ScaleLongToQuantum(*p);
                break;
              }
              case IndexQuantum:
              {
                q->red=ScaleLongToQuantum(*p);
                q->green=q->red;
                q->blue=q->red;
                break;
              }
              default:
                break;
            }
            p++;
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      break;
    }
    case LongPixel:
    {
      register const unsigned long
        *p;

      p=(const unsigned long *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->red=ScaleLongToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->red=ScaleLongToQuantum(*p++);
              q->opacity=QuantumRange-ScaleLongToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->red=ScaleLongToQuantum(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleLongToQuantum(*p++);
              q->green=q->red;
              q->blue=q->red;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->blue=ScaleLongToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->blue=ScaleLongToQuantum(*p++);
              q->opacity=QuantumRange-ScaleLongToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleLongToQuantum(*p++);
              q->green=ScaleLongToQuantum(*p++);
              q->blue=ScaleLongToQuantum(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                q->red=ScaleLongToQuantum(*p);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                q->green=ScaleLongToQuantum(*p);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                q->blue=ScaleLongToQuantum(*p);
                break;
              }
              case AlphaQuantum:
              {
                q->opacity=QuantumRange-ScaleLongToQuantum(*p);
                break;
              }
              case OpacityQuantum:
              {
                q->opacity=ScaleLongToQuantum(*p);
                break;
              }
              case BlackQuantum:
              {
                indexes[x]=ScaleLongToQuantum(*p);
                break;
              }
              case IndexQuantum:
              {
                q->red=ScaleLongToQuantum(*p);
                q->green=q->red;
                q->blue=q->red;
                break;
              }
              default:
                break;
            }
            p++;
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      break;
    }
    case QuantumPixel:
    {
      register const Quantum
        *p;

      p=(const Quantum *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=(*p++);
              q->green=(*p++);
              q->red=(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=(*p++);
              q->green=(*p++);
              q->red=(*p++);
              q->opacity=QuantumRange-(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=(*p++);
              q->green=(*p++);
              q->red=(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=(*p++);
              q->green=q->red;
              q->blue=q->red;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=(*p++);
              q->green=(*p++);
              q->blue=(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=(*p++);
              q->green=(*p++);
              q->blue=(*p++);
              q->opacity=QuantumRange-(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=(*p++);
              q->green=(*p++);
              q->blue=(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                q->red=(*p);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                q->green=(*p);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                q->blue=(*p);
                break;
              }
              case AlphaQuantum:
              {
                q->opacity=QuantumRange-(*p);
                break;
              }
              case OpacityQuantum:
              {
                q->opacity=(*p);
                break;
              }
              case BlackQuantum:
              {
                indexes[x]=(*p);
                break;
              }
              case IndexQuantum:
              {
                q->red=(*p);
                q->green=q->red;
                q->blue=q->red;
                break;
              }
              default:
                break;
            }
            p++;
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      break;
    }
    case ShortPixel:
    {
      register const unsigned short
        *p;

      p=(const unsigned short *) pixels;
      if (LocaleCompare(map,"BGR") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleShortToQuantum(*p++);
              q->green=ScaleShortToQuantum(*p++);
              q->red=ScaleShortToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleShortToQuantum(*p++);
              q->green=ScaleShortToQuantum(*p++);
              q->red=ScaleShortToQuantum(*p++);
              q->opacity=QuantumRange-ScaleShortToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"BGRP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->blue=ScaleShortToQuantum(*p++);
              q->green=ScaleShortToQuantum(*p++);
              q->red=ScaleShortToQuantum(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"I") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleShortToQuantum(*p++);
              q->green=q->red;
              q->blue=q->red;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGB") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleShortToQuantum(*p++);
              q->green=ScaleShortToQuantum(*p++);
              q->blue=ScaleShortToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBA") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleShortToQuantum(*p++);
              q->green=ScaleShortToQuantum(*p++);
              q->blue=ScaleShortToQuantum(*p++);
              q->opacity=QuantumRange-ScaleShortToQuantum(*p++);
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      if (LocaleCompare(map,"RGBP") == 0)
        {
          for (y=0; y < (long) rows; y++)
          {
            q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) columns; x++)
            {
              q->red=ScaleShortToQuantum(*p++);
              q->green=ScaleShortToQuantum(*p++);
              q->blue=ScaleShortToQuantum(*p++);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          break;
        }
      for (y=0; y < (long) rows; y++)
      {
        q=GetImagePixels(image,x_offset,y_offset+y,columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) columns; x++)
        {
          for (i=0; i < (long) length; i++)
          {
            switch (quantum_map[i])
            {
              case RedQuantum:
              case CyanQuantum:
              {
                q->red=ScaleShortToQuantum(*p);
                break;
              }
              case GreenQuantum:
              case MagentaQuantum:
              {
                q->green=ScaleShortToQuantum(*p);
                break;
              }
              case BlueQuantum:
              case YellowQuantum:
              {
                q->blue=ScaleShortToQuantum(*p);
                break;
              }
              case AlphaQuantum:
              {
                q->opacity=QuantumRange-ScaleShortToQuantum(*p);
                break;
              }
              case OpacityQuantum:
              {
                q->opacity=ScaleShortToQuantum(*p);
                break;
              }
              case BlackQuantum:
              {
                indexes[x]=ScaleShortToQuantum(*p);
                break;
              }
              case IndexQuantum:
              {
                q->red=ScaleShortToQuantum(*p);
                q->green=q->red;
                q->blue=q->red;
                break;
              }
              default:
                break;
            }
            p++;
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      break;
    }
    default:
    {
      quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        OptionError,"UnrecognizedPixelMap","`%s'",map);
      break;
    }
  }
  quantum_map=(QuantumType *) RelinquishMagickMemory(quantum_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I m p o r t Q u a n t u m P i x e l s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImportQuantumPixels() transfers one or more pixel components from the image
%  pixel cache to a user supplied buffer.  The pixels are returned in network
%  byte order.  MagickTrue is returned if the pixels are successfully
%  transferred, otherwise MagickFalse.
%
%  The format of the ImportQuantumPixels method is:
%
%      MagickBooleanType ImportQuantumPixels(Image *,
%        const QuantumType quantum_type,const size_t pad,
%        unsigned char *destination)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o quantum_type: Declare which pixel components to transfer (RGB, RGBA,
%      etc).
%
%    o pad: skip components.
%
%    o destination:  The components are transferred to this buffer.
%
%
*/

static inline void PopPixelQuantum(const unsigned long depth,
  const unsigned long quantum,unsigned char *pixels)
{
  register long
    i;

  register unsigned long
    data_bits;

  static unsigned char
    *p;

	static unsigned long
    quantum_bits;

  if (depth == 0UL)
    {
      p=pixels;
      quantum_bits=8UL;
    }
  for (i=(long) depth; i > 0L; )
  {
    data_bits=(unsigned long) i;
    if (data_bits > quantum_bits)
      data_bits=quantum_bits;
    i-=data_bits;
    if (quantum_bits == 8)
      *p='\0';
    quantum_bits-=data_bits;
    *p|=(((quantum >> i) &~ ((~0UL) << data_bits)) << quantum_bits);
    if (quantum_bits == 0UL)
      {
        p++;
        quantum_bits=8UL;
      }
  }
}

MagickExport MagickBooleanType ImportQuantumPixels(Image *image,
  const QuantumType quantum_type,const size_t pad,unsigned char *destination)
{
#define PopCharPixel(pixel,q) \
{ \
  *(q)++=(unsigned char) (pixel); \
}
#define PopLongPixel(pixel,q) \
{ \
  if (swab == MagickFalse) \
    { \
      *(q)++=(unsigned char) ((pixel) >> 24); \
      *(q)++=(unsigned char) ((pixel) >> 16); \
      *(q)++=(unsigned char) ((pixel) >> 8); \
      *(q)++=(unsigned char) (pixel); \
    } \
  else \
    { \
      *(q)++=(unsigned char) (pixel); \
      *(q)++=(unsigned char) ((pixel) >> 8); \
      *(q)++=(unsigned char) ((pixel) >> 16); \
      *(q)++=(unsigned char) ((pixel) >> 24); \
    } \
}
#define PopShortPixel(pixel,q) \
{ \
  if (swab == MagickFalse) \
    { \
      *(q)++=(unsigned char) ((pixel) >> 8); \
      *(q)++=(unsigned char) (pixel); \
    } \
  else \
    { \
      *(q)++=(unsigned char) (pixel); \
      *(q)++=(unsigned char) ((pixel) >> 8); \
    } \
}

  long
    bit;

  MagickBooleanType
    swab;

  MagickSizeType
    number_pixels;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *p;

  register unsigned char
    *q;

  unsigned long
    lsb_first;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(destination != (unsigned char *) NULL);
  swab=MagickFalse;
  lsb_first=1;
  if ((int) (*(char *) &lsb_first) != 0)
    {
      if (image->endian == LSBEndian)
        swab=MagickTrue;
    }
  else
    if (image->endian == LSBEndian)
      swab=MagickTrue;
  number_pixels=GetPixelCacheArea(image);
  x=0;
  p=GetPixels(image);
  indexes=GetIndexes(image);
  q=destination;
  switch (quantum_type)
  {
    case IndexQuantum:
    {
      if (image->storage_class != PseudoClass)
        ThrowBinaryException(ImageError,"ColormappedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=((long) number_pixels-7); x > 0; x-=8)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0x01) << 7);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 6);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 5);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 4);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 3);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 2);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 1);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 0);
            q++;
          }
          if ((number_pixels % 8) != 0)
            {
              *q='\0';
              for (bit=7; bit >= (long) (8-(number_pixels % 8)); bit--)
              {
                pixel=(unsigned char) *indexes++;
                *q|=((pixel & 0x01) << (unsigned char) bit);
              }
              q++;
            }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0x03) << 6);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x03) << 4);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x03) << 2);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x03) << 0);
            q++;
          }
          if ((number_pixels % 4) != 0)
            {
              *q='\0';
              for (i=3; i >= (4-((long) number_pixels % 4)); i--)
              {
                pixel=(unsigned char) *indexes++;
                *q|=((pixel & 0x03) << ((unsigned char) i*2));
              }
              q++;
            }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) (number_pixels-1) ; x+=2)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0xf) << 4);
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0xf) << 0);
            q++;
          }
          if ((number_pixels % 2) != 0)
            {
              pixel=(unsigned char) *indexes++;
              *q=((pixel & 0xf) << 4);
              q++;
            }
          break;
        }
        case 8:
        {
          for (x=0; x < (long) number_pixels; x++)
          {
            PopCharPixel(indexes[x],q);
            q+=pad;
          }
          break;
        }
        case 16:
        {
          for (x=0; x < (long) number_pixels; x++)
          {
            PopShortPixel(indexes[x],q);
            q+=pad;
          }
          break;
        }
        case 32:
        {
          for (x=0; x < (long) number_pixels; x++)
          {
            PopLongPixel(indexes[x],q);
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case IndexAlphaQuantum:
    {
      if (image->storage_class != PseudoClass)
        ThrowBinaryException(ImageError,"ColormappedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=((long) number_pixels-3); x > 0; x-=4)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0x01) << 7);
            pixel=(unsigned char) (p->opacity == TransparentOpacity);
            *q|=((pixel & 0x01) << 6);
            p++;
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 5);
            pixel=(unsigned char) (p->opacity == TransparentOpacity);
            *q|=((pixel & 0x01) << 4);
            p++;
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 3);
            pixel=(unsigned char) (p->opacity == TransparentOpacity);
            *q|=((pixel & 0x01) << 2);
            p++;
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x01) << 1);
            pixel=(unsigned char) (p->opacity == TransparentOpacity);
            *q|=((pixel & 0x01) << 0);
            p++;
            q++;
          }
          if ((number_pixels % 4) != 0)
            {
              *q='\0';
              for (bit=3; bit >= (long) (4-(number_pixels % 4)); bit-=2)
              {
                pixel=(unsigned char) *indexes++;
                *q|=((pixel & 0x01) << (unsigned char) bit);
                pixel=(unsigned char) (p->opacity == TransparentOpacity);
                *q|=((pixel & 0x01) << (unsigned char) (bit-1));
                p++;
              }
              q++;
            }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0x03) << 6);
            pixel=(unsigned char) (4*QuantumScale*p->opacity+0.5);
            *q|=((pixel & 0x03) << 4);
            p++;
            pixel=(unsigned char) *indexes++;
            *q|=((pixel & 0x03) << 2);
            pixel=(unsigned char) (4*QuantumScale*p->opacity+0.5);
            *q|=((pixel & 0x03) << 0);
            p++;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels ; x++)
          {
            pixel=(unsigned char) *indexes++;
            *q=((pixel & 0xf) << 4);
            pixel=(unsigned char)
              (16*QuantumScale*(QuantumRange-p->opacity)+0.5);
            *q|=((pixel & 0xf) << 0);
            p++;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopCharPixel(indexes[x],q);
            pixel=ScaleQuantumToChar(QuantumRange-p->opacity);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopShortPixel(indexes[x],q);
            pixel=ScaleQuantumToShort(QuantumRange-p->opacity);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            PopLongPixel(indexes[x],q);
            pixel=ScaleQuantumToLong(QuantumRange-p->opacity);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case GrayQuantum:
    case GrayPadQuantum:
    {
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=((long) number_pixels-7); x > 0; x-=8)
          {
            *q='\0';
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((int) (int) pixel != 0 ? 0x00 : 0x01) << 7;
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((int) pixel != 0 ? 0x00 : 0x01) << 6;
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((int) pixel != 0 ? 0x00 : 0x01) << 5;
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((int) pixel != 0 ? 0x00 : 0x01) << 4;
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((int) pixel != 0 ? 0x00 : 0x01) << 3;
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((int) pixel != 0 ? 0x00 : 0x01) << 2;
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((int) pixel != 0 ? 0x00 : 0x01) << 1;
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((int) pixel != 0 ? 0x00 : 0x01) << 0;
            p++;
            q++;
          }
          if ((number_pixels % 8) != 0)
            {
              *q='\0';
              for (bit=7; bit >= (long) (8-(number_pixels % 8)); bit--)
              {
                pixel=(unsigned char) PixelIntensityToQuantum(p);
                *q|=(((int) pixel != 0 ? 0x00 : 0x01) << bit);
                p++;
              }
              q++;
            }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < ((long) number_pixels-3); x+=4)
          {
            *q='\0';
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((pixel & 0x03) << 6);
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((pixel & 0x03) << 4);
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((pixel & 0x03) << 2);
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((pixel & 0x03));
            p++;
            q++;
          }
          if ((number_pixels % 4) != 0)
            {
              *q='\0';
              for (i=3; i >= (4-((long) number_pixels % 4)); i--)
              {
                pixel=(unsigned char) PixelIntensityToQuantum(p);
                *q|=(pixel << ((unsigned char) i*2));
                p++;
              }
              q++;
            }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) (number_pixels-1) ; x+=2)
          {
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q=((pixel & 0xf) << 4);
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((pixel & 0xf) << 0);
            p++;
            q++;
          }
          if ((number_pixels % 2) != 0)
            {
              pixel=(unsigned char) PixelIntensityToQuantum(p);
              *q=((pixel & 0xf) << 4);
              p++;
              q++;
            }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 10:
        {
          register unsigned short
            pixel;

          PopPixelQuantum(0,0,q);
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(PixelIntensityToQuantum(p));
            PopPixelQuantum(image->depth,
              ScaleQuantumToAny(pixel,(1UL << image->depth)-1),q);
            p++;
            q+=pad;
          }
          break;
        }
        case 12:
        {
          register unsigned short
            pixel;

          PopPixelQuantum(0,0,q);
          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(PixelIntensityToQuantum(p));
            PopPixelQuantum(image->depth,
              ScaleQuantumToAny(pixel,(1UL << image->depth)-1),q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(PixelIntensityToQuantum(p));
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(PixelIntensityToQuantum(p));
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case GrayAlphaQuantum:
    {
      switch (image->depth)
      {
        case 1:
        {
          register unsigned char
            pixel;

          for (x=((long) number_pixels-3); x > 0; x-=4)
          {
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q=(unsigned char) (((int) pixel != 0 ? 0x00 : 0x01) << 7);
            pixel=(unsigned char) (p->opacity != TransparentOpacity);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 6);
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 5);
            pixel=(unsigned char) (p->opacity != TransparentOpacity);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 4);
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 3);
            pixel=(unsigned char) (p->opacity != TransparentOpacity);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 2);
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 1);
            pixel=(unsigned char) (p->opacity != TransparentOpacity);
            *q|=(((int) pixel != 0 ? 0x00 : 0x01) << 0);
            p++;
            q++;
          }
          if ((number_pixels % 4) != 0)
            {
              *q='\0';
              for (bit=3; bit >= (long) (4-(number_pixels % 4)); bit-=2)
              {
                pixel=(unsigned char) PixelIntensityToQuantum(p);
                *q|=(((int) pixel != 0 ? 0x00 : 0x01) << (unsigned char) bit);
                pixel=(unsigned char) (p->opacity != TransparentOpacity);
                *q|=(((int) pixel != 0 ? 0x00 : 0x01) <<
                  (unsigned char) (bit-1));
                p++;
              }
              q++;
            }
          break;
        }
        case 2:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q=((pixel & 0x03) << 6);
            pixel=(unsigned char) (4*QuantumScale*p->opacity+0.5);
            *q|=((pixel & 0x03) << 4);
            p++;
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q|=((pixel & 0x03) << 2);
            pixel=(unsigned char) (4*QuantumScale*p->opacity+0.5);
            *q|=((pixel & 0x03) << 0);
            p++;
            q++;
          }
          break;
        }
        case 4:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels ; x++)
          {
            pixel=(unsigned char) PixelIntensityToQuantum(p);
            *q=((pixel & 0xf) << 4);
            pixel=(unsigned char)
              (16*QuantumScale*(QuantumRange-p->opacity)+0.5);
            *q|=((pixel & 0xf) << 0);
            p++;
            q++;
          }
          break;
        }
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(PixelIntensityToQuantum(p));
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(QuantumRange-p->opacity);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(PixelIntensityToQuantum(p));
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(QuantumRange-p->opacity);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(PixelIntensityToQuantum(p));
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(QuantumRange-p->opacity);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case RedQuantum:
    case CyanQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case GreenQuantum:
    case MagentaQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case BlueQuantum:
    case YellowQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case AlphaQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(QuantumRange-p->opacity);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel= ScaleQuantumToShort(QuantumRange-p->opacity);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(QuantumRange-p->opacity);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case BlackQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(indexes[x]);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel= ScaleQuantumToShort(indexes[x]);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(indexes[x]);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case RGBQuantum:
    case RGBPadQuantum:
    default:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 10:
        {
          register unsigned long
            pixel;

          if (quantum_type == RGBPadQuantum)
            {
              for (x=0; x < (long) number_pixels; x++)
              {
                pixel=(unsigned long) (((long) ((1023L*p->red+QuantumRange/2)/
                  QuantumRange) & 0x3ff) << 22) | (((long) ((1023L*p->green+
                  QuantumRange/2)/QuantumRange) & 0x3ff) << 12) | (((long)
                  ((1023L*p->blue+QuantumRange/2)/QuantumRange) & 0x3ff) << 2);
                PopLongPixel(pixel,q);
                p++;
                q+=pad;
              }
              break;
            }
          PopPixelQuantum(0,0,q);
          for (x=0; x < (long) number_pixels; x++)
          {
            PopPixelQuantum(image->depth,
              ScaleQuantumToShort(p->red) >> 6UL,q);
            PopPixelQuantum(image->depth,
              ScaleQuantumToShort(p->green) >> 6UL,q);
            PopPixelQuantum(image->depth,
              ScaleQuantumToShort(p->blue) >> 6UL,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 12:
        {
          if (quantum_type == RGBPadQuantum)
            {
              register unsigned short
                pixel;

              for (x=0; x < (long) (3*number_pixels-1); x+=2)
              {
                switch (x % 3)
                {
                  default:
                  case 0:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->red) >> 4);
                    break;
                  }
                  case 1:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->green) >> 4);
                    break;
                  }
                  case 2:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->green) >> 4);
									  p++;
                    break;
                  }
                }
                PopShortPixel(pixel << 4,q);
                switch ((x+1) % 3)
                {
                  default:
                  case 0:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->red) >> 4);
                    break;
                  }
                  case 1:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->green) >> 4);
                    break;
                  }
                  case 2:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->green) >> 4);
									  p++;
                    break;
                  }
                }
                PopShortPixel(pixel << 4,q);
                q+=pad;
              }
              for (bit=0; bit < (long) (3*number_pixels % 2); bit++)
              {
                switch ((x+bit) % 3)
                {
                  default:
                  case 0:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->red) >> 4);
                    break;
                  }
                  case 1:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->green) >> 4);
                    break;
                  }
                  case 2:
                  {
									  pixel=(unsigned short) (ScaleQuantumToShort(p->green) >> 4);
									  p++;
                    break;
                  }
                }
                PopShortPixel(pixel << 4,q);
                q+=pad;
              }
              break;
            }
          PopPixelQuantum(0,0,q);
          for (x=0; x < (long) number_pixels; x++)
          {
            PopPixelQuantum(image->depth,ScaleQuantumToShort(p->red) >> 4UL,q);
            PopPixelQuantum(image->depth,
              ScaleQuantumToShort(p->green) >> 4UL,q);
            PopPixelQuantum(image->depth,ScaleQuantumToShort(p->blue) >> 4UL,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel= ScaleQuantumToShort(p->red);
            PopShortPixel(pixel,q);
            pixel= ScaleQuantumToShort(p->green);
            PopShortPixel(pixel,q);
            pixel= ScaleQuantumToShort(p->blue);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case RGBAQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(QuantumRange-p->opacity);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(pixel,q);
            pixel= ScaleQuantumToShort(p->green);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(QuantumRange-p->opacity);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(QuantumRange-p->opacity);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case RGBOQuantum:
    {
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->opacity);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(p->opacity);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->opacity);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case CMYKQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(indexes[x]);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel= ScaleQuantumToShort(p->red);
            PopShortPixel(pixel,q);
            pixel= ScaleQuantumToShort(p->green);
            PopShortPixel(pixel,q);
            pixel= ScaleQuantumToShort(p->blue);
            PopShortPixel(pixel,q);
            pixel= ScaleQuantumToShort(indexes[x]);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(indexes[x]);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
    case CMYKAQuantum:
    {
      if (image->colorspace != CMYKColorspace)
        ThrowBinaryException(ImageError,"ColorSeparatedImageRequired",
          image->filename);
      switch (image->depth)
      {
        case 8:
        {
          register unsigned char
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToChar(p->red);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->green);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(p->blue);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(indexes[x]);
            PopCharPixel(pixel,q);
            pixel=ScaleQuantumToChar(QuantumRange-p->opacity);
            PopCharPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 16:
        {
          register unsigned short
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToShort(p->red);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(p->green);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(p->blue);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(indexes[x]);
            PopShortPixel(pixel,q);
            pixel=ScaleQuantumToShort(QuantumRange-p->opacity);
            PopShortPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        case 32:
        {
          register unsigned long
            pixel;

          for (x=0; x < (long) number_pixels; x++)
          {
            pixel=ScaleQuantumToLong(p->red);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->green);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(p->blue);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(indexes[x]);
            PopLongPixel(pixel,q);
            pixel=ScaleQuantumToLong(QuantumRange-p->opacity);
            PopLongPixel(pixel,q);
            p++;
            q+=pad;
          }
          break;
        }
        default:
          ThrowBinaryException(ImageError,"ImageDepthNotSupported",
            image->filename);
      }
      break;
    }
  }
  return(MagickTrue);
}
