/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        CCCC   OOO  M   M  PPPP    OOO   SSSSS  IIIII  TTTTT  EEEEE          %
%       C      O   O MM MM  P   P  O   O  SS       I      T    E              %
%       C      O   O M M M  PPPP   O   O   SSS     I      T    EEE            %
%       C      O   O M   M  P      O   O     SS    I      T    E              %
%        CCCC   OOO  M   M  P       OOO   SSSSS  IIIII    T    EEEEE          %
%                                                                             %
%                                                                             %
%                    ImageMagick Image Composite Methods                      %
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
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/constitute.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/mogrify.h"
#include "magick/mogrify-private.h"
#include "magick/option.h"
#include "magick/pixel-private.h"
#include "magick/resource_.h"
#include "magick/string_.h"
#include "magick/utility.h"
#include "magick/version.h"

/*
  Typedef declarations.
*/
typedef struct _CompositeOptions
{
  char
    *blend_geometry,
    *displace_geometry,
    *dissolve_geometry,
    *geometry,
    *unsharp_geometry,
    *watermark_geometry;

  CompositeOperator
    compose;

  GravityType
    gravity;

  long
    stegano;

  MagickBooleanType
    stereo,
    tile;
} CompositeOptions;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p o s i t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImage() returns the second image composited onto the first at the
%  specified offsets.
%
%  The format of the CompositeImage method is:
%
%      MagickBooleanType CompositeImage(Image *image,
%        const CompositeOperator compose,const Image *composite_image,
%        const long x_offset,const long y_offset)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o compose: This operator affects how the composite is applied to
%      the image.  The operators and how they are utilized are listed here
%      http://www.w3.org/TR/SVG12/#compositing.
%
%    o composite_image: The composite image.
%
%    o x_offset: The column offset of the composited image.
%
%    o y_offset: The row offset of the composited image.
%
%
*/

static inline MagickRealType Add(const MagickRealType p,const MagickRealType q)
{
  MagickRealType
    pixel;

  pixel=p+q;
  if (pixel > QuantumRange)
    pixel-=(QuantumRange+1.0);
  return(pixel);
}

static inline void CompositeAdd(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  composite->red=Add(p->red,q->red);
  composite->green=Add(p->green,q->green);
  composite->blue=Add(p->blue,q->blue);
  composite->opacity=Add(alpha,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=Add(p->index,q->index);
}

static inline MagickRealType Atop(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=((1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)+
    (1.0-QuantumScale*beta)*q*QuantumScale*alpha);
  return(pixel);
}

static inline void CompositeAtop(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=(1.0-QuantumScale*beta);
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Atop(p->red,alpha,q->red,beta);
  composite->green=gamma*Atop(p->green,alpha,q->green,beta);
  composite->blue=gamma*Atop(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Atop(p->index,alpha,q->index,beta);
}

static inline void CompositeBumpmap(const MagickPixelPacket *p,
  const MagickRealType magick_unused(alpha),const MagickPixelPacket *q,
  const MagickRealType magick_unused(beta),MagickPixelPacket *composite)
{
  MagickRealType
    intensity;

  intensity=(MagickRealType) PixelIntensityToQuantum(p);
  composite->red=QuantumScale*intensity*q->red;
  composite->green=QuantumScale*intensity*q->green;
  composite->blue=QuantumScale*intensity*q->blue;
  composite->opacity=QuantumScale*intensity*p->opacity;
  if (q->colorspace == CMYKColorspace)
    composite->index=QuantumScale*intensity*q->index;
}

static inline void CompositeClear(const MagickPixelPacket *q,
  MagickPixelPacket *composite)
{
  composite->red=0.0;
  composite->green=0.0;
  composite->blue=0.0;
  composite->opacity=(MagickRealType) TransparentOpacity;
  if (q->colorspace == CMYKColorspace)
    composite->index=0.0;
}

static inline MagickRealType ColorBurn(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    delta,
    pixel;

  delta=QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha);
  if (delta <= ((1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)))
    {
      pixel=QuantumScale*(1.0-QuantumScale*alpha)*p*
        (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
        (1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=(1.0-QuantumScale*alpha)*(QuantumScale*(1.0-QuantumScale*alpha)*p*
    (1.0-QuantumScale*beta)+QuantumScale*(1.0-QuantumScale*beta)*q*
    (1.0-QuantumScale*alpha)-(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta))/
    QuantumScale*(1.0-QuantumScale*alpha)*p+QuantumScale*(1.0-
    QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeColorBurn(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*ColorBurn(p->red,alpha,q->red,beta);
  composite->green=gamma*ColorBurn(p->green,alpha,q->green,beta);
  composite->blue=gamma*ColorBurn(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*ColorBurn(p->index,alpha,q->index,beta);
}

static inline MagickRealType ColorDodge(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    delta,
    pixel;

  delta=QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha);
  if (delta >= ((1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)))
    {
      pixel=(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)+QuantumScale*
        (1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+QuantumScale*
        (1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)/
    (1.0-QuantumScale*(1.0-QuantumScale*alpha)*p/(1.0-QuantumScale*alpha))+
    QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeColorDodge(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*ColorDodge(p->red,alpha,q->red,beta);
  composite->green=gamma*ColorDodge(p->green,alpha,q->green,beta);
  composite->blue=gamma*ColorDodge(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*ColorDodge(p->index,alpha,q->index,beta);
}

static inline MagickRealType Darken(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if (((1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)) <
      ((1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)))
    {
      pixel=(1.0-QuantumScale*alpha)*p+(1.0-QuantumScale*beta)*q*QuantumScale*
        alpha;
      return(pixel);
    }
  pixel=(1.0-QuantumScale*beta)*q+(1.0-QuantumScale*alpha)*p*QuantumScale*beta;
  return(pixel);
}

static inline void CompositeDarken(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Darken(p->red,alpha,q->red,beta);
  composite->green=gamma*Darken(p->green,alpha,q->green,beta);
  composite->blue=gamma*Darken(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Darken(p->index,alpha,q->index,beta);
}

static inline MagickRealType Difference(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=QuantumScale*(1.0-QuantumScale*alpha)*p+QuantumScale*
    (1.0-QuantumScale*beta)*q-2.0*Min(QuantumScale*(1.0-QuantumScale*alpha)*p*
    (1.0-QuantumScale*beta),QuantumScale*(1.0-QuantumScale*beta)*q*
    (1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeDifference(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Difference(p->red,alpha,q->red,beta);
  composite->green=gamma*Difference(p->green,alpha,q->green,beta);
  composite->blue=gamma*Difference(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Difference(p->index,alpha,q->index,beta);
}

static inline MagickRealType Exclusion(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=(QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)-2.0*
    QuantumScale*(1.0-QuantumScale*alpha)*p*QuantumScale*
    (1.0-QuantumScale*beta)*q)+QuantumScale*(1.0-QuantumScale*alpha)*p*
    (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
    (1.0 -(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeExclusion(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Exclusion(p->red,alpha,q->red,beta);
  composite->green=gamma*Exclusion(p->green,alpha,q->green,beta);
  composite->blue=gamma*Exclusion(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Exclusion(p->index,alpha,q->index,beta);
}

static inline MagickRealType HardLight(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if ((2.0*QuantumScale*(1.0-QuantumScale*alpha)*p) < (1.0-QuantumScale*alpha))
    {
      pixel=2.0*QuantumScale*(1.0-QuantumScale*alpha)*p*QuantumScale*
        (1.0-QuantumScale*beta)*q+QuantumScale*(1.0-QuantumScale*alpha)*p*
        (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
        (1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)-2.0*
    ((1.0-QuantumScale*beta)-QuantumScale*(1.0-QuantumScale*beta)*q)*
    ((1.0-QuantumScale*alpha)-QuantumScale*(1.0-QuantumScale*alpha)*p)+
    QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeHardLight(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*HardLight(p->red,alpha,q->red,beta);
  composite->green=gamma*HardLight(p->green,alpha,q->green,beta);
  composite->blue=gamma*HardLight(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*HardLight(p->index,alpha,q->index,beta);
}

static void CompositeHSB(const MagickRealType red,const MagickRealType green,
  const MagickRealType blue,double *hue,double *saturation,double *brightness)
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
  max=(red > green ? red : green);
  if (blue > max)
    max=blue;
  min=(red < green ? red : green);
  if (blue < min)
    min=blue;
  *hue=0.0;
  *saturation=0.0;
  if (max != 0.0)
    *saturation=(double) ((max-min)/max);
  *brightness=(double) (QuantumScale*max);
  if (*saturation == 0.0)
    return;
  delta=max-min;
  if (red == max)
    *hue=(double) ((green-blue)/delta);
  else
    if (green == max)
      *hue=(double) (2.0+(blue-red)/delta);
    else
      if (blue == max)
        *hue=(double) (4.0+(red-green)/delta);
  *hue/=6.0;
  if (*hue < 0.0)
    *hue+=1.0;
  else
    if (*hue > 1.0)
      *hue-=1.0;
}

static inline MagickRealType In(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType magick_unused(q),
  const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta);
  return(pixel);
}

static inline void CompositeIn(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta);
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*In(p->red,alpha,q->red,beta);
  composite->green=gamma*In(p->green,alpha,q->green,beta);
  composite->blue=gamma*In(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*In(p->index,alpha,q->index,beta);
}

static inline MagickRealType Lighten(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if (((1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)) >
      ((1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)))
    {
      pixel=(1.0-QuantumScale*alpha)*p+(1.0-QuantumScale*beta)*q*QuantumScale*
        alpha;
      return(pixel);
    }
  pixel=(1.0-QuantumScale*beta)*q+(1.0-QuantumScale*alpha)*p*QuantumScale*beta;
  return(pixel);
}

static inline void CompositeLighten(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Lighten(p->red,alpha,q->red,beta);
  composite->green=gamma*Lighten(p->green,alpha,q->green,beta);
  composite->blue=gamma*Lighten(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Lighten(p->index,alpha,q->index,beta);
}

static inline MagickRealType Minus(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  return((1.0-QuantumScale*alpha)*p-(1.0-QuantumScale*beta)*q);
}

static inline void CompositeMinus(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)-(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Minus(p->red,alpha,q->red,beta);
  composite->green=gamma*Minus(p->green,alpha,q->green,beta);
  composite->blue=gamma*Minus(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Minus(p->index,alpha,q->index,beta);
}

static inline MagickRealType Multiply(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-QuantumScale*beta)*q+
    (1.0-QuantumScale*alpha)*p*QuantumScale*beta+(1.0-QuantumScale*beta)*q*
    QuantumScale*alpha;
  return(pixel);
}

static inline void CompositeMultiply(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Multiply(p->red,alpha,q->red,beta);
  composite->green=gamma*Multiply(p->green,alpha,q->green,beta);
  composite->blue=gamma*Multiply(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Multiply(p->index,alpha,q->index,beta);
}

static inline MagickRealType Out(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType magick_unused(q),
  const MagickRealType beta)
{
  return((1.0-QuantumScale*alpha)*p*QuantumScale*beta);
}

static inline void CompositeOut(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=(1.0-QuantumScale*alpha)*QuantumScale*beta;
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Out(p->red,alpha,q->red,beta);
  composite->green=gamma*Out(p->green,alpha,q->green,beta);
  composite->blue=gamma*Out(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Out(p->index,alpha,q->index,beta);
}

static inline void CompositeOver(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=1.0-QuantumScale*QuantumScale*alpha*beta;
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*MagickOver_(p->red,alpha,q->red,beta);
  composite->green=gamma*MagickOver_(p->green,alpha,q->green,beta);
  composite->blue=gamma*MagickOver_(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*MagickOver_(p->index,alpha,q->index,beta);
}

static inline MagickRealType Overlay(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if ((2.0*QuantumScale*(1.0-QuantumScale*beta)*q) < (1.0-QuantumScale*beta))
    {
      pixel=2.0*QuantumScale*(1.0-QuantumScale*alpha)*p*QuantumScale*
        (1.0-QuantumScale*beta)*q+QuantumScale*(1.0-QuantumScale*alpha)*p*
        (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
        (1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta)-2.0*
    ((1.0-QuantumScale*beta)-QuantumScale*(1.0-QuantumScale*beta)*q)*
    ((1.0-QuantumScale*alpha)-QuantumScale*(1.0-QuantumScale*alpha)*p)+
    QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeOverlay(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Overlay(p->red,alpha,q->red,beta);
  composite->green=gamma*Overlay(p->green,alpha,q->green,beta);
  composite->blue=gamma*Overlay(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Overlay(p->index,alpha,q->index,beta);
}

static inline MagickRealType Plus(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  return((1.0-QuantumScale*alpha)*p+(1.0-QuantumScale*beta)*q);
}

static inline void CompositePlus(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Plus(p->red,alpha,q->red,beta);
  composite->green=gamma*Plus(p->green,alpha,q->green,beta);
  composite->blue=gamma*Plus(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Plus(p->index,alpha,q->index,beta);
}

static inline MagickRealType Screen(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=QuantumScale*(1.0-QuantumScale*alpha)*p+QuantumScale*
    (1.0-QuantumScale*beta)*q-QuantumScale*(1.0-QuantumScale*alpha)*p*
    QuantumScale*(1.0-QuantumScale*beta)*q;
  return(QuantumRange*pixel);
}

static inline void CompositeScreen(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Screen(p->red,alpha,q->red,beta);
  composite->green=gamma*Screen(p->green,alpha,q->green,beta);
  composite->blue=gamma*Screen(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Screen(p->index,alpha,q->index,beta);
}

static inline MagickRealType SoftLight(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  if ((2.0*QuantumScale*(1.0-QuantumScale*alpha)*p) < (1.0-QuantumScale*alpha))
    {
      pixel=QuantumScale*(1.0-QuantumScale*beta)*q*((1.0-QuantumScale*alpha)-
        (1.0-QuantumScale*(1.0-QuantumScale*beta)*q/(1.0-QuantumScale*beta))*
        (2.0*QuantumScale*(1.0-QuantumScale*alpha)*p-(1.0-QuantumScale*alpha)))+
        QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
        QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  if ((8.0*QuantumScale*(1.0-QuantumScale*beta)*q) <= (1.0-QuantumScale*beta))
    {
      pixel=QuantumScale*(1.0-QuantumScale*beta)*q*((1.0-QuantumScale*alpha)-
        (1.0-QuantumScale*(1.0-QuantumScale*beta)*q/(1.0-QuantumScale*beta))*
        (2.0*QuantumScale*(1.0-QuantumScale*alpha)*p-(1.0-QuantumScale*alpha))*
        (3.0-8.0*QuantumScale*(1.0-QuantumScale*beta)*q/
        (1.0-QuantumScale*beta)))+QuantumScale*(1.0-QuantumScale*alpha)*p*
        (1.0-(1.0-QuantumScale*beta))+QuantumScale*(1.0-QuantumScale*beta)*q*
        (1.0-(1.0-QuantumScale*alpha));
      return(QuantumRange*pixel);
    }
  pixel=(QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-QuantumScale*alpha)+
    (pow(QuantumScale*(1.0-QuantumScale*beta)*q/(1.0-QuantumScale*beta),0*5)*
    (1.0-QuantumScale*beta)-QuantumScale*(1.0-QuantumScale*beta)*q)*
    (2.0*QuantumScale*(1.0-QuantumScale*alpha)*p-(1.0-QuantumScale*alpha)))+
    QuantumScale*(1.0-QuantumScale*alpha)*p*(1.0-(1.0-QuantumScale*beta))+
    QuantumScale*(1.0-QuantumScale*beta)*q*(1.0-(1.0-QuantumScale*alpha));
  return(QuantumRange*pixel);
}

static inline void CompositeSoftLight(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=RoundToUnity((1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    (1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta));
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*SoftLight(p->red,alpha,q->red,beta);
  composite->green=gamma*SoftLight(p->green,alpha,q->green,beta);
  composite->blue=gamma*SoftLight(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*SoftLight(p->index,alpha,q->index,beta);
}

static inline MagickRealType Subtract(const MagickRealType p,
  const MagickRealType magick_unused(alpha),const MagickRealType q,
  const MagickRealType magick_unused(beta))
{
  MagickRealType
    pixel;

  pixel=p-q;
  if (pixel < 0.0)
    pixel+=(QuantumRange+1.0);
  return(pixel);
}

static inline void CompositeSubtract(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  composite->red=Subtract(p->red,alpha,q->red,beta);
  composite->green=Subtract(p->green,alpha,q->green,beta);
  composite->blue=Subtract(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=Subtract(p->index,alpha,q->index,beta);
}

static inline MagickRealType Threshold(const MagickRealType p,
  const MagickRealType magick_unused(alpha),const MagickRealType q,
	const MagickRealType magick_unused(beta),const MagickRealType threshold,
  const MagickRealType amount)
{
  MagickRealType
    pixel;

  pixel=p-q;
  if ((MagickRealType) fabs((double) (2.0*pixel)) < threshold)
    {
      pixel=q;
      return(pixel);
    }
  pixel=q+(pixel*amount);
  return(pixel);
}

static inline void CompositeThreshold(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,const MagickRealType threshold,
  const MagickRealType amount,MagickPixelPacket *composite)
{
  composite->red=Threshold(p->red,alpha,q->red,beta,threshold,amount);
  composite->green=Threshold(p->green,alpha,q->green,beta,threshold,amount);
  composite->blue=Threshold(p->blue,alpha,q->blue,beta,threshold,amount);
  composite->opacity=QuantumRange-
    Threshold(p->opacity,alpha,q->opacity,beta,threshold,amount);
  if (q->colorspace == CMYKColorspace)
    composite->index=Threshold(p->index,alpha,q->index,beta,threshold,amount);
}

static inline MagickRealType Xor(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  MagickRealType
    pixel;

  pixel=(1.0-QuantumScale*alpha)*p*QuantumScale*beta+(1.0-QuantumScale*beta)*q*
    QuantumScale*alpha;
  return(pixel);
}

static inline void CompositeXor(const MagickPixelPacket *p,
  const MagickRealType alpha,const MagickPixelPacket *q,
  const MagickRealType beta,MagickPixelPacket *composite)
{
  MagickRealType
    gamma;

  gamma=(1.0-QuantumScale*alpha)+(1.0-QuantumScale*beta)-
    2*(1.0-QuantumScale*alpha)*(1.0-QuantumScale*beta);
  composite->opacity=QuantumRange*(1.0-gamma);
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=gamma*Xor(p->red,alpha,q->red,beta);
  composite->green=gamma*Xor(p->green,alpha,q->green,beta);
  composite->blue=gamma*Xor(p->blue,alpha,q->blue,beta);
  if (q->colorspace == CMYKColorspace)
    composite->index=gamma*Xor(p->index,alpha,q->index,beta);
}

static void HSBComposite(const double hue,const double saturation,
  const double brightness,MagickRealType *red,MagickRealType *green,
  MagickRealType *blue)
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
  assert(red != (MagickRealType *) NULL);
  assert(green != (MagickRealType *) NULL);
  assert(blue != (MagickRealType *) NULL);
  if (saturation == 0.0)
    {
      *red=QuantumRange*brightness;
      *green=(*red);
      *blue=(*red);
      return;
    }
  h=6.0*hue;
  if (h == 6.0)
    h=0.0;
  f=h-(int) h;
  p=brightness*(1.0-saturation);
  q=brightness*(1.0-saturation*f);
  t=brightness*(1.0-saturation*(1.0-f));
  switch ((int) h)
  {
    case 0:
    default:
    {
      *red=QuantumRange*brightness;
      *green=QuantumRange*t;
      *blue=QuantumRange*p;
      break;
    }
    case 1:
    {
      *red=QuantumRange*q;
      *green=QuantumRange*brightness;
      *blue=QuantumRange*p;
      break;
    }
    case 2:
    {
      *red=QuantumRange*p;
      *green=QuantumRange*brightness;
      *blue=QuantumRange*t;
      break;
    }
    case 3:
    {
      *red=QuantumRange*p;
      *green=QuantumRange*q;
      *blue=QuantumRange*brightness;
      break;
    }
    case 4:
    {
      *red=QuantumRange*t;
      *green=QuantumRange*p;
      *blue=QuantumRange*brightness;
      break;
    }
    case 5:
    {
      *red=QuantumRange*brightness;
      *green=QuantumRange*p;
      *blue=QuantumRange*q;
      break;
    }
  }
}

MagickExport MagickBooleanType CompositeImage(Image *image,
  const CompositeOperator compose,const Image *composite_image,
  const long x_offset,const long y_offset)
{
  const ImageAttribute
    *attribute;

  const PixelPacket
    *pixels;

  double
    brightness,
    hue,
    sans,
    saturation;

  GeometryInfo
    geometry_info;

  IndexPacket
    *composite_indexes,
    *indexes;

  long
    y;

  MagickBooleanType
    modify_outside_overlay;

  MagickPixelPacket
    composite,
    destination,
    source;

  MagickRealType
    amount,
    destination_dissolve,
    midpoint,
    percent_brightness,
    percent_saturation,
    source_dissolve,
    threshold;

  MagickStatusType
    flags;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Prepare composite image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(composite_image != (Image *) NULL);
  assert(composite_image->signature == MagickSignature);
  if (compose == NoCompositeOp)
    return(MagickTrue);
  image->storage_class=DirectClass;
  amount=0.5;
  destination_dissolve=1.0;
  modify_outside_overlay=MagickFalse;
  percent_brightness=100.0;
  percent_saturation=100.0;
  source_dissolve=1.0;
  threshold=0.05f;
  switch (compose)
  {
    case ClearCompositeOp:
    case SrcCompositeOp:
    case SrcInCompositeOp:
    case InCompositeOp:
    case SrcOutCompositeOp:
    case OutCompositeOp:
    case DstInCompositeOp:
    case DstAtopCompositeOp:
    {
      /*
        These operators modify destination outside the overlaid region.
      */
      modify_outside_overlay=MagickTrue;
      break;
    }
    case CopyOpacityCompositeOp:
    {
      if (image->matte == MagickFalse)
        SetImageOpacity(image,OpaqueOpacity);
      modify_outside_overlay=MagickTrue;
      break;
    }
    case DisplaceCompositeOp:
    {
      Image
        *displace_image;

      MagickPixelPacket
        pixel;

      MagickRealType
        horizontal_scale,
        vertical_scale,
        x_displace,
        y_displace;

      register IndexPacket
        *displace_indexes;

      register PixelPacket
        *r;

      /*
        Allocate the displace image.
      */
      displace_image=CloneImage(composite_image,0,0,MagickTrue,
        &image->exception);
      if (displace_image == (Image *) NULL)
        return(MagickFalse);
      horizontal_scale=20.0;
      vertical_scale=20.0;
      if (composite_image->geometry != (char *) NULL)
        {
          /*
            Determine the horizontal and vertical displacement scale.
          */
          SetGeometryInfo(&geometry_info);
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          horizontal_scale=geometry_info.rho;
          vertical_scale=geometry_info.sigma;
          if ((flags & SigmaValue) == 0)
            vertical_scale=horizontal_scale;
        }
      /*
        Shift image pixels as defined by a displacement map.
      */
      GetMagickPixelPacket(displace_image,&pixel);
      for (y=0; y < (long) composite_image->rows; y++)
      {
        if (((y+y_offset) < 0) || ((y+y_offset) >= (long) image->rows))
          continue;
        p=AcquireImagePixels(composite_image,0,y,composite_image->columns,1,
          &image->exception);
        q=GetImagePixels(image,0,y+y_offset,image->columns,1);
        r=GetImagePixels(displace_image,0,y,displace_image->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL) ||
            (r == (PixelPacket *) NULL))
          break;
        displace_indexes=GetIndexes(displace_image);
        q+=x_offset;
        for (x=0; x < (long) composite_image->columns; x++)
        {
          if (((x_offset+x) < 0) || ((x_offset+x) >= (long) image->columns))
            {
              p++;
              q++;
              continue;
            }
          x_displace=(horizontal_scale*(PixelIntensityToQuantum(p)-
            (((MagickRealType) QuantumRange+1.0)/2)))/
            (((MagickRealType) QuantumRange+1.0)/
            2.0);
          y_displace=x_displace;
          if (composite_image->matte != MagickFalse)
            y_displace=(vertical_scale*(p->opacity-(((MagickRealType)
              QuantumRange+1.0)/2)))/(((MagickRealType) QuantumRange+1.0)/2);
          pixel=InterpolateColor(image,(double) (x_offset+x+x_displace),
            (double) (y_offset+y+y_displace),&image->exception);
          SetPixelPacket(&pixel,r,displace_indexes+x);
          p++;
          q++;
          r++;
        }
        if (SyncImagePixels(displace_image) == MagickFalse)
          break;
      }
      composite_image=displace_image;
      break;
    }
    case DissolveCompositeOp:
    {
      if (composite_image->geometry != (char *) NULL)
        {
          /*
            Geometry arguments to dissolve factors.
          */
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          source_dissolve=geometry_info.rho/100.0;
          destination_dissolve=1.0;
          if ((source_dissolve-MagickEpsilon) < 0.0)
            source_dissolve=0.0;
          if ((source_dissolve+MagickEpsilon) > 1.0)
            {
              destination_dissolve=2.0-source_dissolve;
              source_dissolve=1.0;
            }
          if ((flags & SigmaValue) != 0)
            destination_dissolve=geometry_info.sigma/100.0;
          if ((destination_dissolve-MagickEpsilon) < 0.0)
            destination_dissolve=0.0;
          modify_outside_overlay=MagickTrue;
          if ((destination_dissolve+MagickEpsilon) > 1.0 )
            {
              destination_dissolve=1.0;
              modify_outside_overlay=MagickFalse;
            }
        }
      break;
    }
    case BlendCompositeOp:
    {
      if (composite_image->geometry != (char *) NULL)
        {
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          source_dissolve=geometry_info.rho/100.0;
          if ((source_dissolve-MagickEpsilon) < 0.0)
            source_dissolve=0.0;
          if ((source_dissolve+MagickEpsilon) > 1.0)
            source_dissolve=1.0;
          destination_dissolve=1.0-source_dissolve;
          if ((flags & SigmaValue) != 0)
            destination_dissolve=geometry_info.sigma/100.0;
          if ((destination_dissolve-MagickEpsilon) < 0.0)
            destination_dissolve=0.0;
          modify_outside_overlay=MagickTrue;
          if ((destination_dissolve+MagickEpsilon) > 1.0)
            {
              destination_dissolve=1.0;
              modify_outside_overlay=MagickFalse;
            }
        }
      break;
    }
    case ModulateCompositeOp:
    {
      if (composite_image->geometry != (char *) NULL)
        {
          /*
            Determine the brightness and saturation scale.
          */
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          percent_brightness=geometry_info.rho;
          if ((flags & SigmaValue) != 0)
            percent_saturation=geometry_info.sigma;
        }
      break;
    }
    case ThresholdCompositeOp:
    {
      /*
        Determine the amount and threshold.
      */
      if (composite_image->geometry != (char *) NULL)
        {
          flags=ParseGeometry(composite_image->geometry,&geometry_info);
          amount=geometry_info.rho;
          threshold=geometry_info.sigma;
          if ((flags & SigmaValue) == 0)
            threshold=0.05f;
        }
      threshold*=QuantumRange;
      break;
    }
    default:
      break;
  }
  attribute=GetImageAttribute(composite_image,"[modify-outside-overlay]");
  if (attribute != (ImageAttribute *) NULL)
    modify_outside_overlay=MagickFalse;
  /*
    Composite image.
  */
  hue=0.0;
  saturation=0.0;
  brightness=0.0;
  midpoint=((MagickRealType) QuantumRange+1.0)/2;
  GetMagickPixelPacket(composite_image,&source);
  GetMagickPixelPacket(image,&destination);
  for (y=0; y < (long) image->rows; y++)
  {
    if (modify_outside_overlay == MagickFalse)
      {
        if (y < y_offset)
          continue;
        if ((y-y_offset) >= (long) composite_image->rows)
          break;
      }
    /*
      If pixels is NULL, y is outside overlay region.
    */
    pixels=p=(PixelPacket *) NULL;
    if ((y >= y_offset) && ((y-y_offset) < (long) composite_image->rows))
      {
        p=AcquireImagePixels(composite_image,0,y-y_offset,
          composite_image->columns,1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        pixels=p;
        if (x_offset < 0)
          p-=x_offset;
      }
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    composite_indexes=GetIndexes(composite_image);
    for (x=0; x < (long) image->columns; x++)
    {
      if (modify_outside_overlay == MagickFalse)
        {
          if (x < x_offset)
            {
              q++;
              continue;
            }
          if ((x-x_offset) >= (long) composite_image->columns)
            break;
        }
      destination.red=(MagickRealType) q->red;
      destination.green=(MagickRealType) q->green;
      destination.blue=(MagickRealType) q->blue;
      if (image->matte != MagickFalse)
        destination.opacity=(MagickRealType) q->opacity;
      if (image->colorspace == CMYKColorspace)
        {
          destination.red=QuantumRange-destination.red;
          destination.green=QuantumRange-destination.green;
          destination.blue=QuantumRange-destination.blue;
          destination.index=QuantumRange-(MagickRealType) indexes[x];
        }
      /*
        Handle destination modifications outside overlaid region.
      */
      composite=destination;
      if ((pixels == (PixelPacket *) NULL) || (x < x_offset) ||
          ((x-x_offset) >= (long) composite_image->columns))
        {
          switch (compose)
          {
            case DissolveCompositeOp:
            case BlendCompositeOp:
            {
              composite.opacity=QuantumRange-destination_dissolve*
                (QuantumRange-composite.opacity);
              break;
            }
            case ClearCompositeOp:
            case SrcCompositeOp:
            {
              CompositeClear(&destination,&composite);
              break;
            }
            case InCompositeOp:
            case SrcInCompositeOp:
            case SrcOutCompositeOp:
            case DstInCompositeOp:
            case DstAtopCompositeOp:
            case CopyOpacityCompositeOp:
            {
              composite.opacity=(MagickRealType) TransparentOpacity;
              break;
            }
            default:
              break;

          }
          if (image->colorspace == CMYKColorspace)
            {
              composite.red=QuantumRange-composite.red;
              composite.green=QuantumRange-composite.green;
              composite.blue=QuantumRange-composite.blue;
              composite.index=QuantumRange-composite.index;
            }
          q->red=RoundToQuantum(composite.red);
          q->green=RoundToQuantum(composite.green);
          q->blue=RoundToQuantum(composite.blue);
          if (image->matte != MagickFalse)
            q->opacity=RoundToQuantum(composite.opacity);
          if (image->colorspace == CMYKColorspace)
            indexes[x]=RoundToQuantum(composite.index);
          q++;
          continue;
        }
      /*
        Handle normal overlay of source onto destination.
      */
      source.red=(MagickRealType) p->red;
      source.green=(MagickRealType) p->green;
      source.blue=(MagickRealType) p->blue;
      if (composite_image->matte != MagickFalse)
        source.opacity=(MagickRealType) p->opacity;
      if (composite_image->colorspace == CMYKColorspace)
        {
          source.red=QuantumRange-source.red;
          source.green=QuantumRange-source.green;
          source.blue=QuantumRange-source.blue;
          source.index=QuantumRange-(MagickRealType)
            composite_indexes[x-x_offset];
        }
      switch (compose)
      {
        case ClearCompositeOp:
        {
          CompositeClear(&destination,&composite);
          break;
        }
        case SrcCompositeOp:
        case CopyCompositeOp:
        case ReplaceCompositeOp:
        {
          composite=source;
          break;
        }
        case DstCompositeOp:
          break;
        case OverCompositeOp:
        case SrcOverCompositeOp:
        {
          CompositeOver(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case DstOverCompositeOp:
        {
          CompositeOver(&destination,destination.opacity,&source,source.opacity,
            &composite);
          break;
        }
        case SrcInCompositeOp:
        case InCompositeOp:
        {
          CompositeIn(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case DstInCompositeOp:
        {
          CompositeIn(&destination,destination.opacity,&source,source.opacity,
            &composite);
          break;
        }
        case OutCompositeOp:
        case SrcOutCompositeOp:
        {
          CompositeOut(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case DstOutCompositeOp:
        {
          CompositeOut(&destination,destination.opacity,&source,source.opacity,
            &composite);
          break;
        }
        case AtopCompositeOp:
        case SrcAtopCompositeOp:
        {
          CompositeAtop(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case DstAtopCompositeOp:
        {
          CompositeAtop(&destination,destination.opacity,&source,source.opacity,
            &composite);
          break;
        }
        case XorCompositeOp:
        {
          CompositeXor(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case PlusCompositeOp:
        {
          CompositePlus(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case MultiplyCompositeOp:
        {
          CompositeMultiply(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case ScreenCompositeOp:
        {
          CompositeScreen(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case OverlayCompositeOp:
        {
          CompositeOverlay(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case DarkenCompositeOp:
        {
          CompositeDarken(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case LightenCompositeOp:
        {
          CompositeLighten(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case ColorDodgeCompositeOp:
        {
          CompositeColorDodge(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case ColorBurnCompositeOp:
        {
          CompositeColorBurn(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case HardLightCompositeOp:
        {
          CompositeHardLight(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case SoftLightCompositeOp:
        {
          CompositeSoftLight(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case DifferenceCompositeOp:
        {
          CompositeDifference(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case ExclusionCompositeOp:
        {
          CompositeExclusion(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case MinusCompositeOp:
        {
          CompositeMinus(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case BumpmapCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          CompositeBumpmap(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case DissolveCompositeOp:
        {
          CompositeOver(&source,QuantumRange-source_dissolve*
            (QuantumRange-source.opacity),&destination,QuantumRange-
            destination_dissolve*(QuantumRange-destination.opacity),&composite);
          break;
        }
        case BlendCompositeOp:
        {
          CompositePlus(&source,QuantumRange-source_dissolve*(QuantumRange-
            source.opacity),&destination,QuantumRange-destination_dissolve*
            (QuantumRange-destination.opacity),&composite);
          break;
        }
        case DisplaceCompositeOp:
        {
          composite=source;
          break;
        }
        case ThresholdCompositeOp:
        {
          CompositeThreshold(&source,source.opacity,&destination,
            destination.opacity,threshold,amount,&composite);
          break;
        }
        case ModulateCompositeOp:
        {
          long
            offset;

          if (source.opacity == TransparentOpacity)
            break;
          offset=(long) (PixelIntensityToQuantum(&source)-midpoint);
          if (offset == 0)
            break;
          CompositeHSB(destination.red,destination.green,destination.blue,&hue,
            &saturation,&brightness);
          brightness+=(0.01*percent_brightness*offset)/midpoint;
          saturation*=0.01*percent_saturation;
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          break;
        }
        case HueCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          if (destination.opacity == TransparentOpacity)
            {
              composite=source;
              break;
            }
          CompositeHSB(destination.red,destination.green,destination.blue,&hue,
            &saturation,&brightness);
          CompositeHSB(source.red,source.green,source.blue,&hue,&sans,&sans);
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          if (source.opacity < destination.opacity)
            composite.opacity=source.opacity;
          break;
        }
        case SaturateCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          if (destination.opacity == TransparentOpacity)
            {
              composite=source;
              break;
            }
          CompositeHSB(destination.red,destination.green,destination.blue,&hue,
            &saturation,&brightness);
          CompositeHSB(source.red,source.green,source.blue,&sans,&saturation,
            &sans);
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          if (source.opacity < destination.opacity)
            composite.opacity=source.opacity;
          break;
        }
        case LuminizeCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          if (destination.opacity == TransparentOpacity)
            {
              composite=source;
              break;
            }
          CompositeHSB(destination.red,destination.green,destination.blue,&hue,
            &saturation,&brightness);
          CompositeHSB(source.red,source.green,source.blue,&sans,&sans,
            &brightness);
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          if (source.opacity < destination.opacity)
            composite.opacity=source.opacity;
          break;
        }
        case ColorizeCompositeOp:
        {
          if (source.opacity == TransparentOpacity)
            break;
          if (destination.opacity == TransparentOpacity)
            {
              composite=source;
              break;
            }
          CompositeHSB(destination.red,destination.green,destination.blue,&sans,
            &sans,&brightness);
          CompositeHSB(source.red,source.green,source.blue,&hue,&saturation,
            &sans);
          HSBComposite(hue,saturation,brightness,&composite.red,
            &composite.green,&composite.blue);
          if (source.opacity < destination.opacity)
            composite.opacity=source.opacity;
          break;
        }
        case AddCompositeOp:  /* deprecated */
        {
          CompositeAdd(&source,source.opacity,&destination,destination.opacity,
            &composite);
          break;
        }
        case SubtractCompositeOp:  /* deprecated */
        {
          CompositeSubtract(&source,source.opacity,&destination,
            destination.opacity,&composite);
          break;
        }
        case CopyRedCompositeOp:  /* deprecated */
        case CopyCyanCompositeOp:
        {
          composite.red=source.red;
          break;
        }
        case CopyGreenCompositeOp:  /* deprecated */
        case CopyMagentaCompositeOp:
        {
          composite.green=source.green;
          break;
        }
        case CopyBlueCompositeOp:  /* deprecated */
        case CopyYellowCompositeOp:
        {
          composite.blue=source.blue;
          break;
        }
        case CopyOpacityCompositeOp:  /* deprecated */
        {
          if (source.matte != MagickFalse)
            {
              composite.opacity=source.opacity;
              break;
            }
          composite.opacity=(MagickRealType) (QuantumRange-
            PixelIntensityToQuantum(&source));
          break;
        }
        case CopyBlackCompositeOp:  /* deprecated */
        {
          composite.index=source.index;
          break;
        }
        default:
          break;
      }
      if (image->colorspace == CMYKColorspace)
        {
          composite.red=QuantumRange-composite.red;
          composite.green=QuantumRange-composite.green;
          composite.blue=QuantumRange-composite.blue;
          composite.index=QuantumRange-composite.index;
        }
      q->red=RoundToQuantum(composite.red);
      q->green=RoundToQuantum(composite.green);
      q->blue=RoundToQuantum(composite.blue);
      if (image->matte != MagickFalse)
        q->opacity=RoundToQuantum(composite.opacity);
      if (image->colorspace == CMYKColorspace)
        indexes[x]=RoundToQuantum(composite.index);
      p++;
      if (p >= (pixels+composite_image->columns))
        p=pixels;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  C o m p o s i t e I m a g e C o m m a n d                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImageCommand() reads one or more images and an optional mask and
%  composites them into a new image.
%
%  The format of the CompositeImageCommand method is:
%
%      MagickBooleanType CompositeImageCommand(ImageInfo *image_info,int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static MagickBooleanType CompositeImageList(ImageInfo *image_info,Image **image,
  Image *composite_image,CompositeOptions *option_info,ExceptionInfo *exception)
{
  MagickStatusType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  if ((*image)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
  assert(exception != (ExceptionInfo *) NULL);
  status=MagickTrue;
  if (composite_image != (Image *) NULL)
    {
      assert(composite_image->signature == MagickSignature);
      if (option_info->compose == BlendCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->blend_geometry);
      if (option_info->compose == DisplaceCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->displace_geometry);
      if (option_info->compose == DissolveCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->dissolve_geometry);
      if (option_info->compose == ModulateCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->watermark_geometry);
      if (option_info->compose == ThresholdCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->unsharp_geometry);
      /*
        Composite image.
      */
      if (option_info->stegano != 0)
        {
          Image
            *stegano_image;

          (*image)->offset=option_info->stegano-1;
          stegano_image=SteganoImage(*image,composite_image,exception);
          if (stegano_image != (Image *) NULL)
            {
              *image=DestroyImageList(*image);
              *image=stegano_image;
            }
        }
      else
        if (option_info->stereo != MagickFalse)
          {
            Image
              *stereo_image;

            stereo_image=StereoImage(*image,composite_image,exception);
            if (stereo_image != (Image *) NULL)
              {
                *image=DestroyImageList(*image);
                *image=stereo_image;
              }
          }
        else
          if (option_info->tile != MagickFalse)
            {
              long
                x,
                y;

              unsigned long
                columns;

              /*
                Tile the composite image.
              */
              (void) SetImageAttribute(composite_image,
                "[modify-outside-overlay]","false");
              columns=composite_image->columns;
              for (y=0; y < (long) (*image)->rows; y+=composite_image->rows)
                for (x=0; x < (long) (*image)->columns; x+=columns)
                  status&=CompositeImage(*image,option_info->compose,
                    composite_image,x,y);
              GetImageException(*image,exception);
            }
          else
            {
              char
                composite_geometry[MaxTextExtent];

              RectangleInfo
                geometry;

              /*
                Digitally composite image.
              */
              SetGeometry(*image,&geometry);
              (void) ParseAbsoluteGeometry(option_info->geometry,&geometry);
              (void) FormatMagickString(composite_geometry,MaxTextExtent,
                "%lux%lu%+ld%+ld",composite_image->columns,
                composite_image->rows,geometry.x,geometry.y);
              (*image)->gravity=(GravityType) option_info->gravity;
              (void) ParseGravityGeometry(*image,composite_geometry,&geometry);
              status&=CompositeImage(*image,option_info->compose,
                composite_image,geometry.x,geometry.y);
              GetImageException(*image,exception);
            }
    }
  return(status != 0 ? MagickTrue : MagickFalse);
}

static void CompositeUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-affine matrix       affine transform matrix",
      "-authenticate value  decrypt image with this password",
      "-blend geometry      blend images",
      "-blue-primary point  chromaticity blue primary point",
      "-channel type        apply option to select image channels",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compose operator    composite operator",
      "-compress type       type of pixel compression when writing the image",
      "-debug events        display copious debugging information",
      "-define format:option",
      "                     define one or more image format options",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-displace geometry   shift image pixels defined by a displacement map",
      "-display server      get image or font from this X server",
      "-dispose method      GIF disposal method",
      "-dissolve value      dissolve the two images a given percent",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-encoding type       text encoding type",
      "-endian type         endianness (MSB or LSB) of the image",
      "-extract geometry    extract area from image",
      "-filter type         use this filter when resizing an image",
      "-font name           render text with this font",
      "-geometry geometry   location of the composite image",
      "-gravity type        which direction to gravitate towards",
      "-green-primary point chromaticity green primary point",
      "-help                print program options",
      "-interlace type      type of image interlacing scheme",
      "-label name          assign a label to an image",
      "-limit type value    pixel cache resource limit",
      "-log format          format of debugging information",
      "-matte               store matte channel if the image has one",
      "-monitor             monitor progress",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color ",
      "-page geometry       size and location of an image canvas (setting)",
      "-profile filename    add ICM or IPTC information profile to image",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-quiet               suppress all error or warning messages",
      "-red-primary point   chromaticity red primary point",
      "-rotate degrees      apply Paeth rotation to the image",
      "-repage geometry     size and location of an image canvas (operator)",
      "-resize geometry     resize the image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scene value         image scene number",
      "-sharpen geometry    sharpen the image",
      "-size geometry       width and height of image",
      "-stegano offset      hide watermark within an image",
      "-stereo              combine two image to create a stereo anaglyph",
      "-strip               strip image of all profiles and comments",
      "-support factor      resize support: > 1.0 is blurry, < 1.0 is sharp",
      "-thumbnail geometry  create a thumbnail of the image",
      "-tile                repeat composite operation across and down image",
      "-transform           affine transform image",
      "-treedepth value     color tree depth",
      "-type type           image type",
      "-units type          the units of image resolution",
      "-unsharp geometry    sharpen the image",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     virtual pixel access method",
      "-watermark geometry  percent brightness and saturation of a watermark",
      "-white-point point   chromaticity white point",
      "-write filename      write images to this file",
      (char *) NULL
    };

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] image [options ...] composite\n"
    "  [ [options ...] mask ] [options ...] composite\n",
    GetClientName());
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  exit(0);
}

static void RelinquishCompositeOptions(CompositeOptions *option_info)
{
  if (option_info->blend_geometry != (char *) NULL)
    option_info->blend_geometry=(char *)
      RelinquishMagickMemory(option_info->blend_geometry);
  if (option_info->displace_geometry != (char *) NULL)
    option_info->displace_geometry=(char *)
      RelinquishMagickMemory(option_info->displace_geometry);
  if (option_info->dissolve_geometry != (char *) NULL)
    option_info->dissolve_geometry=(char *)
      RelinquishMagickMemory(option_info->dissolve_geometry);
  if (option_info->geometry != (char *) NULL)
    option_info->geometry=(char *)
      RelinquishMagickMemory(option_info->geometry);
  if (option_info->unsharp_geometry != (char *) NULL)
    option_info->unsharp_geometry=(char *)
      RelinquishMagickMemory(option_info->unsharp_geometry);
  if (option_info->watermark_geometry != (char *) NULL)
    option_info->watermark_geometry=(char *)
      RelinquishMagickMemory(option_info->watermark_geometry);
}

MagickExport MagickBooleanType CompositeImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{
#define NotInitialized  (unsigned int) (~0)
#define DestroyComposite() \
{ \
  RelinquishCompositeOptions(&option_info); \
  for ( ; k >= 0; k--) \
    image_stack[k]=DestroyImageList(image_stack[k]); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=(char *) RelinquishMagickMemory(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowCompositeException(asperity,tag,option) \
{ \
  if (exception->severity == UndefinedException) \
    (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag, \
      "`%s'",option); \
  DestroyComposite(); \
  return(MagickFalse); \
}
#define ThrowCompositeInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyComposite(); \
  return(MagickFalse); \
}

  char
    *filename,
    *option;

  CompositeOptions
    option_info;

  const char
    *format;

  Image
    *composite_image,
    *image,
    *image_stack[MaxImageStackDepth+1],
    *mask_image;

  MagickStatusType
    pend,
    status;

  long
    j,
    k;

  register long
    i;

  /*
    Set default.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(exception != (ExceptionInfo *) NULL);
  if (argc < 4)
    CompositeUsage();
  (void) ResetMagickMemory(&option_info,0,sizeof(CompositeOptions));
  option_info.compose=UndefinedCompositeOp;
  filename=(char *) NULL;
  format="%w,%h,%m";
  j=1;
  k=0;
  image_stack[k]=NewImageList();
  option=(char *) NULL;
  pend=MagickFalse;
  status=MagickTrue;
  /*
    Check command syntax.
  */
  composite_image=NewImageList();
  image=NewImageList();
  mask_image=NewImageList();
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    ThrowCompositeException(ResourceLimitError,"MemoryAllocationFailed",
      strerror(errno));
  for (i=1; i < (long) (argc-1); i++)
  {
    option=argv[i];
    if (LocaleCompare(option,"(") == 0)
      {
        if (k == MaxImageStackDepth)
          ThrowCompositeException(OptionError,"ParenthesisNestedTooDeeply",
            option);
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        k++;
        image_stack[k]=NewImageList();
        continue;
      }
    if (LocaleCompare(option,")") == 0)
      {
        if (k == 0)
          ThrowCompositeException(OptionError,"UnableToParseExpression",option);
        if (image_stack[k] != (Image *) NULL)
          {
            MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
            AppendImageToList(&image_stack[k-1],image_stack[k]);
          }
        k--;
        continue;
      }
    if (IsMagickOption(option) == MagickFalse)
      {
        Image
          *image;

        /*
          Read input image.
        */
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        filename=argv[i];
        (void) CopyMagickString(image_info->filename,filename,MaxTextExtent);
        image=ReadImage(image_info,exception);
        status&=(image != (Image *) NULL) &&
          (exception->severity < ErrorException);
        if (image == (Image *) NULL)
          continue;
        AppendImageToList(&image_stack[k],image);
        continue;
      }
    pend=image_stack[k] != (Image *) NULL ? MagickTrue : MagickFalse;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("affine",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("blend",option+1) == 0)
          {
            (void) CloneString(&option_info.blend_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.blend_geometry,argv[i]);
            option_info.compose=BlendCompositeOp;
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            long
              channel;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowCompositeException(OptionError,"MissingArgument",option);
            channel=ParseChannelOption(argv[i]);
            if (channel < 0)
              ThrowCompositeException(OptionError,"UnrecognizedChannelType",
                argv[i]);
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            long
              colorspace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,
              MagickFalse,argv[i]);
            if (colorspace < 0)
              ThrowCompositeException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("compose",option+1) == 0)
          {
            long
              compose;

            option_info.compose=UndefinedCompositeOp;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            compose=ParseMagickOption(MagickCompositeOptions,MagickFalse,
              argv[i]);
            if (compose < 0)
              ThrowCompositeException(OptionError,
                "UnrecognizedComposeOperator",argv[i]);
            option_info.compose=(CompositeOperator) compose;
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            long
              compression;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            compression=ParseMagickOption(MagickCompressionOptions,
              MagickFalse,argv[i]);
            if (compression < 0)
              ThrowCompositeException(OptionError,
                "UnrecognizedImageCompression",argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            LogEventType
              event_mask;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            event_mask=SetLogEventMask(argv[i]);
            if (event_mask == UndefinedEvents)
              ThrowCompositeException(OptionError,"UnrecognizedEventType",
                option);
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                const char
                  *define;

                define=GetImageOption(image_info,argv[i]);
                if (define == (const char *) NULL)
                  ThrowCompositeException(OptionError,"NoSuchOption",argv[i]);
                break;
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("displace",option+1) == 0)
          {
            (void) CloneString(&option_info.displace_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.displace_geometry,argv[i]);
            option_info.compose=DisplaceCompositeOp;
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            long
              dispose;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            dispose=ParseMagickOption(MagickDisposeOptions,MagickFalse,argv[i]);
            if (dispose < 0)
              ThrowCompositeException(OptionError,"UnrecognizedDisposeMethod",
                argv[i]);
            break;
          }
        if (LocaleCompare("dissolve",option+1) == 0)
          {
            (void) CloneString(&option_info.dissolve_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.dissolve_geometry,argv[i]);
            option_info.compose=DissolveCompositeOp;
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            long
              endian;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            endian=ParseMagickOption(MagickEndianOptions,MagickFalse,
              argv[i]);
            if (endian < 0)
              ThrowCompositeException(OptionError,"UnrecognizedEndianType",
                argv[i]);
            break;
          }
        if (LocaleCompare("extract",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("filter",option+1) == 0)
          {
            long
              filter;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            filter=ParseMagickOption(MagickFilterOptions,MagickFalse,
              argv[i]);
            if (filter < 0)
              ThrowCompositeException(OptionError,"UnrecognizedImageFilter",
                argv[i]);
            break;
          }
        if (LocaleCompare("font",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            format=argv[i];
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'g':
      {
        if (LocaleCompare("geometry",option+1) == 0)
          {
            (void) CloneString(&option_info.geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.geometry,argv[i]);
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            long
              gravity;

            option_info.gravity=UndefinedGravity;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            gravity=ParseMagickOption(MagickGravityOptions,MagickFalse,
              argv[i]);
            if (gravity < 0)
              ThrowCompositeException(OptionError,"UnrecognizedGravityType",
                argv[i]);
            option_info.gravity=(GravityType) gravity;
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          CompositeUsage();
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("interlace",option+1) == 0)
          {
            long
              interlace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            interlace=ParseMagickOption(MagickInterlaceOptions,MagickFalse,
              argv[i]);
            if (interlace < 0)
              ThrowCompositeException(OptionError,
                "UnrecognizedInterlaceType",argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            long
              resource;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            resource=ParseMagickOption(MagickResourceOptions,MagickFalse,
              argv[i]);
            if (resource < 0)
              ThrowCompositeException(OptionError,"UnrecognizedResourceType",
                argv[i]);
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if ((LocaleCompare("unlimited",argv[i]) != 0) &&
                (IsGeometry(argv[i]) == MagickFalse))
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) || (strchr(argv[i],'%') == (char *) NULL))
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("monitor",option+1) == 0)
          break;
        if (LocaleCompare("monochrome",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          break;
        if (LocaleCompare("noop",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("process",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'r':
      {
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("render",option+1) == 0)
          break;
        if (LocaleCompare("repage",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("stegano",option+1) == 0)
          {
            option_info.stegano=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            option_info.stegano=atol(argv[i])+1;
            break;
          }
        if (LocaleCompare("stereo",option+1) == 0)
          {
            option_info.stereo=(MagickBooleanType) (*option == '-');
            break;
          }
        if (LocaleCompare("strip",option+1) == 0)
          break;
        if (LocaleCompare("support",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("thumbnail",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("tile",option+1) == 0)
          {
            option_info.tile=(MagickBooleanType) (*option == '-');
            (void) strcpy(argv[i]+1,"{0}");
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          break;
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("type",option+1) == 0)
          {
            long
              type;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            type=ParseMagickOption(MagickImageOptions,MagickFalse,argv[i]);
            if (type < 0)
              ThrowCompositeException(OptionError,"UnrecognizedImageType",
                argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'u':
      {
        if (LocaleCompare("units",option+1) == 0)
          {
            long
              units;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            units=ParseMagickOption(MagickResolutionOptions,MagickFalse,
              argv[i]);
            if (units < 0)
              ThrowCompositeException(OptionError,"UnrecognizedUnitsType",
                argv[i]);
            break;
          }
        if (LocaleCompare("unsharp",option+1) == 0)
          {
            (void) CloneString(&option_info.unsharp_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.unsharp_geometry,argv[i]);
            option_info.compose=ThresholdCompositeOp;
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if (LocaleCompare("version",option+1) == 0)
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            long
              method;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            method=ParseMagickOption(MagickVirtualPixelOptions,MagickFalse,
              argv[i]);
            if (method < 0)
              ThrowCompositeException(OptionError,
                "UnrecognizedVirtualPixelMethod",argv[i]);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'w':
      {
        if (LocaleCompare("watermark",option+1) == 0)
          {
            (void) CloneString(&option_info.watermark_geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            (void) CloneString(&option_info.watermark_geometry,argv[i]);
            option_info.compose=ModulateCompositeOp;
            break;
          }
        if (LocaleCompare("white-point",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompositeInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("write",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompositeException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
    }
    status=(MagickStatusType)
      ParseMagickOption(MagickMogrifyOptions,MagickFalse,option+1);
    if (status == MagickTrue)
      MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  }
  if (k != 0)
    ThrowCompositeException(OptionError,"UnbalancedParenthesis",argv[i]);
  if (i != (long) (argc-1))
    ThrowCompositeException(OptionError,"MissingAnImageFilename",argv[i]);
  if ((image_stack[k] == (Image *) NULL) ||
      (GetImageListLength(image_stack[k]) < 2))
    ThrowCompositeException(OptionError,"MissingAnImageFilename",argv[argc-1]);
  MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  composite_image=GetImageFromList(image_stack[k],0);
  image=GetImageFromList(image_stack[k],1);
  mask_image=GetImageFromList(image_stack[k],2);
  if (mask_image != (Image *) NULL)
    {
      (void) SetImageType(composite_image,TrueColorMatteType);
      if (composite_image->matte == MagickFalse)
        SetImageOpacity(composite_image,OpaqueOpacity);
      status&=CompositeImage(composite_image,CopyOpacityCompositeOp,mask_image,
        0,0);
      if (status == MagickFalse)
        GetImageException(composite_image,exception);
    }
  image=CloneImage(image,0,0,MagickTrue,exception);
  if (image == (Image *) NULL)
    ThrowCompositeException(ResourceLimitError,"MemoryAllocationFailed",
      strerror(errno));
  status&=CompositeImageList(image_info,&image,composite_image,&option_info,
    exception);
  /*
    Write composite images.
  */
  status&=WriteImages(image_info,image,argv[argc-1],exception);
  if (metadata != (char **) NULL)
    {
      char
        *text;

      text=TranslateText(image_info,image,format);
      if (text == (char *) NULL)
        ThrowCompositeException(ResourceLimitError,"MemoryAllocationFailed",
          strerror(errno));
      (void) ConcatenateString(&(*metadata),text);
      (void) ConcatenateString(&(*metadata),"\n");
      text=(char *) RelinquishMagickMemory(text);
    }
  RelinquishCompositeOptions(&option_info);
  DestroyComposite();
  return(status != 0 ? MagickTrue : MagickFalse);
}
