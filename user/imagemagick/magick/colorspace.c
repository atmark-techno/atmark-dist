/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     CCCC   OOO   L       OOO   RRRR   SSSSS  PPPP    AAA    CCCC  EEEEE     %
%    C      O   O  L      O   O  R   R  SS     P   P  A   A  C      E         %
%    C      O   O  L      O   O  RRRR    SSS   PPPP   AAAAA  C      EEE       %
%    C      O   O  L      O   O  R R       SS  P      A   A  C      E         %
%     CCCC   OOO   LLLLL   OOO   R  R   SSSSS  P      A   A   CCCC  EEEEE     %
%                                                                             %
%                                                                             %
%                   ImageMagick Image Colorspace Methods                      %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
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
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/gem.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantize.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     R G B T r a n s f o r m I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RGBTransformImage() converts the reference image from RGB to an alternate
%  colorspace.  The transformation matrices are not the standard ones: the
%  weights are rescaled to normalized the range of the transformed values to
%  be [0..QuantumRange].
%
%  The format of the RGBTransformImage method is:
%
%      MagickBooleanType RGBTransformImage(Image *image,
%        const ColorspaceType colorspace)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o colorspace: the colorspace to transform the image to.
%
%
*/
MagickExport MagickBooleanType RGBTransformImage(Image *image,
  const ColorspaceType colorspace)
{
#define RGBTransformImageTag  "RGBTransform/Image"

  long
    y;

  IndexPacket
    *indexes;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  PrimaryInfo
    primary_info,
    *x_map,
    *y_map,
    *z_map;

  register long
    i,
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(colorspace != RGBColorspace);
  assert(colorspace != TransparentColorspace);
  assert(colorspace != UndefinedColorspace);
  switch (image->colorspace)
  {
    case GRAYColorspace:
    case Rec601LumaColorspace:
    case Rec709LumaColorspace:
    case RGBColorspace:
    case TransparentColorspace:
      break;
    default:
    {
      (void) SetImageColorspace(image,image->colorspace);
      break;
    }
  }
  image->colorspace=colorspace;
  switch (colorspace)
  {
    case CMYKColorspace:
    {
      Quantum
        black,
        cyan,
        magenta,
        yellow;

      /*
        Convert RGB to CMYK colorspace.
      */
      if (image->storage_class == PseudoClass)
        {
          (void) SyncImage(image);
          image->storage_class=DirectClass;
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          cyan=(Quantum) (QuantumRange-q->red);
          magenta=(Quantum) (QuantumRange-q->green);
          yellow=(Quantum) (QuantumRange-q->blue);
          black=(Quantum)
            (cyan < magenta ? Min(cyan,yellow) : Min(magenta,yellow));
          q->red=cyan;
          q->green=magenta;
          q->blue=yellow;
          indexes[x]=black;
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return(MagickTrue);
    }
    case GRAYColorspace:
    {
      IndexPacket
        index;

      /*
        Convert RGB to GRAY colorspace.
      */
      if (IsGrayImage(image,&image->exception) != MagickFalse)
        return(MagickTrue);
      if (AllocateImageColormap(image,MaxColormapSize) == MagickFalse)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          index=PixelIntensityToQuantum(q);
          q->red=index;
          q->green=index;
          q->blue=index;
          indexes[x]=index;
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return(MagickTrue);
    }
    case HSBColorspace:
    {
      double
        brightness,
        hue,
        saturation;

      /*
        Transform image from RGB to HSB.
      */
      if (image->storage_class == PseudoClass)
        {
          (void) SyncImage(image);
          image->storage_class=DirectClass;
        }
      hue=0.0;
      saturation=0.0;
      brightness=0.0;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          TransformHSB(q->red,q->green,q->blue,&hue,&saturation,&brightness);
          q->red=RoundToQuantum(QuantumRange*hue);
          q->green=RoundToQuantum(QuantumRange*saturation);
          q->blue=RoundToQuantum(QuantumRange*brightness);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(MagickTrue);
    }
    case HSLColorspace:
    {
      double
        hue,
        luminosity,
        saturation;

      /*
        Transform image from RGB to HSL.
      */
      if (image->storage_class == PseudoClass)
        {
          (void) SyncImage(image);
          image->storage_class=DirectClass;
        }
      hue=0.0;
      saturation=0.0;
      luminosity=0.0;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          TransformHSL(q->red,q->green,q->blue,&hue,&saturation,&luminosity);
          q->red=RoundToQuantum(QuantumRange*hue);
          q->green=RoundToQuantum(QuantumRange*saturation);
          q->blue=RoundToQuantum(QuantumRange*luminosity);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(MagickTrue);
    }
    case HWBColorspace:
    {
      double
        blackness,
        hue,
        whiteness;

      /*
        Transform image from RGB to HWB.
      */
      if (image->storage_class == PseudoClass)
        {
          (void) SyncImage(image);
          image->storage_class=DirectClass;
        }
      hue=0.0;
      whiteness=0.0;
      blackness=0.0;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          TransformHWB(q->red,q->green,q->blue,&hue,&whiteness,&blackness);
          q->red=RoundToQuantum(QuantumRange*hue);
          q->green=RoundToQuantum(QuantumRange*whiteness);
          q->blue=RoundToQuantum(QuantumRange*blackness);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(MagickTrue);
    }
    case LogColorspace:
    {
#define ReferenceBlack  95.0
#define ReferenceWhite  685.0
#define DisplayGamma  (1.0/1.7)

      const ImageAttribute
        *attribute;

      double
        black,
        density,
        gamma,
        reference_black,
        reference_white;


      Quantum
        *logmap;

      /*
        Transform RGB to Log colorspace.
      */
      density=2.03728;
      gamma=DisplayGamma;
      attribute=GetImageAttribute(image,"Gamma");
      if (attribute != (const ImageAttribute *) NULL)
        gamma=1.0/atof(attribute->value) != 0.0 ? atof(attribute->value) : 1.0;
      reference_black=ReferenceBlack;
      attribute=GetImageAttribute(image,"reference-black");
      if (attribute != (const ImageAttribute *) NULL)
        reference_black=atof(attribute->value);
      reference_white=ReferenceWhite;
      attribute=GetImageAttribute(image,"reference-white");
      if (attribute != (const ImageAttribute *) NULL)
        reference_white=atof(attribute->value);
      logmap=(Quantum *)
        AcquireMagickMemory((size_t) (MaxMap+1)*sizeof(*logmap));
      if (logmap == (Quantum *) NULL)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
      black=pow(10.0,(reference_black-reference_white)*(gamma/density)*
        0.002/0.6);
      for (i=0; i <= (long) MaxMap; i++)
        logmap[i]=ScaleMapToQuantum(MaxMap*(reference_white+log10(black+
          ((double) i/MaxMap)*(1.0-black))/((gamma/density)*0.002/0.6))/1024.0);
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns; x > 0; x--)
        {
          q->red=logmap[ScaleQuantumToMap(q->red)];
          q->green=logmap[ScaleQuantumToMap(q->green)];
          q->blue=logmap[ScaleQuantumToMap(q->blue)];
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      logmap=(Quantum *) RelinquishMagickMemory(logmap);
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    default:
      break;
  }
  /*
    Allocate the tables.
  */
  x_map=(PrimaryInfo *) AcquireMagickMemory((size_t) (MaxMap+1)*sizeof(*x_map));
  y_map=(PrimaryInfo *) AcquireMagickMemory((size_t) (MaxMap+1)*sizeof(*y_map));
  z_map=(PrimaryInfo *) AcquireMagickMemory((size_t) (MaxMap+1)*sizeof(*z_map));
  if ((x_map == (PrimaryInfo *) NULL) || (y_map == (PrimaryInfo *) NULL) ||
      (z_map == (PrimaryInfo *) NULL))
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) ResetMagickMemory(&primary_info,0,sizeof(primary_info));
  switch (colorspace)
  {
    case Rec601LumaColorspace:
    case GRAYColorspace:
    {
      /*
        Initialize Rec601 luma tables:

          G = 0.29900*R+0.58700*G+0.11400*B
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.299*i;
        x_map[i].y=0.299*i;
        x_map[i].z=0.299*i;
        y_map[i].x=0.587*i;
        y_map[i].y=0.587*i;
        y_map[i].z=0.587*i;
        z_map[i].x=0.114*i;
        z_map[i].y=0.114*i;
        z_map[i].z=0.114*i;
      }
      break;
    }
    case Rec709LumaColorspace:
    {
      /*
        Initialize Rec709 luma tables:

          G = 0.21260*R+0.7152*G+0.07220*B
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.2126*i;
        x_map[i].y=0.2126*i;
        x_map[i].z=0.2126*i;
        y_map[i].x=0.7152*i;
        y_map[i].y=0.7152*i;
        y_map[i].z=0.7152*i;
        z_map[i].x=0.0722*i;
        z_map[i].y=0.0722*i;
        z_map[i].z=0.0722*i;
      }
      break;
    }
    case OHTAColorspace:
    {
      /*
        Initialize OHTA tables:

          I1 = 0.33333*R+0.33334*G+0.33333*B
          I2 = 0.50000*R+0.00000*G-0.50000*B
          I3 =-0.25000*R+0.50000*G-0.25000*B

        I and Q, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1)/2.0;
      primary_info.z=(double) (MaxMap+1)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.33333*i;
        x_map[i].y=0.5*i;
        x_map[i].z=(-0.25)*i;
        y_map[i].x=0.33334*i;
        y_map[i].y=0.0;
        y_map[i].z=0.5*i;
        z_map[i].x=0.33333*i;
        z_map[i].y=(-0.5)*i;
        z_map[i].z=(-0.25)*i;
      }
      break;
    }
    case sRGBColorspace:
    {
      /*
        Initialize sRGB tables:

          Y =  0.29900*R+0.58700*G+0.11400*B
          C1= -0.29900*R-0.58700*G+0.88600*B
          C2=  0.70100*R-0.58700*G-0.11400*B

        sRGB is scaled by 1.3584.  C1 zero is 156 and C2 is at 137.
      */
      primary_info.y=(double) ScaleQuantumToMap(ScaleCharToQuantum(156));
      primary_info.z=(double) ScaleQuantumToMap(ScaleCharToQuantum(137));
      for (i=0; i <= (long) (0.018*MaxMap); i++)
      {
        x_map[i].x=0.003962014134275617*i;
        x_map[i].y=(-0.002426619775463276)*i;
        x_map[i].z=0.006927257754597858*i;
        y_map[i].x=0.007778268551236748*i;
        y_map[i].y=(-0.004763965913702149)*i;
        y_map[i].z=(-0.005800713697502058)*i;
        z_map[i].x=0.001510600706713781*i;
        z_map[i].y=0.007190585689165425*i;
        z_map[i].z=(-0.0011265440570958)*i;
      }
      for ( ; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.2201118963486454*(1.099*i-0.099);
        x_map[i].y=(-0.1348122097479598)*(1.099*i-0.099);
        x_map[i].z=0.3848476530332144*(1.099*i-0.099);
        y_map[i].x=0.4321260306242638*(1.099*i-0.099);
        y_map[i].y=(-0.2646647729834528)*(1.099*i-0.099);
        y_map[i].z=(-0.3222618720834477)*(1.099*i-0.099);
        z_map[i].x=0.08392226148409894*(1.099*i-0.099);
        z_map[i].y=0.3994769827314126*(1.099*i-0.099);
        z_map[i].z=(-0.06258578094976668)*(1.099*i-0.099);
      }
      break;
    }
    case XYZColorspace:
    {
      /*
        Initialize CIE XYZ tables (ITU-R 709 RGB):

          X = 0.412453*X+0.357580*Y+0.180423*Z
          Y = 0.212671*X+0.715160*Y+0.072169*Z
          Z = 0.019334*X+0.119193*Y+0.950227*Z
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.412453*i;
        x_map[i].y=0.212671*i;
        x_map[i].z=0.019334*i;
        y_map[i].x=0.35758*i;
        y_map[i].y=0.71516*i;
        y_map[i].z=0.119193*i;
        z_map[i].x=0.180423*i;
        z_map[i].y=0.072169*i;
        z_map[i].z=0.950227*i;
      }
      break;
    }
    case YCbCrColorspace:
    {
      /*
        Initialize YCbCr tables (ITU-R BT.601):

          Y =  0.299000*R+0.587000*G+0.114000*B
          Cb= -0.168736*R-0.331264*G+0.500000*B
          Cr=  0.500000*R-0.418688*G-0.081316*B

        Cb and Cr, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1)/2.0;
      primary_info.z=(double) (MaxMap+1)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.299*i;
        x_map[i].y=(-0.16873)*i;
        x_map[i].z=0.500000*i;
        y_map[i].x=0.587*i;
        y_map[i].y=(-0.331264)*i;
        y_map[i].z=(-0.418688)*i;
        z_map[i].x=0.114*i;
        z_map[i].y=0.500000*i;
        z_map[i].z=(-0.081316)*i;
      }
      break;
    }
    case YCCColorspace:
    {
      /*
        Initialize YCC tables:

          Y =  0.29900*R+0.58700*G+0.11400*B
          C1= -0.29900*R-0.58700*G+0.88600*B
          C2=  0.70100*R-0.58700*G-0.11400*B

        YCC is scaled by 1.3584.  C1 zero is 156 and C2 is at 137.
      */
      primary_info.y=(double) ScaleQuantumToMap(ScaleCharToQuantum(156));
      primary_info.z=(double) ScaleQuantumToMap(ScaleCharToQuantum(137));
      for (i=0; i <= (long) (0.018*MaxMap); i++)
      {
        x_map[i].x=0.003962014134275617*i;
        x_map[i].y=(-0.002426619775463276)*i;
        x_map[i].z=0.006927257754597858*i;
        y_map[i].x=0.007778268551236748*i;
        y_map[i].y=(-0.004763965913702149)*i;
        y_map[i].z=(-0.005800713697502058)*i;
        z_map[i].x=0.001510600706713781*i;
        z_map[i].y=0.007190585689165425*i;
        z_map[i].z=(-0.0011265440570958)*i;
      }
      for ( ; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.2201118963486454*(1.099*i-0.099);
        x_map[i].y=(-0.1348122097479598)*(1.099*i-0.099);
        x_map[i].z=0.3848476530332144*(1.099*i-0.099);
        y_map[i].x=0.4321260306242638*(1.099*i-0.099);
        y_map[i].y=(-0.2646647729834528)*(1.099*i-0.099);
        y_map[i].z=(-0.3222618720834477)*(1.099*i-0.099);
        z_map[i].x=0.08392226148409894*(1.099*i-0.099);
        z_map[i].y=0.3994769827314126*(1.099*i-0.099);
        z_map[i].z=(-0.06258578094976668)*(1.099*i-0.099);
      }
      break;
    }
    case YIQColorspace:
    {
      /*
        Initialize YIQ tables:

          Y = 0.29900*R+0.58700*G+0.11400*B
          I = 0.59600*R-0.27400*G-0.32200*B
          Q = 0.21100*R-0.52300*G+0.31200*B

        I and Q, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1)/2.0;
      primary_info.z=(double) (MaxMap+1)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.299*i;
        x_map[i].y=0.596*i;
        x_map[i].z=0.211*i;
        y_map[i].x=0.587*i;
        y_map[i].y=(-0.274)*i;
        y_map[i].z=(-0.523)*i;
        z_map[i].x=0.114*i;
        z_map[i].y=(-0.322)*i;
        z_map[i].z=0.312*i;
      }
      break;
    }
    case YPbPrColorspace:
    {
      /*
        Initialize YPbPr tables (ITU-R BT.601):

          Y =  0.299000*R+0.587000*G+0.114000*B
          Pb= -0.168736*R-0.331264*G+0.500000*B
          Pr=  0.500000*R-0.418688*G-0.081312*B

        Pb and Pr, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.
      */
      primary_info.y=(double) (MaxMap+1)/2.0;
      primary_info.z=(double) (MaxMap+1)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.299*i;
        x_map[i].y=(-0.168736)*i;
        x_map[i].z=0.5*i;
        y_map[i].x=0.587*i;
        y_map[i].y=(-0.331264)*i;
        y_map[i].z=(-0.418688)*i;
        z_map[i].x=0.114*i;
        z_map[i].y=0.5*i;
        z_map[i].z=(-0.081312)*i;
      }
      break;
    }
    case YUVColorspace:
    default:
    {
      /*
        Initialize YUV tables:

          Y =  0.29900*R+0.58700*G+0.11400*B
          U = -0.14740*R-0.28950*G+0.43690*B
          V =  0.61500*R-0.51500*G-0.10000*B

        U and V, normally -0.5 through 0.5, are normalized to the range 0
        through QuantumRange.  Note that U = 0.493*(B-Y), V = 0.877*(R-Y).
      */
      primary_info.y=(double) (MaxMap+1)/2.0;
      primary_info.z=(double) (MaxMap+1)/2.0;
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=0.299*i;
        x_map[i].y=(-0.1474)*i;
        x_map[i].z=0.615*i;
        y_map[i].x=0.587*i;
        y_map[i].y=(-0.2895)*i;
        y_map[i].z=(-0.515)*i;
        z_map[i].x=0.114*i;
        z_map[i].y=0.4369*i;
        z_map[i].z=(-0.1)*i;
      }
      break;
    }
  }
  /*
    Convert from RGB.
  */
  switch (image->storage_class)
  {
    case DirectClass:
    default:
    {
      ExceptionInfo
        *exception;

      /*
        Convert DirectClass image.
      */
      exception=(&image->exception);
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          pixel.red=x_map[ScaleQuantumToMap(q->red)].x+
            y_map[ScaleQuantumToMap(q->green)].x+
            z_map[ScaleQuantumToMap(q->blue)].x+primary_info.x;
          pixel.green=x_map[ScaleQuantumToMap(q->red)].y+
            y_map[ScaleQuantumToMap(q->green)].y+
            z_map[ScaleQuantumToMap(q->blue)].y+primary_info.y;
          pixel.blue=x_map[ScaleQuantumToMap(q->red)].z+
            y_map[ScaleQuantumToMap(q->green)].z+
            z_map[ScaleQuantumToMap(q->blue)].z+primary_info.z;
          q->red=ScaleMapToQuantum(RoundToMap(pixel.red));
          q->green=ScaleMapToQuantum(RoundToMap(pixel.green));
          q->blue=ScaleMapToQuantum(RoundToMap(pixel.blue));
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RGBTransformImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      break;
    }
    case PseudoClass:
    {
      /*
        Convert PseudoClass image.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        pixel.red=x_map[ScaleQuantumToMap(image->colormap[i].red)].x+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].x+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].x+primary_info.x;
        pixel.green=x_map[ScaleQuantumToMap(image->colormap[i].red)].y+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].y+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].y+primary_info.y;
        pixel.blue=x_map[ScaleQuantumToMap(image->colormap[i].red)].z+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].z+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].z+primary_info.z;
        image->colormap[i].red=ScaleMapToQuantum(RoundToMap(pixel.red));
        image->colormap[i].green=ScaleMapToQuantum(RoundToMap(pixel.green));
        image->colormap[i].blue=ScaleMapToQuantum(RoundToMap(pixel.blue));
      }
      (void) SyncImage(image);
      break;
    }
  }
  /*
    Free resources.
  */
  z_map=(PrimaryInfo *) RelinquishMagickMemory(z_map);
  y_map=(PrimaryInfo *) RelinquishMagickMemory(y_map);
  x_map=(PrimaryInfo *) RelinquishMagickMemory(x_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e C o l o r s p a c e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageColorspace() sets the colorspace member of the Image structure.
%
%  The format of the SetImageColorspace method is:
%
%      MagickBooleanType SetImageColorspace(Image *image,
%        const ColorspaceType colorspace)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o colorspace: The colorspace.
%
%
*/
MagickExport MagickBooleanType SetImageColorspace(Image *image,
  const ColorspaceType colorspace)
{
  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->colorspace != UndefinedColorspace);
  assert(colorspace != UndefinedColorspace);
  if (image->colorspace == colorspace)
    return(MagickTrue);
  if ((colorspace == RGBColorspace) || (colorspace == TransparentColorspace))
    return(TransformRGBImage(image,image->colorspace));
  status=MagickTrue;
  if ((image->colorspace != RGBColorspace) &&
      (image->colorspace != TransparentColorspace) &&
      (image->colorspace != GRAYColorspace))
    status=TransformRGBImage(image,image->colorspace);
  if (RGBTransformImage(image,colorspace) == MagickFalse)
    status=MagickFalse;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     T r a n s f o r m R G B I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransformRGBImage() converts the reference image from an alternate
%  colorspace to RGB.  The transformation matrices are not the standard ones:
%  the weights are rescaled to normalize the range of the transformed values to
%  be [0..QuantumRange].
%
%  The format of the TransformRGBImage method is:
%
%      MagickBooleanType TransformRGBImage(Image *image,
%        const ColorspaceType colorspace)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o colorspace: the colorspace to transform the image to.
%
%
*/
MagickExport MagickBooleanType TransformRGBImage(Image *image,
  const ColorspaceType colorspace)
{
#define MaxYCC  (1.3584*MaxMap+0.5)
#define TransformRGBImageTag  "Transform/Image"

  static const unsigned long
    sRGBMap[351] =
    {
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
       14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
       28,  29,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,
       55,  56,  57,  58,  59,  60,  61,  62,  63,  65,  66,  67,  68,  69,
       70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,
       84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  95,  96,  97,  98,
       99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
      114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
      128, 129, 130, 131, 132, 133, 135, 136, 137, 138, 139, 140, 141, 142,
      143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156,
      157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
      171, 172, 173, 174, 175, 175, 176, 177, 178, 179, 180, 181, 182, 183,
      184, 185, 186, 187, 187, 188, 189, 190, 191, 192, 193, 194, 194, 195,
      196, 197, 198, 199, 199, 200, 201, 202, 203, 203, 204, 205, 206, 207,
      207, 208, 209, 210, 210, 211, 212, 213, 213, 214, 215, 215, 216, 217,
      218, 218, 219, 220, 220, 221, 222, 222, 223, 223, 224, 225, 225, 226,
      227, 227, 228, 228, 229, 229, 230, 230, 231, 232, 232, 233, 233, 234,
      234, 235, 235, 235, 236, 236, 237, 237, 238, 238, 238, 239, 239, 240,
      240, 240, 241, 241, 242, 242, 242, 243, 243, 243, 243, 244, 244, 244,
      245, 245, 245, 245, 246, 246, 246, 247, 247, 247, 247, 247, 248, 248,
      248, 248, 249, 249, 249, 249, 249, 249, 250, 250, 250, 250, 250, 250,
      251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 252, 252,
      252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 254, 254, 254, 254,
      254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, 255,
      255
    },
    YCCMap[351] =  /* Photo CD information beyond 100% white, Gamma 2.2 */
    {
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,
       14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
       28,  29,  30,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
       43,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  56,  57,  58,
       59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  71,  72,  73,
       74,  76,  77,  78,  79,  80,  81,  82,  83,  84,  86,  87,  88,  89,
       90,  91,  92,  93,  94,  95,  97,  98,  99, 100, 101, 102, 103, 104,
      105, 106, 107, 108, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
      120, 121, 122, 123, 124, 125, 126, 127, 129, 130, 131, 132, 133, 134,
      135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
      149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162,
      163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
      176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
      190, 191, 192, 193, 193, 194, 195, 196, 197, 198, 199, 200, 201, 201,
      202, 203, 204, 205, 206, 207, 207, 208, 209, 210, 211, 211, 212, 213,
      214, 215, 215, 216, 217, 218, 218, 219, 220, 221, 221, 222, 223, 224,
      224, 225, 226, 226, 227, 228, 228, 229, 230, 230, 231, 232, 232, 233,
      234, 234, 235, 236, 236, 237, 237, 238, 238, 239, 240, 240, 241, 241,
      242, 242, 243, 243, 244, 244, 245, 245, 245, 246, 246, 247, 247, 247,
      248, 248, 248, 249, 249, 249, 249, 250, 250, 250, 250, 251, 251, 251,
      251, 251, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253,
      253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254,
      254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255
    };

  long
    y;

  LongPixelPacket
    quantum;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  PrimaryInfo
    *y_map,
    *x_map,
    *z_map;

  register long
    x;

  register long
    i;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->colorspace != UndefinedColorspace);
  switch (colorspace)
  {
    case GRAYColorspace:
    case Rec601LumaColorspace:
    case Rec709LumaColorspace:
    case RGBColorspace:
    case TransparentColorspace:
      return(MagickTrue);
    default:
      break;
  }
  switch (colorspace)
  {
    case CMYKColorspace:
    {
      IndexPacket
        *indexes;

      /*
        Transform image from CMYK to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          (void) SyncImage(image);
          image->storage_class=DirectClass;
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          q->red=RoundToQuantum(QuantumScale* ((MagickRealType)
            (QuantumRange-q->red)*(QuantumRange-indexes[x])));
          q->green=RoundToQuantum(QuantumScale* ((MagickRealType)
            (QuantumRange-q->green)*(QuantumRange-indexes[x])));
          q->blue=RoundToQuantum(QuantumScale* ((MagickRealType)
            (QuantumRange-q->blue)*(QuantumRange-indexes[x])));
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case HSBColorspace:
    {
      double
        hue,
        luminosity,
        saturation;

      /*
        Transform image from HSB to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          (void) SyncImage(image);
          image->storage_class=DirectClass;
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          hue=(double) (QuantumScale*q->red);
          saturation=(double) (QuantumScale*q->green);
          luminosity=(double) (QuantumScale*q->blue);
          HSBTransform(hue,saturation,luminosity,&q->red,&q->green,&q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case HSLColorspace:
    {
      double
        hue,
        luminosity,
        saturation;

      /*
        Transform image from HSL to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          (void) SyncImage(image);
          image->storage_class=DirectClass;
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          hue=(double) (QuantumScale*q->red);
          saturation=(double) (QuantumScale*q->green);
          luminosity=(double) (QuantumScale*q->blue);
          HSLTransform(hue,saturation,luminosity,&q->red,&q->green,&q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case HWBColorspace:
    {
      double
        blackness,
        hue,
        whiteness;

      /*
        Transform image from HWB to RGB.
      */
      if (image->storage_class == PseudoClass)
        {
          (void) SyncImage(image);
          image->storage_class=DirectClass;
        }
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          hue=(double) (QuantumScale*q->red);
          whiteness=(double) (QuantumScale*q->green);
          blackness=(double) (QuantumScale*q->blue);
          HWBTransform(hue,whiteness,blackness,&q->red,&q->green,&q->blue);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    case LogColorspace:
    {
      const ImageAttribute
        *attribute;

      double
        black,
        density,
        gamma,
        reference_black,
        reference_white;

      Quantum
        *logmap;

      /*
        Transform Log to RGB colorspace.
      */
      density=2.03728;
      gamma=DisplayGamma;
      attribute=GetImageAttribute(image,"Gamma");
      if (attribute != (const ImageAttribute *) NULL)
        gamma=1.0/atof(attribute->value) != 0.0 ? atof(attribute->value) : 1.0;
      reference_black=ReferenceBlack;
      attribute=GetImageAttribute(image,"reference-black");
      if (attribute != (const ImageAttribute *) NULL)
        reference_black=atof(attribute->value);
      reference_white=ReferenceWhite;
      attribute=GetImageAttribute(image,"reference-white");
      if (attribute != (const ImageAttribute *) NULL)
        reference_white=atof(attribute->value);
      logmap=(Quantum *)
        AcquireMagickMemory((size_t) (MaxMap+1)*sizeof(*logmap));
      if (logmap == (Quantum *) NULL)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
      black=pow(10.0,(reference_black-reference_white)*(gamma/density)*
        0.002/0.6);
      for (i=0; i <= (long) (reference_black*MaxMap/1024.0); i++)
        logmap[i]=0;
      for ( ; i < (long) (reference_white*MaxMap/1024.0); i++)
        logmap[i]=RoundToQuantum(QuantumRange/(1.0-black)*(pow(10.0,(1024.0*i/
          MaxMap-reference_white)*(gamma/density)*0.002/0.6)-black));
      for ( ; i <= (long) MaxMap; i++)
        logmap[i]=QuantumRange;
      image->storage_class=DirectClass;
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns; x > 0; x--)
        {
          q->red=logmap[ScaleQuantumToMap(q->red)];
          q->green=logmap[ScaleQuantumToMap(q->green)];
          q->blue=logmap[ScaleQuantumToMap(q->blue)];
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      logmap=(Quantum *) RelinquishMagickMemory(logmap);
      image->colorspace=RGBColorspace;
      return(y == (long) image->rows ? MagickTrue : MagickFalse);
    }
    default:
      break;
  }
  /*
    Allocate the tables.
  */
  x_map=(PrimaryInfo *) AcquireMagickMemory((size_t) (MaxMap+1)*sizeof(*x_map));
  y_map=(PrimaryInfo *) AcquireMagickMemory((size_t) (MaxMap+1)*sizeof(*y_map));
  z_map=(PrimaryInfo *) AcquireMagickMemory((size_t) (MaxMap+1)*sizeof(*z_map));
  if ((x_map == (PrimaryInfo *) NULL) || (y_map == (PrimaryInfo *) NULL) ||
      (z_map == (PrimaryInfo *) NULL))
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  switch (colorspace)
  {
    case OHTAColorspace:
    {
      /*
        Initialize OHTA tables:

          R = I1+1.00000*I2-0.66668*I3
          G = I1+0.00000*I2+1.33333*I3
          B = I1-1.00000*I2-0.66668*I3

        I and Q, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        x_map[i].y=(double) i;
        x_map[i].z=(double) i;
        y_map[i].x=0.5*(2.0*i-MaxMap);
        y_map[i].y=0.0;
        y_map[i].z=(-0.5)*(2.0*i-MaxMap);
        z_map[i].x=(-0.33334)*(2.0*i-MaxMap);
        z_map[i].y=0.666665*(2.0*i-MaxMap);
        z_map[i].z=(-0.33334)*(2.0*i-MaxMap);
      }
      break;
    }
    case sRGBColorspace:
    {
      /*
        Initialize sRGB tables:

          R = Y            +1.032096*C2
          G = Y-0.326904*C1-0.704445*C2
          B = Y+1.685070*C1

        sRGB is scaled by 1.3584.  C1 zero is 156 and C2 is at 137.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=1.40200*i;
        x_map[i].y=1.40200*i;
        x_map[i].z=1.40200*i;
        y_map[i].x=0.0;
        y_map[i].y=(-0.444066)*(i-(long)
          ScaleQuantumToMap(ScaleCharToQuantum(156)));
        y_map[i].z=2.28900*(i-(long)
          ScaleQuantumToMap(ScaleCharToQuantum(156)));
        z_map[i].x=1.88000*(i-(long)
          ScaleQuantumToMap(ScaleCharToQuantum(137)));
        z_map[i].y=(-0.95692)*(i-(long)
          ScaleQuantumToMap(ScaleCharToQuantum(137)));
        z_map[i].z=0.0;
      }
      break;
    }
    case XYZColorspace:
    {
      /*
        Initialize CIE XYZ tables (ITU R-709 RGB):

          R =  3.240479*R-1.537150*G-0.498535*B
          G = -0.969256*R+1.875992*G+0.041556*B
          B =  0.055648*R-0.204043*G+1.057311*B
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=3.240479*i;
        x_map[i].y=(-0.969256)*i;
        x_map[i].z=0.055648*i;
        y_map[i].x=(-1.537150)*i;
        y_map[i].y=1.875992*i;
        y_map[i].z=(-0.204043)*i;
        z_map[i].x=(-0.498535)*i;
        z_map[i].y=0.041556*i;
        z_map[i].z=1.057311*i;
      }
      break;
    }
    case YCbCrColorspace:
    {
      /*
        Initialize YCbCr tables:

          R = Y            +1.402000*Cr
          G = Y-0.344136*Cb-0.714136*Cr
          B = Y+1.772000*Cb

        Cb and Cr, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        x_map[i].y=(double) i;
        x_map[i].z=(double) i;
        y_map[i].x=0.0;
        y_map[i].y=(-0.344136*0.5)*(2.0*i-MaxMap);
        y_map[i].z=(1.772000*0.5)*(2.0*i-MaxMap);
        z_map[i].x=(1.402000*0.5)*(2.0*i-MaxMap);
        z_map[i].y=(-0.714136*0.5)*(2.0*i-MaxMap);
        z_map[i].z=0.0;
      }
      break;
    }
    case YCCColorspace:
    {
      /*
        Initialize YCC tables:

          R = Y            +1.340762*C2
          G = Y-0.317038*C1-0.682243*C2
          B = Y+1.632639*C1

        YCC is scaled by 1.3584.  C1 zero is 156 and C2 is at 137.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=1.3584*i;
        x_map[i].y=1.3584*i;
        x_map[i].z=1.3584*i;
        y_map[i].x=0.0;
        y_map[i].y=(-0.4302726)*(i-(double)
          ScaleQuantumToMap(ScaleCharToQuantum(156)));
        y_map[i].z=2.2179*(i-(double)
          ScaleQuantumToMap(ScaleCharToQuantum(156)));
        z_map[i].x=1.8215*(i-(double)
          ScaleQuantumToMap(ScaleCharToQuantum(137)));
        z_map[i].y=(-0.9271435)*(i-(double)
          ScaleQuantumToMap(ScaleCharToQuantum(137)));
        z_map[i].z=0.0;
      }
      break;
    }
    case YIQColorspace:
    {
      /*
        Initialize YIQ tables:

          R = Y+0.95620*I+0.62140*Q
          G = Y-0.27270*I-0.64680*Q
          B = Y-1.10370*I+1.70060*Q

        I and Q, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        x_map[i].y=(double) i;
        x_map[i].z=(double) i;
        y_map[i].x=0.4781*(2.0*i-MaxMap);
        y_map[i].y=(-0.13635)*(2.0*i-MaxMap);
        y_map[i].z=(-0.55185)*(2.0*i-MaxMap);
        z_map[i].x=0.3107*(2.0*i-MaxMap);
        z_map[i].y=(-0.3234)*(2.0*i-MaxMap);
        z_map[i].z=0.8503*(2.0*i-MaxMap);
      }
      break;
    }
    case YPbPrColorspace:
    {
      /*
        Initialize YPbPr tables:

          R = Y            +1.402000*C2
          G = Y-0.344136*C1+0.714136*C2
          B = Y+1.772000*C1

        Pb and Pr, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        x_map[i].y=(double) i;
        x_map[i].z=(double) i;
        y_map[i].x=0.0;
        y_map[i].y=(-0.172068)*(2.0*i-MaxMap);
        y_map[i].z=0.886*(2.0*i-MaxMap);
        z_map[i].x=0.701*(2.0*i-MaxMap);
        z_map[i].y=0.357068*(2.0*i-MaxMap);
        z_map[i].z=0.0;
      }
      break;
    }
    case YUVColorspace:
    default:
    {
      /*
        Initialize YUV tables:

          R = Y          +1.13980*V
          G = Y-0.39380*U-0.58050*V
          B = Y+2.02790*U

        U and V, normally -0.5 through 0.5, must be normalized to the range 0
        through QuantumRange.
      */
      for (i=0; i <= (long) MaxMap; i++)
      {
        x_map[i].x=(double) i;
        x_map[i].y=(double) i;
        x_map[i].z=(double) i;
        y_map[i].x=0.0;
        y_map[i].y=(-0.1969)*(2.0*i-MaxMap);
        y_map[i].z=1.01395*(2.0*i-MaxMap);
        z_map[i].x=0.5699*(2.0*i-MaxMap);
        z_map[i].y=(-0.29025)*(2.0*i-MaxMap);
        z_map[i].z=0.0;
      }
      break;
    }
  }
  /*
    Convert to RGB.
  */
  switch (image->storage_class)
  {
    case DirectClass:
    default:
    {
      ExceptionInfo
        *exception;

      /*
        Convert DirectClass image.
      */
      exception=(&image->exception);
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          pixel.red=x_map[ScaleQuantumToMap(q->red)].x+
            y_map[ScaleQuantumToMap(q->green)].x+
            z_map[ScaleQuantumToMap(q->blue)].x;
          pixel.green=x_map[ScaleQuantumToMap(q->red)].y+
            y_map[ScaleQuantumToMap(q->green)].y+
            z_map[ScaleQuantumToMap(q->blue)].y;
          pixel.blue=x_map[ScaleQuantumToMap(q->red)].z+
            y_map[ScaleQuantumToMap(q->green)].z+
            z_map[ScaleQuantumToMap(q->blue)].z;
          switch (colorspace)
          {
            case sRGBColorspace:
            case YCCColorspace:
            {
              quantum.red=(unsigned long) ((pixel.red < 0.0) ? 0.0 :
                (pixel.red > MaxYCC) ? MaxYCC : pixel.red+0.5);
              quantum.green=(unsigned long) ((pixel.green < 0.0) ? 0.0 :
                (pixel.green > MaxYCC) ?  MaxYCC : pixel.green+0.5);
              quantum.blue=(unsigned long) ((pixel.blue < 0.0) ? 0.0 :
                (pixel.blue > MaxYCC) ? MaxYCC : pixel.blue+0.5);
              if (colorspace == sRGBColorspace)
                {
                  q->red=(Quantum)
                    ScaleToQuantum(sRGBMap[ScaleQuantum(quantum.red)]);
                  q->green=(Quantum)
                    ScaleToQuantum(sRGBMap[ScaleQuantum(quantum.green)]);
                  q->blue=(Quantum)
                    ScaleToQuantum(sRGBMap[ScaleQuantum(quantum.blue)]);
                  break;
                }
              q->red=(Quantum)
                ScaleToQuantum(YCCMap[ScaleQuantum(quantum.red)]);
              q->green=(Quantum)
                ScaleToQuantum(YCCMap[ScaleQuantum(quantum.green)]);
              q->blue=(Quantum)
                ScaleToQuantum(YCCMap[ScaleQuantum(quantum.blue)]);
              break;
            }
            default:
            {
              quantum.red=RoundToMap(pixel.red);
              quantum.green=RoundToMap(pixel.green);
              quantum.blue=RoundToMap(pixel.blue);
              q->red=ScaleMapToQuantum(quantum.red);
              q->green=ScaleMapToQuantum(quantum.green);
              q->blue=ScaleMapToQuantum(quantum.blue);
              break;
            }
          }
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(TransformRGBImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      break;
    }
    case PseudoClass:
    {
      /*
        Convert PseudoClass image.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        pixel.red=x_map[ScaleQuantumToMap(image->colormap[i].red)].x+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].x+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].x;
        pixel.green=x_map[ScaleQuantumToMap(image->colormap[i].red)].y+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].y+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].y;
        pixel.blue=x_map[ScaleQuantumToMap(image->colormap[i].red)].z+
          y_map[ScaleQuantumToMap(image->colormap[i].green)].z+
          z_map[ScaleQuantumToMap(image->colormap[i].blue)].z;
        switch (colorspace)
        {
          case sRGBColorspace:
          case YCCColorspace:
          {
            quantum.red=(unsigned long) ((pixel.red < 0.0) ? 0.0 :
              (pixel.red > MaxYCC) ? MaxYCC : pixel.red+0.5);
            quantum.green=(unsigned long) ((pixel.green < 0.0) ? 0.0 :
              (pixel.green > MaxYCC) ? MaxYCC : pixel.green+0.5);
            quantum.blue=(unsigned long) ((pixel.blue < 0.0) ? 0.0 :
              (pixel.blue > MaxYCC) ? MaxYCC : pixel.blue+0.5);
            if (colorspace == sRGBColorspace)
              {
                image->colormap[i].red=(Quantum)
                  ScaleToQuantum(sRGBMap[ScaleQuantum(quantum.red)]);
                image->colormap[i].green=(Quantum)
                  ScaleToQuantum(sRGBMap[ScaleQuantum(quantum.green)]);
                image->colormap[i].blue=(Quantum)
                  ScaleToQuantum(sRGBMap[ScaleQuantum(quantum.blue)]);
                break;
              }
            image->colormap[i].red=(Quantum)
              ScaleToQuantum(YCCMap[ScaleQuantum(quantum.red)]);
            image->colormap[i].green=(Quantum)
              ScaleToQuantum(YCCMap[ScaleQuantum(quantum.green)]);
            image->colormap[i].blue=(Quantum)
              ScaleToQuantum(YCCMap[ScaleQuantum(quantum.blue)]);
            break;
          }
          default:
          {
            quantum.red=RoundToMap(pixel.red);
            quantum.green=RoundToMap(pixel.green);
            quantum.blue=RoundToMap(pixel.blue);
            image->colormap[i].red=ScaleMapToQuantum(quantum.red);
            image->colormap[i].green=ScaleMapToQuantum(quantum.green);
            image->colormap[i].blue=ScaleMapToQuantum(quantum.blue);
            break;
          }
        }
      }
      (void) SyncImage(image);
      break;
    }
  }
  image->colorspace=RGBColorspace;
  /*
    Free resources.
  */
  z_map=(PrimaryInfo *) RelinquishMagickMemory(z_map);
  y_map=(PrimaryInfo *) RelinquishMagickMemory(y_map);
  x_map=(PrimaryInfo *) RelinquishMagickMemory(x_map);
  return(MagickTrue);
}
