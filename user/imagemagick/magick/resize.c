/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                 RRRR   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                    %
%                 R   R  E      SS       I       ZZ  E                        %
%                 RRRR   EEE     SSS     I     ZZZ   EEE                      %
%                 R R    E         SS    I    ZZ     E                        %
%                 R  R   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                    %
%                                                                             %
%                                                                             %
%                     ImageMagick Image Resize Methods                        %
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
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/resize.h"
#include "magick/string_.h"

/*
  Typedef declarations.
*/
typedef struct _ContributionInfo
{
  MagickRealType
    weight;

  long
    pixel;
} ContributionInfo;

typedef struct _FilterInfo
{
  MagickRealType
    (*function)(const MagickRealType,const MagickRealType),
    support;
} FilterInfo;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   B e s s e l O r d e r O n e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BesselOrderOne() computes the Bessel function of x of the first kind of
%  order 0:
%
%    Reduce x to |x| since j1(x)= -j1(-x), and for x in (0,8]
%
%       j1(x) = x*j1(x);
%
%    For x in (8,inf)
%
%       j1(x) = sqrt(2/(pi*x))*(p1(x)*cos(x1)-q1(x)*sin(x1))
%
%    where x1 = x-3*pi/4. Compute sin(x1) and cos(x1) as follow:
%
%       cos(x1) =  cos(x)cos(3pi/4)+sin(x)sin(3pi/4)
%               =  1/sqrt(2) * (sin(x) - cos(x))
%       sin(x1) =  sin(x)cos(3pi/4)-cos(x)sin(3pi/4)
%               = -1/sqrt(2) * (sin(x) + cos(x))
%
%  The format of the BesselOrderOne method is:
%
%      MagickRealType BesselOrderOne(MagickRealType x)
%
%  A description of each parameter follows:
%
%    o value: Method BesselOrderOne returns the Bessel function of x of the
%      first kind of orders 1.
%
%    o x: MagickRealType value.
%
%
*/

static MagickRealType J1(MagickRealType x)
{
  MagickRealType
    p,
    q;

  register long
    i;

  static const double
    Pone[] =
    {
       0.581199354001606143928050809e+21,
      -0.6672106568924916298020941484e+20,
       0.2316433580634002297931815435e+19,
      -0.3588817569910106050743641413e+17,
       0.2908795263834775409737601689e+15,
      -0.1322983480332126453125473247e+13,
       0.3413234182301700539091292655e+10,
      -0.4695753530642995859767162166e+7,
       0.270112271089232341485679099e+4
    },
    Qone[] =
    {
      0.11623987080032122878585294e+22,
      0.1185770712190320999837113348e+20,
      0.6092061398917521746105196863e+17,
      0.2081661221307607351240184229e+15,
      0.5243710262167649715406728642e+12,
      0.1013863514358673989967045588e+10,
      0.1501793594998585505921097578e+7,
      0.1606931573481487801970916749e+4,
      0.1e+1
    };

  p=Pone[8];
  q=Qone[8];
  for (i=7; i >= 0; i--)
  {
    p=p*x*x+Pone[i];
    q=q*x*x+Qone[i];
  }
  return(p/q);
}

static MagickRealType P1(MagickRealType x)
{
  MagickRealType
    p,
    q;

  register long
    i;

  static const double
    Pone[] =
    {
      0.352246649133679798341724373e+5,
      0.62758845247161281269005675e+5,
      0.313539631109159574238669888e+5,
      0.49854832060594338434500455e+4,
      0.2111529182853962382105718e+3,
      0.12571716929145341558495e+1
    },
    Qone[] =
    {
      0.352246649133679798068390431e+5,
      0.626943469593560511888833731e+5,
      0.312404063819041039923015703e+5,
      0.4930396490181088979386097e+4,
      0.2030775189134759322293574e+3,
      0.1e+1
    };

  p=Pone[5];
  q=Qone[5];
  for (i=4; i >= 0; i--)
  {
    p=p*(8.0/x)*(8.0/x)+Pone[i];
    q=q*(8.0/x)*(8.0/x)+Qone[i];
  }
  return(p/q);
}

static MagickRealType Q1(MagickRealType x)
{
  MagickRealType
    p,
    q;

  register long
    i;

  static const double
    Pone[] =
    {
      0.3511751914303552822533318e+3,
      0.7210391804904475039280863e+3,
      0.4259873011654442389886993e+3,
      0.831898957673850827325226e+2,
      0.45681716295512267064405e+1,
      0.3532840052740123642735e-1
    },
    Qone[] =
    {
      0.74917374171809127714519505e+4,
      0.154141773392650970499848051e+5,
      0.91522317015169922705904727e+4,
      0.18111867005523513506724158e+4,
      0.1038187585462133728776636e+3,
      0.1e+1
    };

  p=Pone[5];
  q=Qone[5];
  for (i=4; i >= 0; i--)
  {
    p=p*(8.0/x)*(8.0/x)+Pone[i];
    q=q*(8.0/x)*(8.0/x)+Qone[i];
  }
  return(p/q);
}

static MagickRealType BesselOrderOne(MagickRealType x)
{
  MagickRealType
    p,
    q;

  if (x == 0.0)
    return(0.0);
  p=x;
  if (x < 0.0)
    x=(-x);
  if (x < 8.0)
    return(p*J1(x));
  q=sqrt((double) (2.0/(MagickPI*x)))*(P1(x)*(1.0/sqrt(2.0)*(sin((double) x)-
    cos((double) x)))-8.0/x*Q1(x)*(-1.0/sqrt(2.0)*(sin((double) x)+
    cos((double) x))));
  if (p < 0.0)
    q=(-q);
  return(q);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g n i f y I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagnifyImage() is a convenience method that scales an image proportionally
%  to twice its size.
%
%  The format of the MagnifyImage method is:
%
%      Image *MagnifyImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *MagnifyImage(const Image *image,ExceptionInfo *exception)
{
  Image
    *magnify_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  magnify_image=ResizeImage(image,2*image->columns,2*image->rows,CubicFilter,
    1.0,exception);
  return(magnify_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M i n i f y I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MinifyImage() is a convenience method that scales an image proportionally
%  to half its size.
%
%  The format of the MinifyImage method is:
%
%      Image *MinifyImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *MinifyImage(const Image *image,ExceptionInfo *exception)
{
  Image
    *minify_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  minify_image=ResizeImage(image,image->columns/2,image->rows/2,CubicFilter,
    1.0,exception);
  return(minify_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s i z e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResizeImage() scales an image to the desired dimensions with one of these
%  filters:
%
%    Bessel   Blackman   Box
%    Catrom   Cubic      Gaussian
%    Hanning  Hermite    Lanczos
%    Mitchell Point      Quandratic
%    Sinc     Triangle
%
%  Most of the filters are FIR (finite impulse response), however, Bessel,
%  Gaussian, and Sinc are IIR (infinite impulse response).  Bessel and Sinc
%  are windowed (brought down to zero) with the Blackman filter.
%
%  ResizeImage() was inspired by Paul Heckbert's zoom program.
%
%  The format of the ResizeImage method is:
%
%      Image *ResizeImage(Image *image,const unsigned long columns,
%        const unsigned long rows,const FilterTypes filter,const double blur,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
%    o filter: Image filter to use.
%
%    o blur: The blur factor where > 1 is blurry, < 1 is sharp.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static MagickRealType Bessel(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  if (x == 0.0)
    return((MagickRealType) (MagickPI/4.0));
  return(BesselOrderOne(MagickPI*x)/(2.0*x));
}

static MagickRealType Sinc(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  if (x == 0.0)
    return(1.0);
  return(sin(MagickPI*(double) x)/(MagickPI*(double) x));
}

static MagickRealType Blackman(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  return(0.42+0.5*cos(MagickPI*(double) x)+0.08*cos(2*MagickPI*(double) x));
}

static MagickRealType BlackmanBessel(const MagickRealType x,
  const MagickRealType support)
{
  return(Blackman(x/support,support)*Bessel(x,support));
}

static MagickRealType BlackmanSinc(const MagickRealType x,
  const MagickRealType support)
{
  return(Blackman(x/support,support)*Sinc(x,support));
}

static MagickRealType Box(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  if (x < -0.5)
    return(0.0);
  if (x < 0.5)
    return(1.0);
  return(0.0);
}

static MagickRealType Catrom(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return(0.5*(4.0+x*(8.0+x*(5.0+x))));
  if (x < 0.0)
    return(0.5*(2.0+x*x*(-5.0-3.0*x)));
  if (x < 1.0)
    return(0.5*(2.0+x*x*(-5.0+3.0*x)));
  if (x < 2.0)
    return(0.5*(4.0+x*(-8.0+x*(5.0-x))));
  return(0.0);
}

static MagickRealType Cubic(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return((2.0+x)*(2.0+x)*(2.0+x)/6.0);
  if (x < 0.0)
    return((4.0+x*x*(-6.0-3.0*x))/6.0);
  if (x < 1.0)
    return((4.0+x*x*(-6.0+3.0*x))/6.0);
  if (x < 2.0)
    return((2.0-x)*(2.0-x)*(2.0-x)/6.0);
  return(0.0);
}

static MagickRealType Gaussian(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  return(exp((double) (-2.0*x*x))*sqrt(2.0/MagickPI));
}

static MagickRealType Hanning(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  return(0.5+0.5*cos(MagickPI*(double) x));
}

static MagickRealType Hamming(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  return(0.54+0.46*cos(MagickPI*(double) x));
}

static MagickRealType Hermite(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  if (x < -1.0)
    return(0.0);
  if (x < 0.0)
    return((2.0*(-x)-3.0)*(-x)*(-x)+1.0);
  if (x < 1.0)
    return((2.0*x-3.0)*x*x+1.0);
  return(0.0);
}

static MagickRealType Lanczos(const MagickRealType x,
  const MagickRealType support)
{
  if (x < -3.0)
    return(0.0);
  if (x < 0.0)
    return(Sinc(-x,support)*Sinc(-x/3.0,support));
  if (x < 3.0)
    return(Sinc(x,support)*Sinc(x/3.0,support));
  return(0.0);
}

static MagickRealType Mitchell(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
#define B   (1.0/3.0)
#define C   (1.0/3.0)
#define P0  ((  6.0- 2.0*B       )/6.0)
#define P2  ((-18.0+12.0*B+ 6.0*C)/6.0)
#define P3  (( 12.0- 9.0*B- 6.0*C)/6.0)
#define Q0  ((       8.0*B+24.0*C)/6.0)
#define Q1  ((     -12.0*B-48.0*C)/6.0)
#define Q2  ((       6.0*B+30.0*C)/6.0)
#define Q3  ((     - 1.0*B- 6.0*C)/6.0)

  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return(Q0-x*(Q1-x*(Q2-x*Q3)));
  if (x < 0.0)
    return(P0+x*x*(P2-x*P3));
  if (x < 1.0)
    return(P0+x*x*(P2+x*P3));
  if (x < 2.0)
    return(Q0+x*(Q1+x*(Q2+x*Q3)));
  return(0.0);
}

static MagickRealType Quadratic(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  if (x < -1.5)
    return(0.0);
  if (x < -0.5)
    return(0.5*(x+1.5)*(x+1.5));
  if (x < 0.5)
    return(0.75-x*x);
  if (x < 1.5)
    return(0.5*(x-1.5)*(x-1.5));
  return(0.0);
}

static MagickRealType Triangle(const MagickRealType x,
  const MagickRealType magick_unused(support))
{
  if (x < -1.0)
    return(0.0);
  if (x < 0.0)
    return(1.0+x);
  if (x < 1.0)
    return(1.0-x);
  return(0.0);
}

static MagickBooleanType HorizontalFilter(const Image *source,
  Image *destination,const MagickRealType x_factor,
  const FilterInfo *filter_info,const MagickRealType blur,
  ContributionInfo *contribution,const MagickSizeType span,
  MagickOffsetType *quantum,ExceptionInfo *exception)
{
#define ResizeImageTag  "Resize/Image"

  long
    j,
    n,
    start,
    stop,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel,
    zero;

  MagickRealType
    alpha,
    center,
    density,
    gamma,
    scale,
    support;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *source_indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Apply filter to resize horizontally from source to destination.
  */
  scale=blur*Max(1.0/x_factor,1.0);
  support=scale*filter_info->support;
  destination->storage_class=source->storage_class;
  if (support > 0.5)
    destination->storage_class=DirectClass;
  else
    {
      /*
        Reduce to point sampling.
      */
      support=(MagickRealType) (0.5+MagickEpsilon);
      scale=1.0;
    }
  scale=1.0/scale;
  (void) ResetMagickMemory(&zero,0,sizeof(zero));
  for (x=0; x < (long) destination->columns; x++)
  {
    center=(MagickRealType) (x+0.5)/x_factor;
    start=(long) Max(center-support+0.5,0);
    stop=(long) Min(center+support+0.5,(MagickRealType) source->columns);
    density=0.0;
    for (n=0; n < (stop-start); n++)
    {
      contribution[n].pixel=start+n;
      contribution[n].weight=filter_info->function(scale*
        ((MagickRealType) (start+n)-center+0.5),filter_info->support);
      density+=contribution[n].weight;
    }
    if ((density != 0.0) && (density != 1.0))
      {
        /*
          Normalize.
        */
        density=1.0/density;
        for (i=0; i < n; i++)
          contribution[i].weight*=density;
      }
    p=AcquireImagePixels(source,contribution[0].pixel,0,(unsigned long)
      (contribution[n-1].pixel-contribution[0].pixel+1),source->rows,exception);
    q=SetImagePixels(destination,x,0,1,destination->rows);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    source_indexes=GetIndexes(source);
    indexes=GetIndexes(destination);
    for (y=0; y < (long) destination->rows; y++)
    {
      gamma=0.0;
      pixel=zero;
      for (i=0; i < n; i++)
      {
        j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
          (contribution[i].pixel-contribution[0].pixel);
        alpha=1.0;
        if (source->matte != MagickFalse)
          alpha=((MagickRealType) QuantumRange-(p+j)->opacity)/QuantumRange;
        pixel.red+=contribution[i].weight*alpha*(p+j)->red;
        pixel.green+=contribution[i].weight*alpha*(p+j)->green;
        pixel.blue+=contribution[i].weight*alpha*(p+j)->blue;
        gamma+=contribution[i].weight*alpha;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      q->red=RoundToQuantum(gamma*pixel.red);
      q->green=RoundToQuantum(gamma*pixel.green);
      q->blue=RoundToQuantum(gamma*pixel.blue);
      if ((source->matte != MagickFalse) && (destination->matte != MagickFalse))
        {
          for (i=0; i < n; i++)
          {
            j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
              (contribution[i].pixel-contribution[0].pixel);
            pixel.opacity+=contribution[i].weight*(p+j)->opacity;
          }
          q->opacity=RoundToQuantum(pixel.opacity);
        }
      if ((source->colorspace == CMYKColorspace) &&
          (destination->colorspace == CMYKColorspace))
        {
          gamma=0.0;
          for (i=0; i < n; i++)
          {
            j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
              (contribution[i].pixel-contribution[0].pixel);
            alpha=1.0;
            if (source->matte != MagickFalse)
              alpha=((MagickRealType) QuantumRange-(p+j)->opacity)/QuantumRange;
            pixel.index+=contribution[i].weight*alpha*source_indexes[j];
            gamma+=contribution[i].weight*alpha;
          }
          gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
          indexes[y]=(IndexPacket) RoundToQuantum(gamma*pixel.index);
        }
      if ((destination->storage_class == PseudoClass) &&
          (source->storage_class == PseudoClass))
        {
          i=Min(Max((long) (center+0.5),start),stop-1);
          j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
            (contribution[i-start].pixel-contribution[0].pixel);
          indexes[y]=source_indexes[j];
        }
      q++;
    }
    if (SyncImagePixels(destination) == MagickFalse)
      break;
    if ((source->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(*quantum,span) != MagickFalse))
      {
        status=source->progress_monitor(ResizeImageTag,(MagickOffsetType)
          *quantum,span,source->client_data);
        if (status == MagickFalse)
          break;
      }
    (*quantum)++;
  }
  return((MagickBooleanType) (x == (long) destination->columns));
}

static MagickBooleanType VerticalFilter(const Image *source,Image *destination,
  const MagickRealType y_factor,const FilterInfo *filter_info,
  const MagickRealType blur,ContributionInfo *contribution,
  const MagickSizeType span,MagickOffsetType *quantum,ExceptionInfo *exception)
{
  long
    j,
    n,
    start,
    stop,
    x;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel,
    zero;

  MagickRealType
    alpha,
    center,
    density,
    gamma,
    scale,
    support;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *source_indexes;

  register long
    i,
    y;

  register PixelPacket
    *q;

  /*
    Apply filter to resize vertically from source to destination.
  */
  scale=blur*Max(1.0/y_factor,1.0);
  support=scale*filter_info->support;
  destination->storage_class=source->storage_class;
  if (support > 0.5)
    destination->storage_class=DirectClass;
  else
    {
      /*
        Reduce to point sampling.
      */
      support=(MagickRealType) (0.5+MagickEpsilon);
      scale=1.0;
    }
  scale=1.0/scale;
  (void) ResetMagickMemory(&zero,0,sizeof(zero));
  for (y=0; y < (long) destination->rows; y++)
  {
    center=(MagickRealType) (y+0.5)/y_factor;
    start=(long) Max(center-support+0.5,0);
    stop=(long) Min(center+support+0.5,(MagickRealType) source->rows);
    density=0.0;
    for (n=0; n < (stop-start); n++)
    {
      contribution[n].pixel=start+n;
      contribution[n].weight=filter_info->function(scale*
        ((MagickRealType) (start+n)-center+0.5),filter_info->support);
      density+=contribution[n].weight;
    }
    if ((density != 0.0) && (density != 1.0))
      {
        /*
          Normalize.
        */
        density=1.0/density;
        for (i=0; i < n; i++)
          contribution[i].weight*=density;
      }
    p=AcquireImagePixels(source,0,contribution[0].pixel,source->columns,
      (unsigned long) (contribution[n-1].pixel-contribution[0].pixel+1),
      exception);
    q=SetImagePixels(destination,0,y,destination->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    source_indexes=GetIndexes(source);
    indexes=GetIndexes(destination);
    for (x=0; x < (long) destination->columns; x++)
    {
      gamma=0.0;
      pixel=zero;
      for (i=0; i < n; i++)
      {
        j=(long) ((contribution[i].pixel-contribution[0].pixel)*
          source->columns+x);
        alpha=1.0;
        if (source->matte != MagickFalse)
          alpha=((MagickRealType) QuantumRange-(p+j)->opacity)/QuantumRange;
        pixel.red+=contribution[i].weight*alpha*(p+j)->red;
        pixel.green+=contribution[i].weight*alpha*(p+j)->green;
        pixel.blue+=contribution[i].weight*alpha*(p+j)->blue;
        gamma+=contribution[i].weight*alpha;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      q->red=RoundToQuantum(gamma*pixel.red);
      q->green=RoundToQuantum(gamma*pixel.green);
      q->blue=RoundToQuantum(gamma*pixel.blue);
      if ((source->matte != MagickFalse) && (destination->matte != MagickFalse))
        {
          for (i=0; i < n; i++)
          {
            j=(long) ((contribution[i].pixel-contribution[0].pixel)*
              source->columns+x);
            pixel.opacity+=contribution[i].weight*(p+j)->opacity;
          }
          q->opacity=RoundToQuantum(pixel.opacity);
        }
      if ((source->colorspace == CMYKColorspace) &&
          (destination->colorspace == CMYKColorspace))
        {
          gamma=0.0;
          for (i=0; i < n; i++)
          {
            j=(long) ((contribution[i].pixel-contribution[0].pixel)*
              source->columns+x);
            alpha=1.0;
            if (source->matte != MagickFalse)
              alpha=((MagickRealType) QuantumRange-(p+j)->opacity)/QuantumRange;
            pixel.index+=contribution[i].weight*alpha*source_indexes[j];
            gamma+=contribution[i].weight*alpha;
          }
          gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
          indexes[x]=(IndexPacket) RoundToQuantum(gamma*pixel.index);
        }
      if ((destination->storage_class == PseudoClass) &&
          (source->storage_class == PseudoClass))
        {
          i=Min(Max((long) (center+0.5),start),stop-1);
          j=(long) ((contribution[i-start].pixel-contribution[0].pixel)*
            source->columns+x);
          indexes[x]=source_indexes[j];
        }
      q++;
    }
    if (SyncImagePixels(destination) == MagickFalse)
      break;
    if ((source->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(*quantum,span) != MagickFalse))
      {
        status=source->progress_monitor(ResizeImageTag,(MagickOffsetType)
          *quantum,span,source->client_data);
        if (status == MagickFalse)
          break;
      }
    (*quantum)++;
  }
  return((MagickBooleanType) (y == (long) destination->rows));
}

MagickExport Image *ResizeImage(const Image *image,const unsigned long columns,
  const unsigned long rows,const FilterTypes filter,const double blur,
  ExceptionInfo *exception)
{
  ContributionInfo
    *contribution;

  Image
    *source_image,
    *resize_image;

  MagickRealType
    support,
    x_factor,
    x_support,
    y_factor,
    y_support;

  MagickSizeType
    span;

  MagickStatusType
    status;

  register long
    i;

  static const FilterInfo
    filters[SincFilter+1] =
    {
      { Box, 0.0f },
      { Box, 0.0f },
      { Box, 0.5f },
      { Triangle, 1.0f },
      { Hermite, 1.0f },
      { Hanning, 1.0f },
      { Hamming, 1.0f },
      { Blackman, 1.0f },
      { Gaussian, 1.25f },
      { Quadratic, 1.5f },
      { Cubic, 2.0f },
      { Catrom, 2.0f },
      { Mitchell, 2.0f },
      { Lanczos, 3.0f },
      { BlackmanBessel, 3.2383f },
      { BlackmanSinc, 4.0f }
    };

  MagickOffsetType
    quantum;

  /*
    Initialize resize image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  assert(((int) filter >= 0) && ((int) filter <= SincFilter));
  if ((columns == 0) || (rows == 0))
    ThrowImageException(ImageError,"NegativeOrZeroImageSize");
  resize_image=CloneImage(image,columns,rows,MagickTrue,exception);
  if (resize_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Allocate filter contribution info.
  */
  x_factor=(MagickRealType)
    resize_image->columns/(MagickRealType) image->columns;
  y_factor=(MagickRealType) resize_image->rows/(MagickRealType) image->rows;
  i=(long) LanczosFilter;
  if (filter != UndefinedFilter)
    i=(long) filter;
  else
    if ((x_factor == 1.0) && (y_factor == 1.0))
      i=(long) PointFilter;
    else
      if ((image->storage_class == PseudoClass) ||
          (image->matte != MagickFalse) || ((x_factor*y_factor) > 1.0))
        i=(long) MitchellFilter;
  x_support=blur*Max(1.0/x_factor,1.0)*filters[i].support;
  y_support=blur*Max(1.0/y_factor,1.0)*filters[i].support;
  support=Max(x_support,y_support);
  if (support < filters[i].support)
    support=filters[i].support;
  contribution=(ContributionInfo *) AcquireMagickMemory((size_t)
    (2.0*Max(support,0.5)+3)*sizeof(*contribution));
  if (contribution == (ContributionInfo *) NULL)
    {
      resize_image=DestroyImage(resize_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Resize image.
  */
  quantum=0;
  if ((columns*((MagickSizeType) image->rows+rows)) >
      (rows*((MagickSizeType) image->columns+columns)))
    {
      source_image=CloneImage(image,columns,image->rows,MagickTrue,exception);
      if (source_image == (Image *) NULL)
        {
          contribution=(ContributionInfo *)
            RelinquishMagickMemory(contribution);
          resize_image=DestroyImage(resize_image);
          return((Image *) NULL);
        }
      span=(MagickSizeType) (source_image->columns+resize_image->rows);
      status=HorizontalFilter(image,source_image,x_factor,&filters[i],blur,
        contribution,span,&quantum,exception);
      status|=VerticalFilter(source_image,resize_image,y_factor,&filters[i],
        blur,contribution,span,&quantum,exception);
    }
  else
    {
      source_image=CloneImage(image,image->columns,rows,MagickTrue,exception);
      if (source_image == (Image *) NULL)
        {
          contribution=(ContributionInfo *)
            RelinquishMagickMemory(contribution);
          resize_image=DestroyImage(resize_image);
          return((Image *) NULL);
        }
      span=(MagickSizeType) (resize_image->columns+source_image->rows);
      status=VerticalFilter(image,source_image,y_factor,&filters[i],blur,
        contribution,span,&quantum,exception);
      status|=HorizontalFilter(source_image,resize_image,x_factor,&filters[i],
        blur,contribution,span,&quantum,exception);
    }
  /*
    Free allocated memory.
  */
  contribution=(ContributionInfo *) RelinquishMagickMemory(contribution);
  source_image=DestroyImage(source_image);
  if (status == MagickFalse)
    {
      resize_image=DestroyImage(resize_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  return(resize_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S a m p l e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SampleImage() scales an image to the desired dimensions with pixel
%  sampling.  Unlike other scaling methods, this method does not introduce
%  any additional color into the scaled image.
%
%  The format of the SampleImage method is:
%
%      Image *SampleImage(const Image *image,const unsigned long columns,
%        const unsigned long rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o columns: The number of columns in the sampled image.
%
%    o rows: The number of rows in the sampled image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *SampleImage(const Image *image,const unsigned long columns,
  const unsigned long rows,ExceptionInfo *exception)
{
#define SampleImageTag  "Sample/Image"


  Image
    *sample_image;

  long
    j,
    *x_offset,
    y,
    *y_offset;

  MagickBooleanType
    status;

  PixelPacket
    *pixels;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *sample_indexes;

  register long
    x;

  register PixelPacket
    *q;
  /*
    Initialize sampled image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((columns == 0) || (rows == 0))
    ThrowImageException(ImageError,"NegativeOrZeroImageSize");
  sample_image=CloneImage(image,columns,rows,MagickTrue,exception);
  if (sample_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Allocate scan line buffer and column offset buffers.
  */
  pixels=(PixelPacket *)
    AcquireMagickMemory((size_t) image->columns*sizeof(*pixels));
  x_offset=(long *)
    AcquireMagickMemory((size_t) sample_image->columns*sizeof(*x_offset));
  y_offset=(long *)
    AcquireMagickMemory((size_t) sample_image->rows*sizeof(*y_offset));
  if ((pixels == (PixelPacket *) NULL) || (x_offset == (long *) NULL) ||
      (y_offset == (long *) NULL))
    {
      sample_image=DestroyImage(sample_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Initialize pixel offsets.
  */
  for (x=0; x < (long) sample_image->columns; x++)
    x_offset[x]=(long)
      (((MagickRealType) x+0.5)*image->columns/sample_image->columns);
  for (y=0; y < (long) sample_image->rows; y++)
    y_offset[y]=(long)
      (((MagickRealType) y+0.5)*image->rows/sample_image->rows);
  /*
    Sample each row.
  */
  j=(-1);
  p=AcquireImagePixels(image,0,0,image->columns,1,exception);
  indexes=GetIndexes(image);
  for (y=0; y < (long) sample_image->rows; y++)
  {
    q=SetImagePixels(sample_image,0,y,sample_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    sample_indexes=GetIndexes(sample_image);
    if (j != y_offset[y])
      {
        /*
          Read a scan line.
        */
        j=y_offset[y];
        p=AcquireImagePixels(image,0,j,image->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        (void) CopyMagickMemory(pixels,p,(size_t)
          image->columns*sizeof(*pixels));
      }
    /*
      Sample each column.
    */
    for (x=0; x < (long) sample_image->columns; x++)
      *q++=pixels[x_offset[x]];
    if (sample_indexes != (IndexPacket *) NULL)
      for (x=0; x < (long) sample_image->columns; x++)
        sample_indexes[x]=indexes[x_offset[x]];
    if (SyncImagePixels(sample_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SampleImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  y_offset=(long *) RelinquishMagickMemory(y_offset);
  x_offset=(long *) RelinquishMagickMemory(x_offset);
  pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
  return(sample_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S c a l e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ScaleImage() changes the size of an image to the given dimensions.
%
%  The format of the ScaleImage method is:
%
%      Image *ScaleImage(const Image *image,const unsigned long columns,
%        const unsigned long rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ScaleImage(const Image *image,const unsigned long columns,
  const unsigned long rows,ExceptionInfo *exception)
{
#define ScaleImageTag  "Scale/Image"

  Image
    *scale_image;

  long
    number_rows,
    y;

  MagickBooleanType
    next_column,
    next_row,
    status;

  MagickPixelPacket
    pixel,
    *scale_scanline,
    *scanline,
    *x_vector,
    *y_vector,
    zero;

  PointInfo
    scale,
    span;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *scale_indexes;

  register long
    i,
    x;

  register MagickPixelPacket
    *s,
    *t;

  register PixelPacket
    *q;

  /*
    Initialize scaled image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((columns == 0) || (rows == 0))
    return((Image *) NULL);
  scale_image=CloneImage(image,columns,rows,MagickTrue,exception);
  if (scale_image == (Image *) NULL)
    return((Image *) NULL);
  scale_image->storage_class=DirectClass;
  /*
    Allocate memory.
  */
  x_vector=(MagickPixelPacket *)
    AcquireMagickMemory((size_t) image->columns*sizeof(*x_vector));
  scanline=x_vector;
  if (image->rows != scale_image->rows)
    scanline=(MagickPixelPacket *)
      AcquireMagickMemory((size_t) image->columns*sizeof(*scanline));
  scale_scanline=(MagickPixelPacket *) AcquireMagickMemory((size_t)
    scale_image->columns*sizeof(*scale_scanline));
  y_vector=(MagickPixelPacket *)
    AcquireMagickMemory((size_t) image->columns*sizeof(*y_vector));
  if ((scanline == (MagickPixelPacket *) NULL) ||
      (scale_scanline == (MagickPixelPacket *) NULL) ||
      (x_vector == (MagickPixelPacket *) NULL) ||
      (y_vector == (MagickPixelPacket *) NULL))
    {
      scale_image=DestroyImage(scale_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Scale image.
  */
  number_rows=0;
  next_row=MagickTrue;
  span.y=1.0;
  scale.y=(MagickRealType) scale_image->rows/(MagickRealType) image->rows;
  (void) ResetMagickMemory(y_vector,0,(size_t) image->columns*
    sizeof(*y_vector));
  GetMagickPixelPacket(image,&pixel);
  (void) ResetMagickMemory(&zero,0,sizeof(zero));
  i=0;
  for (y=0; y < (long) scale_image->rows; y++)
  {
    q=SetImagePixels(scale_image,0,y,scale_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    scale_indexes=GetIndexes(scale_image);
    if (scale_image->rows == image->rows)
      {
        /*
          Read a new scanline.
        */
        p=AcquireImagePixels(image,0,i++,image->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          x_vector[x].red=(MagickRealType) p->red;
          x_vector[x].green=(MagickRealType) p->green;
          x_vector[x].blue=(MagickRealType) p->blue;
          if (image->matte != MagickFalse)
            x_vector[x].opacity=(MagickRealType) p->opacity;
          if (indexes != (IndexPacket *) NULL)
            x_vector[x].index=(MagickRealType) indexes[x];
          p++;
        }
      }
    else
      {
        /*
          Scale Y direction.
        */
        while (scale.y < span.y)
        {
          if ((next_row != MagickFalse) && (number_rows < (long) image->rows))
            {
              /*
                Read a new scanline.
              */
              p=AcquireImagePixels(image,0,i++,image->columns,1,exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetIndexes(image);
              for (x=0; x < (long) image->columns; x++)
              {
                x_vector[x].red=(MagickRealType) p->red;
                x_vector[x].green=(MagickRealType) p->green;
                x_vector[x].blue=(MagickRealType) p->blue;
                if (image->matte != MagickFalse)
                  x_vector[x].opacity=(MagickRealType) p->opacity;
                if (indexes != (IndexPacket *) NULL)
                  x_vector[x].index=(MagickRealType) indexes[x];
                p++;
              }
              number_rows++;
            }
          for (x=0; x < (long) image->columns; x++)
          {
            y_vector[x].red+=scale.y*x_vector[x].red;
            y_vector[x].green+=scale.y*x_vector[x].green;
            y_vector[x].blue+=scale.y*x_vector[x].blue;
            if (scale_image->matte != MagickFalse)
              y_vector[x].opacity+=scale.y*x_vector[x].opacity;
            if (scale_indexes != (IndexPacket *) NULL)
              y_vector[x].index+=scale.y*x_vector[x].index;
          }
          span.y-=scale.y;
          scale.y=(MagickRealType) scale_image->rows/
            (MagickRealType) image->rows;
          next_row=MagickTrue;
        }
        if ((next_row != MagickFalse) && (number_rows < (long) image->rows))
          {
            /*
              Read a new scanline.
            */
            p=AcquireImagePixels(image,0,i++,image->columns,1,exception);
            if (p == (const PixelPacket *) NULL)
              break;
            indexes=GetIndexes(image);
            for (x=0; x < (long) image->columns; x++)
            {
              x_vector[x].red=(MagickRealType) p->red;
              x_vector[x].green=(MagickRealType) p->green;
              x_vector[x].blue=(MagickRealType) p->blue;
              if (image->matte != MagickFalse)
                x_vector[x].opacity=(MagickRealType) p->opacity;
              if (indexes != (IndexPacket *) NULL)
                x_vector[x].index=(MagickRealType) indexes[x];
              p++;
            }
            number_rows++;
            next_row=MagickFalse;
          }
        s=scanline;
        for (x=0; x < (long) image->columns; x++)
        {
          pixel.red=y_vector[x].red+span.y*x_vector[x].red;
          pixel.green=y_vector[x].green+span.y*x_vector[x].green;
          pixel.blue=y_vector[x].blue+span.y*x_vector[x].blue;
          if (image->matte != MagickFalse)
            pixel.opacity=y_vector[x].opacity+span.y*x_vector[x].opacity;
          if (scale_indexes != (IndexPacket *) NULL)
            pixel.index=y_vector[x].index+span.y*x_vector[x].index;
          s->red=pixel.red;
          s->green=pixel.green;
          s->blue=pixel.blue;
          if (scale_image->matte != MagickFalse)
            s->opacity=pixel.opacity;
          if (scale_indexes != (IndexPacket *) NULL)
            s->index=pixel.index;
          s++;
          y_vector[x]=zero;
        }
        scale.y-=span.y;
        if (scale.y <= 0)
          {
            scale.y=(MagickRealType) scale_image->rows/
              (MagickRealType) image->rows;
            next_row=MagickTrue;
          }
        span.y=1.0;
      }
    if (scale_image->columns == image->columns)
      {
        /*
          Transfer scanline to scaled image.
        */
        s=scanline;
        for (x=0; x < (long) scale_image->columns; x++)
        {
          q->red=RoundToQuantum(s->red);
          q->green=RoundToQuantum(s->green);
          q->blue=RoundToQuantum(s->blue);
          if (scale_image->matte != MagickFalse)
            q->opacity=RoundToQuantum(s->opacity);
          if (scale_indexes != (IndexPacket *) NULL)
            scale_indexes[x]=(IndexPacket) RoundToQuantum(s->index);
          q++;
          s++;
        }
      }
    else
      {
        /*
          Scale X direction.
        */
        pixel=zero;
        next_column=MagickFalse;
        span.x=1.0;
        s=scanline;
        t=scale_scanline;
        for (x=0; x < (long) image->columns; x++)
        {
          scale.x=(MagickRealType) scale_image->columns/
            (MagickRealType) image->columns;
          while (scale.x >= span.x)
          {
            if (next_column != MagickFalse)
              {
                pixel=zero;
                t++;
              }
            pixel.red+=span.x*s->red;
            pixel.green+=span.x*s->green;
            pixel.blue+=span.x*s->blue;
            if (image->matte != MagickFalse)
              pixel.opacity+=span.x*s->opacity;
            if (scale_indexes != (IndexPacket *) NULL)
              pixel.index+=span.x*s->index;
            t->red=pixel.red;
            t->green=pixel.green;
            t->blue=pixel.blue;
            if (scale_image->matte != MagickFalse)
              t->opacity=pixel.opacity;
            if (scale_indexes != (IndexPacket *) NULL)
              t->index=pixel.index;
            scale.x-=span.x;
            span.x=1.0;
            next_column=MagickTrue;
          }
        if (scale.x > 0)
          {
            if (next_column != MagickFalse)
              {
                pixel=zero;
                next_column=MagickFalse;
                t++;
              }
            pixel.red+=scale.x*s->red;
            pixel.green+=scale.x*s->green;
            pixel.blue+=scale.x*s->blue;
            if (scale_image->matte != MagickFalse)
              pixel.opacity+=scale.x*s->opacity;
            if (scale_indexes != (IndexPacket *) NULL)
              pixel.index+=scale.x*s->index;
            span.x-=scale.x;
          }
        s++;
      }
      if (span.x > 0)
        {
          s--;
          pixel.red+=span.x*s->red;
          pixel.green+=span.x*s->green;
          pixel.blue+=span.x*s->blue;
          if (scale_image->matte != MagickFalse)
            pixel.opacity+=span.x*s->opacity;
          if (scale_indexes != (IndexPacket *) NULL)
            pixel.index+=span.x*s->index;
        }
      if ((next_column == MagickFalse) &&
          ((long) (t-scale_scanline) < (long) scale_image->columns))
        {
          t->red=pixel.red;
          t->green=pixel.green;
          t->blue=pixel.blue;
          if (scale_image->matte != MagickFalse)
            t->opacity=pixel.opacity;
          if (scale_indexes != (IndexPacket *) NULL)
            t->index=pixel.index;
        }
      /*
        Transfer scanline to scaled image.
      */
      t=scale_scanline;
      for (x=0; x < (long) scale_image->columns; x++)
      {
        q->red=RoundToQuantum(t->red);
        q->green=RoundToQuantum(t->green);
        q->blue=RoundToQuantum(t->blue);
        if (scale_image->matte != MagickFalse)
          q->opacity=RoundToQuantum(t->opacity);
        if (scale_indexes != (IndexPacket *) NULL)
          scale_indexes[x]=(IndexPacket) RoundToQuantum(t->index);
        t++;
        q++;
      }
    }
    if (SyncImagePixels(scale_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ScaleImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  /*
    Free allocated memory.
  */
  y_vector=(MagickPixelPacket *) RelinquishMagickMemory(y_vector);
  scale_scanline=(MagickPixelPacket *) RelinquishMagickMemory(scale_scanline);
  if (scale_image->rows != image->rows)
    scanline=(MagickPixelPacket *) RelinquishMagickMemory(scanline);
  x_vector=(MagickPixelPacket *) RelinquishMagickMemory(x_vector);
  return(scale_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T h u m b n a i l I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ThumbnailImage() changes the size of an image to the given dimensions and
%  removes any associated profiles.  The goal is to produce small low cost
%  thumbnail images suited for display on the Web.
%
%  The format of the ThumbnailImage method is:
%
%      Image *ThumbnailImage(const Image *image,const unsigned long columns,
%        const unsigned long rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ThumbnailImage(const Image *image,
  const unsigned long columns,const unsigned long rows,ExceptionInfo *exception)
{
  Image
    *sample_image,
    *thumbnail_image;

  MagickRealType
    x_factor,
    y_factor;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  x_factor=(MagickRealType) columns/(MagickRealType) image->columns;
  y_factor=(MagickRealType) rows/(MagickRealType) image->rows;
  if ((x_factor*y_factor) > 0.1)
    {
      thumbnail_image=ZoomImage(image,columns,rows,exception);
      (void) StripImage(thumbnail_image);
      return(thumbnail_image);
    }
  sample_image=SampleImage(image,5*columns,5*rows,exception);
  if (sample_image == (Image *) NULL)
    return((Image *) NULL);
  thumbnail_image=ZoomImage(sample_image,columns,rows,exception);
  sample_image=DestroyImage(sample_image);
  (void) StripImage(thumbnail_image);
  return(thumbnail_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   Z o o m I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ZoomImage() creates a new image that is a scaled size of an existing one.
%  It allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.  The Point filter gives fast pixel replication,
%  Triangle is equivalent to bi-linear interpolation, and Mitchel giver slower,
%  very high-quality results.  See Graphic Gems III for details on this
%  algorithm.
%
%  The filter member of the Image structure specifies which image filter to
%  use. Blur specifies the blur factor where > 1 is blurry, < 1 is sharp.
%
%  The format of the ZoomImage method is:
%
%      Image *ZoomImage(const Image *image,const unsigned long columns,
%        const unsigned long rows,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o zoom_image: Method ZoomImage returns a pointer to the image after
%      scaling.  A null image is returned if there is a memory shortage.
%
%    o image: The image.
%
%    o columns: An integer that specifies the number of columns in the zoom
%      image.
%
%    o rows: An integer that specifies the number of rows in the scaled
%      image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ZoomImage(const Image *image,const unsigned long columns,
  const unsigned long rows,ExceptionInfo *exception)
{
  Image
    *zoom_image;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  zoom_image=ResizeImage(image,columns,rows,image->filter,image->blur,
    exception);
  return(zoom_image);
}
