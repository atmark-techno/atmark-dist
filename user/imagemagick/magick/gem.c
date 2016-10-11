/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                              GGGG  EEEEE  M   M                             %
%                             G      E      MM MM                             %
%                             G GG   EEE    M M M                             %
%                             G   G  E      M   M                             %
%                              GGGG  EEEEE  M   M                             %
%                                                                             %
%                                                                             %
%                    Graphic Gems - Graphic Support Methods                   %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                 August 1996                                 %
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
#include "magick/studio.h"
#include "magick/draw.h"
#include "magick/gem.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/pixel-private.h"
#include "magick/random_.h"
#include "magick/signature.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n s t r a s t                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Contrast() enhances the intensity differences between the lighter and darker
%  elements of the image.
%
%  The format of the Contrast method is:
%
%      void Contrast(const int sign,Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o sign: A positive value enhances the contrast otherwise it is reduced.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void Contrast(const int sign,Quantum *red,Quantum *green,
  Quantum *blue)
{
  double
    brightness,
    hue,
    saturation;

  /*
    Enhance contrast: dark color become darker, light color become lighter.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  hue=0.0;
  saturation=0.0;
  brightness=0.0;
  TransformHSB(*red,*green,*blue,&hue,&saturation,&brightness);
  brightness+=0.5*sign*(0.5*(sin(MagickPI*(brightness-0.5))+1.0)-brightness);
  if (brightness > 1.0)
    brightness=1.0;
  else
    if (brightness < 0.0)
      brightness=0.0;
  HSBTransform(hue,saturation,brightness,red,green,blue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p a n d A f f i n e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExpandAffine() computes the affine's expansion factor, i.e. the square root
%  of the factor by which the affine transform affects area. In an affine
%  transform composed of scaling, rotation, shearing, and translation, returns
%  the amount of scaling.
%
%  The format of the ExpandAffine method is:
%
%      double ExpandAffine(const AffineMatrix *affine)
%
%  A description of each parameter follows:
%
%    o expansion: Method ExpandAffine returns the affine's expansion factor.
%
%    o affine: A pointer the the affine transform of type AffineMatrix.
%
%
*/
MagickExport double ExpandAffine(const AffineMatrix *affine)
{
  assert(affine != (const AffineMatrix *) NULL);
  return(sqrt(fabs(affine->sx*affine->sy-affine->rx*affine->ry)));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p t i m a l K e r n e l W i d t h                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOptimalKernelWidth() computes the optimal kernel radius for a convolution
%  filter.  Start with the minimum value of 3 pixels and walk out until we drop
%  below the threshold of one pixel numerical accuracy.
%
%  The format of the GetOptimalKernelWidth method is:
%
%      unsigned long GetOptimalKernelWidth(const double radius,
%        const double sigma)
%
%  A description of each parameter follows:
%
%    o width: Method GetOptimalKernelWidth returns the optimal width of
%      a convolution kernel.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%
*/

MagickExport unsigned long GetOptimalKernelWidth1D(const double radius,
  const double sigma)
{
  long
    width;

  MagickRealType
    normalize,
    value;

  register long
    u;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(sigma != 0.0);
  if (radius > 0.0)
    return(Max(2*((unsigned long) radius)+1,3));
  for (width=5; ;)
  {
    normalize=0.0;
    for (u=(-width/2); u <= (width/2); u++)
      normalize+=exp(-((double) u*u)/(2.0*sigma*sigma))/(MagickSQ2PI*sigma);
    u=width/2;
    value=exp(-((double) u*u)/(2.0*sigma*sigma))/(MagickSQ2PI*sigma)/normalize;
    if ((long) (QuantumRange*value) <= 0)
      break;
    width+=2;
  }
  return((unsigned long) (width-2));
}

MagickExport unsigned long GetOptimalKernelWidth2D(const double radius,
  const double sigma)
{

  long
    width;

  MagickRealType
    alpha,
    normalize,
    value;
  register long
    u,
    v;

  assert(sigma != 0.0);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  if (radius > 0.0)
    return(Max(2*((unsigned long) radius)+1,3));
  for (width=5; ;)
  {
    normalize=0.0;
    for (v=(-width/2); v <= (width/2); v++)
    {
      for (u=(-width/2); u <= (width/2); u++)
      {
        alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
        normalize+=alpha/(2.0*MagickPI*sigma*sigma);
      }
    }
    v=width/2;
    value=exp(-((double) v*v)/(2.0*sigma*sigma))/(MagickSQ2PI*sigma)/normalize;
    if ((long) (QuantumRange*value) <= 0)
      break;
    width+=2;
  }
  return((unsigned long) (width-2));
}

MagickExport unsigned long  GetOptimalKernelWidth(const double radius,
  const double sigma)
{
  return(GetOptimalKernelWidth1D(radius,sigma));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H S B T r a n s f o r m                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HSBTransform() converts a (hue, saturation, brightness) to a (red, green,
%  blue) triple.
%
%  The format of the HSBTransformImage method is:
%
%      void HSBTransform(const double hue,const double saturation,
%        const double brightness,Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o hue, saturation, brightness: A double value representing a
%      component of the HSB color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void HSBTransform(const double hue,const double saturation,
  const double brightness,Quantum *red,Quantum *green,Quantum *blue)
{
  MagickRealType
    f,
    h,
    p,
    q,
    t;

  /*
    Convert HSB to RGB colorspace.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  if (saturation == 0.0)
    {
      *red=RoundToQuantum(QuantumRange*brightness);
      *green=(*red);
      *blue=(*red);
      return;
    }
  h=6.0*(hue-floor(hue));
  f=h-floor((double) h);
  p=brightness*(1.0-saturation);
  q=brightness*(1.0-saturation*f);
  t=brightness*(1.0-(saturation*(1.0-f)));
  switch ((int) h)
  {
    case 0:
    default:
    {
      *red=RoundToQuantum(QuantumRange*brightness);
      *green=RoundToQuantum(QuantumRange*t);
      *blue=RoundToQuantum(QuantumRange*p);
      break;
    }
    case 1:
    {
      *red=RoundToQuantum(QuantumRange*q);
      *green=RoundToQuantum(QuantumRange*brightness);
      *blue=RoundToQuantum(QuantumRange*p);
      break;
    }
    case 2:
    {
      *red=RoundToQuantum(QuantumRange*p);
      *green=RoundToQuantum(QuantumRange*brightness);
      *blue=RoundToQuantum(QuantumRange*t);
      break;
    }
    case 3:
    {
      *red=RoundToQuantum(QuantumRange*p);
      *green=RoundToQuantum(QuantumRange*q);
      *blue=RoundToQuantum(QuantumRange*brightness);
      break;
    }
    case 4:
    {
      *red=RoundToQuantum(QuantumRange*t);
      *green=RoundToQuantum(QuantumRange*p);
      *blue=RoundToQuantum(QuantumRange*brightness);
      break;
    }
    case 5:
    {
      *red=RoundToQuantum(QuantumRange*brightness);
      *green=RoundToQuantum(QuantumRange*p);
      *blue=RoundToQuantum(QuantumRange*q);
      break;
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H S L T r a n s f o r m                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HSLTransform() converts a (hue, saturation, luminosity) to a (red, green,
%  blue) triple.
%
%  The format of the HSLTransformImage method is:
%
%      void HSLTransform(const double hue,const double saturation,
%        const double luminosity,Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o hue, saturation, luminosity: A double value representing a
%      component of the HSL color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void HSLTransform(const double hue,const double saturation,
  const double luminosity,Quantum *red,Quantum *green,Quantum *blue)
{
  MagickRealType
    b,
    g,
    r,
    v,
    x,
    y,
    z;

  /*
    Convert HSL to RGB colorspace.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  if (saturation == 0.0)
    {
      *red=(Quantum) (QuantumRange*luminosity+0.5);
      *green=(Quantum) (QuantumRange*luminosity+0.5);
      *blue=(Quantum) (QuantumRange*luminosity+0.5);
      return;
    }
  v=(luminosity <= 0.5) ? (luminosity*(1.0+saturation)) :
    (luminosity+saturation-luminosity*saturation);
  y=2.0*luminosity-v;
  x=y+(v-y)*(6.0*hue-floor(6.0*hue));
  z=v-(v-y)*(6.0*hue-floor(6.0*hue));
  switch ((int) (6.0*hue))
  {
    case 0: r=v; g=x; b=y; break;
    case 1: r=z; g=v; b=y; break;
    case 2: r=y; g=v; b=x; break;
    case 3: r=y; g=z; b=v; break;
    case 4: r=x; g=y; b=v; break;
    case 5: r=v; g=y; b=z; break;
    default: r=v; g=x; b=y; break;
  }
  *red=RoundToQuantum(QuantumRange*r);
  *green=RoundToQuantum(QuantumRange*g);
  *blue=RoundToQuantum(QuantumRange*b);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H W B T r a n s f o r m                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HWBTransform() converts a (hue, whiteness, blackness) to a (red, green,
%  blue) triple.
%
%  The format of the HWBTransformImage method is:
%
%      void HWBTransform(const double hue,const double whiteness,
%        const double blackness,Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o hue, whiteness, blackness: A double value representing a
%      component of the HWB color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void HWBTransform(const double hue,const double whiteness,
  const double blackness,Quantum *red,Quantum *green,Quantum *blue)
{
  MagickRealType
    b,
    f,
    g,
    n,
    r,
    v;

  register long
    i;

  /*
    Convert HWB to RGB colorspace.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  v=1.0-blackness;
  if (hue == 0.0)
    {
      *red=(Quantum) (QuantumRange*v+0.5);
      *green=(Quantum) (QuantumRange*v+0.5);
      *blue=(Quantum) (QuantumRange*v+0.5);
      return;
    }
  i=(long) floor(hue);
  f=hue-i;
  if ((i & 0x01) != 0)
    f=1.0-f;
  n=whiteness+f*(v-whiteness);  /* linear interpolation */
  switch (i)
  {
    default:
    case 6:
    case 0: r=v; g=n; b=whiteness; break;
    case 1: r=n; g=v; b=whiteness; break;
    case 2: r=whiteness; g=v; b=n; break;
    case 3: r=whiteness; g=n; b=v; break;
    case 4: r=n; g=whiteness; b=v; break;
    case 5: r=v; g=whiteness; b=n; break;
  }
  *red=(Quantum) (QuantumRange*r+0.5);
  *green=(Quantum) (QuantumRange*g+0.5);
  *blue=(Quantum) (QuantumRange*b+0.5);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H u l l                                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Hull() implements the eight hull algorithm described in Applied Optics, Vol
%  24, No. 10, 15 May 1985, "Geometric filter for Speckle Reduction", by Thomas
%  R Crimmins.  Each pixel in the image is replaced by one of its eight of its
%  surrounding pixels using a polarity and negative hull function.
%
%  The format of the Hull method is:
%
%      void Hull(const long x_offset,const long y_offset,
%        const unsigned long columns,const unsigned long rows,Quantum *f,
%        Quantum *g,const int polarity)
%
%  A description of each parameter follows:
%
%    o x_offset, y_offset: An integer value representing the offset of the
%      current pixel within the image.
%
%    o columns, rows: Specifies the number of rows and columns in the image.
%
%    o polarity: An integer value declaring the polarity (+,-).
%
%    o f, g: A pointer to an image pixel and one of it's neighbor.
%
%
*/
MagickExport void Hull(const long x_offset,const long y_offset,
  const unsigned long columns,const unsigned long rows,Quantum *f,Quantum *g,
  const int polarity)
{
  long
    y;

  MagickRealType
    v;

  register long
    x;

  register Quantum
    *p,
    *q,
    *r,
    *s;

  assert(f != (Quantum *) NULL);
  assert(g != (Quantum *) NULL);
  p=f+(columns+2);
  q=g+(columns+2);
  r=p+(y_offset*((long) columns+2)+x_offset);
  for (y=0; y < (long) rows; y++)
  {
    p++;
    q++;
    r++;
    if (polarity > 0)
      for (x=(long) columns; x > 0; x--)
      {
        v=(MagickRealType) (*p);
        if ((MagickRealType) *r >= (v+(MagickRealType) ScaleCharToQuantum(2)))
          v+=ScaleCharToQuantum(1);
        *q=(Quantum) v;
        p++;
        q++;
        r++;
      }
    else
      for (x=(long) columns; x > 0; x--)
      {
        v=(MagickRealType) (*p);
        if ((MagickRealType) *r <= (v-(MagickRealType) ScaleCharToQuantum(2)))
          v-=(long) ScaleCharToQuantum(1);
        *q=(Quantum) v;
        p++;
        q++;
        r++;
      }
    p++;
    q++;
    r++;
  }
  p=f+(columns+2);
  q=g+(columns+2);
  r=q+(y_offset*((long) columns+2)+x_offset);
  s=q-(y_offset*((long) columns+2)+x_offset);
  for (y=0; y < (long) rows; y++)
  {
    p++;
    q++;
    r++;
    s++;
    if (polarity > 0)
      for (x=(long) columns; x > 0; x--)
      {
        v=(MagickRealType) (*q);
        if (((MagickRealType) *s >=
             (v+(MagickRealType) ScaleCharToQuantum(2))) &&
            ((MagickRealType) *r > v))
          v+=ScaleCharToQuantum(1);
        *p=(Quantum) v;
        p++;
        q++;
        r++;
        s++;
      }
    else
      for (x=(long) columns; x > 0; x--)
      {
        v=(MagickRealType) (*q);
        if (((MagickRealType) *s <=
             (v-(MagickRealType) ScaleCharToQuantum(2))) &&
            ((MagickRealType) *r < v))
          v-=(MagickRealType) ScaleCharToQuantum(1);
        *p=(Quantum) v;
        p++;
        q++;
        r++;
        s++;
      }
    p++;
    q++;
    r++;
    s++;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t e C o l o r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InterpolateColor() applies bi-linear interpolation between a pixel and its
%  neighbors.
%
%  The format of the InterpolateColor method is:
%
%      MagickPixelPacket InterpolateColor(const Image *image,
%        const double x_offset,const double y_offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x_offset,y_offset: A double representing the current (x,y) position of
%      the pixel.
%
%
*/

MagickExport MagickPixelPacket InterpolateColor(const Image *image,
  const double x_offset,const double y_offset,ExceptionInfo *exception)
{
  MagickPixelPacket
    pixel,
    pixels[4];

  MagickRealType
    alpha[4],
    gamma;

  PointInfo
    delta;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  GetMagickPixelPacket(image,&pixel);
  p=AcquireImagePixels(image,(long) x_offset,(long) y_offset,2,2,exception);
  if (p == (const PixelPacket *) NULL)
    return(pixel);
  for (i=0; i < 4L; i++)
  {
    indexes=GetIndexes(image);
    alpha[i]=1.0;
    if (image->matte != MagickFalse)
      alpha[i]=((MagickRealType) QuantumRange-p->opacity)/QuantumRange;
    GetMagickPixelPacket(image,pixels+i);
    SetMagickPixelPacket(p,indexes,pixels+i);
    pixels[i].red*=alpha[i];
    pixels[i].green*=alpha[i];
    pixels[i].blue*=alpha[i];
    if (image->colorspace == CMYKColorspace)
      pixels[i].index*=alpha[i];
    p++;
  }
  delta.x=x_offset-floor(x_offset);
  delta.y=y_offset-floor(y_offset);
  gamma=(((1.0-delta.y)*((1.0-delta.x)*alpha[0]+delta.x*alpha[1])+
    delta.y*((1.0-delta.x)*alpha[2]+delta.x*alpha[3])));
  gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
  pixel.red=(gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].red+delta.x*
    pixels[1].red)+delta.y*((1.0-delta.x)*pixels[2].red+delta.x*
    pixels[3].red)));
  pixel.green=(gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].green+delta.x*
    pixels[1].green)+delta.y*((1.0-delta.x)*pixels[2].green+delta.x*
    pixels[3].green)));
  pixel.blue=(gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].blue+delta.x*
    pixels[1].blue)+delta.y*((1.0-delta.x)*pixels[2].blue+delta.x*
    pixels[3].blue)));
  if (image->matte != MagickFalse)
    pixel.opacity=((1.0-delta.y)*((1.0-delta.x)*pixels[0].opacity+delta.x*
      pixels[1].opacity)+delta.y*((1.0-delta.x)*pixels[2].opacity+delta.x*
      pixels[3].opacity));
  if (image->colorspace == CMYKColorspace)
    pixel.index=(gamma*((1.0-delta.y)*((1.0-delta.x)*pixels[0].index+delta.x*
      pixels[1].index)+delta.y*((1.0-delta.x)*pixels[2].index+delta.x*
      pixels[3].index)));
  return(pixel);
}

MagickExport MagickPixelPacket IInterpolateColor(const Image *image,
  const double x_offset,const double y_offset,ExceptionInfo *exception)
{
  MagickPixelPacket
    pixel,
    pixels[4];

  MagickRealType
    alpha[4],
    gamma;

  PointInfo
    delta;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  GetMagickPixelPacket(image,&pixel);
  p=AcquireImagePixels(image,(long) x_offset,(long) y_offset,2,2,exception);
  if (p == (const PixelPacket *) NULL)
    return(pixel);
  indexes=GetIndexes(image);
  for (i=0; i < 4L; i++)
  {
    alpha[i]=1.0;
    if (image->matte != MagickFalse)
      alpha[i]=((MagickRealType) QuantumRange-p->opacity)/QuantumRange;
    GetMagickPixelPacket(image,pixels+i);
    SetMagickPixelPacket(p,indexes+i,pixels+i);
    pixels[i].red*=alpha[i];
    pixels[i].green*=alpha[i];
    pixels[i].blue*=alpha[i];
    if (image->colorspace == CMYKColorspace)
      pixels[i].index*=alpha[i];
    p++;
  }
  delta.x=x_offset-(long) x_offset;
  delta.y=y_offset-(long) y_offset;
  gamma=(alpha[0]+(alpha[2]-alpha[0])*delta.y+(alpha[1]-alpha[0])*delta.x+
    (alpha[0]+alpha[3]-alpha[2]-alpha[1])*delta.x*delta.y);
  gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
  pixel.red=gamma*(pixels[0].red+(pixels[2].red-pixels[0].red)*delta.y+
    (pixels[1].red-pixels[0].red)*delta.x+(pixels[0].red+pixels[3].red-
    pixels[2].red-pixels[1].red)*delta.x*delta.y);
  pixel.green=gamma*(pixels[0].green+(pixels[2].green-pixels[0].green)*
    delta.y+(pixels[1].green-pixels[0].green)*delta.x+(pixels[0].green+
    pixels[3].green-pixels[2].green-pixels[1].green)*delta.x*delta.y);
  pixel.blue=gamma*(pixels[0].blue+(pixels[2].blue-pixels[0].blue)*delta.y+
    (pixels[1].blue-pixels[0].blue)*delta.x+(pixels[0].blue+pixels[3].blue-
    pixels[2].blue-pixels[1].blue)*delta.x*delta.y);
  if (image->matte != MagickFalse)
    pixel.opacity=(pixels[0].opacity+(pixels[2].opacity-pixels[0].opacity)*
      delta.y+(pixels[1].opacity-pixels[0].opacity)*delta.x+(pixels[0].opacity+
      pixels[3].opacity-pixels[2].opacity-pixels[1].opacity)*delta.x*delta.y);
  if (image->colorspace == CMYKColorspace)
    pixel.index=gamma*(pixels[0].index+(pixels[2].index-pixels[0].index)*
      delta.y+(pixels[1].index-pixels[0].index)*delta.x+(pixels[0].index+
      pixels[3].index-pixels[2].index-pixels[1].index)*delta.x*delta.y);
  return(pixel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o d u l a t e H S B                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModulateHSB() modulates the hue, saturation, and brightness of an image.
%
%  The format of the ModulateHSB method is:
%
%      void ModulateHSB(const double percent_hue,
%        const double percent_saturation,const double percent_brightness,
%        Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o percent_hue, percent_saturation, percent_brightness: A double value
%      representing the percent change in a component of the HSB color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void ModulateHSB(const double percent_hue,
  const double percent_saturation,const double percent_brightness,
  Quantum *red,Quantum *green,Quantum *blue)
{
  double
    brightness,
    hue,
    saturation;

  /*
    Increase or decrease color brightness, saturation, or hue.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  TransformHSB(*red,*green,*blue,&hue,&saturation,&brightness);
  hue+=0.5*(0.01*percent_hue-1.0);
  while (hue < 0.0)
    hue+=1.0;
  while (hue > 1.0)
    hue-=1.0;
  saturation*=0.01*percent_saturation;
  brightness*=0.01*percent_brightness;
  HSBTransform(hue,saturation,brightness,red,green,blue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o d u l a t e H S L                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModulateHSL() modulates the hue, saturation, and brightness of an image.
%
%  The format of the ModulateHSL method is:
%
%      void ModulateHSL(const double percent_hue,
%        const double percent_saturation,const double percent_luminosity,
%        Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o percent_hue, percent_saturation, percent_luminosity: A double value
%      representing the percent change in a component of the HSL color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void ModulateHSL(const double percent_hue,
  const double percent_saturation,const double percent_luminosity,
  Quantum *red,Quantum *green,Quantum *blue)
{
  double
    hue,
    luminosity,
    saturation;

  /*
    Increase or decrease color luminosity, saturation, or hue.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  TransformHSL(*red,*green,*blue,&hue,&saturation,&luminosity);
  hue+=0.5*(0.01*percent_hue-1.0);
  while (hue < 0.0)
    hue+=1.0;
  while (hue > 1.0)
    hue-=1.0;
  saturation*=0.01*percent_saturation;
  luminosity*=0.01*percent_luminosity;
  HSLTransform(hue,saturation,luminosity,red,green,blue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o d u l a t e H W B                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModulateHWB() modulates the hue, whiteness, and blackness of an image.
%
%  The format of the ModulateHWB method is:
%
%      void ModulateHWB(const double percent_hue,
%        const double percent_whiteness,const double percent_blackness,
%        Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o percent_hue, percent_whiteness, percent_blackness: A double value
%      representing the percent change in a component of the HWB color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void ModulateHWB(const double percent_hue,
  const double percent_whiteness,const double percent_blackness,
  Quantum *red,Quantum *green,Quantum *blue)
{
  double
    blackness,
    hue,
    whiteness;

  /*
    Increase or decrease color blackness, whiteness, or hue.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  TransformHWB(*red,*green,*blue,&hue,&whiteness,&blackness);
  hue+=0.5*(0.01*percent_hue-1.0);
  while (hue < 0.0)
    hue+=1.0;
  while (hue > 1.0)
    hue-=1.0;
  blackness*=0.01*percent_blackness;
  whiteness*=0.01*percent_whiteness;
  HWBTransform(hue,whiteness,blackness,red,green,blue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s f o r m H S B                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransformHSB() converts a (red, green, blue) to a (hue, saturation,
%  brightness) triple.
%
%  The format of the TransformHSB method is:
%
%      void TransformHSB(const Quantum red,const Quantum green,
%        const Quantum blue,double *hue,double *saturation,double *brightness)
%
%  A description of each parameter follows:
%
%    o red, green, blue: A Quantum value representing the red, green, and
%      blue component of a pixel..
%
%    o hue, saturation, brightness: A pointer to a double value representing a
%      component of the HSB color space.
%
%
*/
MagickExport void TransformHSB(const Quantum red,const Quantum green,
  const Quantum blue,double *hue,double *saturation,double *brightness)
{
  MagickRealType
    delta,
    max,
    min;

  /*
    Convert RGB to HSB colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(brightness != (double *) NULL);
  max=(MagickRealType) (red > green ? red : green);
  if ((MagickRealType) blue > max)
    max=(MagickRealType) blue;
  min=(MagickRealType) (red < green ? red : green);
  if ((MagickRealType) blue < min)
    min=(MagickRealType) blue;
  *brightness=(double) (QuantumScale*max);
  if (max == 0.0)
    {
      *saturation=0.0;
      *hue=0.0;
      return;
    }
  *saturation=(double) (1.0-min/max);
  delta=max-min;
  if ((MagickRealType) red == max)
    *hue=(double) ((green-(MagickRealType) blue)/delta);
  else
    if ((MagickRealType) green == max)
      *hue=(double) (2.0+(blue-(MagickRealType) red)/delta);
    else
      *hue=(double) (4.0+(red-(MagickRealType) green)/delta);
  *hue/=6.0;
  if (*hue < 0.0)
    *hue+=1.0;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s f o r m H S L                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransformHSL() converts a (red, green, blue) to a (hue, saturation,
%  luminosity) triple.
%
%  The format of the TransformHSL method is:
%
%      void TransformHSL(const Quantum red,const Quantum green,
%        const Quantum blue,double *hue,double *saturation,double *luminosity)
%
%  A description of each parameter follows:
%
%    o red, green, blue: A Quantum value representing the red, green, and
%      blue component of a pixel..
%
%    o hue, saturation, luminosity: A pointer to a double value representing a
%      component of the HSL color space.
%
%
*/
MagickExport void TransformHSL(const Quantum red,const Quantum green,
  const Quantum blue,double *hue,double *saturation,double *luminosity)
{
  MagickRealType
    b,
    delta,
    g,
    max,
    min,
    r;

  /*
    Convert RGB to HSL colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(luminosity != (double *) NULL);
  r=(MagickRealType) red/QuantumRange;
  g=(MagickRealType) green/QuantumRange;
  b=(MagickRealType) blue/QuantumRange;
  max=Max(r,Max(g,b));
  min=Min(r,Min(g,b));
  *hue=0.0;
  *saturation=0.0;
  *luminosity=(double) ((min+max)/2.0);
  delta=max-min;
  if (delta == 0.0)
    return;
  *saturation=(double) (delta/((*luminosity < 0.5) ? (min+max) :
    (2.0-max-min)));
  if (r == max)
    *hue=(double) (g == min ? 5.0+(max-b)/delta : 1.0-(max-g)/delta);
  else
    if (g == max)
      *hue=(double) (b == min ? 1.0+(max-r)/delta : 3.0-(max-b)/delta);
    else
      *hue=(double) (r == min ? 3.0+(max-g)/delta : 5.0-(max-r)/delta);
  *hue/=6.0;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s f o r m H W B                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TransformHWB() converts a (red, green, blue) to a (hue, whiteness,
%  blackness) triple.
%
%  The format of the TransformHWB method is:
%
%      void TransformHWB(const Quantum red,const Quantum green,
%        const Quantum blue,double *hue,double *whiteness,double *blackness)
%
%  A description of each parameter follows:
%
%    o red, green, blue: A Quantum value representing the red, green, and
%      blue component of a pixel.
%
%    o hue, whiteness, blackness: A pointer to a double value representing a
%      component of the HWB color space.
%
%
*/
MagickExport void TransformHWB(const Quantum red,const Quantum green,
  const Quantum blue,double *hue,double *whiteness,double *blackness)
{
  MagickRealType
    f;

  register long
    i;

  Quantum
    v,
    w;

  /*
    Convert RGB to HWB colorspace.
  */
  assert(hue != (double *) NULL);
  assert(whiteness != (double *) NULL);
  assert(blackness != (double *) NULL);
  w=Min(red,Min(green,blue));
  v=Max(red,Max(green,blue));
  *blackness=(double) (QuantumRange-v)/QuantumRange;
  if (v == w)
    {
      *hue=0.0;
      *whiteness=1.0-(*blackness);
      return;
    }
  f=(red == w) ? (MagickRealType) green-blue : ((green == w) ?
    (MagickRealType) blue-red : (MagickRealType) red-green);
  i=(red == w) ? 3 : ((green == w) ? 5 : 1);
  *hue=(double) (i-f/(v-w));
  *whiteness=(double) w/QuantumRange;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U p s a m p l e                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Upsample() doubles the size of the image.
%
%  The format of the Upsample method is:
%
%      void Upsample(const unsigned long width,const unsigned long height,
%        const unsigned long scaled_width,unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o width,height:  Unsigned values representing the width and height of
%      the image pixel array.
%
%    o scaled_width:  Specifies the final width of the upsampled pixel array.
%
%    o pixels:  An unsigned char containing the pixel data.  On output the
%      upsampled pixels are returns here.
%
%
*/
MagickExport void Upsample(const unsigned long width,const unsigned long height,
  const unsigned long scaled_width,unsigned char *pixels)
{
  register long
    x,
    y;

  register unsigned char
    *p,
    *q,
    *r;

  /*
    Create a new image that is a integral size greater than an existing one.
  */
  assert(pixels != (unsigned char *) NULL);
  for (y=0; y < (long) height; y++)
  {
    p=pixels+(height-1-y)*scaled_width+(width-1);
    q=pixels+((height-1-y) << 1)*scaled_width+((width-1) << 1);
    *q=(*p);
    *(q+1)=(*(p));
    for (x=1; x < (long) width; x++)
    {
      p--;
      q-=2;
      *q=(*p);
      *(q+1)=(unsigned char) ((((unsigned long) *p)+
        ((unsigned long) *(p+1))+1) >> 1);
    }
  }
  for (y=0; y < (long) (height-1); y++)
  {
    p=pixels+((unsigned long) y << 1)*scaled_width;
    q=p+scaled_width;
    r=q+scaled_width;
    for (x=0; x < (long) (width-1); x++)
    {
      *q=(unsigned char) ((((unsigned long) *p)+((unsigned long) *r)+1) >> 1);
      *(q+1)=(unsigned char) ((((unsigned long) *p)+((unsigned long) *(p+2))+
        ((unsigned long) *r)+((unsigned long) *(r+2))+2) >> 2);
      q+=2;
      p+=2;
      r+=2;
    }
    *q++=(unsigned char) ((((unsigned long) *p++)+
      ((unsigned long) *r++)+1) >> 1);
    *q++=(unsigned char) ((((unsigned long) *p++)+
      ((unsigned long) *r++)+1) >> 1);
  }
  p=pixels+(2*height-2)*scaled_width;
  q=pixels+(2*height-1)*scaled_width;
  (void) CopyMagickMemory(q,p,(size_t) (2*width));
}
