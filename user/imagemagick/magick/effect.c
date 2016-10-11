/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   EEEEE  FFFFF  FFFFF  EEEEE  CCCC  TTTTT                   %
%                   E      F      F      E     C        T                     %
%                   EEE    FFF    FFF    EEE   C        T                     %
%                   E      F      F      E     C        T                     %
%                   EEEEE  F      F      EEEEE  CCCC    T                     %
%                                                                             %
%                                                                             %
%                      ImageMagick Image Effects Methods                      %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                 October 1996                                %
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
#include "magick/blob.h"
#include "magick/cache-view.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/constitute.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/effect.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/montage.h"
#include "magick/pixel-private.h"
#include "magick/quantize.h"
#include "magick/random_.h"
#include "magick/resize.h"
#include "magick/resource_.h"
#include "magick/segment.h"
#include "magick/shear.h"
#include "magick/signature.h"
#include "magick/string_.h"
#include "magick/transform.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     A d a p t i v e T h r e s h o l d I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AdaptiveThresholdImage() selects an individual threshold for each pixel
%  based on the range of intensity values in its local neighborhood.  This
%  allows for thresholding of an image whose global intensity histogram
%  doesn't contain distinctive peaks.
%
%  The format of the AdaptiveThresholdImage method is:
%
%      Image *AdaptiveThresholdImage(const Image *image,
%        const unsigned long width,const unsigned long height,
%        const long offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o width: The width of the local neighborhood.
%
%    o height: The height of the local neighborhood.
%
%    o offset: The mean offset.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *AdaptiveThresholdImage(const Image *image,
  const unsigned long width,const unsigned long height,const long offset,
  ExceptionInfo *exception)
{
#define ThresholdImageTag  "Threshold/Image"

  Image
    *threshold_image;

  IndexPacket
    *indexes,
    *threshold_indexes;

  long
    u,
    v,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel,
    mean;

  MagickRealType
    number_pixels;

  register const PixelPacket
    *p,
    *r;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize thresholded image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((image->columns < width) || (image->rows < height))
    ThrowImageException(OptionError,"ImageSmallerThanRadius");
  threshold_image=CloneImage(image,0,0,MagickTrue,exception);
  if (threshold_image == (Image *) NULL)
    return((Image *) NULL);
  threshold_image->storage_class=DirectClass;
  /*
    Threshold each row of the image.
  */
  GetMagickPixelPacket(image,&mean);
  number_pixels=(MagickRealType) (width*height);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-height/2L,image->columns+
      width,height,exception);
    q=GetImagePixels(threshold_image,0,y,threshold_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    threshold_indexes=GetIndexes(threshold_image);
    for (x=0; x < (long) image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      r=p;
      for (v=0; v < (long) height; v++)
      {
        for (u=0; u < (long) width; u++)
        {
          pixel.red+=r[u].red;
          pixel.green+=r[u].green;
          pixel.blue+=r[u].blue;
          if (image->matte != MagickFalse)
            pixel.opacity+=r[u].opacity;
          if (image->colorspace == CMYKColorspace)
            pixel.index=(MagickRealType) indexes[x+(r-p)+u];
        }
        r+=image->columns+width;
      }
      mean.red=(MagickRealType) (pixel.red/number_pixels+offset);
      mean.green=(MagickRealType) (pixel.green/number_pixels+offset);
      mean.blue=(MagickRealType) (pixel.blue/number_pixels+offset);
      if (image->matte != MagickFalse)
        mean.opacity=(MagickRealType) (pixel.opacity/number_pixels+offset);
      if (image->colorspace == CMYKColorspace)
        mean.index=(MagickRealType) (pixel.index/number_pixels+offset);
      q->red=(Quantum) (((MagickRealType)
        q->red <= mean.red) ? 0 : QuantumRange);
      q->green=(Quantum) (((MagickRealType)
        q->green <= mean.green) ? 0 : QuantumRange);
      q->blue=(Quantum) (((MagickRealType)
        q->blue <= mean.blue) ? 0 : QuantumRange);
      if (image->matte != MagickFalse)
        q->opacity=(Quantum) (((MagickRealType)
          q->opacity <= mean.opacity) ? 0 : QuantumRange);
      if (image->colorspace == CMYKColorspace)
        threshold_indexes[x]=(IndexPacket) (((MagickRealType)
          threshold_indexes[x] <= mean.index) ? 0 : QuantumRange);
      p++;
      q++;
    }
    if (SyncImagePixels(threshold_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ThresholdImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(threshold_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     A d d N o i s e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AddNoiseImage() adds random noise to the image.
%
%  The format of the AddNoiseImage method is:
%
%      Image *AddNoiseImage(const Image *image,const NoiseType noise_type,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o noise_type:  The type of noise: Uniform, Gaussian, Multiplicative,
%      Impulse, Laplacian, or Poisson.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static inline Quantum GenerateNoise(const Quantum pixel,
  const NoiseType noise_type)
{
#define NoiseEpsilon  1.0e-5
#define SigmaUniform  ScaleCharToQuantum(4)
#define SigmaGaussian  ScaleCharToQuantum(4)
#define SigmaImpulse  0.10
#define SigmaLaplacian ScaleCharToQuantum(10)
#define SigmaMultiplicativeGaussian  ScaleCharToQuantum(1)
#define SigmaPoisson  0.05
#define TauGaussian  ScaleCharToQuantum(20)

  MagickRealType
    alpha,
    beta,
    noise,
    sigma;

  alpha=GetRandomValue();
  if (alpha == 0.0)
    alpha=1.0;
  switch (noise_type)
  {
    case UniformNoise:
    default:
    {
      noise=(MagickRealType) pixel+SigmaUniform*(alpha-0.5);
      break;
    }
    case GaussianNoise:
    {
      MagickRealType
        tau;

      beta=GetRandomValue();
      sigma=sqrt(-2.0*log((double) alpha))*cos((double) (2.0*MagickPI*beta));
      tau=sqrt(-2.0*log((double) alpha))*sin((double) (2.0*MagickPI*beta));
      noise=(MagickRealType) pixel+sqrt((double) pixel)*SigmaGaussian*sigma+
        TauGaussian*tau;
      break;
    }
    case MultiplicativeGaussianNoise:
    {
      if (alpha <= NoiseEpsilon)
        sigma=(MagickRealType) QuantumRange;
      else
        sigma=sqrt(-2.0*log((double) alpha));
      beta=GetRandomValue();
      noise=(MagickRealType) pixel+pixel*SigmaMultiplicativeGaussian*sigma/2.0*
        cos((double) (2.0*MagickPI*beta));
      break;
    }
    case ImpulseNoise:
    {
      if (alpha < (SigmaImpulse/2.0))
        noise=0.0;
       else
         if (alpha >= (1.0-(SigmaImpulse/2.0)))
           noise=(MagickRealType) QuantumRange;
         else
           noise=(MagickRealType) pixel;
      break;
    }
    case LaplacianNoise:
    {
      if (alpha <= 0.5)
        {
          if (alpha <= NoiseEpsilon)
            noise=(MagickRealType) pixel-(MagickRealType) QuantumRange;
          else
            noise=(MagickRealType) pixel+
              ScaleCharToQuantum(SigmaLaplacian*log((double) (2.0*alpha))+0.5);
          break;
        }
      beta=1.0-alpha;
      if (beta <= (0.5*NoiseEpsilon))
        noise=(MagickRealType) pixel+QuantumRange;
      else
        noise=(MagickRealType) pixel-
          ScaleCharToQuantum(SigmaLaplacian*log((double) (2.0*beta))+0.5);
      break;
    }
    case PoissonNoise:
    {
      MagickRealType
        poisson;

      register long
        i;

      poisson=exp(-SigmaPoisson*(double) ScaleQuantumToChar(pixel));
      for (i=0; alpha > poisson; i++)
      {
        beta=GetRandomValue();
        alpha=alpha*beta;
      }
      noise=(MagickRealType) ScaleCharToQuantum(i/SigmaPoisson);
      break;
    }
  }
  return(RoundToQuantum(noise));
}

MagickExport Image *AddNoiseImage(const Image *image,const NoiseType noise_type,
  ExceptionInfo *exception)
{
#define AddNoiseImageTag  "AddNoise/Image"

  Image
    *noise_image;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize noise image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  noise_image=CloneImage(image,0,0,MagickTrue,exception);
  if (noise_image == (Image *) NULL)
    return((Image *) NULL);
  noise_image->storage_class=DirectClass;
  /*
    Add noise in each row.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=GetImagePixels(noise_image,0,y,noise_image->columns,1);
    if ((p == (PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->red=GenerateNoise(p->red,noise_type);
      q->green=GenerateNoise(p->green,noise_type);
      q->blue=GenerateNoise(p->blue,noise_type);
      p++;
      q++;
    }
    if (SyncImagePixels(noise_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(AddNoiseImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(noise_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     B i l e v e l I m a g e C h a n n e l                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BilevelImageChannel() changes the value of individual pixels based on
%  the intensity of each pixel channel.  The result is a high-contrast image.
%
%  The format of the BilevelImageChannel method is:
%
%      MagickBooleanType BilevelImageChannel(Image *image,
%        const ChannelType channel,const double threshold)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o threshold: define the threshold values.
%
%
*/

MagickExport MagickBooleanType BilevelImage(Image *image,const double threshold)
{
  MagickBooleanType
    status;

  status=BilevelImageChannel(image,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),threshold);
  return(status);
}

MagickExport MagickBooleanType BilevelImageChannel(Image *image,
  const ChannelType channel,const double threshold)
{
#define ThresholdImageTag  "Threshold/Image"

  long
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Threshold image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image->storage_class=DirectClass;
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        q->red=(Quantum) ((MagickRealType)
          q->red <= threshold ? 0 : QuantumRange);
      if ((channel & GreenChannel) != 0)
        q->green=(Quantum) ((MagickRealType)
          q->green <= threshold ? 0 : QuantumRange);
      if ((channel & BlueChannel) != 0)
        q->blue=(Quantum) ((MagickRealType)
          q->blue <= threshold ? 0 : QuantumRange);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=(Quantum) ((MagickRealType)
          q->opacity <= threshold ?  OpaqueOpacity : TransparentOpacity);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        indexes[x]=(IndexPacket) ((MagickRealType)
          indexes[x] <= threshold ? 0 : QuantumRange);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ThresholdImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     B l a c k T h r e s h o l d I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlackThresholdImage() is like ThresholdImage() but forces all pixels below
%  the threshold into black while leaving all pixels above the threshold
%  unchanged.
%
%  The format of the BlackThresholdImage method is:
%
%      MagickBooleanType BlackThresholdImage(Image *image,const char *threshold)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o threshold: Define the threshold value
%
%
*/
MagickExport MagickBooleanType BlackThresholdImage(Image *image,
  const char *threshold)
{
#define ThresholdImageTag  "Threshold/Image"

  GeometryInfo
    geometry_info;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickStatusType
    flags;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Threshold image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (threshold == (const char *) NULL)
    return(MagickTrue);
  image->storage_class=DirectClass;
  flags=ParseGeometry(threshold,&geometry_info);
  pixel.red=geometry_info.rho;
  pixel.green=geometry_info.sigma;
  if ((flags & SigmaValue) == 0)
    pixel.green=pixel.red;
  pixel.blue=geometry_info.xi;
  if ((flags & XiValue) == 0)
    pixel.blue=pixel.red;
  pixel.opacity=geometry_info.psi;
  if ((flags & PsiValue) == 0)
    pixel.opacity=(MagickRealType) OpaqueOpacity;
  if ((flags & PercentValue) != 0)
    {
      pixel.red*=QuantumRange/100.0f;
      pixel.green*=QuantumRange/100.0f;
      pixel.blue*=QuantumRange/100.0f;
      pixel.opacity*=QuantumRange/100.0f;
    }
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    if (IsGray(pixel) != MagickFalse)
      for (x=(long) image->columns-1; x >= 0; x--)
      {
        if ((MagickRealType) PixelIntensityToQuantum(q) < pixel.red)
          {
            q->red=0;
            q->green=0;
            q->blue=0;
          }
        q++;
      }
    else
      for (x=(long) image->columns-1; x >= 0; x--)
      {
        if ((MagickRealType) q->red < pixel.red)
          q->red=0;
        if ((MagickRealType) q->green < pixel.green)
          q->green=0;
        if ((MagickRealType) q->blue < pixel.blue)
          q->blue=0;
        if ((MagickRealType) q->opacity < pixel.opacity)
          q->opacity=0;
        q++;
      }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ThresholdImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
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
%     B l u r I m a g e C h a n n e l                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlurImageChannel() blurs an image.  We convolve the image with a Gaussian
%  operator of the given radius and standard deviation (sigma).  For reasonable
%  results, the radius should be larger than sigma.  Use a radius of 0 and
%  BlurImageChannel() selects a suitable radius for you.
%
%  BlurImageChannel() differs from GaussianBlurImageChannel() in that it uses
%  a separable kernel which is faster but mathematically equivalent to the
%  non-separable kernel.
%
%  The format of the BlurImageChannel method is:
%
%      Image *BlurImageChannel(const Image *image,const ChannelType channel,
%        const double radius,const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport Image *BlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *blur_image;

  blur_image=BlurImageChannel(image,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),radius,sigma,exception);
  return(blur_image);
}

static double *GetBlurKernel(unsigned long width,const MagickRealType sigma)
{
#define KernelRank 3

  double
    *kernel;

  long
    bias;

  MagickRealType
    alpha,
    normalize;

  register long
    i;

  /*
    Generate a 1-D convolution kernel.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  kernel=(double *) AcquireMagickMemory((size_t) width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    return(0);
  (void) ResetMagickMemory(kernel,0,(size_t) width*sizeof(*kernel));
  bias=KernelRank*(long) width/2;
  for (i=(-bias); i <= bias; i++)
  {
    alpha=exp((-((double) (i*i))/(2.0*KernelRank*KernelRank*sigma*sigma)));
    kernel[(i+bias)/KernelRank]+=(double) (alpha/(MagickSQ2PI*sigma));
  }
  normalize=0.0;
  for (i=0; i < (long) width; i++)
    normalize+=kernel[i];
  for (i=0; i < (long) width; i++)
    kernel[i]/=normalize;
  return(kernel);
}

MagickExport Image *BlurImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  ExceptionInfo *exception)
{
#define BlurImageTag  "Blur/Image"

  double
    *kernel;

  Image
    *blur_image;

  IndexPacket
    *blur_indexes,
    *indexes;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickOffsetType
    offset;

  MagickRealType
    alpha,
    gamma;

  register const double
    *k;

  register const PixelPacket
    *p,
    *pixels;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    width;

  ViewInfo
    *view;

  /*
    Initialize blur image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if (sigma == 0.0)
    ThrowImageException(OptionError,"ZeroSigmaNotPermitted");
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel=GetBlurKernel(width,sigma);
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  blur_image->storage_class=DirectClass;
  if (image->debug != MagickFalse)
    {
      char
        format[MaxTextExtent],
        *message;

      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  BlurImage with %ld kernel:",width);
      message=AcquireString("");
      k=kernel;
      for (i=0; i < (long) width; i++)
      {
        *message='\0';
        (void) FormatMagickString(format,MaxTextExtent,"%ld: ",i);
        (void) ConcatenateString(&message,format);
        (void) FormatMagickString(format,MaxTextExtent,"%g ",*k++);
        (void) ConcatenateString(&message,format);
        (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
      }
    }
  /*
    Blur rows.
  */
  for (y=0; y < (long) blur_image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y,image->columns+width,1,
      exception);
    q=GetImagePixels(blur_image,0,y,blur_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    blur_indexes=GetIndexes(blur_image);
    for (x=0; x < (long) blur_image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      pixels=p;
      k=kernel;
      for (i=0; i < (long) width; i++)
      {
        alpha=1.0;
        if (((channel & OpacityChannel) != 0) &&
            (image->matte != MagickFalse))
          {
            alpha=((MagickRealType) QuantumRange-pixels[i].opacity)/
              QuantumRange;
            pixel.opacity+=(*k)*pixels[i].opacity;
          }
        if ((channel & RedChannel) != 0)
          pixel.red+=(*k)*alpha*pixels[i].red;
        if ((channel & GreenChannel) != 0)
          pixel.green+=(*k)*alpha*pixels[i].green;
        if ((channel & BlueChannel) != 0)
          pixel.blue+=(*k)*alpha*pixels[i].blue;
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          pixel.index+=(*k)*alpha*indexes[x+(pixels-p)+i];
        gamma+=(*k)*alpha;
        k++;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      if ((channel & RedChannel) != 0)
        q->red=RoundToQuantum(gamma*pixel.red+image->bias);
      if ((channel & GreenChannel) != 0)
        q->green=RoundToQuantum(gamma*pixel.green+image->bias);
      if ((channel & BlueChannel) != 0)
        q->blue=RoundToQuantum(gamma*pixel.blue+image->bias);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=RoundToQuantum(pixel.opacity+image->bias);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        blur_indexes[x]=RoundToQuantum(gamma*pixel.index+image->bias);
      p++;
      q++;
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows+image->columns) != MagickFalse))
      {
        status=image->progress_monitor(BlurImageTag,y,image->rows+
          image->columns,image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  /*
    Blur columns.
  */
  view=OpenCacheView(blur_image);
  for (x=0; x < (long) blur_image->columns; x++)
  {
    p=AcquireCacheView(view,x,-((long) width/2L),1,blur_image->rows+width,
      exception);
    q=GetImagePixels(blur_image,x,0,1,blur_image->rows);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetCacheViewIndexes(view);
    blur_indexes=GetIndexes(blur_image);
    for (y=0; y < (long) blur_image->rows; y++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      pixels=p;
      k=kernel;
      for (i=0; i < (long) width; i++)
      {
        alpha=1.0;
        if (((channel & OpacityChannel) != 0) &&
            (image->matte != MagickFalse))
          {
            alpha=((MagickRealType) QuantumRange-pixels[i].opacity)/
              QuantumRange;
            pixel.opacity+=(*k)*pixels[i].opacity;
          }
        if ((channel & RedChannel) != 0)
          pixel.red+=(*k)*alpha*pixels[i].red;
        if ((channel & GreenChannel) != 0)
          pixel.green+=(*k)*alpha*pixels[i].green;
        if ((channel & BlueChannel) != 0)
          pixel.blue+=(*k)*alpha*pixels[i].blue;
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          pixel.index+=(*k)*alpha*indexes[x+(pixels-p)+i];
        gamma+=(*k)*alpha;
        k++;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      if ((channel & RedChannel) != 0)
        q->red=RoundToQuantum(gamma*pixel.red+image->bias);
      if ((channel & GreenChannel) != 0)
        q->green=RoundToQuantum(gamma*pixel.green+image->bias);
      if ((channel & BlueChannel) != 0)
        q->blue=RoundToQuantum(gamma*pixel.blue+image->bias);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=RoundToQuantum(pixel.opacity+image->bias);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        blur_indexes[x]=RoundToQuantum(gamma*pixel.index+image->bias);
      p++;
      q++;
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    offset=(MagickOffsetType) image->rows+x;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(offset,image->rows+image->columns) != MagickFalse))
      {
        status=image->progress_monitor(BlurImageTag,offset,image->rows+
          image->columns,image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  CloseCacheView(view);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     D e s p e c k l e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DespeckleImage() reduces the speckle noise in an image while perserving the
%  edges of the original image.
%
%  The format of the DespeckleImage method is:
%
%      Image *DespeckleImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *DespeckleImage(const Image *image,ExceptionInfo *exception)
{
#define DespeckleImageTag  "Despeckle/Image"

  Image
    *despeckle_image;

  int
    layer;

  long
    j,
    y;

  MagickBooleanType
    status;

  Quantum
    *buffer,
    *pixels;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register PixelPacket
    *q;

  size_t
    length;

  static const int
    X[4]= {0, 1, 1,-1},
    Y[4]= {1, 0, 1, 1};

  /*
    Allocate despeckled image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  despeckle_image=CloneImage(image,0,0,MagickTrue,exception);
  if (despeckle_image == (Image *) NULL)
    return((Image *) NULL);
  despeckle_image->storage_class=DirectClass;
  /*
    Allocate image buffers.
  */
  length=(size_t) (image->columns+2)*(image->rows+2)*sizeof(*pixels);
  pixels=(Quantum *) AcquireMagickMemory(length);
  buffer=(Quantum *) AcquireMagickMemory(length);
  if ((buffer == (Quantum *) NULL) || (pixels == (Quantum *) NULL))
    {
      despeckle_image=DestroyImage(despeckle_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Reduce speckle in the image.
  */
  for (layer=0; layer <= 3; layer++)
  {
    (void) ResetMagickMemory(pixels,0,length);
    j=(long) image->columns+2;
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,exception);
      if (p == (const PixelPacket *) NULL)
        break;
      j++;
      for (x=(long) image->columns-1; x >= 0; x--)
      {
        switch (layer)
        {
          case 0: pixels[j]=p->red; break;
          case 1: pixels[j]=p->green; break;
          case 2: pixels[j]=p->blue; break;
          case 3: pixels[j]=p->opacity; break;
          default: break;
        }
        p++;
        j++;
      }
      j++;
    }
    (void) ResetMagickMemory(buffer,0,length);
    for (i=0; i < 4; i++)
    {
      Hull(X[i],Y[i],image->columns,image->rows,pixels,buffer,1);
      Hull(-X[i],-Y[i],image->columns,image->rows,pixels,buffer,1);
      Hull(-X[i],-Y[i],image->columns,image->rows,pixels,buffer,-1);
      Hull(X[i],Y[i],image->columns,image->rows,pixels,buffer,-1);
    }
    j=(long) image->columns+2;
    for (y=0; y < (long) image->rows; y++)
    {
      q=GetImagePixels(despeckle_image,0,y,despeckle_image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      j++;
      for (x=(long) image->columns-1; x >= 0; x--)
      {
        switch (layer)
        {
          case 0: q->red=pixels[j]; break;
          case 1: q->green=pixels[j]; break;
          case 2: q->blue=pixels[j]; break;
          case 3: q->opacity=pixels[j]; break;
          default: break;
        }
        q++;
        j++;
      }
      if (SyncImagePixels(despeckle_image) == MagickFalse)
        break;
      j++;
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(layer,3) != MagickFalse))
      {
        status=image->progress_monitor(DespeckleImageTag,layer,3,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  /*
    Free resources.
  */
  buffer=(Quantum *) RelinquishMagickMemory(buffer);
  pixels=(Quantum *) RelinquishMagickMemory(pixels);
  return(despeckle_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     E d g e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EdgeImage() finds edges in an image.  Radius defines the radius of the
%  convolution filter.  Use a radius of 0 and EdgeImage() selects a suitable
%  radius for you.
%
%  The format of the EdgeImage method is:
%
%      Image *EdgeImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *EdgeImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
  Image
    *edge_image;

  double
    *kernel;

  register long
    i;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth1D(radius,0.5);
  kernel=(double *) AcquireMagickMemory((size_t) width*width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  for (i=0; i < (long) (width*width); i++)
    kernel[i]=(-1.0);
  kernel[i/2]=(double) width*width-1.0;
  edge_image=ConvolveImage(image,width,kernel,exception);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(edge_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     E m b o s s I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EmbossImage() returns a grayscale image with a three-dimensional effect.
%  We convolve the image with a Gaussian operator of the given radius and
%  standard deviation (sigma).  For reasonable results, radius should be
%  larger than sigma.  Use a radius of 0 and Emboss() selects a suitable
%  radius for you.
%
%  The format of the EmbossImage method is:
%
%      Image *EmbossImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *EmbossImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  double
    *kernel;

  Image
    *emboss_image;

  long
    j;

  MagickRealType
    alpha;

  register long
    i,
    u,
    v;

  unsigned long
    width;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if (sigma == 0.0)
    ThrowImageException(OptionError,"ZeroSigmaNotPermitted");
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double *) AcquireMagickMemory((size_t) width*width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  i=0;
  j=(long) width/2;
  for (v=(-((long) width/2)); v <= (long) (width/2); v++)
  {
    for (u=(-((long) width/2)); u <= (long) (width/2); u++)
    {
      alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
      kernel[i]=(double) (((u < 0) || (v < 0) ? -8.0 : 8.0)*alpha/
        (2.0*MagickPI*sigma*sigma));
      if (u == j)
        kernel[i]=0.0;
      i++;
    }
    j--;
  }
  emboss_image=ConvolveImage(image,width,kernel,exception);
  if (emboss_image != (Image *) NULL)
    (void) EqualizeImage(emboss_image);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(emboss_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     G a u s s i a n B l u r I m a g e C h a n n e l                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GaussianBlurImageChannel() blurs an image.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, the radius should be larger than sigma.  Use a
%  radius of 0 and GaussianBlurImage() selects a suitable radius for you
%
%  The format of the GaussianBlurImageChannel method is:
%
%      Image *GaussianBlurImageChannel(const Image *image,
%        const ChannelType channel,const double radius,const double sigma,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o radius: the radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o channel: The channel type.
%
%    o sigma: the standard deviation of the Gaussian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport Image *GaussianBlurImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *blur_image;

  blur_image=GaussianBlurImageChannel(image,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),radius,sigma,exception);
  return(blur_image);
}

MagickExport Image *GaussianBlurImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  ExceptionInfo *exception)
{
  double
    *kernel;

  Image
    *blur_image;

  MagickRealType
    alpha;

  register long
    i,
    u,
    v;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if (sigma == 0.0)
    ThrowImageException(OptionError,"ZeroSigmaNotPermitted");
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double *) AcquireMagickMemory((size_t) width*width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  i=0;
  for (v=(-((long) width/2)); v <= (long) (width/2); v++)
  {
    for (u=(-((long) width/2)); u <= (long) (width/2); u++)
    {
      alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
      kernel[i]=(double) (alpha/(2.0*MagickPI*sigma*sigma));
      i++;
    }
  }
  blur_image=ConvolveImageChannel(image,channel,width,kernel,exception);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     M e d i a n F i l t e r I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MedianFilterImage() applies a digital filter that improves the quality
%  of a noisy image.  Each pixel is replaced by the median in a set of
%  neighboring pixels as defined by radius.
%
%  The algorithm was contributed by Mike Edmonds and implements an insertion
%  sort for selecting median color-channel values.  For more on this algorithm
%  see "Skip Lists: A probabilistic Alternative to Balanced Trees" by William
%  Pugh in the June 1990 of Communications of the ACM.
%
%  The format of the MedianFilterImage method is:
%
%      Image *MedianFilterImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the pixel neighborhood.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

typedef struct _MedianListNode
{
  unsigned long
    next[9],
    count,
    signature;
} MedianListNode;

typedef struct _MedianSkipList
{
  long
    level;

  MedianListNode
    nodes[65537];
} MedianSkipList;

typedef struct _MedianPixelList
{
  unsigned long
    center,
    seed,
    signature;

  MedianSkipList
    lists[5];
} MedianPixelList;

static void AddNodeMedianList(MedianPixelList *pixel_list,int channel,
  unsigned long color)
{
  register long
    level;

  register MedianSkipList
    *list;

  unsigned long
    search,
    update[9];

  /*
    Initialize the node.
  */
  list=pixel_list->lists+channel;
  list->nodes[color].signature=pixel_list->signature;
  list->nodes[color].count=1;
  /*
    Determine where it belongs in the list.
  */
  search=65536UL;
  for (level=list->level; level >= 0; level--)
  {
    while (list->nodes[search].next[level] < color)
      search=list->nodes[search].next[level];
    update[level]=search;
  }
  /*
    Generate a pseudo-random level for this node.
  */
  for (level=0; ; level++)
  {
    pixel_list->seed=(pixel_list->seed*42893621L)+1L;
    if ((pixel_list->seed & 0x300) != 0x300)
      break;
  }
  if (level > 8)
    level=8;
  if (level > (list->level+2))
    level=list->level+2;
  /*
    If we're raising the list's level, link back to the root node.
  */
  while (level > list->level)
  {
    list->level++;
    update[list->level]=65536UL;
  }
  /*
    Link the node into the skip-list.
  */
  do
  {
    list->nodes[color].next[level]=list->nodes[update[level]].next[level];
    list->nodes[update[level]].next[level]=color;
  }
  while (level-- > 0);
}

static MagickPixelPacket GetMedianList(MedianPixelList *pixel_list)
{
  MagickPixelPacket
    pixel;

  register long
    channel;

  register MedianSkipList
    *list;

  unsigned long
    center,
    channels[5],
    color,
    count;

  /*
    Find the median value for each of the color.
  */
  center=pixel_list->center;
  for (channel=0; channel < 5; channel++)
  {
    list=pixel_list->lists+channel;
    color=65536UL;
    count=0;
    do
    {
      color=list->nodes[color].next[0];
      count+=list->nodes[color].count;
    }
    while (count <= center);
    channels[channel]=color;
  }
  GetMagickPixelPacket((const Image *) NULL,&pixel);
  pixel.red=(MagickRealType) ScaleShortToQuantum(channels[0]);
  pixel.green=(MagickRealType) ScaleShortToQuantum(channels[1]);
  pixel.blue=(MagickRealType) ScaleShortToQuantum(channels[2]);
  pixel.opacity=(MagickRealType) ScaleShortToQuantum(channels[3]);
  pixel.index=(MagickRealType) ScaleShortToQuantum(channels[4]);
  return(pixel);
}

static void InitializeMedianList(MedianPixelList *pixel_list,
  unsigned long width)
{
  pixel_list->center=width*width/2;
  pixel_list->signature=MagickSignature;
  (void) ResetMagickMemory((void *) pixel_list->lists,0,
    5*sizeof(*pixel_list->lists));
}

static inline void InsertMedianList(const Image *image,const PixelPacket *pixel,
  const IndexPacket *indexes,MedianPixelList *pixel_list)
{
  unsigned long
    signature;

  unsigned short
    index;

  index=ScaleQuantumToShort(pixel->red);
  signature=pixel_list->lists[0].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[0].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,0,index);
  index=ScaleQuantumToShort(pixel->green);
  signature=pixel_list->lists[1].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[1].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,1,index);
  index=ScaleQuantumToShort(pixel->blue);
  signature=pixel_list->lists[2].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[2].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,2,index);
  index=ScaleQuantumToShort(pixel->opacity);
  signature=pixel_list->lists[3].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[3].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,3,index);
  if (image->colorspace == CMYKColorspace)
    index=ScaleQuantumToShort(*indexes);
  signature=pixel_list->lists[4].nodes[index].signature;
  if (signature == pixel_list->signature)
    pixel_list->lists[4].nodes[index].count++;
  else
    AddNodeMedianList(pixel_list,4,index);
}

static void ResetMedianList(MedianPixelList *pixel_list)
{
  int
    level;

  register long
    channel;

  register MedianListNode
    *root;

  register MedianSkipList
    *list;

  /*
    Reset the skip-list.
  */
  for (channel=0; channel < 5; channel++)
  {
    list=pixel_list->lists+channel;
    root=list->nodes+65536UL;
    list->level=0;
    for (level=0; level < 9; level++)
      root->next[level]=65536UL;
  }
  pixel_list->seed=pixel_list->signature++;
}

MagickExport Image *MedianFilterImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
#define MedianFilterImageTag  "MedianFilter/Image"

  Image
    *median_image;

  long
    x,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MedianPixelList
    *skiplist;

  register const PixelPacket
    *p,
    *r;

  register IndexPacket
    *indexes,
    *median_indexes,
    *s;

  register long
    u,
    v;

  register PixelPacket
    *q;

  unsigned long
    width;

  /*
    Initialize median image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,0.5);
  if ((image->columns < width) || (image->rows < width))
    ThrowImageException(OptionError,"ImageSmallerThanKernelRadius");
  median_image=CloneImage(image,0,0,MagickTrue,exception);
  if (median_image == (Image *) NULL)
    return((Image *) NULL);
  median_image->storage_class=DirectClass;
  /*
    Allocate skip-lists.
  */
  skiplist=(MedianPixelList *) AcquireMagickMemory(sizeof(*skiplist));
  if (skiplist == (MedianPixelList *) NULL)
    {
      median_image=DestroyImage(median_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Median filter each image row.
  */
  InitializeMedianList(skiplist,width);
  for (y=0; y < (long) median_image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    q=GetImagePixels(median_image,0,y,median_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    median_indexes=GetIndexes(median_image);
    for (x=0; x < (long) median_image->columns; x++)
    {
      r=p;
      s=indexes+x;
      ResetMedianList(skiplist);
      for (v=0; v < (long) width; v++)
      {
        for (u=0; u < (long) width; u++)
          InsertMedianList(image,r+u,s+u,skiplist);
        r+=image->columns+width;
        s+=image->columns+width;
      }
      pixel=GetMedianList(skiplist);
      SetPixelPacket(&pixel,q,median_indexes+x);
      p++;
      q++;
    }
    if (SyncImagePixels(median_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(MedianFilterImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  skiplist=(MedianPixelList *) RelinquishMagickMemory(skiplist);
  return(median_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o t i o n B l u r I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MotionBlurImage() simulates motion blur.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and MotionBlurImage() selects a suitable radius for you.
%  Angle gives the angle of the blurring motion.
%
%  Andrew Protano contributed this effect.
%
%  The format of the MotionBlurImage method is:
%
%    Image *MotionBlurImage(const Image *image,const double radius,
%      const double sigma,const double angle,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the Gaussian, in pixels, not counting
%     the center pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o angle: Apply the effect along this angle.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static double *GetMotionBlurKernel(unsigned long width,
  const MagickRealType sigma)
{
#define KernelRank 3

  double
    *kernel;

  long
    bias;

  MagickRealType
    alpha,
    normalize;

  register long
    i;

  /*
    Generate a 1-D convolution kernel.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  kernel=(double *) AcquireMagickMemory((size_t) width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    return(0);
  (void) ResetMagickMemory(kernel,0,(size_t) width*sizeof(*kernel));
  bias=(long) (KernelRank*width);
  for (i=0; i < (long) bias; i++)
  {
    alpha=exp((-((double) (i*i))/(2.0*KernelRank*KernelRank*sigma*sigma)));
    kernel[i/KernelRank]+=(double) alpha/(MagickSQ2PI*sigma);
  }
  normalize=0.0;
  for (i=0; i < (long) width; i++)
    normalize+=kernel[i];
  for (i=0; i < (long) width; i++)
    kernel[i]/=normalize;
  return(kernel);
}

MagickExport Image *MotionBlurImage(const Image *image,const double radius,
  const double sigma,const double angle,ExceptionInfo *exception)
{
  double
    *kernel;

  Image
    *blur_image;

  long
    u,
    v,
    y;

  MagickBooleanType
    status;

  MagickRealType
    alpha,
    gamma;

  PointInfo
    *offsets;

  MagickPixelPacket
    pixel;

  register const PixelPacket
    *p;

  register double
    *k;

  register IndexPacket
    *blur_indexes,
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    width;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  if (sigma == 0.0)
    ThrowImageException(OptionError,"ZeroSigmaNotPermitted");
  width=GetOptimalKernelWidth1D(radius,sigma);
  kernel=GetMotionBlurKernel(width,sigma);
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  offsets=(PointInfo *) AcquireMagickMemory(width*sizeof(*offsets));
  if (offsets == (PointInfo *) NULL)
    {
      kernel=(double *) RelinquishMagickMemory(kernel);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Allocate blur image.
  */
  blur_image=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    {
      kernel=(double *) RelinquishMagickMemory(kernel);
      offsets=(PointInfo *) RelinquishMagickMemory(offsets);
      return((Image *) NULL);
    }
  blur_image->storage_class=DirectClass;
  x=(long) (width*sin(DegreesToRadians(angle)));
  y=(long) (width*cos(DegreesToRadians(angle)));
  for (i=0; i < (long) width; i++)
  {
    offsets[i].y=(double) (-i*x)/sqrt((double) x*x+y*y);
    offsets[i].x=(double) (i*y)/sqrt((double) x*x+y*y);
  }
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(blur_image,0,y,blur_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    blur_indexes=GetIndexes(blur_image);
    for (x=0; x < (long) image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      k=kernel;
      for (i=0; i < (long) width; i++)
      {
        u=(long) (x+offsets[i].x);
        v=(long) (y+offsets[i].y);
        p=AcquireImagePixels(image,u,v,1,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        alpha=1.0;
        if (image->matte != MagickFalse)
          alpha=((MagickRealType) QuantumRange-p->opacity)/QuantumRange;
        pixel.red+=(*k)*alpha*p->red;
        pixel.green+=(*k)*alpha*p->green;
        pixel.blue+=(*k)*alpha*p->blue;
        if (image->matte != MagickFalse)
          pixel.opacity+=(*k)*p->opacity;
        if (image->colorspace == CMYKColorspace)
          pixel.index+=(*k)*alpha*(*indexes);
        gamma+=(*k)*alpha;
        k++;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      q->red=RoundToQuantum(gamma*pixel.red);
      q->green=RoundToQuantum(gamma*pixel.green);
      q->blue=RoundToQuantum(gamma*pixel.blue);
      q->opacity=OpaqueOpacity;
      if (image->matte != MagickFalse)
        q->opacity=RoundToQuantum(pixel.opacity);
      if (image->colorspace == CMYKColorspace)
        blur_indexes[x]=(IndexPacket) RoundToQuantum(gamma*pixel.index);
      q++;
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(BlurImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  kernel=(double *) RelinquishMagickMemory(kernel);
  offsets=(PointInfo *) RelinquishMagickMemory(offsets);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     P r e v i e w I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PreviewImage() tiles 9 thumbnails of the specified image with an image
%  processing operation applied with varying parameters.  This may be helpful
%  pin-pointing an appropriate parameter for a particular image processing
%  operation.
%
%  The format of the PreviewImages method is:
%
%      Image *PreviewImages(const Image *image,const PreviewType preview,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o preview: The image processing operation.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *PreviewImage(const Image *image,const PreviewType preview,
  ExceptionInfo *exception)
{
#define NumberTiles  9
#define PreviewImageTag  "Preview/Image"
#define DefaultPreviewGeometry  "204x204+10+10"

  char
    factor[MaxTextExtent],
    label[MaxTextExtent];

  double
    degrees,
    gamma,
    percentage,
    radius,
    sigma,
    threshold;

  Image
    *images,
    *montage_image,
    *preview_image,
    *thumbnail;

  ImageInfo
    *preview_info;

  long
    y;

  MagickBooleanType
    status;

  MontageInfo
    *montage_info;

  QuantizeInfo
    quantize_info;

  RectangleInfo
    geometry;

  register long
    i,
    x;

  unsigned long
    colors;

  /*
    Open output image file.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  colors=2;
  degrees=0.0;
  gamma=(-0.2f);
  preview_info=CloneImageInfo((ImageInfo *) NULL);
  SetGeometry(image,&geometry);
  (void) ParseMetaGeometry(DefaultPreviewGeometry,&geometry.x,&geometry.y,
    &geometry.width,&geometry.height);
  images=NewImageList();
  percentage=12.5;
  GetQuantizeInfo(&quantize_info);
  radius=0.0;
  sigma=1.0;
  threshold=0.0;
  x=0;
  y=0;
  for (i=0; i < NumberTiles; i++)
  {
    thumbnail=ZoomImage(image,geometry.width,geometry.height,exception);
    if (thumbnail == (Image *) NULL)
      break;
    (void) SetImageProgressMonitor(thumbnail,(MagickProgressMonitor) NULL,
      (void *) NULL);
    (void) SetImageAttribute(thumbnail,"Label",DefaultTileLabel);
    if (i == (NumberTiles/2))
      {
        (void) QueryColorDatabase("#dfdfdf",&thumbnail->matte_color,exception);
        AppendImageToList(&images,thumbnail);
        continue;
      }
    switch (preview)
    {
      case RotatePreview:
      {
        degrees+=45.0;
        preview_image=RotateImage(thumbnail,degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"rotate %g",degrees);
        break;
      }
      case ShearPreview:
      {
        degrees+=5.0;
        preview_image=ShearImage(thumbnail,degrees,degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"shear %gx%g",
          degrees,2.0*degrees);
        break;
      }
      case RollPreview:
      {
        x=(long) ((i+1)*thumbnail->columns)/NumberTiles;
        y=(long) ((i+1)*thumbnail->rows)/NumberTiles;
        preview_image=RollImage(thumbnail,x,y,exception);
        (void) FormatMagickString(label,MaxTextExtent,"roll %ldx%ld",x,y);
        break;
      }
      case HuePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatMagickString(factor,MaxTextExtent,"100,100,%g",
          2.0*percentage);
        (void) ModulateImage(preview_image,factor);
        (void) FormatMagickString(label,MaxTextExtent,"modulate %s",factor);
        break;
      }
      case SaturationPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatMagickString(factor,MaxTextExtent,"100,%g",2.0*percentage);
        (void) ModulateImage(preview_image,factor);
        (void) FormatMagickString(label,MaxTextExtent,"modulate %s",factor);
        break;
      }
      case BrightnessPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatMagickString(factor,MaxTextExtent,"%g",2.0*percentage);
        (void) ModulateImage(preview_image,factor);
        (void) FormatMagickString(label,MaxTextExtent,"modulate %s",factor);
        break;
      }
      case GammaPreview:
      default:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        gamma+=0.4f;
        (void) GammaImageChannel(preview_image,(ChannelType)
          ((long) AllChannels &~ (long) OpacityChannel),gamma);
        (void) FormatMagickString(label,MaxTextExtent,"gamma %g",gamma);
        break;
      }
      case SpiffPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image != (Image *) NULL)
          for (x=0; x < i; x++)
            (void) ContrastImage(preview_image,MagickTrue);
        (void) FormatMagickString(label,MaxTextExtent,"contrast (%ld)",i+1);
        break;
      }
      case DullPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        for (x=0; x < i; x++)
          (void) ContrastImage(preview_image,MagickFalse);
        (void) FormatMagickString(label,MaxTextExtent,"+contrast (%ld)",i+1);
        break;
      }
      case GrayscalePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        colors<<=1;
        quantize_info.number_colors=colors;
        quantize_info.colorspace=GRAYColorspace;
        (void) QuantizeImage(&quantize_info,preview_image);
        (void) FormatMagickString(label,MaxTextExtent,
          "-colorspace gray -colors %ld",colors);
        break;
      }
      case QuantizePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        colors<<=1;
        quantize_info.number_colors=colors;
        (void) QuantizeImage(&quantize_info,preview_image);
        (void) FormatMagickString(label,MaxTextExtent,"colors %ld",colors);
        break;
      }
      case DespecklePreview:
      {
        for (x=0; x < (i-1); x++)
        {
          preview_image=DespeckleImage(thumbnail,exception);
          if (preview_image == (Image *) NULL)
            break;
          thumbnail=DestroyImage(thumbnail);
          thumbnail=preview_image;
        }
        preview_image=DespeckleImage(thumbnail,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) FormatMagickString(label,MaxTextExtent,"despeckle (%ld)",i+1);
        break;
      }
      case ReduceNoisePreview:
      {
        preview_image=ReduceNoiseImage(thumbnail,radius,exception);
        (void) FormatMagickString(label,MaxTextExtent,"noise %g",radius);
        break;
      }
      case AddNoisePreview:
      {
        switch ((int) i)
        {
          case 0: (void) strcpy(factor,"uniform"); break;
          case 1: (void) strcpy(factor,"gaussian"); break;
          case 2: (void) strcpy(factor,"multiplicative"); break;
          case 3: (void) strcpy(factor,"impulse"); break;
          case 4: (void) strcpy(factor,"laplacian"); break;
          case 5: (void) strcpy(factor,"Poisson"); break;
          default: (void) strcpy(thumbnail->magick,"NULL"); break;
        }
        preview_image=ReduceNoiseImage(thumbnail,(double) i,exception);
        (void) FormatMagickString(label,MaxTextExtent,"+noise %s",factor);
        break;
      }
      case SharpenPreview:
      {
        preview_image=SharpenImage(thumbnail,radius,sigma,exception);
        (void) FormatMagickString(label,MaxTextExtent,"sharpen %gx%g",radius,
          sigma);
        break;
      }
      case BlurPreview:
      {
        preview_image=BlurImage(thumbnail,radius,sigma,exception);
        (void) FormatMagickString(label,MaxTextExtent,"blur %gx%g",radius,
          sigma);
        break;
      }
      case ThresholdPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) BilevelImage(thumbnail,
          (double) (percentage*((MagickRealType) QuantumRange+1.0))/100.0);
        (void) FormatMagickString(label,MaxTextExtent,"threshold %g",
          (double) (percentage*((MagickRealType) QuantumRange+1.0))/100.0);
        break;
      }
      case EdgeDetectPreview:
      {
        preview_image=EdgeImage(thumbnail,radius,exception);
        (void) FormatMagickString(label,MaxTextExtent,"edge %g",radius);
        break;
      }
      case SpreadPreview:
      {
        preview_image=SpreadImage(thumbnail,radius,exception);
        (void) FormatMagickString(label,MaxTextExtent,"spread %g",radius+0.5);
        break;
      }
      case SolarizePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        (void) SolarizeImage(preview_image,QuantumRange*percentage/100.0);
        (void) FormatMagickString(label,MaxTextExtent,"solarize %g",
          (QuantumRange*percentage)/100.0);
        break;
      }
      case ShadePreview:
      {
        degrees+=10.0;
        preview_image=ShadeImage(thumbnail,MagickTrue,degrees,degrees,
          exception);
        (void) FormatMagickString(label,MaxTextExtent,"shade %gx%g",degrees,
          degrees);
        break;
      }
      case RaisePreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        geometry.width=(unsigned long) (2*i+2);
        geometry.height=(unsigned long) (2*i+2);
        geometry.x=i/2;
        geometry.y=i/2;
        (void) RaiseImage(preview_image,&geometry,MagickTrue);
        (void) FormatMagickString(label,MaxTextExtent,"raise %lux%lu%+ld%+ld",
          geometry.width,geometry.height,geometry.x,geometry.y);
        break;
      }
      case SegmentPreview:
      {
        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        threshold+=0.4f;
        (void) SegmentImage(preview_image,RGBColorspace,MagickFalse,threshold,
          threshold);
        (void) FormatMagickString(label,MaxTextExtent,"segment %gx%g",
          threshold,threshold);
        break;
      }
      case SwirlPreview:
      {
        preview_image=SwirlImage(thumbnail,degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"swirl %g",degrees);
        degrees+=45.0;
        break;
      }
      case ImplodePreview:
      {
        degrees+=0.1f;
        preview_image=ImplodeImage(thumbnail,degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"implode %g",degrees);
        break;
      }
      case WavePreview:
      {
        degrees+=5.0f;
        preview_image=WaveImage(thumbnail,0.5*degrees,2.0*degrees,exception);
        (void) FormatMagickString(label,MaxTextExtent,"wave %gx%g",0.5*degrees,
          2.0*degrees);
        break;
      }
      case OilPaintPreview:
      {
        preview_image=OilPaintImage(thumbnail,(double) radius,exception);
        (void) FormatMagickString(label,MaxTextExtent,"paint %g",radius);
        break;
      }
      case CharcoalDrawingPreview:
      {
        preview_image=CharcoalImage(thumbnail,(double) radius,(double) sigma,
          exception);
        (void) FormatMagickString(label,MaxTextExtent,"charcoal %gx%g",radius,
          sigma);
        break;
      }
      case JPEGPreview:
      {
        char
          filename[MaxTextExtent];

        int
          file;

        MagickBooleanType
          status;

        preview_image=CloneImage(thumbnail,0,0,MagickTrue,exception);
        if (preview_image == (Image *) NULL)
          break;
        preview_info->quality=(unsigned long) percentage;
        (void) FormatMagickString(factor,MaxTextExtent,"%lu",
          preview_info->quality);
        file=AcquireUniqueFileResource(filename);
        if (file != -1)
          file=close(file)-1;
        (void) FormatMagickString(preview_image->filename,MaxTextExtent,
          "jpeg:%s",filename);
        status=WriteImage(preview_info,preview_image);
        if (status != MagickFalse)
          {
            Image
              *quality_image;

            (void) CopyMagickString(preview_info->filename,
              preview_image->filename,MaxTextExtent);
            quality_image=ReadImage(preview_info,exception);
            if (quality_image != (Image *) NULL)
              {
                preview_image=DestroyImage(preview_image);
                preview_image=quality_image;
              }
          }
        (void) RelinquishUniqueFileResource(preview_image->filename);
        if ((GetBlobSize(preview_image)/1024) >= 1024)
          (void) FormatMagickString(label,MaxTextExtent,"quality %s\n%gmb ",
            factor,(double) ((MagickOffsetType) GetBlobSize(preview_image))/
            1024.0/1024.0);
        else
          if (GetBlobSize(preview_image) >= 1024)
            (void) FormatMagickString(label,MaxTextExtent,"quality %s\n%gkb ",
              factor,(double) ((MagickOffsetType) GetBlobSize(preview_image))/
              1024.0);
          else
            (void) FormatMagickString(label,MaxTextExtent,"quality %s\n%lub ",
              factor,(unsigned long) GetBlobSize(thumbnail));
        break;
      }
    }
    thumbnail=DestroyImage(thumbnail);
    percentage+=12.5;
    radius+=0.5;
    sigma+=0.25;
    if (preview_image == (Image *) NULL)
      break;
    (void) DeleteImageAttribute(preview_image,"Label");
    (void) SetImageAttribute(preview_image,"Label",label);
    AppendImageToList(&images,preview_image);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(i,NumberTiles) != MagickFalse))
      {
        status=image->progress_monitor(PreviewImageTag,i,NumberTiles,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  if (images == (Image *) NULL)
    {
      preview_info=DestroyImageInfo(preview_info);
      return((Image *) NULL);
    }
  /*
    Create the montage.
  */
  montage_info=CloneMontageInfo(preview_info,(MontageInfo *) NULL);
  (void) CopyMagickString(montage_info->filename,image->filename,MaxTextExtent);
  montage_info->shadow=MagickTrue;
  (void) CloneString(&montage_info->tile,"3x3");
  (void) CloneString(&montage_info->geometry,DefaultPreviewGeometry);
  (void) CloneString(&montage_info->frame,DefaultTileFrame);
  montage_image=MontageImages(images,montage_info,exception);
  montage_info=DestroyMontageInfo(montage_info);
  images=DestroyImageList(images);
  if (montage_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  if (montage_image->montage != (char *) NULL)
    {
      /*
        Free image directory.
      */
      montage_image->montage=(char *)
        RelinquishMagickMemory(montage_image->montage);
      if (image->directory != (char *) NULL)
        montage_image->directory=(char *)
          RelinquishMagickMemory(montage_image->directory);
    }
  preview_info=DestroyImageInfo(preview_info);
  return(montage_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     R a d i a l B l u r I m a g e C h a n n e l                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RadialBlurImageChannel() applies a radial blur to the image.
%
%  Andrew Protano contributed this effect.
%
%  The format of the RadialBlurImage method is:
%
%    Image *RadialBlurImage(const Image *image,const ChannelType channel,
%      const double angle,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o angle: The angle of the radial blur.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport Image *RadialBlurImage(const Image *image,const double angle,
  ExceptionInfo *exception)
{
  Image
    *blur_image;

  blur_image=RadialBlurImageChannel(image,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),angle,exception);
  return(blur_image);
}

MagickExport Image *RadialBlurImageChannel(const Image *image,
  const ChannelType channel,const double angle,ExceptionInfo *exception)
{
  Image
    *blur_image;

  long
    y;

  PointInfo
    blur_center,
    center;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    alpha,
    blur_radius,
    *cos_theta,
    gamma,
    normalize,
    offset,
    radius,
    *sin_theta,
    theta;

  register const PixelPacket
    *p;

  register IndexPacket
    *blur_indexes,
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    n,
    step;

  /*
    Allocate blur image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  blur_image=CloneImage(image,0,0,MagickTrue,exception);
  if (blur_image == (Image *) NULL)
    return((Image *) NULL);
  blur_image->storage_class=DirectClass;
  blur_center.x=(double) image->columns/2.0;
  blur_center.y=(double) image->rows/2.0;
  blur_radius=sqrt(blur_center.x*blur_center.x+blur_center.y*blur_center.y);
  n=(unsigned long)
    AbsoluteValue(4*DegreesToRadians(angle)*sqrt((double) blur_radius)+2);
  theta=DegreesToRadians(angle)/(MagickRealType) (n-1);
  cos_theta=(MagickRealType *)
    AcquireMagickMemory((size_t) n*sizeof(*cos_theta));
  sin_theta=(MagickRealType *)
    AcquireMagickMemory((size_t) n*sizeof(*sin_theta));
  if ((cos_theta == (MagickRealType *) NULL) ||
      (sin_theta == (MagickRealType *) NULL))
    {
      blur_image=DestroyImage(blur_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  offset=theta*(MagickRealType) (n-1)/2.0;
  for (i=0; i < (long) n; i++)
  {
    cos_theta[i]=cos((double) (theta*i-offset));
    sin_theta[i]=sin((double) (theta*i-offset));
  }
  /*
    Radial blur image.
  */
  for (y=0; y < (long) blur_image->rows; y++)
  {
    q=GetImagePixels(blur_image,0,y,blur_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    blur_indexes=GetIndexes(blur_image);
    for (x=0; x < (long) blur_image->columns; x++)
    {
      center.x=(double) x-blur_center.x;
      center.y=(double) y-blur_center.y;
      radius=sqrt(center.x*center.x+center.y*center.y);
      if (radius == 0)
        step=1;
      else
        {
          step=(unsigned long) (blur_radius/radius);
          if (step == 0)
            step=1;
          else
            if (step >= n)
              step=n-1;
        }
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      normalize=0.0;
      for (i=0; i < (long) n; i+=step)
      {
        p=AcquireImagePixels(image,(long) (blur_center.x+center.x*cos_theta[i]-
          center.y*sin_theta[i]+0.5),(long) (blur_center.y+center.x*
          sin_theta[i]+center.y*cos_theta[i]+0.5),1,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        alpha=1.0;
        if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
          {
            alpha=((MagickRealType) QuantumRange-p->opacity)/QuantumRange;
            pixel.opacity+=p->opacity;
          }
        if ((channel & RedChannel) != 0)
          pixel.red+=alpha*p->red;
        if ((channel & GreenChannel) != 0)
          pixel.green+=alpha*p->green;
        if ((channel & BlueChannel) != 0)
          pixel.blue+=alpha*p->blue;
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          pixel.index+=alpha*(*indexes);
        gamma+=alpha;
        normalize+=1.0;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      normalize=1.0/
        (fabs((double) normalize) <= MagickEpsilon ? 1.0 : normalize);
      if ((channel & RedChannel) != 0)
        q->red=RoundToQuantum(gamma*pixel.red);
      if ((channel & GreenChannel) != 0)
        q->green=RoundToQuantum(gamma*pixel.green);
      if ((channel & BlueChannel) != 0)
        q->blue=RoundToQuantum(gamma*pixel.blue);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=RoundToQuantum(normalize*pixel.opacity);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        blur_indexes[x]=(IndexPacket) RoundToQuantum(gamma*pixel.index);
      q++;
    }
    if (SyncImagePixels(blur_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(BlurImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  cos_theta=(MagickRealType *) RelinquishMagickMemory(cos_theta);
  sin_theta=(MagickRealType *) RelinquishMagickMemory(sin_theta);
  return(blur_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     R a n d o m T h r e s h o l d I m a g e C h a n n e l                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RandomThresholdImageChannel() changes the value of individual pixels based
%  on the intensity of each pixel compared to a random threshold.  The result
%  is a low-contrast, two color image.
%
%  The format of the RandomThresholdImageChannel method is:
%
%      MagickBooleanType RandomThresholdImageChannel(Image *image,
%        const ChannelType channel,const char *thresholds,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel or channels to be thresholded.
%
%    o thresholds: a geometry string containing low,high thresholds.  If the
%      string contains 2x2, 3x3, or 4x4, an ordered dither of order 2, 3, or 4
%      is performed instead.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

MagickExport MagickBooleanType RandomThresholdImage(Image *image,
  const char *thresholds,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=RandomThresholdImageChannel(image,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),thresholds,exception);
  return(status);
}

MagickExport MagickBooleanType RandomThresholdImageChannel(Image *image,
  const ChannelType channel,const char *thresholds,ExceptionInfo *exception)
{
  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    threshold;

  MagickRealType
    min_threshold,
    max_threshold;

  register long
    x;

  register IndexPacket
    *indexes;

  register PixelPacket
    *q;

  static MagickRealType
    o2[4] = { 0.2f, 0.6f, 0.8f, 0.4f },
    o3[9] = { 0.1f, 0.6f, 0.3f, 0.7f, 0.5f, 0.8f, 0.4f, 0.9f, 0.2f },
    o4[16] = { 0.1f, 0.7f, 1.1f, 0.3f, 1.0f, 0.5f, 1.5f, 0.8f,
               1.4f, 1.6f, 0.6f, 1.2f, 0.4f, 0.9f, 1.3f, 0.2f };

  unsigned long
    order;

  /*
    Threshold image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if (thresholds == (const char *) NULL)
    return(MagickTrue);
  GetMagickPixelPacket(image,&threshold);
  min_threshold=0.0;
  max_threshold=(MagickRealType) QuantumRange;
  order=1;
  if (LocaleCompare(thresholds,"2x2") == 0)
    order=2;
  else
    if (LocaleCompare(thresholds,"3x3") == 0)
      order=3;
    else
      if (LocaleCompare(thresholds,"4x4") == 0)
        order=4;
      else
        {
          GeometryInfo
            geometry_info;

          MagickStatusType
            flags;

          flags=ParseGeometry(thresholds,&geometry_info);
          min_threshold=geometry_info.rho;
          max_threshold=geometry_info.sigma;
          if ((flags & SigmaValue) == 0)
            max_threshold=min_threshold;
          if (strchr(thresholds,'%') != (char *) NULL)
            {
              max_threshold*=(0.01f*QuantumRange);
              min_threshold*=(0.01f*QuantumRange);
            }
        }
  if (channel == AllChannels)
    {
      IndexPacket
        index;

      MagickRealType
        intensity;

      if (AllocateImageColormap(image,2) == MagickFalse)
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
          intensity=(MagickRealType) PixelIntensityToQuantum(q);
          switch (order)
          {
            case 1:
            {
              if (intensity < min_threshold)
                threshold.index=min_threshold;
              else
                if (intensity > max_threshold)
                  threshold.index=max_threshold;
                else
                  threshold.index=(MagickRealType)
                    (QuantumRange*GetRandomValue());
              break;
            }
            case 2:
            {
              threshold.index=(MagickRealType)
                QuantumRange*o2[(x % 2)+2*(y % 2)];
              break;
            }
            case 3:
            {
              threshold.index=(MagickRealType)
                QuantumRange*o3[(x % 3)+3*(y % 3)];
              break;
            }
            case 4:
            {
              threshold.index=(MagickRealType)
                QuantumRange*o4[(x % 4)+4*(y % 4)]/1.7;
              break;
            }
          }
          index=(IndexPacket)
            ((MagickRealType) intensity < threshold.index ? 0 : 1);
          indexes[x]=index;
          *q++=image->colormap[index];
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(ThresholdImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      return(MagickTrue);
    }
  image->storage_class=DirectClass;
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      switch (order)
      {
        case 1:
        {
          if ((channel & RedChannel) != 0)
            {
              if ((MagickRealType) q->red < min_threshold)
                threshold.red=min_threshold;
              else
                if ((MagickRealType) q->red > max_threshold)
                  threshold.red=max_threshold;
                else
                  threshold.red=(MagickRealType) (QuantumRange*GetRandomValue());
            }
          if ((channel & GreenChannel) != 0)
            {
              if ((MagickRealType) q->green < min_threshold)
                threshold.green=min_threshold;
              else
                if ((MagickRealType) q->green > max_threshold)
                  threshold.green=max_threshold;
                else
                  threshold.green=(MagickRealType) (QuantumRange*GetRandomValue());
            }
          if ((channel & BlueChannel) != 0)
            {
              if ((MagickRealType) q->blue < min_threshold)
                threshold.blue=min_threshold;
              else
                if ((MagickRealType) q->blue > max_threshold)
                  threshold.blue=max_threshold;
                else
                  threshold.blue=(MagickRealType) (QuantumRange*GetRandomValue());
            }
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse))
            {
              if ((MagickRealType) q->opacity < min_threshold)
                threshold.opacity=min_threshold;
              else
                if ((MagickRealType) q->opacity > max_threshold)
                  threshold.opacity=max_threshold;
                else
                  threshold.opacity=(MagickRealType) (QuantumRange*GetRandomValue());
            }
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            {
              if ((MagickRealType) indexes[x] < min_threshold)
                threshold.index=min_threshold;
              else
                if ((MagickRealType) indexes[x] > max_threshold)
                  threshold.index=max_threshold;
                else
                  threshold.index=(MagickRealType) (QuantumRange*GetRandomValue());
            }
          break;
        }
        case 2:
        {
          threshold.red=(MagickRealType) QuantumRange*o2[(x % 2)+2*(y % 2)];
          threshold.green=threshold.red;
          threshold.blue=threshold.red;
          threshold.opacity=threshold.red;
          threshold.index=threshold.red;
          break;
        }
        case 3:
        {
          threshold.red=(MagickRealType) QuantumRange*o3[(x % 3)+3*(y % 3)];
          threshold.green=threshold.red;
          threshold.blue=threshold.red;
          threshold.opacity=threshold.red;
          threshold.index=threshold.red;
          break;
        }
        case 4:
        {
          threshold.red=(MagickRealType) QuantumRange*o4[(x % 4)+4*(y % 4)]/1.7;
          threshold.green=threshold.red;
          threshold.blue=threshold.red;
          threshold.opacity=threshold.red;
          threshold.index=threshold.red;
          break;
        }
      }
      if ((channel & RedChannel) != 0)
        q->red=(Quantum)
          ((MagickRealType) q->red <= threshold.red ? 0 : QuantumRange);
      if ((channel & GreenChannel) != 0)
        q->green=(Quantum)
          ((MagickRealType) q->green <= threshold.green ? 0 : QuantumRange);
      if ((channel & BlueChannel) != 0)
        q->blue=(Quantum)
          ((MagickRealType) q->blue <= threshold.blue ? 0 : QuantumRange);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=(Quantum)
          ((MagickRealType) q->opacity <= threshold.opacity ? 0 : QuantumRange);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        indexes[x]=(IndexPacket)
          ((MagickRealType) indexes[x] <= threshold.index ? 0 : QuantumRange);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ThresholdImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     R e d u c e N o i s e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReduceNoiseImage() smooths the contours of an image while still preserving
%  edge information.  The algorithm works by replacing each pixel with its
%  neighbor closest in value.  A neighbor is defined by radius.  Use a radius
%  of 0 and ReduceNoise() selects a suitable radius for you.
%
%  The format of the ReduceNoiseImage method is:
%
%      Image *ReduceNoiseImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the pixel neighborhood.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static MagickPixelPacket GetNonpeakMedianList(MedianPixelList *pixel_list)
{
  MagickPixelPacket
    pixel;

  register MedianSkipList
    *list;

  register long
    channel;

  unsigned long
    channels[5],
    center,
    color,
    count,
    previous,
    next;

  /*
    Finds the median value for each of the color.
  */
  center=pixel_list->center;
  for (channel=0; channel < 5; channel++)
  {
    list=pixel_list->lists+channel;
    color=65536UL;
    next=list->nodes[color].next[0];
    count=0;
    do
    {
      previous=color;
      color=next;
      next=list->nodes[color].next[0];
      count+=list->nodes[color].count;
    }
    while (count <= center);
    if ((previous == 65536UL) && (next != 65536UL))
      color=next;
    else
      if ((previous != 65536UL) && (next == 65536UL))
        color=previous;
    channels[channel]=color;
  }
  GetMagickPixelPacket((const Image *) NULL,&pixel);
  pixel.red=(MagickRealType) ScaleShortToQuantum(channels[0]);
  pixel.green=(MagickRealType) ScaleShortToQuantum(channels[1]);
  pixel.blue=(MagickRealType) ScaleShortToQuantum(channels[2]);
  pixel.opacity=(MagickRealType) ScaleShortToQuantum(channels[3]);
  pixel.index=(MagickRealType) ScaleShortToQuantum(channels[4]);
  return(pixel);
}

MagickExport Image *ReduceNoiseImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
#define ReduceNoiseImageTag  "ReduceNoise/Image"

  Image
    *noise_image;

  long
    x,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MedianPixelList
    *skiplist;

  register const PixelPacket
    *p,
    *r;

  register IndexPacket
    *indexes,
    *noise_indexes,
    *s;

  register long
    u,
    v;

  register PixelPacket
    *q;

  unsigned long
    width;

  /*
    Initialize noised image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,0.5);
  if ((image->columns < width) || (image->rows < width))
    ThrowImageException(OptionError,"ImageSmallerThanKernelRadius");
  noise_image=CloneImage(image,0,0,MagickTrue,exception);
  if (noise_image == (Image *) NULL)
    return((Image *) NULL);
  noise_image->storage_class=DirectClass;
  /*
    Allocate skip-lists.
  */
  skiplist=(MedianPixelList *) AcquireMagickMemory(sizeof(*skiplist));
  if (skiplist == (MedianPixelList *) NULL)
    {
      noise_image=DestroyImage(noise_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Median filter each image row.
  */
  InitializeMedianList(skiplist,width);
  for (y=0; y < (long) noise_image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    q=GetImagePixels(noise_image,0,y,noise_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    noise_indexes=GetIndexes(noise_image);
    for (x=0; x < (long) noise_image->columns; x++)
    {
      r=p;
      s=indexes+x;
      ResetMedianList(skiplist);
      for (v=0; v < (long) width; v++)
      {
        for (u=0; u < (long) width; u++)
          InsertMedianList(image,r+u,s+u,skiplist);
        r+=image->columns+width;
        s+=image->columns+width;
      }
      pixel=GetNonpeakMedianList(skiplist);
      SetPixelPacket(&pixel,q,noise_indexes+x);
      p++;
      q++;
    }
    if (SyncImagePixels(noise_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ReduceNoiseImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  skiplist=(MedianPixelList *) RelinquishMagickMemory(skiplist);
  return(noise_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S h a d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShadeImage() shines a distant light on an image to create a
%  three-dimensional effect. You control the positioning of the light with
%  azimuth and elevation; azimuth is measured in degrees off the x axis
%  and elevation is measured in pixels above the Z axis.
%
%  The format of the ShadeImage method is:
%
%      Image *ShadeImage(const Image *image,const MagickBooleanType gray,
%        const double azimuth,const double elevation,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o gray: A value other than zero shades the intensity of each pixel.
%
%    o azimuth, elevation:  Define the light source direction.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ShadeImage(const Image *image,const MagickBooleanType gray,
  const double azimuth,const double elevation,ExceptionInfo *exception)
{
#define ShadeImageTag  "Shade/Image"

  Image
    *shade_image;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    distance,
    normal_distance,
    shade;

  PrimaryInfo
    light,
    normal;

  register const PixelPacket
    *p,
    *s0,
    *s1,
    *s2;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize shaded image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  shade_image=CloneImage(image,0,0,MagickTrue,exception);
  if (shade_image == (Image *) NULL)
    return((Image *) NULL);
  shade_image->storage_class=DirectClass;
  /*
    Compute the light vector.
  */
  light.x=(double) QuantumRange*cos(DegreesToRadians(azimuth))*
    cos(DegreesToRadians(elevation));
  light.y=(double) QuantumRange*sin(DegreesToRadians(azimuth))*
    cos(DegreesToRadians(elevation));
  light.z=(double) QuantumRange*sin(DegreesToRadians(elevation));
  normal.z=2.0*QuantumRange;  /* constant Z of surface normal */
  /*
    Shade image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,-1,y-1,image->columns+2,3,exception);
    q=GetImagePixels(shade_image,0,y,shade_image->columns,1);
    if ((p == (PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    /*
      Shade this row of pixels.
    */
    s0=p+1;
    s1=s0+image->columns+2;
    s2=s1+image->columns+2;
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Determine the surface normal and compute shading.
      */
      normal.x=(double) (PixelIntensity(s0-1)+PixelIntensity(s1-1)+
        PixelIntensity(s2-1)-PixelIntensity(s0+1)-PixelIntensity(s1+1)-
        PixelIntensity(s2+1));
      normal.y=(double) (PixelIntensity(s2-1)+PixelIntensity(s2)+
        PixelIntensity(s2+1)-PixelIntensity(s0-1)-PixelIntensity(s0)-
        PixelIntensity(s0+1));
      if ((normal.x == 0.0) && (normal.y == 0.0))
        shade=light.z;
      else
        {
          shade=0.0;
          distance=normal.x*light.x+normal.y*light.y+normal.z*light.z;
          if (distance > MagickEpsilon)
            {
              normal_distance=
                normal.x*normal.x+normal.y*normal.y+normal.z*normal.z;
              if (normal_distance > (MagickEpsilon*MagickEpsilon))
                shade=distance/sqrt((double) normal_distance);
            }
        }
      if (gray != MagickFalse)
        {
          q->red=(Quantum) shade;
          q->green=(Quantum) shade;
          q->blue=(Quantum) shade;
        }
      else
        {
          q->red=RoundToQuantum(QuantumScale*shade*s1->red);
          q->green=RoundToQuantum(QuantumScale*shade*s1->green);
          q->blue=RoundToQuantum(QuantumScale*shade*s1->blue);
        }
      q->opacity=s1->opacity;
      s0++;
      s1++;
      s2++;
      q++;
    }
    if (SyncImagePixels(shade_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ShadeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(shade_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S h a r p e n I m a g e C h a n n e l                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SharpenImageChannel() sharpens one or more image channels.  We convolve the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and SharpenImage() selects a suitable radius for you.
%
%  Using a separable kernel would be faster, but the negative weights cancel
%  out on the corners of the kernel producing often undesirable ringing inthe
%  filtered result; this can be avoided by using a 2D gaussian shaped image
%  sharpening kernel instead.
%
%  The format of the SharpenImage method is:
%
%    Image *SharpenImageChannel(const Image *image,const ChannelType channel,
%      const double radius,const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Laplacian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport Image *SharpenImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *sharp_image;

  sharp_image=SharpenImageChannel(image,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),radius,sigma,exception);
  return(sharp_image);
}

MagickExport Image *SharpenImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  ExceptionInfo *exception)
{
  double
    *kernel;

  Image
    *sharp_image;

  MagickRealType
    alpha,
    normalize;

  register long
    i,
    u,
    v;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if (sigma == 0.0)
    ThrowImageException(OptionError,"ZeroSigmaNotPermitted");
  width=GetOptimalKernelWidth2D(radius,sigma);
  kernel=(double *) AcquireMagickMemory((size_t) width*width*sizeof(*kernel));
  if (kernel == (double *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  i=0;
  normalize=0.0;
  for (v=(-((long) width/2)); v <= (long) (width/2); v++)
  {
    for (u=(-((long) width/2)); u <= (long) (width/2); u++)
    {
      alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
      kernel[i]=(double) (alpha/(2.0*MagickPI*sigma*sigma));
      normalize+=kernel[i];
      i++;
    }
  }
  kernel[i/2]=(double) ((-2.0)*normalize);
  sharp_image=ConvolveImageChannel(image,channel,width,kernel,exception);
  kernel=(double *) RelinquishMagickMemory(kernel);
  return(sharp_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S p r e a d I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SpreadImage() is a special effects method that randomly displaces each
%  pixel in a block defined by the radius parameter.
%
%  The format of the SpreadImage method is:
%
%      Image *SpreadImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius:  Choose a random pixel in a neighborhood of this extent.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *SpreadImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
#define SpreadImageTag  "Spread/Image"

  Image
    *spread_image;

  long
    x_distance,
    y,
    y_distance;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  unsigned long
    width;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((image->columns < 3) || (image->rows < 3))
    return((Image *) NULL);
  /*
    Initialize spread image attributes.
  */
  spread_image=CloneImage(image,0,0,MagickTrue,exception);
  if (spread_image == (Image *) NULL)
    return((Image *) NULL);
  spread_image->storage_class=DirectClass;
  /*
    Convolve each row.
  */
  width=2*((unsigned long) radius)+1;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    q=GetImagePixels(spread_image,0,y,spread_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      x_distance=(long) ((MagickRealType) width*GetRandomValue());
      y_distance=(long) ((MagickRealType) width*GetRandomValue());
      *q++=(*(p+(image->columns+width)*y_distance+x+x_distance));
    }
    if (SyncImagePixels(spread_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SpreadImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(spread_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     U n s h a r p M a s k I m a g e C h a n n e l                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnsharpMaskImage() sharpens one or more image channels.  We convolve the
%  image with a Gaussian operator of the given radius and standard deviation
%  (sigma).  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and UnsharpMaskImage() selects a suitable radius for you.
%
%  The format of the UnsharpMaskImage method is:
%
%    Image *UnsharpMaskImageChannel(const Image *image,
%      const ChannelType channel,const double radius,const double sigma,
%      const double amount,const double threshold,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o amount: The percentage of the difference between the original and the
%      blur image that is added back into the original.
%
%    o threshold: The threshold in pixels needed to apply the diffence amount.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport Image *UnsharpMaskImage(const Image *image,const double radius,
  const double sigma,const double amount,const double threshold,
  ExceptionInfo *exception)
{
  Image
    *sharp_image;

  sharp_image=UnsharpMaskImageChannel(image,(ChannelType) ((long)
    AllChannels &~ (long) OpacityChannel),radius,sigma,amount,threshold,
    exception);
  return(sharp_image);
}

MagickExport Image *UnsharpMaskImageChannel(const Image *image,
  const ChannelType channel,const double radius,const double sigma,
  const double amount,const double threshold,ExceptionInfo *exception)
{
#define SharpenImageTag  "Sharpen/Image"

  Image
    *sharp_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *sharp_indexes;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  if (sigma == 0.0)
    ThrowImageException(OptionError,"ZeroSigmaNotPermitted");
  sharp_image=BlurImageChannel(image,channel,radius,sigma,exception);
  if (sharp_image == (Image *) NULL)
    return((Image *) NULL);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=GetImagePixels(sharp_image,0,y,sharp_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    sharp_indexes=GetIndexes(sharp_image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        {
          pixel.red=p->red-(MagickRealType) q->red;
          if (AbsoluteValue(2.0*pixel.red) < (QuantumRange*threshold))
            pixel.red=(MagickRealType) p->red;
          else
            pixel.red=p->red+(pixel.red*amount);
          q->red=RoundToQuantum(pixel.red);
        }
      if ((channel & GreenChannel) != 0)
        {
          pixel.green=p->green-(MagickRealType) q->green;
          if (AbsoluteValue(2.0*pixel.green) < (QuantumRange*threshold))
            pixel.green=(MagickRealType) p->green;
          else
            pixel.green=p->green+(pixel.green*amount);
          q->green=RoundToQuantum(pixel.green);
        }
      if ((channel & BlueChannel) != 0)
        {
          pixel.blue=p->blue-(MagickRealType) q->blue;
          if (AbsoluteValue(2.0*pixel.blue) < (QuantumRange*threshold))
            pixel.blue=(MagickRealType) p->blue;
          else
            pixel.blue=p->blue+(pixel.blue*amount);
          q->blue=RoundToQuantum(pixel.blue);
        }
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        {
          pixel.opacity=p->opacity-(MagickRealType) q->opacity;
          if (AbsoluteValue(2.0*pixel.opacity) < (QuantumRange*threshold))
            pixel.opacity=(MagickRealType) p->opacity;
          else
            pixel.opacity=p->opacity+(pixel.opacity*amount);
          q->opacity=RoundToQuantum(pixel.opacity);
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          pixel.index=sharp_indexes[x]-(MagickRealType) indexes[x];
          if (AbsoluteValue(2.0*pixel.index) < (QuantumRange*threshold))
            pixel.index=(MagickRealType) sharp_indexes[x];
          else
            pixel.index=sharp_indexes[x]+(pixel.index*amount);
          indexes[x]=RoundToQuantum(pixel.index);
        }
      p++;
      q++;
    }
    if (SyncImagePixels(sharp_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SharpenImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(sharp_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     W h i t e T h r e s h o l d I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WhiteThresholdImage() is like ThresholdImage() but forces all pixels above
%  the threshold into white while leaving all pixels below the threshold
%  unchanged.
%
%  The format of the WhiteThresholdImage method is:
%
%      MagickBooleanType WhiteThresholdImage(Image *image,const char *threshold)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o threshold: Define the threshold value
%
%
*/
MagickExport MagickBooleanType WhiteThresholdImage(Image *image,
  const char *threshold)
{
#define ThresholdImageTag  "Threshold/Image"

  GeometryInfo
    geometry_info;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickStatusType
    flags;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Threshold image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (threshold == (const char *) NULL)
    return(MagickTrue);
  image->storage_class=DirectClass;
  flags=ParseGeometry(threshold,&geometry_info);
  pixel.red=geometry_info.rho;
  pixel.green=geometry_info.sigma;
  if ((flags & SigmaValue) == 0)
    pixel.green=pixel.red;
  pixel.blue=geometry_info.xi;
  if ((flags & XiValue) == 0)
    pixel.blue=pixel.red;
  pixel.opacity=geometry_info.psi;
  if ((flags & PsiValue) == 0)
    pixel.opacity=(MagickRealType) OpaqueOpacity;
  if ((flags & PercentValue) != 0)
    {
      pixel.red*=QuantumRange/100.0f;
      pixel.green*=QuantumRange/100.0f;
      pixel.blue*=QuantumRange/100.0f;
      pixel.opacity*=QuantumRange/100.0f;
    }
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    if (IsGray(pixel) != MagickFalse)
      for (x=(long) image->columns-1; x >= 0; x--)
      {
        if ((MagickRealType) PixelIntensityToQuantum(q) > pixel.red)
          {
            q->red=QuantumRange;
            q->green=QuantumRange;
            q->blue=QuantumRange;
          }
        q++;
      }
    else
      for (x=(long) image->columns-1; x >= 0; x--)
      {
        if ((MagickRealType) q->red > pixel.red)
          q->red=QuantumRange;
        if ((MagickRealType) q->green > pixel.green)
          q->green=QuantumRange;
        if ((MagickRealType) q->blue > pixel.blue)
          q->blue=QuantumRange;
        if ((MagickRealType) q->opacity > pixel.opacity)
          q->opacity=QuantumRange;
        q++;
      }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ThresholdImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}
