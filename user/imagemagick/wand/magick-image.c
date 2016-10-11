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
%                     IIIII  M   M   AAA    GGGG  EEEEE                       %
%                       I    MM MM  A   A  G      E                           %
%                       I    M M M  AAAAA  G  GG  EEE                         %
%                       I    M   M  A   A  G   G  E                           %
%                     IIIII  M   M  A   A   GGGG  EEEEE                       %
%                                                                             %
%                                                                             %
%                    ImageMagick MagickWand Image Mthods                      %
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
#define MagickWandId  "MagickWand"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o n e M a g i c k W a n d F r o m I m a g e s                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneMagickWandFromImages() clones the magick wand and inserts a new image
%  list.
%
%  The format of the CloneMagickWandFromImages method is:
%
%      MagickWand *CloneMagickWandFromImages(const MagickWand *wand,
%        Image *images)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o images: replace the image list with these image(s).
%
*/
static MagickWand *CloneMagickWandFromImages(const MagickWand *wand,
  Image *images)
{
  MagickWand
    *clone_wand;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  clone_wand=(MagickWand *) AcquireMagickMemory(sizeof(*clone_wand));
  if (clone_wand == (MagickWand *) NULL)
    ThrowWandFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      images->filename);
  (void) ResetMagickMemory(clone_wand,0,sizeof(*clone_wand));
  clone_wand->id=AcquireWandId();
  (void) FormatMagickString(clone_wand->name,MaxTextExtent,"MagickWand-%lu",
    clone_wand->id);
  GetExceptionInfo(&clone_wand->exception);
  InheritException(&clone_wand->exception,&wand->exception);
  clone_wand->image_info=CloneImageInfo(wand->image_info);
  clone_wand->quantize_info=CloneQuantizeInfo(wand->quantize_info);
  clone_wand->images=images;
  clone_wand->debug=IsEventLogging();
  if (clone_wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",clone_wand->name);
  clone_wand->signature=MagickSignature;
  return(clone_wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e F r o m M a g i c k W a n d                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageFromMagickWand() returns the current image from the magick wand.
%
%  The format of the GetImageFromMagickWand method is:
%
%      Image *GetImageFromMagickWand(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%
*/
WandExport Image *GetImageFromMagickWand(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((Image *) NULL);
    }
  return(wand->images);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k A d a p t i v e T h r e s h o l d I m a g e                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickAdaptiveThresholdImage() selects an individual threshold for each pixel
%  based on the range of intensity values in its local neighborhood.  This
%  allows for thresholding of an image whose global intensity histogram
%  doesn't contain distinctive peaks.
%
%  The format of the AdaptiveThresholdImage method is:
%
%      MagickBooleanType MagickAdaptiveThresholdImage(MagickWand *wand,
%        const unsigned long width,const unsigned long height,const long offset)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The width of the local neighborhood.
%
%    o height: The height of the local neighborhood.
%
%    o offset: The mean offset.
%
*/
WandExport MagickBooleanType MagickAdaptiveThresholdImage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long offset)
{
  Image
    *threshold_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  threshold_image=AdaptiveThresholdImage(wand->images,width,height,offset,
    &wand->exception);
  if (threshold_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,threshold_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k A d d I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickAddImage() adds the specified images at the current image location.
%
%  The format of the MagickAddImage method is:
%
%      MagickBooleanType MagickAddImage(MagickWand *wand,
%        const MagickWand *add_wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o insert: The splice wand.
%
*/
WandExport MagickBooleanType MagickAddImage(MagickWand *wand,
  const MagickWand *add_wand)
{
  Image
    *images;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(add_wand != (MagickWand *) NULL);
  assert(add_wand->signature == MagickSignature);
  if (add_wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",add_wand->name);
  images=CloneImageList(add_wand->images,&wand->exception);
  if (images == (Image *) NULL)
    return(MagickFalse);
  if (wand->images == (Image *) NULL)
    AppendImageToList(&wand->images,images);
  else
    if (GetNextImageInList(wand->images) == (Image *) NULL)
      AppendImageToList(&wand->images,images);
    else
      InsertImageInList(&wand->images,images);
  wand->images=GetFirstImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     M a g i c k A d d N o i s e I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickAddNoiseImage() adds random noise to the image.
%
%  The format of the MagickAddNoiseImage method is:
%
%      MagickBooleanType MagickAddNoiseImage(MagickWand *wand,
%        const NoiseType noise_type)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o noise_type:  The type of noise: Uniform, Gaussian, Multiplicative,
%      Impulse, Laplacian, or Poisson.
%
*/
WandExport MagickBooleanType MagickAddNoiseImage(MagickWand *wand,
  const NoiseType noise_type)
{
  Image
    *noise_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  noise_image=AddNoiseImage(wand->images,noise_type,&wand->exception);
  if (noise_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,noise_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k A f f i n e T r a n s f o r m I m a g e                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickAffineTransformImage() transforms an image as dictated by the affine
%  matrix of the drawing wand.
%
%  The format of the MagickAffineTransformImage method is:
%
%      MagickBooleanType MagickAffineTransformImage(MagickWand *wand,
%        const DrawingWand *drawing_wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o drawing_wand: The draw wand.
%
*/
WandExport MagickBooleanType MagickAffineTransformImage(MagickWand *wand,
  const DrawingWand *drawing_wand)
{
  DrawInfo
    *draw_info;

  Image
    *affine_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  draw_info=PeekDrawingWand(drawing_wand);
  if (draw_info == (DrawInfo *) NULL)
    return(MagickFalse);
  affine_image=AffineTransformImage(wand->images,&draw_info->affine,
    &wand->exception);
  draw_info=DestroyDrawInfo(draw_info);
  if (affine_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,affine_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k A n n o t a t e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickAnnotateImage() annotates an image with text.
%
%  The format of the MagickAnnotateImage method is:
%
%      MagickBooleanType MagickAnnotateImage(MagickWand *wand,
%        const DrawingWand *drawing_wand,const double x,const double y,
%        const double angle,const char *text)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o drawing_wand: The draw wand.
%
%    o x: x ordinate to left of text
%
%    o y: y ordinate to text baseline
%
%    o angle: rotate text relative to this angle.
%
%    o text: text to draw
%
*/
WandExport MagickBooleanType MagickAnnotateImage(MagickWand *wand,
  const DrawingWand *drawing_wand,const double x,const double y,
  const double angle,const char *text)
{
  char
    geometry[MaxTextExtent];

  DrawInfo
    *draw_info;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  draw_info=PeekDrawingWand(drawing_wand);
  if (draw_info == (DrawInfo *) NULL)
    return(MagickFalse);
  (void) CloneString(&draw_info->text,text);
  (void) FormatMagickString(geometry,MaxTextExtent,"%+g%+g",x,y);
  draw_info->affine.sx=cos(DegreesToRadians(fmod(angle,360.0)));
  draw_info->affine.rx=sin(DegreesToRadians(fmod(angle,360.0)));
  draw_info->affine.ry=(-sin(DegreesToRadians(fmod(angle,360.0))));
  draw_info->affine.sy=cos(DegreesToRadians(fmod(angle,360.0)));
  (void) CloneString(&draw_info->geometry,geometry);
  status=AnnotateImage(wand->images,draw_info);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  draw_info=DestroyDrawInfo(draw_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k A n i m a t e I m a g e s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickAnimateImages() animates an image or image sequence.
%
%  The format of the MagickAnimateImages method is:
%
%      MagickBooleanType MagickAnimateImages(MagickWand *wand,
%        const char *server_name)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o server_name: The X server name.
%
%
*/
WandExport MagickBooleanType MagickAnimateImages(MagickWand *wand,
  const char *server_name)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) CloneString(&wand->image_info->server_name,server_name);
  status=AnimateImages(wand->image_info,wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k A p p e n d I m a g e s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickAppendImages() append a set of images.
%
%  The format of the MagickAppendImages method is:
%
%      MagickWand *MagickAppendImages(MagickWand *wand,
%        const MagickBooleanType stack)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o stack: By default, images are stacked left-to-right. Set stack to
%      MagickTrue to stack them top-to-bottom.
%
*/
WandExport MagickWand *MagickAppendImages(MagickWand *wand,
  const MagickBooleanType stack)
{
  Image
    *append_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  append_image=AppendImages(wand->images,stack,&wand->exception);
  if (append_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,append_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k A v e r a g e I m a g e s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickAverageImages() average a set of images.
%
%  The format of the MagickAverageImages method is:
%
%      MagickWand *MagickAverageImages(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickWand *MagickAverageImages(MagickWand *wand)
{
  Image
    *average_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  average_image=AverageImages(wand->images,&wand->exception);
  if (average_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,average_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k B l a c k T h r e s h o l d I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickBlackThresholdImage() is like MagickThresholdImage() but  forces all
%  pixels below the threshold into black while leaving all pixels above the
%  threshold unchanged.
%
%  The format of the MagickBlackThresholdImage method is:
%
%      MagickBooleanType MagickBlackThresholdImage(MagickWand *wand,
%        const PixelWand *threshold)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o threshold: The pixel wand.
%
*/
WandExport MagickBooleanType MagickBlackThresholdImage(MagickWand *wand,
  const PixelWand *threshold)
{
  char
    thresholds[MaxTextExtent];

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  (void) FormatMagickString(thresholds,MaxTextExtent,
    QuantumFormat "," QuantumFormat "," QuantumFormat "," QuantumFormat,
    PixelGetRedQuantum(threshold),PixelGetGreenQuantum(threshold),
    PixelGetBlueQuantum(threshold),PixelGetOpacityQuantum(threshold));
  status=BlackThresholdImage(wand->images,thresholds);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k B l u r I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickBlurImage() blurs an image.  We convolve the image with a
%  gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, the radius should be larger than sigma.  Use a
%  radius of 0 and BlurImage() selects a suitable radius for you.
%
%  The format of the MagickBlurImage method is:
%
%      MagickBooleanType MagickBlurImage(MagickWand *wand,const double radius,
%        const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the , in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the , in pixels.
%
*/
WandExport MagickBooleanType MagickBlurImage(MagickWand *wand,
  const double radius,const double sigma)
{
  Image
    *blur_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  blur_image=BlurImage(wand->images,radius,sigma,&wand->exception);
  if (blur_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,blur_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k B l u r I m a g e C h a n n e l                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickBlurImageChannel() blurs one or more image channels.  We convolve the
%  image channel with a gaussian operator of the given radius and standard
%  deviation (sigma).  For reasonable results, the radius should be larger than
%  sigma.  Use a radius of 0 and GaussinBlurImageChannel() selects a suitable
%  radius for you.
%
%  The format of the MagickBlurImageChannel method is:
%
%      MagickBooleanType MagickBlurImageChannel(MagickWand *wand,
%        const ChannelType channel,const double radius,const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o radius: The radius of the , in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the , in pixels.
%
*/
WandExport MagickBooleanType MagickBlurImageChannel(MagickWand *wand,
  const ChannelType channel,const double radius,const double sigma)
{
  Image
    *blur_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  blur_image=BlurImageChannel(wand->images,channel,radius,sigma,
    &wand->exception);
  if (blur_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,blur_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k B o r d e r I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickBorderImage() surrounds the image with a border of the color defined
%  by the bordercolor pixel wand.
%
%  The format of the MagickBorderImage method is:
%
%      MagickBooleanType MagickBorderImage(MagickWand *wand,
%        const PixelWand *bordercolor,const unsigned long width,
%        const unsigned long height)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o bordercolor: The border color pixel wand.
%
%    o width: The border width.
%
%    o height: The border height.
%
*/
WandExport MagickBooleanType MagickBorderImage(MagickWand *wand,
  const PixelWand *bordercolor,const unsigned long width,
  const unsigned long height)
{
  Image
    *border_image;

  RectangleInfo
    border_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  border_info.width=width;
  border_info.height=height;
  border_info.x=0;
  border_info.y=0;
  PixelGetQuantumColor(bordercolor,&wand->images->border_color);
  border_image=BorderImage(wand->images,&border_info,&wand->exception);
  if (border_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,border_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C h a r c o a l I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCharcoalImage() simulates a charcoal drawing.
%
%  The format of the MagickCharcoalImage method is:
%
%      MagickBooleanType MagickCharcoalImage(MagickWand *wand,
%        const double radius,const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
*/
WandExport MagickBooleanType MagickCharcoalImage(MagickWand *wand,
  const double radius,const double sigma)
{
  Image
    *charcoal_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  charcoal_image=CharcoalImage(wand->images,radius,sigma,&wand->exception);
  if (charcoal_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,charcoal_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C h o p I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickChopImage() removes a region of an image and collapses the image to
%  occupy the removed portion
%
%  The format of the MagickChopImage method is:
%
%      MagickBooleanType MagickChopImage(MagickWand *wand,
%        const unsigned long width,const unsigned long height,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The region width.
%
%    o height: The region height.
%
%    o x: The region x offset.
%
%    o y: The region y offset.
%
%
*/
WandExport MagickBooleanType MagickChopImage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y)
{
  Image
    *chop_image;

  RectangleInfo
    chop;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  chop.width=width;
  chop.height=height;
  chop.x=x;
  chop.y=y;
  chop_image=ChopImage(wand->images,&chop,&wand->exception);
  if (chop_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,chop_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C l i p I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickClipImage() clips along the first path from the 8BIM profile, if
%  present.
%
%  The format of the MagickClipImage method is:
%
%      MagickBooleanType MagickClipImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickClipImage(MagickWand *wand)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=ClipImage(wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C l i p P a t h I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickClipPathImage() clips along the named paths from the 8BIM profile, if
%  present. Later operations take effect inside the path.  Id may be a number
%  if preceded with #, to work on a numbered path, e.g., "#1" to use the first
%  path.
%
%  The format of the MagickClipPathImage method is:
%
%      MagickBooleanType MagickClipPathImage(MagickWand *wand,
%        const char *pathname,const MagickBooleanType inside)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o pathname: name of clipping path resource. If name is preceded by #, use
%      clipping path numbered by name.
%
%    o inside: if non-zero, later operations take effect inside clipping path.
%      Otherwise later operations take effect outside clipping path.
%
*/
WandExport MagickBooleanType MagickClipPathImage(MagickWand *wand,
  const char *pathname,const MagickBooleanType inside)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=ClipPathImage(wand->images,pathname,inside);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o a l e s c e I m a g e s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCoalesceImages() composites a set of images while respecting any page
%  offsets and disposal methods.  GIF, MIFF, and MNG animation sequences
%  typically start with an image background and each subsequent image
%  varies in size and offset.  MagickCoalesceImages() returns a new sequence
%  where each image in the sequence is the same size as the first and
%  composited with the next image in the sequence.
%
%  The format of the MagickCoalesceImages method is:
%
%      MagickWand *MagickCoalesceImages(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickWand *MagickCoalesceImages(MagickWand *wand)
{
  Image
    *coalesce_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  coalesce_image=CoalesceImages(wand->images,&wand->exception);
  if (coalesce_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,coalesce_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o l o r F l o o d f i l l I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickColorFloodfillImage() changes the color value of any pixel that matches
%  target and is an immediate neighbor.  If the method FillToBorderMethod is
%  specified, the color value is changed for any neighbor pixel that does not
%  match the bordercolor member of image.
%
%  The format of the MagickColorFloodfillImage method is:
%
%      MagickBooleanType MagickColorFloodfillImage(MagickWand *wand,
%        const PixelWand *fill,const double fuzz,const PixelWand *bordercolor,
%        const long x,const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o fill: The floodfill color pixel wand.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
%    o bordercolor: The border color pixel wand.
%
%    o x,y: The starting location of the operation.
%
*/
WandExport MagickBooleanType MagickColorFloodfillImage(MagickWand *wand,
  const PixelWand *fill,const double fuzz,const PixelWand *bordercolor,
  const long x,const long y)
{
  DrawInfo
    *draw_info;

  MagickBooleanType
    status;

  PixelPacket
    target;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  draw_info=CloneDrawInfo(wand->image_info,(DrawInfo *) NULL);
  PixelGetQuantumColor(fill,&draw_info->fill);
  target=AcquireOnePixel(wand->images,x % wand->images->columns,
    y % wand->images->rows,&wand->exception);
  if (bordercolor != (PixelWand *) NULL)
    PixelGetQuantumColor(bordercolor,&target);
  wand->images->fuzz=fuzz;
  status=ColorFloodfillImage(wand->images,draw_info,target,x,y,
    bordercolor != (PixelWand *) NULL ? FillToBorderMethod : FloodfillMethod);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  draw_info=DestroyDrawInfo(draw_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o l o r i z e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickColorizeImage() blends the fill color with each pixel in the image.
%
%  The format of the MagickColorizeImage method is:
%
%      MagickBooleanType MagickColorizeImage(MagickWand *wand,
%        const PixelWand *colorize,const PixelWand *opacity)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o colorize: The colorize pixel wand.
%
%    o opacity: The opacity pixel wand.
%
*/
WandExport MagickBooleanType MagickColorizeImage(MagickWand *wand,
  const PixelWand *colorize,const PixelWand *opacity)
{
  char
    percent_opaque[MaxTextExtent];

  Image
    *colorize_image;

  PixelPacket
    target;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  (void) FormatMagickString(percent_opaque,MaxTextExtent,"%g,%g,%g,%g",
    100.0*PixelGetRedQuantum(opacity)/QuantumRange,
    100.0*PixelGetGreenQuantum(opacity)/QuantumRange,
    100.0*PixelGetBlueQuantum(opacity)/QuantumRange,
    100.0*PixelGetOpacityQuantum(opacity)/QuantumRange);
  PixelGetQuantumColor(colorize,&target);
  colorize_image=ColorizeImage(wand->images,percent_opaque,target,
    &wand->exception);
  if (colorize_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,colorize_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o m b i n e I m a g e s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCombineImages() combines one or more images into a single image.  The
%  grayscale value of the pixels of each image in the sequence is assigned in
%  order to the specified  hannels of the combined image.   The typical
%  ordering would be image 1 => Red, 2 => Green, 3 => Blue, etc.

%
%  The format of the MagickCombineImages method is:
%
%      MagickWand *MagickCombineImages(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickWand *MagickCombineImages(MagickWand *wand,
  const ChannelType channel)
{
  Image
    *combine_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  combine_image=CombineImages(wand->images,channel,&wand->exception);
  if (combine_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,combine_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o m m e n t I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCommentImage() adds a comment to your image.
%
%  The format of the MagickCommentImage method is:
%
%      MagickBooleanType MagickCommentImage(MagickWand *wand,
%        const char *comment)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o comment: The image comment.
%
*/
WandExport MagickBooleanType MagickCommentImage(MagickWand *wand,
  const char *comment)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=SetImageAttribute(wand->images,"comment",comment);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o m p a r e I m a g e C h a n n e l s                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCompareImageChannels() compares one or more image channels of an image
%  to a reconstructed image and returns the difference image.
%
%  The format of the MagickCompareImageChannels method is:
%
%      MagickWand *MagickCompareImageChannels(MagickWand *wand,
%        const MagickWand *reference,const ChannelType channel,
%        const MetricType metric,double *distortion)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o reference: The reference wand.
%
%    o channel: The channel.
%
%    o metric: The metric.
%
%    o distortion: The computed distortion between the images.
%
*/
WandExport MagickWand *MagickCompareImageChannels(MagickWand *wand,
  const MagickWand *reference,const ChannelType channel,const MetricType metric,
  double *distortion)
{
  Image
    *compare_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) || (reference->images == (Image *) NULL))
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((MagickWand *) NULL);
    }
  compare_image=CompareImageChannels(wand->images,reference->images,channel,
    metric,distortion,&wand->images->exception);
  if (compare_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,compare_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o m p a r e I m a g e s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCompareImage() compares an image to a reconstructed image and returns
%  the specified difference image.
%
%  The format of the MagickCompareImages method is:
%
%      MagickWand *MagickCompareImages(MagickWand *wand,
%        const MagickWand *reference,const MetricType metric,
%        double *distortion)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o reference: The reference wand.
%
%    o metric: The metric.
%
%    o distortion: The computed distortion between the images.
%
*/
WandExport MagickWand *MagickCompareImages(MagickWand *wand,
  const MagickWand *reference,const MetricType metric,double *distortion)
{
  Image
    *compare_image;


  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) || (reference->images == (Image *) NULL))
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((MagickWand *) NULL);
    }
  compare_image=CompareImages(wand->images,reference->images,metric,distortion,
    &wand->images->exception);
  if (compare_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,compare_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o m p o s i t e I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCompositeImage() composite one image onto another at the specified
%  offset.
%
%  The format of the MagickCompositeImage method is:
%
%      MagickBooleanType MagickCompositeImage(MagickWand *wand,
%        const MagickWand *composite_wand,const CompositeOperator compose,
%        const long x,const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o composite_image: The composite image.
%
%    o compose: This operator affects how the composite is applied to the
%      image.  The default is Over.  Choose from these operators:
%
%        OverCompositeOp       InCompositeOp         OutCompositeOP
%        AtopCompositeOP       XorCompositeOP        PlusCompositeOP
%        MinusCompositeOP      AddCompositeOP        SubtractCompositeOP
%        DifferenceCompositeOP BumpmapCompositeOP    CopyCompositeOP
%        DisplaceCompositeOP
%
%    o x: The column offset of the composited image.
%
%    o y: The row offset of the composited image.
%
%
*/
WandExport MagickBooleanType MagickCompositeImage(MagickWand *wand,
  const MagickWand *composite_wand,const CompositeOperator compose,const long x,
  const long y)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) ||
      (composite_wand->images == (Image *) NULL))
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=CompositeImage(wand->images,compose,composite_wand->images,x,y);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o m p u t e I m a g e C h a n n e l                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickEvaluateImage() applies  an arithmetic, relational, or logical
%  operator to an image. These operations can be used to lighten or darken an
%  image, to increase or decrease contrast in an image, or to produce the
%  "negative" of an image.
%
%  The format of the MagickEvaluateImageChannel method is:
%
%      MagickBooleanType MagickEvaluateImageChannel(MagickWand *wand,
%        const MagickEvaluateOperator op,const double constant)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: The channel.
%
%    o op: A channel operator.
%
%    o constant: A constant value.
%
%
*/
WandExport MagickBooleanType MagickEvaluateImageChannel(MagickWand *wand,
  const ChannelType channel,const MagickEvaluateOperator op,
  const double constant)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=EvaluateImageChannel(wand->images,channel,op,constant,
    &wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o n t r a s t I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickContrastImage() enhances the intensity differences between the lighter
%  and darker elements of the image.  Set sharpen to a value other than 0 to
%  increase the image contrast otherwise the contrast is reduced.
%
%  The format of the MagickContrastImage method is:
%
%      MagickBooleanType MagickContrastImage(MagickWand *wand,
%        const MagickBooleanType sharpen)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o sharpen: Increase or decrease image contrast.
%
%
*/
WandExport MagickBooleanType MagickContrastImage(MagickWand *wand,
  const MagickBooleanType sharpen)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=ContrastImage(wand->images,sharpen);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o n v o l v e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickConvolveImage() applies a custom convolution kernel to the image.
%
%  The format of the MagickConvolveImage method is:
%
%      MagickBooleanType MagickConvolveImage(MagickWand *wand,
%        const unsigned long order,const double *kernel)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o order: The number of columns and rows in the filter kernel.
%
%    o kernel: An array of doubles representing the convolution kernel.
%
*/
WandExport MagickBooleanType MagickConvolveImage(MagickWand *wand,
  const unsigned long order,const double *kernel)
{
  Image
    *convolve_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (kernel == (const double *) NULL)
    return(MagickFalse);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  convolve_image=ConvolveImage(wand->images,order,kernel,&wand->exception);
  if (convolve_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,convolve_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o n v o l v e I m a g e C h a n n e l                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickConvolveImageChannel() applies a custom convolution kernel to one or
%  more image channels.
%
%  The format of the MagickConvolveImage method is:
%
%      MagickBooleanType MagickConvolveImageChannel(MagickWand *wand,
%        const ChannelType channel,const unsigned long order,
%        const double *kernel)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: The channel.
%
%    o order: The number of columns and rows in the filter kernel.
%
%    o kernel: An array of doubles representing the convolution kernel.
%
*/
WandExport MagickBooleanType MagickConvolveImageChannel(MagickWand *wand,
  const ChannelType channel,const unsigned long order,const double *kernel)
{
  Image
    *convolve_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (kernel == (const double *) NULL)
    return(MagickFalse);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  convolve_image=ConvolveImageChannel(wand->images,channel,order,kernel,
    &wand->exception);
  if (convolve_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,convolve_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C r o p I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCropImage() extracts a region of the image.
%
%  The format of the MagickCropImage method is:
%
%      MagickBooleanType MagickCropImage(MagickWand *wand,
%        const unsigned long width,const unsigned long height,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The region width.
%
%    o height: The region height.
%
%    o x: The region x-offset.
%
%    o y: The region y-offset.
%
*/
WandExport MagickBooleanType MagickCropImage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y)
{
  Image
    *crop_image;

  RectangleInfo
    crop;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  crop.width=width;
  crop.height=height;
  crop.x=x;
  crop.y=y;
  crop_image=CropImage(wand->images,&crop,&wand->exception);
  if (crop_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,crop_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C y c l e C o l o r m a p I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickCycleColormapImage() displaces an image's colormap by a given number
%  of positions.  If you cycle the colormap a number of times you can produce
%  a psychodelic effect.
%
%  The format of the MagickCycleColormapImage method is:
%
%      MagickBooleanType MagickCycleColormapImage(MagickWand *wand,
%        const long displace)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o pixel_wand: The pixel wand.
%
*/
WandExport MagickBooleanType MagickCycleColormapImage(MagickWand *wand,
  const long displace)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=CycleColormapImage(wand->images,displace);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k C o n s t i t u t e I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickConstituteImage() adds an image to the wand comprised of the the pixel
%  data you supply.  The pixel data must be in scanline order top-to-bottom.
%  The data can be char, short int, int, float, or double.  Float and double
%  require the pixels to be normalized [0..1], otherwise [0..Max],  where Max
%  is the maximum value the type can accomodate (e.g. 255 for char).  For
%  example, to create a 640x480 image from unsigned red-green-blue character
%  data, use
%
%      MagickConstituteImage(wand,640,640,"RGB",CharPixel,pixels);
%
%  The format of the MagickConstituteImage method is:
%
%      MagickBooleanType MagickConstituteImage(MagickWand *wand,
%        const unsigned long columns,const unsigned long rows,const char *map,
%        const StorageType storage,void *pixels)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns: width in pixels of the image.
%
%    o rows: height in pixels of the image.
%
%    o map:  This string reflects the expected ordering of the pixel array.
%      It can be any combination or order of R = red, G = green, B = blue,
%      A = alpha (0 is transparent), O = opacity (0 is opaque), C = cyan,
%      Y = yellow, M = magenta, K = black, I = intensity (for grayscale),
%      P = pad.
%
%    o storage: Define the data type of the pixels.  Float and double types are
%      expected to be normalized [0..1] otherwise [0..QuantumRange].  Choose from
%      these types: CharPixel, DoublePixel, FloatPixel, IntegerPixel,
%      LongPixel, QuantumPixel, or ShortPixel.
%
%    o pixels: This array of values contain the pixel components as defined by
%      map and type.  You must preallocate this array where the expected
%      length varies depending on the values of width, height, map, and type.
%
%
*/
WandExport MagickBooleanType MagickConstituteImage(MagickWand *wand,
  const unsigned long columns,const unsigned long rows,const char *map,
  const StorageType storage,const void *pixels)
{
  Image
    *images;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  images=ConstituteImage(columns,rows,map,storage,pixels,&wand->exception);
  if (images == (Image *) NULL)
    return(MagickFalse);
  if (wand->images == (Image *) NULL)
    AppendImageToList(&wand->images,images);
  else
    if (GetNextImageInList(wand->images) == (Image *) NULL)
      AppendImageToList(&wand->images,images);
    else
      InsertImageInList(&wand->images,images);
  wand->images=GetFirstImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k D e c o n s t r u c t I m a g e s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickDeconstructImages() compares each image with the next in a sequence
%  and returns the maximum bounding region of any pixel differences it
%  discovers.
%
%  The format of the MagickDeconstructImages method is:
%
%      MagickWand *MagickDeconstructImages(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickWand *MagickDeconstructImages(MagickWand *wand)
{
  Image
    *deconstruct_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  deconstruct_image=DeconstructImages(wand->images,&wand->exception);
  if (deconstruct_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,deconstruct_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     M a g i c k D e s p e c k l e I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickDespeckleImage() reduces the speckle noise in an image while
%  perserving the edges of the original image.
%
%  The format of the MagickDespeckleImage method is:
%
%      MagickBooleanType MagickDespeckleImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickDespeckleImage(MagickWand *wand)
{
  Image
    *despeckle_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  despeckle_image=DespeckleImage(wand->images,&wand->exception);
  if (despeckle_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,despeckle_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k D i s p l a y I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickDisplayImage() displays an image.
%
%  The format of the MagickDisplayImage method is:
%
%      MagickBooleanType MagickDisplayImage(MagickWand *wand,
%        const char *server_name)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o server_name: The X server name.
%
*/
WandExport MagickBooleanType MagickDisplayImage(MagickWand *wand,
  const char *server_name)
{
  Image
    *image;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  image=CloneImage(wand->images,0,0,MagickTrue,&wand->exception);
  if (image == (Image *) NULL)
    return(MagickFalse);
  (void) CloneString(&wand->image_info->server_name,server_name);
  status=DisplayImages(wand->image_info,image);
  if (status == MagickFalse)
    InheritException(&wand->exception,&image->exception);
  image=DestroyImage(image);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k D i s p l a y I m a g e s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickDisplayImages() displays an image or image sequence.
%
%  The format of the MagickDisplayImages method is:
%
%      MagickBooleanType MagickDisplayImages(MagickWand *wand,
%        const char *server_name)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o server_name: The X server name.
%
%
*/
WandExport MagickBooleanType MagickDisplayImages(MagickWand *wand,
  const char *server_name)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) CloneString(&wand->image_info->server_name,server_name);
  status=DisplayImages(wand->image_info,wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k D r a w I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickDrawImage() renders the drawing wand on the current image.
%
%  The format of the MagickDrawImage method is:
%
%      MagickBooleanType MagickDrawImage(MagickWand *wand,
%        const DrawingWand *drawing_wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o drawing_wand: The draw wand.
%
*/
WandExport MagickBooleanType MagickDrawImage(MagickWand *wand,
  const DrawingWand *drawing_wand)
{
  DrawInfo
    *draw_info;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  draw_info=PeekDrawingWand(drawing_wand);
  if ((draw_info == (DrawInfo *) NULL) ||
      (draw_info->primitive == (char *) NULL))
    return(MagickFalse);
  status=DrawImage(wand->images,draw_info);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  draw_info=DestroyDrawInfo(draw_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k E d g e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickEdgeImage() enhance edges within the image with a convolution filter
%  of the given radius.  Use a radius of 0 and Edge() selects a suitable
%  radius for you.
%
%  The format of the MagickEdgeImage method is:
%
%      MagickBooleanType MagickEdgeImage(MagickWand *wand,const double radius)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: the radius of the pixel neighborhood.
%
*/
WandExport MagickBooleanType MagickEdgeImage(MagickWand *wand,
  const double radius)
{
  Image
    *edge_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  edge_image=EdgeImage(wand->images,radius,&wand->exception);
  if (edge_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,edge_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k E m b o s s I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickEmbossImage() returns a grayscale image with a three-dimensional
%  effect.  We convolve the image with a Gaussian operator of the given radius
%  and standard deviation (sigma).  For reasonable results, radius should be
%  larger than sigma.  Use a radius of 0 and Emboss() selects a suitable
%  radius for you.
%
%  The format of the MagickEmbossImage method is:
%
%      MagickBooleanType MagickEmbossImage(MagickWand *wand,const double radius,
%        const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
*/
WandExport MagickBooleanType MagickEmbossImage(MagickWand *wand,
  const double radius,const double sigma)
{
  Image
    *emboss_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  emboss_image=EmbossImage(wand->images,radius,sigma,&wand->exception);
  if (emboss_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,emboss_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k E n h a n c e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickEnhanceImage() applies a digital filter that improves the quality of a
%  noisy image.
%
%  The format of the MagickEnhanceImage method is:
%
%      MagickBooleanType MagickEnhanceImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickEnhanceImage(MagickWand *wand)
{
  Image
    *enhance_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  enhance_image=EnhanceImage(wand->images,&wand->exception);
  if (enhance_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,enhance_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k E q u a l i z e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickEqualizeImage() equalizes the image histogram.
%
%  The format of the MagickEqualizeImage method is:
%
%      MagickBooleanType MagickEqualizeImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickEqualizeImage(MagickWand *wand)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=EqualizeImage(wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k E v a l u a t e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickEvaluateImage() applys an arithmetic, relational, or logical
%  expression to an image.  Use these operators to lighten or darken an image,
%  to increase or decrease contrast in an image, or to produce the "negative"
%  of an image.
%
%  The format of the MagickEvaluateImage method is:
%
%      MagickBooleanType MagickEvaluateImage(MagickWand *wand,
%        const MagickEvaluateOperator op,const double constant)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o op: A channel operator.
%
%    o constant: A constant value.
%
*/
WandExport MagickBooleanType MagickEvaluateImage(MagickWand *wand,
  const MagickEvaluateOperator op,const double constant)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=EvaluateImage(wand->images,op,constant,&wand->images->exception);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k F l a t t e n I m a g e s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickFlattenImages() merges a sequence of images.  This is useful for
%  combining Photoshop layers into a single image.
%
%  The format of the MagickFlattenImages method is:
%
%      MagickWand *MagickFlattenImages(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickWand *MagickFlattenImages(MagickWand *wand)
{
  Image
    *flatten_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  flatten_image=FlattenImages(wand->images,&wand->exception);
  if (flatten_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,flatten_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k F l i p I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickFlipImage() creates a vertical mirror image by reflecting the pixels
%  around the central x-axis.
%
%  The format of the MagickFlipImage method is:
%
%      MagickBooleanType MagickFlipImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickFlipImage(MagickWand *wand)
{
  Image
    *flip_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  flip_image=FlipImage(wand->images,&wand->exception);
  if (flip_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,flip_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k F l o p I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickFlopImage() creates a horizontal mirror image by reflecting the pixels
%  around the central y-axis.
%
%  The format of the MagickFlopImage method is:
%
%      MagickBooleanType MagickFlopImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickFlopImage(MagickWand *wand)
{
  Image
    *flop_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  flop_image=FlopImage(wand->images,&wand->exception);
  if (flop_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,flop_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k F r a m e I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickFrameImage() adds a simulated three-dimensional border around the
%  image.  The width and height specify the border width of the vertical and
%  horizontal sides of the frame.  The inner and outer bevels indicate the
%  width of the inner and outer shadows of the frame.
%
%  The format of the MagickFrameImage method is:
%
%      MagickBooleanType MagickFrameImage(MagickWand *wand,
%        const PixelWand *matte_color,const unsigned long width,
%        const unsigned long height,const long inner_bevel,
%        const long outer_bevel)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o matte_color: The frame color pixel wand.
%
%    o width: The border width.
%
%    o height: The border height.
%
%    o inner_bevel: The inner bevel width.
%
%    o outer_bevel: The outer bevel width.
%
*/
WandExport MagickBooleanType MagickFrameImage(MagickWand *wand,
  const PixelWand *matte_color,const unsigned long width,
  const unsigned long height,const long inner_bevel,const long outer_bevel)
{
  Image
    *frame_image;

  FrameInfo
    frame_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  (void) ResetMagickMemory(&frame_info,0,sizeof(frame_info));
  frame_info.width=wand->images->columns+2*width;
  frame_info.height=wand->images->rows+2*height;
  frame_info.x=(long) width;
  frame_info.y=(long) height;
  frame_info.inner_bevel=inner_bevel;
  frame_info.outer_bevel=outer_bevel;
  PixelGetQuantumColor(matte_color,&wand->images->matte_color);
  frame_image=FrameImage(wand->images,&frame_info,&wand->exception);
  if (frame_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,frame_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k F x I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickFxImage() evaluate expression for each pixel in the image.
%
%  The format of the MagickFxImage method is:
%
%      MagickWand *MagickFxImage(MagickWand *wand,const char *expression)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o expression: The expression.
%
*/
WandExport MagickWand *MagickFxImage(MagickWand *wand,const char *expression)
{
  Image
    *fx_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  fx_image=FxImage(wand->images,expression,&wand->exception);
  if (fx_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,fx_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k F x I m a g e C h a n n e l                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickFxImageChannel() evaluate expression for each pixel in the specified
%  channel.
%
%  The format of the MagickFxImageChannel method is:
%
%      MagickWand *MagickFxImageChannel(MagickWand *wand,
%        const ChannelType channel,const char *expression)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to level: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o expression: The expression.
%
*/
WandExport MagickWand *MagickFxImageChannel(MagickWand *wand,
  const ChannelType channel,const char *expression)
{
  Image
    *fx_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  fx_image=FxImageChannel(wand->images,channel,expression,&wand->exception);
  if (fx_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,fx_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G a m m a I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGammaImage() gamma-corrects an image.  The same image viewed on
%  different devices will have perceptual differences in the way the
%  image's intensities are represented on the screen.  Specify individual
%  gamma levels for the red, green, and blue channels, or adjust all three
%  with the gamma parameter.  Values typically range from 0.8 to 2.3.
%
%  You can also reduce the influence of a particular channel with a gamma
%  value of 0.
%
%  The format of the MagickGammaImage method is:
%
%      MagickBooleanType MagickGammaImage(MagickWand *wand,const double gamma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o gamma: Define the level of gamma correction.
%
*/
WandExport MagickBooleanType MagickGammaImage(MagickWand *wand,
  const double gamma)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GammaImageChannel(wand->images,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),gamma);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G a m m a I m a g e C h a n n e l                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGammaImageChannel() gamma-corrects a particular image channel.
%  The same image viewed on different devices will have perceptual differences
%  in the way the image's intensities are represented on the screen.  Specify
%  individual gamma levels for the red, green, and blue channels, or adjust all
%  three with the gamma parameter.  Values typically range from 0.8 to 2.3.
%
%  You can also reduce the influence of a particular channel with a gamma
%  value of 0.
%
%  The format of the MagickGammaImageChannel method is:
%
%      MagickBooleanType MagickGammaImageChannel(MagickWand *wand,
%        const ChannelType channel,const double gamma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: The channel.
%
%    o level: Define the level of gamma correction.
%
*/
WandExport MagickBooleanType MagickGammaImageChannel(MagickWand *wand,
  const ChannelType channel,const double gamma)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GammaImageChannel(wand->images,channel,gamma);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G a u s s i a n B l u r I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGaussianBlurImage() blurs an image.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, the radius should be larger than sigma.  Use a
%  radius of 0 and MagickGaussianBlurImage() selects a suitable radius for you.
%
%  The format of the MagickGaussianBlurImage method is:
%
%      MagickBooleanType MagickGaussianBlurImage(MagickWand *wand,
%        const double radius,const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
*/
WandExport MagickBooleanType MagickGaussianBlurImage(MagickWand *wand,
  const double radius,const double sigma)
{
  Image
    *blur_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  blur_image=GaussianBlurImage(wand->images,radius,sigma,&wand->exception);
  if (blur_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,blur_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G a u s s i a n B l u r I m a g e C h a n n e l               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGaussianBlurImageChannel() blurs one or more image channels.  We
%  convolve the image cnannel with a Gaussian operator of the given radius and
%  standard deviation (sigma).  For reasonable results, the radius should be
%  larger than sigma.  Use a radius of 0 and MagickGaussianBlurImageChannel()
%  selects a suitable radius for you.
%
%  The format of the MagickGaussianBlurImageChannel method is:
%
%      MagickBooleanType MagickGaussianBlurImageChannel(MagickWand *wand,
%        const ChannelType channel,const double radius,const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
*/
WandExport MagickBooleanType MagickGaussianBlurImageChannel(MagickWand *wand,
  const ChannelType channel,const double radius,const double sigma)
{
  Image
    *blur_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  blur_image=GaussianBlurImageChannel(wand->images,channel,radius,sigma,
    &wand->exception);
  if (blur_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,blur_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImage() gets the image at the current image index.
%
%  The format of the MagickGetImage method is:
%
%      MagickWand *MagickGetImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickWand *MagickGetImage(MagickWand *wand)
{
  Image
    *image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((MagickWand *) NULL);
    }
  image=CloneImage(wand->images,0,0,MagickTrue,&wand->exception);
  if (image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e R e g i o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageRegion() extracts a region of the image and returns it as a
%  a new wand.
%
%  The format of the MagickGetImageRegion method is:
%
%      MagickBooleanType MagickGetImageRegion(MagickWand *wand,
%        const unsigned long width,const unsigned long height,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The region width.
%
%    o height: The region height.
%
%    o x: The region x offset.
%
%    o y: The region y offset.
%
*/

WandExport MagickWand *MagickRegionOfInterestImage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y)
{
  return(MagickGetImageRegion(wand,width,height,x,y));
}

WandExport MagickWand *MagickGetImageRegion(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y)
{
  Image
    *region_image;

  RectangleInfo
    region;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  region.width=width;
  region.height=height;
  region.x=x;
  region.y=y;
  region_image=CropImage(wand->images,&region,&wand->exception);
  if (region_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,region_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e B a c k g r o u n d C o l o r                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageBackgroundColor() returns the image background color.
%
%  The format of the MagickGetImageBackgroundColor method is:
%
%      MagickBooleanType MagickGetImageBackgroundColor(MagickWand *wand,
%        PixelWand *background_color)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o background_color: Return the background color.
%
*/
WandExport MagickBooleanType MagickGetImageBackgroundColor(MagickWand *wand,
  PixelWand *background_color)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelSetQuantumColor(background_color,&wand->images->background_color);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e B l o b                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageBlob() implements direct to memory image formats.  It
%  returns the image as a blob and its length.  The format of the image
%  determines the format of the returned blob (GIF, JPEG,  PNG, etc.).  To
%  return a different image format, use MagickSetImageFormat().
%
%  Use MagickRelinquishMemory() to free the blob when you are done with it.
%
%  The format of the MagickGetImageBlob method is:
%
%      unsigned char *MagickGetImageBlob(MagickWand *wand,size_t *length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o length: The length of the blob.
%
*/

WandExport unsigned char *MagickWriteImageBlob(MagickWand *wand,size_t *length)
{
  return(MagickGetImageBlob(wand,length));
}

WandExport unsigned char *MagickGetImageBlob(MagickWand *wand,size_t *length)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((unsigned char *) NULL);
    }
  return(ImageToBlob(wand->image_info,wand->images,length,&wand->exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e s B l o b                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageBlob() implements direct to memory image formats.  It
%  returns the image sequence as a blob and its length.  The format of the image
%  determines the format of the returned blob (GIF, JPEG,  PNG, etc.).  To
%  return a different image format, use MagickSetImageFormat().
%
%  Note, some image formats do not permit multiple images to the same image
%  stream (e.g. JPEG).  in this instance, just the first image of the
%  sequence is returned as a blob.
%
%  The format of the MagickGetImagesBlob method is:
%
%      unsigned char *MagickGetImagesBlob(MagickWand *wand,size_t *length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o length: The length of the blob.
%
*/
WandExport unsigned char *MagickGetImagesBlob(MagickWand *wand,size_t *length)
{
  unsigned char
    *blob;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((unsigned char *) NULL);
    }
  blob=ImagesToBlob(wand->image_info,GetFirstImageInList(wand->images),length,
    &wand->exception);
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e B l u e P r i m a r y                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageBluePrimary() returns the chromaticy blue primary point for the
%  image.
%
%  The format of the MagickGetImageBluePrimary method is:
%
%      MagickBooleanType MagickGetImageBluePrimary(MagickWand *wand,double *x,
%        double *y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The chromaticity blue primary x-point.
%
%    o y: The chromaticity blue primary y-point.
%
*/
WandExport MagickBooleanType MagickGetImageBluePrimary(MagickWand *wand,
  double *x,double *y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  *x=wand->images->chromaticity.blue_primary.x;
  *y=wand->images->chromaticity.blue_primary.y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e B o r d e r C o l o r                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageBorderColor() returns the image border color.
%
%  The format of the MagickGetImageBorderColor method is:
%
%      MagickBooleanType MagickGetImageBorderColor(MagickWand *wand,
%        PixelWand *border_color)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o border_color: Return the border color.
%
*/
WandExport MagickBooleanType MagickGetImageBorderColor(MagickWand *wand,
  PixelWand *border_color)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelSetQuantumColor(border_color,&wand->images->border_color);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C h a n n e l D e p t h                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageChannelDepth() gets the depth for a particular image channel.
%
%  The format of the MagickGetImageChannelDepth method is:
%
%      unsigned long MagickGetImageChannelDepth(MagickWand *wand,
%        const ChannelType channel)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
*/
WandExport unsigned long MagickGetImageChannelDepth(MagickWand *wand,
  const ChannelType channel)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(GetImageChannelDepth(wand->images,channel,&wand->exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C h a n n e l D i s t o r t i o n             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageChannelDistortion() compares one or more image channels of an
%  image to a reconstructed image and returns the specified distortion metric.
%
%  The format of the MagickGetImageChannelDistortion method is:
%
%      MagickBooleanType MagickGetImageChannelDistortion(MagickWand *wand,
%        const MagickWand *reference,const ChannelType channel,
%        const MetricType metric,double *distortion)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o reference: The reference wand.
%
%    o channel: The channel.
%
%    o metric: The metric.
%
%    o distortion: The computed distortion between the images.
%
*/
WandExport MagickBooleanType MagickGetImageChannelDistortion(MagickWand *wand,
  const MagickWand *reference,const ChannelType channel,const MetricType metric,
  double *distortion)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) || (reference->images == (Image *) NULL))
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GetImageChannelDistortion(wand->images,reference->images,channel,
    metric,distortion,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C h a n n e l E x t r e m a                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageChannelExtrema() gets the extrema for one or more image
%  channels.
%
%  The format of the MagickGetImageChannelExtrema method is:
%
%      MagickBooleanType MagickGetImageChannelExtrema(MagickWand *wand,
%        const ChannelType channel,unsigned long *minima,unsigned long *maxima)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o minima:  The minimum pixel value for the specified channel(s).
%
%    o maxima:  The maximum pixel value for the specified channel(s).
%
*/
WandExport MagickBooleanType MagickGetImageChannelExtrema(MagickWand *wand,
  const ChannelType channel,unsigned long *minima,unsigned long *maxima)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GetImageChannelExtrema(wand->images,channel,minima,maxima,
    &wand->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C h a n n e l M e a n                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageChannelMean() gets the mean and standard deviation of one or
%  more image channels.
%
%  The format of the MagickGetImageChannelMean method is:
%
%      MagickBooleanType MagickGetImageChannelMean(MagickWand *wand,
%        const ChannelType channel,double *mean,double *standard_deviation)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o mean:  The mean pixel value for the specified channel(s).
%
%    o standard_deviation:  The standard deviation for the specified channel(s).
%
*/
WandExport MagickBooleanType MagickGetImageChannelMean(MagickWand *wand,
  const ChannelType channel,double *mean,double *standard_deviation)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GetImageChannelMean(wand->images,channel,mean,standard_deviation,
    &wand->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C h a n n e l S t a t i s t i c s             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageChannelStatistics() returns statistics for each channel in the
%  image.  The statistics incude the channel depth, its minima and
%  maxima, the mean, and the standard deviation.  You can access the red
%  channel mean, for example, like this:
%
%      channel_statistics=MagickGetImageChannelStatistics(image,excepton);
%      red_mean=channel_statistics[RedChannel].mean;
%
%  Use MagickRelinquishMemory() to free the statistics buffer.
%
%  The format of the MagickGetImageChannelStatistics method is:
%
%      ChannelStatistics *MagickGetImageChannelStatistics(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport ChannelStatistics *MagickGetImageChannelStatistics(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((ChannelStatistics *) NULL);
    }
  return(GetImageChannelStatistics(wand->images,&wand->exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C o l o r m a p C o l o r                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageColormapColor() returns the color of the specified colormap
%  index.
%
%  The format of the MagickGetImageColormapColor method is:
%
%      MagickBooleanType MagickGetImageColormapColor(MagickWand *wand,
%        const unsigned long index,PixelWand *color)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o index: The offset into the image colormap.
%
%    o color: Return the colormap color in this wand.
%
*/
WandExport MagickBooleanType MagickGetImageColormapColor(MagickWand *wand,
  const unsigned long index,PixelWand *color)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if ((wand->images->colormap == (PixelPacket *) NULL) ||
      (index >= wand->images->colors))
    ThrowWandException(WandError,"Invalid colormap index",strerror(errno));
  PixelSetQuantumColor(color,wand->images->colormap+index);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C o l o r s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageColors() gets the number of unique colors in the image.
%
%  The format of the MagickGetImageColors method is:
%
%      unsigned long MagickGetImageColors(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageColors(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(0);
    }
  return(GetNumberColors(wand->images,(FILE *) NULL,&wand->exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C o l o r s p a c e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageColorspace() gets the image colorspace.
%
%  The format of the MagickGetImageColorspace method is:
%
%      ColorspaceType MagickGetImageColorspace(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport ColorspaceType MagickGetImageColorspace(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedColorspace);
    }
  return(wand->images->colorspace);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C o m p o s e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageCompose() returns the composite operator associated with the
%  image.
%
%  The format of the MagickGetImageCompose method is:
%
%      CompositeOperator MagickGetImageCompose(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport CompositeOperator MagickGetImageCompose(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedCompositeOp);
    }
  return(wand->images->compose);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C o m p r e s s i o n                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageCompression() gets the image compression.
%
%  The format of the MagickGetImageCompression method is:
%
%      CompressionType MagickGetImageCompression(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport CompressionType MagickGetImageCompression(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedCompression);
    }
  return(wand->images->compression);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e C o m p r e s s i o n Q u a l i t y           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageCompression() gets the image compression quality.
%
%  The format of the MagickGetImageCompression method is:
%
%      unsigned long MagickGetImageCompression(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageCompressionQuality(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(0UL);
    }
  return(wand->images->quality);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e D e l a y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageDelay() gets the image delay.
%
%  The format of the MagickGetImageDelay method is:
%
%      unsigned long MagickGetImageDelay(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageDelay(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(wand->images->delay);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e D e p t h                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageDepth() gets the image depth.
%
%  The format of the MagickGetImageDepth method is:
%
%      unsigned long MagickGetImageDepth(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageDepth(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(GetImageDepth(wand->images,&wand->exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e D i s t o r t i o n                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageDistortion() compares an image to a reconstructed image and
%  returns the specified distortion metric.
%
%  The format of the MagickGetImageDistortion method is:
%
%      MagickBooleanType MagickGetImageDistortion(MagickWand *wand,
%        const MagickWand *reference,const MetricType metric,
%        double *distortion)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o reference: The reference wand.
%
%    o metric: The metric.
%
%    o distortion: The computed distortion between the images.
%
*/
WandExport MagickBooleanType MagickGetImageDistortion(MagickWand *wand,
  const MagickWand *reference,const MetricType metric,double *distortion)
{
  MagickBooleanType
    status;


  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) || (reference->images == (Image *) NULL))
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GetImageDistortion(wand->images,reference->images,metric,distortion,
    &wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e E x t r e m a                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageExtrema() gets the extrema for the image.
%
%  The format of the MagickGetImageExtrema method is:
%
%      MagickBooleanType MagickGetImageExtrema(MagickWand *wand,
%        unsigned long *min,unsigned long *max)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o min:  The minimum pixel value for the specified channel(s).
%
%    o max:  The maximum pixel value for the specified channel(s).
%
*/
WandExport MagickBooleanType MagickGetImageExtrema(MagickWand *wand,
  unsigned long *min,unsigned long *max)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=GetImageExtrema(wand->images,min,max,&wand->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e D i s p o s e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageDispose() gets the image disposal method.
%
%  The format of the MagickGetImageDispose method is:
%
%      DisposeType MagickGetImageDispose(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport DisposeType MagickGetImageDispose(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedDispose);
    }
  return((DisposeType) wand->images->dispose);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e A t t r i b u t e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageAttribute() returns a value associated with the specified
%  key.  Use MagickRelinquishMemory() to free the value when you are finished
%  with it.
%
%  The format of the MagickGetImageAttribute method is:
%
%      char *MagickGetImageAttribute(MagickWand *wand,const char *key)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o key: The key.
%
*/
WandExport char *MagickGetImageAttribute(MagickWand *wand,const char *key)
{
  const ImageAttribute
    *attribute;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((char *) NULL);
    }
  attribute=GetImageAttribute(wand->images,key);
  if (attribute == (const ImageAttribute *) NULL)
    return((char *) NULL);
  return(ConstantString(AcquireString(attribute->value)));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e F i l e n a m e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageFilename() returns the filename of a particular image in a
%  sequence.
%
%  The format of the MagickGetImageFilename method is:
%
%      char *MagickGetImageFilename(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport char *MagickGetImageFilename(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((char *) NULL);
    }
  return(AcquireString(wand->images->filename));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e F o r m a t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageFormat() returns the format of a particular image in a
%  sequence.
%
%  The format of the MagickGetImageFormat method is:
%
%      const char MagickGetImageFormat(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport char *MagickGetImageFormat(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((char *) NULL);
    }
  return(AcquireString(wand->images->magick));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e G a m m a                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageGamma() gets the image gamma.
%
%  The format of the MagickGetImageGamma method is:
%
%      double MagickGetImageGamma(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport double MagickGetImageGamma(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(0.0);
    }
  return(wand->images->gamma);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e G r e e n P r i m a r y                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageGreenPrimary() returns the chromaticy green primary point.
%
%  The format of the MagickGetImageGreenPrimary method is:
%
%      MagickBooleanType MagickGetImageGreenPrimary(MagickWand *wand,double *x,
%        double *y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The chromaticity green primary x-point.
%
%    o y: The chromaticity green primary y-point.
%
*/
WandExport MagickBooleanType MagickGetImageGreenPrimary(MagickWand *wand,
  double *x,double *y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  *x=wand->images->chromaticity.green_primary.x;
  *y=wand->images->chromaticity.green_primary.y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e H e i g h t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageHeight() returns the image height.
%
%  The format of the MagickGetImageHeight method is:
%
%      unsigned long MagickGetImageHeight(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageHeight(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(wand->images->rows);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e H i s t o g r a m                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageHistogram() returns the image histogram as an array of
%  PixelWand wands.
%
%  The format of the MagickGetImageHistogram method is:
%
%      PixelWand *MagickGetImageHistogram(MagickWand *wand,
%        unsigned long *number_colors)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o number_colors: The number of unique colors in the image and the number
%      of pixel wands returned.
%
%
*/
WandExport PixelWand **MagickGetImageHistogram(MagickWand *wand,
  unsigned long *number_colors)
{
  ColorPacket
    *histogram;

  PixelWand
    **pixel_wands;

  register long
    i;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((PixelWand **) NULL);
    }
  histogram=GetImageHistogram(wand->images,number_colors,&wand->exception);
  if (histogram == (ColorPacket *) NULL)
    return((PixelWand **) NULL);
  pixel_wands=NewPixelWands(*number_colors);
  for (i=0; i < (long) *number_colors; i++)
  {
    PixelSetQuantumColor(pixel_wands[i],&histogram[i].pixel);
    PixelSetIndex(pixel_wands[i],histogram[i].index);
    PixelSetColorCount(pixel_wands[i],histogram[i].count);
  }
  histogram=(ColorPacket *) RelinquishMagickMemory(histogram);
  return(pixel_wands);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e I n d e x                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageIndex() returns the index of the current image.
%
%  The format of the MagickGetImageIndex method is:
%
%      MagickBooleanType MagickGetImageIndex(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport long MagickGetImageIndex(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"WandContainsNoImageIndexs",wand->name);
  return(GetImageIndexInList(wand->images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e I n t e r l a c e S c h e m e                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageInterlaceScheme() gets the image interlace scheme.
%
%  The format of the MagickGetImageInterlaceScheme method is:
%
%      InterlaceType MagickGetImageInterlaceScheme(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport InterlaceType MagickGetImageInterlaceScheme(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedInterlace);
    }
  return(wand->images->interlace);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e I t e r a t i o n s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageIterations() gets the image iterations.
%
%  The format of the MagickGetImageIterations method is:
%
%      unsigned long MagickGetImageIterations(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageIterations(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(wand->images->iterations);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e M a t t e C o l o r                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageMatteColor() returns the image matte color.
%
%  The format of the MagickGetImageMatteColor method is:
%
%      MagickBooleanType MagickGetImagematteColor(MagickWand *wand,
%        PixelWand *matte_color)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o matte_color: Return the matte color.
%
*/
WandExport MagickBooleanType MagickGetImageMatteColor(MagickWand *wand,
  PixelWand *matte_color)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelSetQuantumColor(matte_color,&wand->images->matte_color);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e P a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImagePage() returns the page geometry associated with the image.
%
%  The format of the MagickGetImagePage method is:
%
%      MagickBooleanType MagickGetImagePage(MagickWand *wand,
%        unsigned long *width,unsigned long *height,long *x,long *y)
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
WandExport MagickBooleanType MagickGetImagePage(MagickWand *wand,
  unsigned long *width,unsigned long *height,long *x,long *y)
{
  assert(wand != (const MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  *width=wand->images->page.width;
  *height=wand->images->page.height;
  *x=wand->images->page.x;
  *y=wand->images->page.y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e P i x e l C o l o r                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImagePixelColor() returns the color of the specified pixel.
%
%  The format of the MagickGetImagePixelColor method is:
%
%      MagickBooleanType MagickGetImagePixelColor(MagickWand *wand,
%        const long x,const long y,PixelWand *color)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x,y: The pixel offset into the image.
%
%    o color: Return the colormap color in this wand.
%
*/
WandExport MagickBooleanType MagickGetImagePixelColor(MagickWand *wand,
  const long x,const long y,PixelWand *color)
{
  IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  ViewInfo
    *view;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  view=OpenCacheView(wand->images);
  if (view == (ViewInfo *) NULL)
    return(MagickFalse);
  p=AcquireCacheView(view,x,y,1,1,&wand->exception);
  if (p == (const PixelPacket *) NULL)
    {
      CloseCacheView(view);
      return(MagickFalse);
    }
  indexes=GetCacheViewIndexes(view);
  PixelSetQuantumColor(color,p);
  if (view->image->colorspace == CMYKColorspace)
    PixelSetBlackQuantum(color,*indexes);
  else
    if (view->image->storage_class == PseudoClass)
      PixelSetIndex(color,*indexes);
  CloseCacheView(view);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e P i x e l s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImagePixels() extracts pixel data from an image and returns it to
%  you.  The method returns MagickTrue on success otherwise MagickFalse if an
%  error is encountered.  The data is returned as char, short int, int, long,
%  float, or double in the order specified by map.
%
%  Suppose you want to extract the first scanline of a 640x480 image as
%  character data in red-green-blue order:
%
%      MagickGetImagePixels(wand,0,0,640,1,"RGB",CharPixel,pixels);
%
%  The format of the MagickGetImagePixels method is:
%
%      MagickBooleanType MagickGetImagePixels(MagickWand *wand,
%        const long x,const long y,const unsigned long columns,
%        const unsigned long rows,const char *map,const StorageType storage,
%        void *pixels)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x, y, columns, rows:  These values define the perimeter
%      of a region of pixels you want to extract.
%
%    o map:  This string reflects the expected ordering of the pixel array.
%      It can be any combination or order of R = red, G = green, B = blue,
%      A = alpha (0 is transparent), O = opacity (0 is opaque), C = cyan,
%      Y = yellow, M = magenta, K = black, I = intensity (for grayscale),
%      P = pad.
%
%    o storage: Define the data type of the pixels.  Float and double types are
%      expected to be normalized [0..1] otherwise [0..QuantumRange].  Choose from
%      these types: CharPixel, DoublePixel, FloatPixel, IntegerPixel,
%      LongPixel, QuantumPixel, or ShortPixel.
%
%    o pixels: This array of values contain the pixel components as defined by
%      map and type.  You must preallocate this array where the expected
%      length varies depending on the values of width, height, map, and type.
%
%
*/
WandExport MagickBooleanType MagickGetImagePixels(MagickWand *wand,
  const long x,const long y,const unsigned long columns,
  const unsigned long rows,const char *map,const StorageType storage,
  void *pixels)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=ExportImagePixels(wand->images,x,y,columns,rows,map,
    storage,pixels,&wand->exception);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e P r o f i l e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageProfile() returns the named image profile.
%
%  The format of the MagickGetImageProfile method is:
%
%      unsigned char *MagickGetImageProfile(MagickWand *wand,const char *name,
%        unsigned long *length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o name: Name of profile to return: ICC, IPTC, or generic profile.
%
%    o length: The length of the profile.
%
*/
WandExport unsigned char *MagickGetImageProfile(MagickWand *wand,
  const char *name,unsigned long *length)
{
  const StringInfo
    *profile;

  unsigned char
    *datum;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((unsigned char *) NULL);
    }
  *length=0;
  if (wand->images->profiles == (SplayTreeInfo *) NULL)
    return((unsigned char *) NULL);
  profile=GetImageProfile(wand->images,name);
  if (profile == (StringInfo *) NULL)
    return((unsigned char *) NULL);
  datum=(unsigned char *) AcquireMagickMemory(profile->length);
  if (datum == (unsigned char *) NULL)
    return((unsigned char *) NULL);
  (void) CopyMagickMemory(datum,profile->datum,profile->length);
  *length=(unsigned long) profile->length;
  return(datum);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e R e d P r i m a r y                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageRedPrimary() returns the chromaticy red primary point.
%
%  The format of the MagickGetImageRedPrimary method is:
%
%      MagickBooleanType MagickGetImageRedPrimary(MagickWand *wand,double *x,
%        double *y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The chromaticity red primary x-point.
%
%    o y: The chromaticity red primary y-point.
%
*/
WandExport MagickBooleanType MagickGetImageRedPrimary(MagickWand *wand,
  double *x,double *y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  *x=wand->images->chromaticity.red_primary.x;
  *y=wand->images->chromaticity.red_primary.y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e R e n d e r i n g I n t e n t                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageRenderingIntent() gets the image rendering intent.
%
%  The format of the MagickGetImageRenderingIntent method is:
%
%      RenderingIntent MagickGetImageRenderingIntent(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport RenderingIntent MagickGetImageRenderingIntent(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedIntent);
    }
  return((RenderingIntent) wand->images->rendering_intent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e R e s o l u t i o n                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageResolution() gets the image X and Y resolution.
%
%  The format of the MagickGetImageResolution method is:
%
%      MagickBooleanType MagickGetImageResolution(MagickWand *wand,double *x,
%        double *y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The image x-resolution.
%
%    o y: The image y-resolution.
%
*/
WandExport MagickBooleanType MagickGetImageResolution(MagickWand *wand,
  double *x,double *y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  *x=wand->images->x_resolution;
  *y=wand->images->y_resolution;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e S c e n e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageScene() gets the image scene.
%
%  The format of the MagickGetImageScene method is:
%
%      unsigned long MagickGetImageScene(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageScene(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(wand->images->scene);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e S i g n a t u r e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageSignature() generates an SHA-256 message digest for the image
%  pixel stream.
%
%  The format of the MagickGetImageSignature method is:
%
%      const char MagickGetImageSignature(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport char *MagickGetImageSignature(MagickWand *wand)
{
  const ImageAttribute
    *attribute;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((char *) NULL);
    }
  status=SignatureImage(wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  attribute=GetImageAttribute(wand->images,"Signature");
  if (attribute != (ImageAttribute *) NULL)
    return(AcquireString(attribute->value));
  InheritException(&wand->exception,&wand->images->exception);
  return((char *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e S i z e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageSize() returns the image size.
%
%  The format of the MagickGetImageSize method is:
%
%      MagickSizeType MagickGetImageSize(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickSizeType MagickGetImageSize(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(GetBlobSize(wand->images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e T i c k s P e r S e c o n d                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageTicksPerSecond() gets the image ticks-per-second.
%
%  The format of the MagickGetImageTicksPerSecond method is:
%
%      unsigned long MagickGetImageTicksPerSecond(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageTicksPerSecond(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(wand->images->ticks_per_second);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e T y p e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageType() gets the potential image type:
%
%        Bilevel        Grayscale       GrayscaleMatte
%        Palette        PaletteMatte    TrueColor
%        TrueColorMatte ColorSeparation ColorSeparationMatte
%
%  To ensure the image type matches its potential, use MagickSetImageType():
%
%    (void) MagickSetImageType(wand,MagickGetImageType(wand));
%
%  The format of the MagickGetImageType method is:
%
%      ImageType MagickGetImageType(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport ImageType MagickGetImageType(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedType);
    }
  return(GetImageType(wand->images,&wand->exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e U n i t s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageUnits() gets the image units of resolution.
%
%  The format of the MagickGetImageUnits method is:
%
%      ResolutionType MagickGetImageUnits(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport ResolutionType MagickGetImageUnits(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedResolution);
    }
  return(wand->images->units);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e V i r t u a l P i x e l M e t h o d           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageVirtualPixelMethod() returns the virtual pixel method for the
%  sepcified image.
%
%  The format of the MagickGetImageVirtualPixelMethod method is:
%
%      VirtualPixelMethod MagickGetImageVirtualPixelMethod(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport VirtualPixelMethod MagickGetImageVirtualPixelMethod(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(UndefinedVirtualPixelMethod);
    }
  return(GetImageVirtualPixelMethod(wand->images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e W h i t e P o i n t                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageWhitePoint() returns the chromaticy white point.
%
%  The format of the MagickGetImageWhitePoint method is:
%
%      MagickBooleanType MagickGetImageWhitePoint(MagickWand *wand,double *x,
%        double *y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The chromaticity white x-point.
%
%    o y: The chromaticity white y-point.
%
*/
WandExport MagickBooleanType MagickGetImageWhitePoint(MagickWand *wand,
  double *x,double *y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  *x=wand->images->chromaticity.white_point.x;
  *y=wand->images->chromaticity.white_point.y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t I m a g e W i d t h                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageWidth() returns the image width.
%
%  The format of the MagickGetImageWidth method is:
%
%      unsigned long MagickGetImageWidth(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetImageWidth(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(wand->images->columns);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k G e t N u m b e r I m a g e s                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetNumberImages() returns the number of images associated with a
%  magick wand.
%
%  The format of the MagickGetNumberImages method is:
%
%      unsigned long MagickGetNumberImages(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport unsigned long MagickGetNumberImages(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  return(GetImageListLength(wand->images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k I m a g e G e t T o t a l I n k D e n s i t y                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickGetImageTotalInkDensity() gets the image total ink density.
%
%  The format of the MagickGetImageTotalInkDensity method is:
%
%      double MagickGetImageTotalInkDensity(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport double MagickGetImageTotalInkDensity(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return(0.0);
    }
  return(GetImageTotalInkDensity(wand->images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k H a s N e x t I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickHasNextImage() returns MagickTrue if the wand has more images when
%  traversing the list in the forward direction
%
%  The format of the MagickHasNextImage method is:
%
%      MagickBooleanType MagickHasNextImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickHasNextImage(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if (GetNextImageInList(wand->images) == (Image *) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k H a s P r e v i o u s I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickHasPreviousImage() returns MagickTrue if the wand has more images when
%  traversing the list in the reverse direction
%
%  The format of the MagickHasPreviousImage method is:
%
%      MagickBooleanType MagickHasPreviousImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickHasPreviousImage(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if (GetPreviousImageInList(wand->images) == (Image *) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k I d e n t i f y I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickIdentifyImage() identifies an image by printing its attributes to the
%  file.  Attributes include the image width, height, size, and others.
%
%  The format of the MagickIdentifyImage method is:
%
%      const char *MagickIdentifyImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/

WandExport char *MagickDescribeImage(MagickWand *wand)
{
  return(MagickIdentifyImage(wand));
}

WandExport char *MagickIdentifyImage(MagickWand *wand)
{
  char
    *description,
    filename[MaxTextExtent];

  FILE
    *file;

  int
    unique_file;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((char *) NULL);
    }
  description=(char *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  file=(FILE *) NULL;
  if (unique_file != -1)
    file=fdopen(unique_file,"wb");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "UnableToCreateTemporaryFile","`%s'",wand->name);
      return((char *) NULL);
    }
  (void) IdentifyImage(wand->images,file,MagickTrue);
  (void) fclose(file);
  description=FileToString(filename,~0,&wand->exception);
  (void) RelinquishUniqueFileResource(filename);
  return(description);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k I m p l o d e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickImplodeImage() creates a new image that is a copy of an existing
%  one with the image pixels "implode" by the specified percentage.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the MagickImplodeImage method is:
%
%      MagickBooleanType MagickImplodeImage(MagickWand *wand,
%        const double radius)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o amount: Define the extent of the implosion.
%
*/
WandExport MagickBooleanType MagickImplodeImage(MagickWand *wand,
  const double amount)
{
  Image
    *implode_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  implode_image=ImplodeImage(wand->images,amount,&wand->exception);
  if (implode_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,implode_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k L a b e l I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickLabelImage() adds a label to your image.
%
%  The format of the MagickLabelImage method is:
%
%      MagickBooleanType MagickLabelImage(MagickWand *wand,const char *label)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o label: The image label.
%
*/
WandExport MagickBooleanType MagickLabelImage(MagickWand *wand,
  const char *label)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=SetImageAttribute(wand->images,"label",label);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k L e v e l I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickLevelImage() adjusts the levels of an image by scaling the colors
%  falling between specified white and black points to the full available
%  quantum range. The parameters provided represent the black, mid, and white
%  points. The black point specifies the darkest color in the image. Colors
%  darker than the black point are set to zero. Mid point specifies a gamma
%  correction to apply to the image.  White point specifies the lightest color
%  in the image. Colors brighter than the white point are set to the maximum
%  quantum value.
%
%  The format of the MagickLevelImage method is:
%
%      MagickBooleanType MagickLevelImage(MagickWand *wand,
%        const double black_point,const double gamma,const double white_point)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o black_point: The black point.
%
%    o gamma: The gamma.
%
%    o white_point: The white point.
%
*/
WandExport MagickBooleanType MagickLevelImage(MagickWand *wand,
  const double black_point,const double gamma,const double white_point)
{
  char
    levels[MaxTextExtent];

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  (void) FormatMagickString(levels,MaxTextExtent,"%g,%g,%g",black_point,
    white_point,gamma);
  status=LevelImage(wand->images,levels);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k L e v e l I m a g e C h a n n e l                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickLevelImageChannel() adjusts the levels of the specified channel of the
%  reference image by scaling the colors falling between specified white and
%  black points to the full available quantum range. The parameters provided
%  represent the black, mid, and white points. The black point specifies the
%  darkest color in the image. Colors darker than the black point are set to
%  zero.  Mid point specifies a gamma correction to apply to the image.  White
%  point specifies the lightest color in the image. Colors brighter than the
%  white point are set to the maximum quantum value.
%
%  The format of the MagickLevelImageChannel method is:
%
%      MagickBooleanType MagickLevelImageChannel(MagickWand *wand,
%        const ChannelType channel,const double black_point,const double gamma,
%        const double white_point)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to level: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o black_point: The black point.
%
%    o gamma: The gamma.
%
%    o white_point: The white point.
%
*/
WandExport MagickBooleanType MagickLevelImageChannel(MagickWand *wand,
  const ChannelType channel,const double black_point,const double gamma,
  const double white_point)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=LevelImageChannel(wand->images,channel,black_point,white_point,gamma);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M a g n i f y I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMagnifyImage() is a convenience method that scales an image
%  proportionally to twice its original size.
%
%  The format of the MagickMagnifyImage method is:
%
%      MagickBooleanType MagickMagnifyImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickMagnifyImage(MagickWand *wand)
{
  Image
    *magnify_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  magnify_image=MagnifyImage(wand->images,&wand->exception);
  if (magnify_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,magnify_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M a p I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMapImage() replaces the colors of an image with the closest color
%  from a reference image.
%
%  The format of the MagickMapImage method is:
%
%      MagickBooleanType MagickMapImage(MagickWand *wand,
%        const MagickWand *map_wand,const MagickBooleanType dither)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o map: The map wand.
%
%    o dither: Set this integer value to something other than zero to dither
%      the mapped image.
%
*/
WandExport MagickBooleanType MagickMapImage(MagickWand *wand,
  const MagickWand *map_wand,const MagickBooleanType dither)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) || (map_wand->images == (Image *) NULL))
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=MapImage(wand->images,map_wand->images,dither);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M a t t e F l o o d f i l l I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMatteFloodfillImage() changes the transparency value of any pixel that
%  matches target and is an immediate neighbor.  If the method
%  FillToBorderMethod is specified, the transparency value is changed for any
%  neighbor pixel that does not match the bordercolor member of image.
%
%  The format of the MagickMatteFloodfillImage method is:
%
%      MagickBooleanType MagickMatteFloodfillImage(MagickWand *wand,
%        const Quantum opacity,const double fuzz,const PixelWand *bordercolor,
%        const long x,const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o opacity: The opacity.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
%    o bordercolor: The border color pixel wand.
%
%    o x,y: The starting location of the operation.
%
*/
WandExport MagickBooleanType MagickMatteFloodfillImage(MagickWand *wand,
  const Quantum opacity,const double fuzz,const PixelWand *bordercolor,
  const long x,const long y)
{
  DrawInfo
    *draw_info;

  MagickBooleanType
    status;

  PixelPacket
    target;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  draw_info=CloneDrawInfo(wand->image_info,(DrawInfo *) NULL);
  target=AcquireOnePixel(wand->images,x % wand->images->columns,
    y % wand->images->rows,&wand->exception);
  if (bordercolor != (PixelWand *) NULL)
    PixelGetQuantumColor(bordercolor,&target);
  status=MatteFloodfillImage(wand->images,target,opacity,x,y,
    bordercolor != (PixelWand *) NULL ? FillToBorderMethod : FloodfillMethod);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  draw_info=DestroyDrawInfo(draw_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M e d i a n F i l t e r I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMedianFilterImage() applies a digital filter that improves the quality
%  of a noisy image.  Each pixel is replaced by the median in a set of
%  neighboring pixels as defined by radius.
%
%  The format of the MagickMedianFilterImage method is:
%
%      MagickBooleanType MagickMedianFilterImage(MagickWand *wand,
%        const double radius)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the pixel neighborhood.
%
*/
WandExport MagickBooleanType MagickMedianFilterImage(MagickWand *wand,
  const double radius)
{
  Image
    *median_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  median_image=MedianFilterImage(wand->images,radius,&wand->exception);
  if (median_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,median_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M i n i f y I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMinifyImage() is a convenience method that scales an image
%  proportionally to one-half its original size
%
%  The format of the MagickMinifyImage method is:
%
%      MagickBooleanType MagickMinifyImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickMinifyImage(MagickWand *wand)
{
  Image
    *minify_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  minify_image=MinifyImage(wand->images,&wand->exception);
  if (minify_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,minify_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M o d u l a t e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickModulateImage() lets you control the brightness, saturation, and hue
%  of an image.  Hue is the percentage of absolute rotation from the current
%  position.  For example 50 results in a counter-clockwise rotation of 90
%  degrees, 150 results in a clockwise rotation of 90 degrees, with 0 and 200
%  both resulting in a rotation of 180 degrees.
%
%  To increase the color brightness by 20% and decrease the color saturation by
%  10% and leave the hue unchanged, use: 120,90,100.
%
%  The format of the MagickModulateImage method is:
%
%      MagickBooleanType MagickModulateImage(MagickWand *wand,
%        const double brightness,const double saturation,const double hue)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o brightness: The percent change in brighness.
%
%    o saturation: The percent change in saturation.
%
%    o hue: The percent change in hue.
%
*/
WandExport MagickBooleanType MagickModulateImage(MagickWand *wand,
  const double brightness,const double saturation,const double hue)
{
  char
    modulate[MaxTextExtent];

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  (void) FormatMagickString(modulate,MaxTextExtent,"%g,%g,%g",brightness,
    saturation,hue);
  status=ModulateImage(wand->images,modulate);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M o n t a g e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMontageImage() creates a composite image by combining several
%  separate images. The images are tiled on the composite image with the name
%  of the image optionally appearing just below the individual tile.
%
%  The format of the MagickMontageImage method is:
%
%      MagickWand *MagickMontageImage(MagickWand *wand,
%        const DrawingWand drawing_wand,const char *tile_geometry,
%        const char *thumbnail_geometry,const MontageMode mode,
%        const char *frame)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o drawing_wand: The drawing wand.  The font name, size, and color are
%      obtained from this wand.
%
%    o tile_geometry: the number of tiles per row and page (e.g. 6x4+0+0).
%
%    o thumbnail_geometry: Preferred image size and border size of each
%      thumbnail (e.g. 120x120+4+3>).
%
%    o mode: Thumbnail framing mode: Frame, Unframe, or Concatenate.
%
%    o frame: Surround the image with an ornamental border (e.g. 15x15+3+3).
%      The frame color is that of the thumbnail's matte color.
%
*/
WandExport MagickWand *MagickMontageImage(MagickWand *wand,
  const DrawingWand *drawing_wand,const char *tile_geometry,
  const char *thumbnail_geometry,const MontageMode mode,const char *frame)
{
  char
    *font;

  Image
    *montage_image;

  MontageInfo
    *montage_info;

  PixelWand
    *pixel_wand;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  montage_info=CloneMontageInfo(wand->image_info,(MontageInfo *) NULL);
  switch (mode)
  {
    case FrameMode:
    {
      (void) CloneString(&montage_info->frame,"15x15+3+3");
      montage_info->shadow=MagickTrue;
      break;
    }
    case UnframeMode:
    {
      montage_info->frame=(char *) NULL;
      montage_info->shadow=MagickFalse;
      montage_info->border_width=0;
      break;
    }
    case ConcatenateMode:
    {
      montage_info->frame=(char *) NULL;
      montage_info->shadow=MagickFalse;
      (void) CloneString(&montage_info->geometry,"+0+0");
      montage_info->border_width=0;
      break;
    }
    default:
      break;
  }
  font=DrawGetFont(drawing_wand);
  if (font != (char *) NULL)
    (void) CloneString(&montage_info->font,font);
  if (frame != (char *) NULL)
    (void) CloneString(&montage_info->frame,frame);
  montage_info->pointsize=DrawGetFontSize(drawing_wand);
  pixel_wand=NewPixelWand();
  DrawGetFillColor(drawing_wand,pixel_wand);
  PixelGetQuantumColor(pixel_wand,&montage_info->fill);
  DrawGetStrokeColor(drawing_wand,pixel_wand);
  PixelGetQuantumColor(pixel_wand,&montage_info->stroke);
  pixel_wand=DestroyPixelWand(pixel_wand);
  if (thumbnail_geometry != (char *) NULL)
    (void) CloneString(&montage_info->geometry,thumbnail_geometry);
  if (tile_geometry != (char *) NULL)
    (void) CloneString(&montage_info->tile,tile_geometry);
  montage_image=MontageImages(wand->images,montage_info,&wand->exception);
  montage_info=DestroyMontageInfo(montage_info);
  if (montage_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,montage_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M o r p h I m a g e s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMorphImages() method morphs a set of images.  Both the image pixels
%  and size are linearly interpolated to give the appearance of a
%  meta-morphosis from one image to the next.
%
%  The format of the MagickMorphImages method is:
%
%      MagickWand *MagickMorphImages(MagickWand *wand,
%        const unsigned long number_frames)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o number_frames: The number of in-between images to generate.
%
*/
WandExport MagickWand *MagickMorphImages(MagickWand *wand,
  const unsigned long number_frames)
{
  Image
    *morph_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  morph_image=MorphImages(wand->images,number_frames,&wand->exception);
  if (morph_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,morph_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M o s a i c I m a g e s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMosaicImages() inlays an image sequence to form a single coherent
%  picture.  It returns a wand with each image in the sequence composited at
%  the location defined by the page offset of the image.
%
%  The format of the MagickMosaicImages method is:
%
%      MagickWand *MagickMosaicImages(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickWand *MagickMosaicImages(MagickWand *wand)
{
  Image
    *mosaic_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  mosaic_image=MosaicImages(wand->images,&wand->exception);
  if (mosaic_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,mosaic_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k M o t i o n B l u r I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickMotionBlurImage() simulates motion blur.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, radius should be larger than sigma.  Use a
%  radius of 0 and MotionBlurImage() selects a suitable radius for you.
%  Angle gives the angle of the blurring motion.
%
%  The format of the MagickMotionBlurImage method is:
%
%      MagickBooleanType MagickMotionBlurImage(MagickWand *wand,
%        const double radius,const double sigma,const double angle)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the Gaussian, in pixels, not counting
%      the center pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o angle: Apply the effect along this angle.
%
*/
WandExport MagickBooleanType MagickMotionBlurImage(MagickWand *wand,
  const double radius,const double sigma,const double angle)
{
  Image
    *blur_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  blur_image=MotionBlurImage(wand->images,radius,sigma,angle,&wand->exception);
  if (blur_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,blur_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k N e g a t e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickNegateImage() negates the colors in the reference image.  The
%  Grayscale option means that only grayscale values within the image are
%  negated.
%
%  You can also reduce the influence of a particular channel with a gamma
%  value of 0.
%
%  The format of the MagickNegateImage method is:
%
%      MagickBooleanType MagickNegateImage(MagickWand *wand,
%        const MagickBooleanType gray)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o gray: If MagickTrue, only negate grayscale pixels within the image.
%
*/
WandExport MagickBooleanType MagickNegateImage(MagickWand *wand,
  const MagickBooleanType gray)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=NegateImage(wand->images,gray);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k N e g a t e I m a g e C h a n n e l                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickNegateImage() negates the colors in the specified channel of the
%  reference image.  The Grayscale option means that only grayscale values
%  within the image are negated.
%
%  You can also reduce the influence of a particular channel with a gamma
%  value of 0.
%
%  The format of the MagickNegateImage method is:
%
%      MagickBooleanType MagickNegateImage(MagickWand *wand,
%        const ChannelType channel,const MagickBooleanType gray)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o gray: If MagickTrue, only negate grayscale pixels within the image.
%
*/
WandExport MagickBooleanType MagickNegateImageChannel(MagickWand *wand,
  const ChannelType channel,const MagickBooleanType gray)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=NegateImageChannel(wand->images,channel,gray);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k N e w I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickNewImage() adds a blank image canvas of the specified size and
%  background color to the wand.
%
%  The format of the MagickNewImage method is:
%
%      MagickBooleanType MagickNewImage(MagickWand *wand,
%        const unsigned long columns,const unsigned long rows,
%        const PixelWand *background)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The image width.
%
%    o height: The image height.
%
%    o background: The image color.
%
*/
WandExport MagickBooleanType MagickNewImage(MagickWand *wand,
  const unsigned long width,const unsigned long height,
  const PixelWand *background)
{
  Image
    *image;

  MagickPixelPacket
    pixel;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  PixelGetMagickColor(background,&pixel);
  image=NewMagickImage(wand->image_info,width,height,&pixel);
  if (image->exception.severity != UndefinedException)
    InheritException(&wand->exception,&image->exception);
  if (wand->images == (Image *) NULL)
    AppendImageToList(&wand->images,image);
  else
    if (GetNextImageInList(wand->images) == (Image *) NULL)
      AppendImageToList(&wand->images,image);
    else
      InsertImageInList(&wand->images,image);
  wand->images=GetFirstImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k N e x t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickNextImage() associates the next image in the image list with a magick
%  wand.
%
%  The format of the MagickNextImage method is:
%
%      MagickBooleanType MagickNextImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickNextImage(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if (wand->iterator != MagickFalse)
    {
      wand->iterator=MagickFalse;
      return(MagickTrue);
    }
  if (GetNextImageInList(wand->images) == (Image *) NULL)
    {
      wand->iterator=MagickTrue;
      return(MagickFalse);
    }
  wand->images=GetNextImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k N o r m a l i z e I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickNormalizeImage() enhances the contrast of a color image by adjusting
%  the pixels color to span the entire range of colors available
%
%  You can also reduce the influence of a particular channel with a gamma
%  value of 0.
%
%  The format of the MagickNormalizeImage method is:
%
%      MagickBooleanType MagickNormalizeImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickNormalizeImage(MagickWand *wand)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=NormalizeImage(wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k O i l P a i n t I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickOilPaintImage() applies a special effect filter that simulates an oil
%  painting.  Each pixel is replaced by the most frequent color occurring
%  in a circular region defined by radius.
%
%  The format of the MagickOilPaintImage method is:
%
%      MagickBooleanType MagickOilPaintImage(MagickWand *wand,
%        const double radius)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the circular neighborhood.
%
*/
WandExport MagickBooleanType MagickOilPaintImage(MagickWand *wand,
  const double radius)
{
  Image
    *paint_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  paint_image=OilPaintImage(wand->images,radius,&wand->exception);
  if (paint_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,paint_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k O p a q u e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickPaintOpaqueImage() changes any pixel that matches color with the color
%  defined by fill.
%
%  The format of the MagickPaintOpaqueImage method is:
%
%      MagickBooleanType MagickPaintOpaqueImage(MagickWand *wand,
%        const PixelWand *target,const PixelWand *fill,const double fuzz)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o target: Change this target color to the fill color within the image.
%
%    o fill: The fill pixel wand.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
*/

WandExport MagickBooleanType MagickOpaqueImage(MagickWand *wand,
  const PixelWand *target,const PixelWand *fill,const double fuzz)
{
  return(MagickPaintOpaqueImage(wand,target,fill,fuzz));
}

WandExport MagickBooleanType MagickPaintOpaqueImage(MagickWand *wand,
  const PixelWand *target,const PixelWand *fill,const double fuzz)
{
  MagickBooleanType
    status;

  MagickPixelPacket
    fill_pixel,
    target_pixel;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelGetMagickColor(target,&target_pixel);
  PixelGetMagickColor(fill,&fill_pixel);
  wand->images->fuzz=fuzz;
  status=PaintOpaqueImage(wand->images,&target_pixel,&fill_pixel);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k P a i n t T r a n s p a r e n t I m a g e                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickPaintTransparentImage() changes any pixel that matches color with the
%  color defined by fill.
%
%  The format of the MagickPaintTransparentImage method is:
%
%      MagickBooleanType MagickPaintTransparentImage(MagickWand *wand,
%        const PixelWand *target,const Quantum opacity,const double fuzz)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o target: Change this target color to specified opacity value within
%      the image.
%
%    o opacity: The replacement opacity value.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
*/

WandExport MagickBooleanType MagickTransparentImage(MagickWand *wand,
  const PixelWand *target,const Quantum opacity,const double fuzz)
{
  return(MagickPaintTransparentImage(wand,target,opacity,fuzz));
}

WandExport MagickBooleanType MagickPaintTransparentImage(MagickWand *wand,
  const PixelWand *target,const Quantum opacity,const double fuzz)
{
  MagickBooleanType
    status;

  MagickPixelPacket
    target_pixel;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelGetMagickColor(target,&target_pixel);
  wand->images->fuzz=fuzz;
  status=PaintTransparentImage(wand->images,&target_pixel,opacity);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k P i n g I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickPingImage() is like MagickReadImage() except the only valid
%  information returned is the image width, height, size, and format.  It
%  is designed to efficiently obtain this information from a file without
%  reading the entire image sequence into memory.
%
%  The format of the MagickPingImage method is:
%
%      MagickBooleanType MagickPingImage(MagickWand *wand,const char *filename)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o filename: The image filename.
%
%
*/
WandExport MagickBooleanType MagickPingImage(MagickWand *wand,
  const char *filename)
{
  Image
    *images;

  ImageInfo
    *ping_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  ping_info=CloneImageInfo(wand->image_info);
  if (filename != (const char *) NULL)
    (void) CopyMagickString(ping_info->filename,filename,MaxTextExtent);
  images=PingImage(ping_info,&wand->exception);
  ping_info=DestroyImageInfo(ping_info);
  if (images == (Image *) NULL)
    return(MagickFalse);
  if (wand->images == (Image *) NULL)
    AppendImageToList(&wand->images,images);
  else
    if (GetNextImageInList(wand->images) == (Image *) NULL)
      AppendImageToList(&wand->images,images);
    else
      InsertImageInList(&wand->images,images);
  wand->images=GetFirstImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k P o s t e r i z e I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickPosterizeImage() reduces the image to a limited number of color level.
%
%  The format of the MagickPosterizeImage method is:
%
%      MagickBooleanType MagickPosterizeImage(MagickWand *wand,
%        const unsigned levels,const MagickBooleanType dither)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o levels: Number of color levels allowed in each channel.  Very low values
%      (2, 3, or 4) have the most visible effect.
%
%    o dither: Set this integer value to something other than zero to dither
%      the mapped image.
%
*/
WandExport MagickBooleanType MagickPosterizeImage(MagickWand *wand,
  const unsigned long levels,const MagickBooleanType dither)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=PosterizeImage(wand->images,levels,dither);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k P r e v i e w I m a g e s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickPreviewImages() tiles 9 thumbnails of the specified image with an
%  image processing operation applied at varying strengths.  This is helpful
%  to quickly pin-point an appropriate parameter for an image processing
%  operation.
%
%  The format of the MagickPreviewImages method is:
%
%      MagickWand *MagickPreviewImages(MagickWand *wand,
%        const PreviewType preview)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o preview: The preview type.
%
*/
WandExport MagickWand *MagickPreviewImages(MagickWand *wand,
  const PreviewType preview)
{
  Image
    *preview_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  preview_image=PreviewImage(wand->images,preview,&wand->exception);
  if (preview_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,preview_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k P r e v i o u s I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickPreviousImage() assocates the previous image in an image list with
%  the magick wand.
%
%  The format of the MagickPreviousImage method is:
%
%      MagickBooleanType MagickPreviousImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickPreviousImage(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if (wand->iterator != MagickFalse)
    {
      wand->iterator=MagickFalse;
      return(MagickTrue);
    }
  if (GetPreviousImageInList(wand->images) == (Image *) NULL)
    {
      wand->iterator=MagickTrue;
      return(MagickFalse);
    }
  wand->images=GetPreviousImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k P r o f i l e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickProfileImage() adds or removes a ICC, IPTC, or generic profile
%  from an image.  If the profile is NULL, it is removed from the image
%  otherwise added.  Use a name of '*' and a profile of NULL to remove all
%  profiles from the image.
%
%  The format of the MagickProfileImage method is:
%
%      MagickBooleanType MagickProfileImage(MagickWand *wand,const char *name,
%        const void *profile,const unsigned long length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o name: Name of profile to add or remove: ICC, IPTC, or generic profile.
%
%    o profile: The profile.
%
%    o length: The length of the profile.
%
*/
WandExport MagickBooleanType MagickProfileImage(MagickWand *wand,
  const char *name,const void *profile,const unsigned long length)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=ProfileImage(wand->images,name,profile,length,MagickTrue);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k Q u a n t i z e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickQuantizeImage() analyzes the colors within a reference image and
%  chooses a fixed number of colors to represent the image.  The goal of the
%  algorithm is to minimize the color difference between the input and output
%  image while minimizing the processing time.
%
%  The format of the MagickQuantizeImage method is:
%
%      MagickBooleanType MagickQuantizeImage(MagickWand *wand,
%        const unsigned long number_colors,const ColorspaceType colorspace,
%        const unsigned long treedepth,const MagickBooleanType dither,
%        const MagickBooleanType measure_error)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o number_colors: The number of colors.
%
%    o colorspace: Perform color reduction in this colorspace, typically
%      RGBColorspace.
%
%    o treedepth: Normally, this integer value is zero or one.  A zero or
%      one tells Quantize to choose a optimal tree depth of Log4(number_colors).%      A tree of this depth generally allows the best representation of the
%      reference image with the least amount of memory and the fastest
%      computational speed.  In some cases, such as an image with low color
%      dispersion (a few number of colors), a value other than
%      Log4(number_colors) is required.  To expand the color tree completely,
%      use a value of 8.
%
%    o dither: A value other than zero distributes the difference between an
%      original image and the corresponding color reduced image to
%      neighboring pixels along a Hilbert curve.
%
%    o measure_error: A value other than zero measures the difference between
%      the original and quantized images.  This difference is the total
%      quantization error.  The error is computed by summing over all pixels
%      in an image the distance squared in RGB space between each reference
%      pixel value and its quantized value.
%
*/
WandExport MagickBooleanType MagickQuantizeImage(MagickWand *wand,
  const unsigned long number_colors,const ColorspaceType colorspace,
  const unsigned long treedepth,const MagickBooleanType dither,
  const MagickBooleanType measure_error)
{
  MagickBooleanType
    status;

  QuantizeInfo
    *quantize_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  quantize_info=CloneQuantizeInfo((QuantizeInfo *) NULL);
  quantize_info->number_colors=number_colors;
  quantize_info->dither=dither;
  quantize_info->tree_depth=treedepth;
  quantize_info->colorspace=colorspace;
  quantize_info->measure_error=measure_error;
  status=QuantizeImage(quantize_info,wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  quantize_info=DestroyQuantizeInfo(quantize_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k Q u a n t i z e I m a g e s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickQuantizeImages() analyzes the colors within a sequence of images and
%  chooses a fixed number of colors to represent the image.  The goal of the
%  algorithm is to minimize the color difference between the input and output
%  image while minimizing the processing time.
%
%  The format of the MagickQuantizeImages method is:
%
%      MagickBooleanType MagickQuantizeImages(MagickWand *wand,
%        const unsigned long number_colors,const ColorspaceType colorspace,
%        const unsigned long treedepth,const MagickBooleanType dither,
%        const MagickBooleanType measure_error)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o number_colors: The number of colors.
%
%    o colorspace: Perform color reduction in this colorspace, typically
%      RGBColorspace.
%
%    o treedepth: Normally, this integer value is zero or one.  A zero or
%      one tells Quantize to choose a optimal tree depth of Log4(number_colors).%      A tree of this depth generally allows the best representation of the
%      reference image with the least amount of memory and the fastest
%      computational speed.  In some cases, such as an image with low color
%      dispersion (a few number of colors), a value other than
%      Log4(number_colors) is required.  To expand the color tree completely,
%      use a value of 8.
%
%    o dither: A value other than zero distributes the difference between an
%      original image and the corresponding color reduced algorithm to
%      neighboring pixels along a Hilbert curve.
%
%    o measure_error: A value other than zero measures the difference between
%      the original and quantized images.  This difference is the total
%      quantization error.  The error is computed by summing over all pixels
%      in an image the distance squared in RGB space between each reference
%      pixel value and its quantized value.
%
*/
WandExport MagickBooleanType MagickQuantizeImages(MagickWand *wand,
  const unsigned long number_colors,const ColorspaceType colorspace,
  const unsigned long treedepth,const MagickBooleanType dither,
  const MagickBooleanType measure_error)
{
  MagickBooleanType
    status;

  QuantizeInfo
    *quantize_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  quantize_info=CloneQuantizeInfo((QuantizeInfo *) NULL);
  quantize_info->number_colors=number_colors;
  quantize_info->dither=dither;
  quantize_info->tree_depth=treedepth;
  quantize_info->colorspace=colorspace;
  quantize_info->measure_error=measure_error;
  status=QuantizeImages(quantize_info,wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  quantize_info=DestroyQuantizeInfo(quantize_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R a d i a l B l u r I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickRadialBlurImage() radial blurs an image.
%
%  The format of the MagickRadialBlurImage method is:
%
%      MagickBooleanType MagickRadialBlurImage(MagickWand *wand,
%        const double angle)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o angle: The angle of the blur in degrees.
%
*/
WandExport MagickBooleanType MagickRadialBlurImage(MagickWand *wand,
  const double angle)
{
  Image
    *blur_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  blur_image=RadialBlurImage(wand->images,angle,&wand->exception);
  if (blur_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,blur_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R a d i a l B l u r I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickRadialBlurImageChannel() radial blurs an image channel.
%
%  The format of the MagickRadialBlurImageChannel method is:
%
%      MagickBooleanType MagickRadialBlurImageChannel(MagickWand *wand,
%        const ChannelType channel,const double angle)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o angle: The angle of the blur in degrees.
%
*/
WandExport MagickBooleanType MagickRadialBlurImageChannel(MagickWand *wand,
  const ChannelType channel,const double angle)
{
  Image
    *blur_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  blur_image=RadialBlurImageChannel(wand->images,channel,angle,
    &wand->exception);
  if (blur_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,blur_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R a i s e I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickRaiseImage() creates a simulated three-dimensional button-like effect
%  by lightening and darkening the edges of the image.  Members width and
%  height of raise_info define the width of the vertical and horizontal
%  edge of the effect.
%
%  The format of the MagickRaiseImage method is:
%
%      MagickBooleanType MagickRaiseImage(MagickWand *wand,
%        const unsigned long width,const unsigned long height,const long x,
%        const long y,const MagickBooleanType raise)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width,height,x,y:  Define the dimensions of the area to raise.
%
%    o raise: A value other than zero creates a 3-D raise effect,
%      otherwise it has a lowered effect.
%
*/
WandExport MagickBooleanType MagickRaiseImage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y,const MagickBooleanType raise)
{
  MagickBooleanType
    status;

  RectangleInfo
    raise_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  raise_info.width=width;
  raise_info.height=height;
  raise_info.x=x;
  raise_info.y=y;
  status=RaiseImage(wand->images,&raise_info,raise);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R e a d I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickReadImage() reads an image or image sequence.
%
%  The format of the MagickReadImage method is:
%
%      MagickBooleanType MagickReadImage(MagickWand *wand,const char *filename)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o filename: The image filename.
%
*/
WandExport MagickBooleanType MagickReadImage(MagickWand *wand,
  const char *filename)
{
  Image
    *images;

  ImageInfo
    *read_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  read_info=CloneImageInfo(wand->image_info);
  if (filename != (const char *) NULL)
    (void) CopyMagickString(read_info->filename,filename,MaxTextExtent);
  images=ReadImage(read_info,&wand->exception);
  read_info=DestroyImageInfo(read_info);
  if (images == (Image *) NULL)
    return(MagickFalse);
  if (wand->images == (Image *) NULL)
    AppendImageToList(&wand->images,images);
  else
    if (GetNextImageInList(wand->images) == (Image *) NULL)
      AppendImageToList(&wand->images,images);
    else
      InsertImageInList(&wand->images,images);
  wand->images=GetFirstImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R e a d I m a g e B l o b                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickReadImageBlob() reads an image or image sequence from a blob.
%
%  The format of the MagickReadImageBlob method is:
%
%      MagickBooleanType MagickReadImageBlob(MagickWand *wand,
%        const void *blob,const size_t length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o blob: The blob.
%
%    o length: The blob length.
%
*/
WandExport MagickBooleanType MagickReadImageBlob(MagickWand *wand,
  const void *blob,const size_t length)
{
  Image
    *images;

  ImageInfo
    *read_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  read_info=CloneImageInfo(wand->image_info);
  SetImageInfoBlob(read_info,blob,length);
  images=ReadImage(read_info,&wand->exception);
  read_info=DestroyImageInfo(read_info);
  if (images == (Image *) NULL)
    return(MagickFalse);
  if (wand->images == (Image *) NULL)
    AppendImageToList(&wand->images,images);
  else
    if (GetNextImageInList(wand->images) == (Image *) NULL)
      AppendImageToList(&wand->images,images);
    else
      InsertImageInList(&wand->images,images);
  wand->images=GetFirstImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R e a d I m a g e F i l e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickReadImageFile() reads an image or image sequence from an open file
%  descriptor.
%
%  The format of the MagickReadImageFile method is:
%
%      MagickBooleanType MagickReadImageFile(MagickWand *wand,FILE *file)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o file: The file descriptor.
%
*/
WandExport MagickBooleanType MagickReadImageFile(MagickWand *wand,FILE *file)
{
  Image
    *images;

  ImageInfo
    *read_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  assert(file != (FILE *) NULL);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  read_info=CloneImageInfo(wand->image_info);
  SetImageInfoFile(read_info,file);
  images=ReadImage(read_info,&wand->exception);
  read_info=DestroyImageInfo(read_info);
  if (images == (Image *) NULL)
    return(MagickFalse);
  if (wand->images == (Image *) NULL)
    AppendImageToList(&wand->images,images);
  else
    if (GetNextImageInList(wand->images) == (Image *) NULL)
      AppendImageToList(&wand->images,images);
    else
      InsertImageInList(&wand->images,images);
  wand->images=GetFirstImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     M a g i c k R e d u c e N o i s e I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickReduceNoiseImage() smooths the contours of an image while still
%  preserving edge information.  The algorithm works by replacing each pixel
%  with its neighbor closest in value.  A neighbor is defined by radius.  Use
%  a radius of 0 and ReduceNoise() selects a suitable radius for you.
%
%  The format of the MagickReduceNoiseImage method is:
%
%      MagickBooleanType MagickReduceNoiseImage(MagickWand *wand,
%        const double radius)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the pixel neighborhood.
%
*/
WandExport MagickBooleanType MagickReduceNoiseImage(MagickWand *wand,
  const double radius)
{
  Image
    *noise_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  noise_image=ReduceNoiseImage(wand->images,radius,&wand->exception);
  if (noise_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,noise_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R e m o v e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickRemoveImage() removes an image from the image list.
%
%  The format of the MagickRemoveImage method is:
%
%      MagickBooleanType MagickRemoveImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o insert: The splice wand.
%
*/
WandExport MagickBooleanType MagickRemoveImage(MagickWand *wand)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  DeleteImageFromList(&wand->images);
  wand->images=GetFirstImageInList(wand->images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R e m o v e I m a g e P r o f i l e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickRemoveImageProfile() removes the named image profile and returns it.
%
%  The format of the MagickRemoveImageProfile method is:
%
%      unsigned char *MagickRemoveImageProfile(MagickWand *wand,
%        const char *name,unsigned long *length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o name: Name of profile to return: ICC, IPTC, or generic profile.
%
%    o length: The length of the profile.
%
*/
WandExport unsigned char *MagickRemoveImageProfile(MagickWand *wand,
  const char *name,unsigned long *length)
{
  const StringInfo
    *profile;

  unsigned char
    *datum;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((unsigned char *) NULL);
    }
  *length=0;
  if (wand->images->profiles == (SplayTreeInfo *) NULL)
    return((unsigned char *) NULL);
  profile=GetImageProfile(wand->images,name);
  if (profile == (StringInfo *) NULL)
    return((unsigned char *) NULL);
  datum=(unsigned char *) AcquireMagickMemory(profile->length);
  if (datum == (unsigned char *) NULL)
    return((unsigned char *) NULL);
  (void) CopyMagickMemory(datum,profile->datum,profile->length);
  (void) RemoveImageProfile(wand->images,name);
  return(datum);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R e s a m p l e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickResampleImage() resample image to desired resolution.
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
%  The format of the MagickResampleImage method is:
%
%      MagickBooleanType MagickResampleImage(MagickWand *wand,
%        const double x_resolution,const double y_resolution,
%        const FilterTypes filter,const double blur)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x_resolution: The new image x resolution.
%
%    o y_resolution: The new image y resolution.
%
%    o filter: Image filter to use.
%
%    o blur: The blur factor where > 1 is blurry, < 1 is sharp.
%
%
*/
WandExport MagickBooleanType MagickResampleImage(MagickWand *wand,
  const double x_resolution,const double y_resolution,const FilterTypes filter,
  const double blur)
{
  Image
    *resample_image;

  unsigned long
    height,
    width;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  width=(unsigned long) (x_resolution*wand->images->columns/
    (wand->images->x_resolution == 0.0 ? 72.0 :
    wand->images->x_resolution)+0.5);
  height=(unsigned long) (y_resolution*wand->images->rows/
    (wand->images->y_resolution == 0.0 ? 72.0 :
    wand->images->y_resolution)+0.5);
  resample_image=ResizeImage(wand->images,width,height,filter,blur,
    &wand->exception);
  if (resample_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,resample_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R e s i z e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickResizeImage() scales an image to the desired dimensions with one of
%  these filters:
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
%  The format of the MagickResizeImage method is:
%
%      MagickBooleanType MagickResizeImage(MagickWand *wand,
%        const unsigned long columns,const unsigned long rows,
%        const FilterTypes filter,const double blur)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
%    o filter: Image filter to use.
%
%    o blur: The blur factor where > 1 is blurry, < 1 is sharp.
%
%
*/
WandExport MagickBooleanType MagickResizeImage(MagickWand *wand,
  const unsigned long columns,const unsigned long rows,const FilterTypes filter,
  const double blur)
{
  Image
    *resize_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  resize_image=ResizeImage(wand->images,columns,rows,filter,blur,
    &wand->exception);
  if (resize_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,resize_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R o l l I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickRollImage() offsets an image as defined by x and y.
%
%  The format of the MagickRollImage method is:
%
%      MagickBooleanType MagickRollImage(MagickWand *wand,const long x,
%        const unsigned long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The x offset.
%
%    o y: The y offset.
%
%
*/
WandExport MagickBooleanType MagickRollImage(MagickWand *wand,
  const long x,const long y)
{
  Image
    *roll_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  roll_image=RollImage(wand->images,x,y,&wand->exception);
  if (roll_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,roll_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k R o t a t e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickRotateImage() rotates an image the specified number of degrees. Empty
%  triangles left over from rotating the image are filled with the
%  background color.
%
%  The format of the MagickRotateImage method is:
%
%      MagickBooleanType MagickRotateImage(MagickWand *wand,
%        const PixelWand *background,const double degrees)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o background: The background pixel wand.
%
%    o degrees: The number of degrees to rotate the image.
%
%
*/
WandExport MagickBooleanType MagickRotateImage(MagickWand *wand,
  const PixelWand *background,const double degrees)
{
  Image
    *rotate_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelGetQuantumColor(background,&wand->images->background_color);
  rotate_image=RotateImage(wand->images,degrees,&wand->exception);
  if (rotate_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,rotate_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S a m p l e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSampleImage() scales an image to the desired dimensions with pixel
%  sampling.  Unlike other scaling methods, this method does not introduce
%  any additional color into the scaled image.
%
%  The format of the MagickSampleImage method is:
%
%      MagickBooleanType MagickSampleImage(MagickWand *wand,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
%
*/
WandExport MagickBooleanType MagickSampleImage(MagickWand *wand,
  const unsigned long columns,const unsigned long rows)
{
  Image
    *sample_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  sample_image=SampleImage(wand->images,columns,rows,&wand->exception);
  if (sample_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,sample_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S c a l e I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickScaleImage() scales the size of an image to the given dimensions.
%
%  The format of the MagickScaleImage method is:
%
%      MagickBooleanType MagickScaleImage(MagickWand *wand,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
%
*/
WandExport MagickBooleanType MagickScaleImage(MagickWand *wand,
  const unsigned long columns,const unsigned long rows)
{
  Image
    *scale_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  scale_image=ScaleImage(wand->images,columns,rows,&wand->exception);
  if (scale_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,scale_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e p a r a t e I m a g e C h a n n e l                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickChannelImage() separates a channel from the image and returns a
%  grayscale image.  A channel is a particular color component of each pixel
%  in the image.
%
%  The format of the MagickChannelImage method is:
%
%      MagickBooleanType MagickSeparateImageChannel(MagickWand *wand,
%        const ChannelType channel)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
*/
WandExport MagickBooleanType MagickSeparateImageChannel(MagickWand *wand,
  const ChannelType channel)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=SeparateImageChannel(wand->images,channel);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     M a g i c k S e p i a T o n e I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSolarizeImage() applies a special effect to the image, similar to the
%  effect achieved in a photo darkroom by sepia toning.  Threshold ranges from
%  0 to QuantumRange and is a measure of the extent of the sepia toning.  A
%  threshold of 80% is a good starting point for a reasonable tone.
%
%  The format of the MagickSolarizeImage method is:
%
%      MagickBooleanType MagickSolarizeImage(MagickWand *wand,
%        const double threshold)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o threshold:  Define the extent of the sepia toning.
%
*/
WandExport MagickBooleanType MagickSepiaToneImage(MagickWand *wand,
  const double threshold)
{
  Image
    *sepia_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  sepia_image=SepiaToneImage(wand->images,threshold,&wand->exception);
  if (sepia_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,sepia_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImage() replaces the last image returned by MagickSetImageIndex(),
%  MagickNextImage(), MagickPreviousImage() with the images from the specified
%  wand.
%
%  The format of the MagickSetImage method is:
%
%      MagickBooleanType MagickSetImage(MagickWand *wand,
%        const MagickWand *set_wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o set_wand: The set_wand wand.
%
*/
WandExport MagickBooleanType MagickSetImage(MagickWand *wand,
  const MagickWand *set_wand)
{
  Image
    *images;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  assert(set_wand != (MagickWand *) NULL);
  assert(set_wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),set_wand->name);
  if (set_wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  images=CloneImageList(set_wand->images,&wand->exception);
  if (images == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,images);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e A t t r i b u t e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageAttribute() associates an attribute with an image.
%
%  The format of the MagickSetImageAttribute method is:
%
%      MagickBooleanType MagickSetImageAttribute(MagickWand *wand,
%        const char *key,const char *attribute)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o key: The key.
%
%    o value: The value.
%
*/
WandExport MagickBooleanType MagickSetImageAttribute(MagickWand *wand,
  const char *key,const char *value)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=SetImageAttribute(wand->images,key,value);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e B a c k g r o u n d C o l o r                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageBackgroundColor() sets the image background color.
%
%  The format of the MagickSetImageBackgroundColor method is:
%
%      MagickBooleanType MagickSetImageBackgroundColor(MagickWand *wand,
%        const PixelWand *background)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o background: The background pixel wand.
%
*/
WandExport MagickBooleanType MagickSetImageBackgroundColor(MagickWand *wand,
  const PixelWand *background)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelGetQuantumColor(background,&wand->images->background_color);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e G a m m a                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageBias() sets the image bias for any method that convolves an
%  image (e.g. MagickConvolveImage()).
%
%  The format of the MagickSetImageBias method is:
%
%      MagickBooleanType MagickSetImageBias(MagickWand *wand,
%        const double bias)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o bias: The image bias.
%
*/
WandExport MagickBooleanType MagickSetImageBias(MagickWand *wand,
  const double bias)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->bias=bias;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e B l u e P r i m a r y                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageBluePrimary() sets the image chromaticity blue primary point.
%
%  The format of the MagickSetImageBluePrimary method is:
%
%      MagickBooleanType MagickSetImageBluePrimary(MagickWand *wand,
%        const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The blue primary x-point.
%
%    o y: The blue primary y-point.
%
%
*/
WandExport MagickBooleanType MagickSetImageBluePrimary(MagickWand *wand,
  const double x,const double y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->chromaticity.blue_primary.x=x;
  wand->images->chromaticity.blue_primary.y=y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e B o r d e r C o l o r                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageBorderColor() sets the image border color.
%
%  The format of the MagickSetImageBorderColor method is:
%
%      MagickBooleanType MagickSetImageBorderColor(MagickWand *wand,
%        const PixelWand *border)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o border: The border pixel wand.
%
*/
WandExport MagickBooleanType MagickSetImageBorderColor(MagickWand *wand,
  const PixelWand *border)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelGetQuantumColor(border,&wand->images->border_color);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e C h a n n e l D e p t h                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageChannelDepth() sets the depth of a particular image channel.
%
%  The format of the MagickSetImageChannelDepth method is:
%
%      MagickBooleanType MagickSetImageChannelDepth(MagickWand *wand,
%        const ChannelType channel,const unsigned long depth)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o depth: The image depth in bits.
%
*/
WandExport MagickBooleanType MagickSetImageChannelDepth(MagickWand *wand,
  const ChannelType channel,const unsigned long depth)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(SetImageChannelDepth(wand->images,channel,depth));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e C o l o r m a p C o l o r                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageColormapColor() sets the color of the specified colormap
%  index.
%
%  The format of the MagickSetImageColormapColor method is:
%
%      MagickBooleanType MagickSetImageColormapColor(MagickWand *wand,
%        const unsigned long index,const PixelWand *color)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o index: The offset into the image colormap.
%
%    o color: Return the colormap color in this wand.
%
*/
WandExport MagickBooleanType MagickSetImageColormapColor(MagickWand *wand,
  const unsigned long index,const PixelWand *color)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if ((wand->images->colormap == (PixelPacket *) NULL) ||
      (index >= wand->images->colors))
    ThrowWandException(WandError,"InvalidColormapIndex",strerror(errno));
  PixelGetQuantumColor(color,wand->images->colormap+index);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e C o l o r s p a c e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageColorspace() sets the image colorspace.
%
%  The format of the MagickSetImageColorspace method is:
%
%      MagickBooleanType MagickSetImageColorspace(MagickWand *wand,
%        const ColorspaceType colorspace)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o colorspace: The image colorspace:   UndefinedColorspace, RGBColorspace,
%      GRAYColorspace, TransparentColorspace, OHTAColorspace, XYZColorspace,
%      YCbCrColorspace, YCCColorspace, YIQColorspace, YPbPrColorspace,
%      YPbPrColorspace, YUVColorspace, CMYKColorspace, sRGBColorspace,
%      HSLColorspace, or HWBColorspace.
%
*/
WandExport MagickBooleanType MagickSetImageColorspace(MagickWand *wand,
  const ColorspaceType colorspace)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(SetImageColorspace(wand->images,colorspace));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e C o m p o s e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageCompose() sets the image composite operator, useful for
%  specifying how to composite the image thumbnail when using the
%  MagickMontageImage() method.
%
%  The format of the MagickSetImageCompose method is:
%
%      MagickBooleanType MagickSetImageCompose(MagickWand *wand,
%        const CompositeOperator compose)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o compose: The image composite operator.
%
*/
WandExport MagickBooleanType MagickSetImageCompose(MagickWand *wand,
  const CompositeOperator compose)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->compose=compose;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e C o m p r e s s i o n                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageCompression() sets the image compression.
%
%  The format of the MagickSetImageCompression method is:
%
%      MagickBooleanType MagickSetImageCompression(MagickWand *wand,
%        const CompressionType compression)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o compression: The image compression type.
%
*/
WandExport MagickBooleanType MagickSetImageCompression(MagickWand *wand,
  const CompressionType compression)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->compression=compression;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e C o m p r e s s i o n Q u a l i t y           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageCompressionQuality() sets the image compression quality.
%
%  The format of the MagickSetImageCompressionQuality method is:
%
%      MagickBooleanType MagickSetImageCompressionQuality(MagickWand *wand,
%        const unsigned long quality)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o quality: The image compression tlityype.
%
*/
WandExport MagickBooleanType MagickSetImageCompressionQuality(MagickWand *wand,
  const unsigned long quality)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->quality=quality;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e D e l a y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageDelay() sets the image delay.
%
%  The format of the MagickSetImageDelay method is:
%
%      MagickBooleanType MagickSetImageDelay(MagickWand *wand,
%        const unsigned long delay)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o delay: The image delay in ticks-per-second units.
%
*/
WandExport MagickBooleanType MagickSetImageDelay(MagickWand *wand,
  const unsigned long delay)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->delay=delay;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e D e p t h                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageDepth() sets the image depth.
%
%  The format of the MagickSetImageDepth method is:
%
%      MagickBooleanType MagickSetImageDepth(MagickWand *wand,
%        const unsigned long depth)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o depth: The image depth in bits: 8, 16, or 32.
%
*/
WandExport MagickBooleanType MagickSetImageDepth(MagickWand *wand,
  const unsigned long depth)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(SetImageDepth(wand->images,depth));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e D i s p o s e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageDispose() sets the image disposal method.
%
%  The format of the MagickSetImageDispose method is:
%
%      MagickBooleanType MagickSetImageDispose(MagickWand *wand,
%        const DisposeType dispose)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o dispose: The image disposeal type.
%
*/
WandExport MagickBooleanType MagickSetImageDispose(MagickWand *wand,
  const DisposeType dispose)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->dispose=dispose;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e E x t e n t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageExtent() sets the image size (i.e. columns & rows).
%
%  The format of the MagickSetImageExtent method is:
%
%      MagickBooleanType MagickSetImageExtent(MagickWand *wand,
%        const unsigned long columns,const unsigned rows)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns:  The image width in pixels.
%
%    o rows:  The image height in pixels.
%
*/
WandExport MagickBooleanType MagickSetImageExtent(MagickWand *wand,
  const unsigned long columns,const unsigned long rows)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(SetImageExtent(wand->images,columns,rows));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e F i l e n a m e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageFilename() sets the filename of a particular image in a
%  sequence.
%
%  The format of the MagickSetImageFilename method is:
%
%      MagickBooleanType MagickSetImageFilename(MagickWand *wand,
%        const char *filename)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o filename: The image filename.
%
*/
WandExport MagickBooleanType MagickSetImageFilename(MagickWand *wand,
  const char *filename)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if (filename != (const char *) NULL)
    (void) CopyMagickString(wand->images->filename,filename,MaxTextExtent);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e F o r m a t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageFormat() sets the format of a particular image in a
%  sequence.
%
%  The format of the MagickSetImageFormat method is:
%
%      MagickBooleanType MagickSetImageFormat(MagickWand *wand,
%        const char *format)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o format: The image format.
%
*/
WandExport MagickBooleanType MagickSetImageFormat(MagickWand *wand,
  const char *format)
{
  const MagickInfo
    *magick_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if ((format == (char *) NULL) || (*format == '\0'))
    {
      *wand->images->magick='\0';
      return(MagickTrue);
    }
  magick_info=GetMagickInfo(format,&wand->exception);
  if (magick_info == (const MagickInfo *) NULL)
    return(MagickFalse);
  (void) SetExceptionInfo(&wand->exception,UndefinedException);
  (void) CopyMagickString(wand->images->magick,format,MaxTextExtent);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e G a m m a                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageGamma() sets the image gamma.
%
%  The format of the MagickSetImageGamma method is:
%
%      MagickBooleanType MagickSetImageGamma(MagickWand *wand,
%        const double gamma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o gamma: The image gamma.
%
*/
WandExport MagickBooleanType MagickSetImageGamma(MagickWand *wand,
  const double gamma)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->gamma=gamma;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e G r e e n P r i m a r y                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageGreenPrimary() sets the image chromaticity green primary
%  point.
%
%  The format of the MagickSetImageGreenPrimary method is:
%
%      MagickBooleanType MagickSetImageGreenPrimary(MagickWand *wand,
%        const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The green primary x-point.
%
%    o y: The green primary y-point.
%
%
*/
WandExport MagickBooleanType MagickSetImageGreenPrimary(MagickWand *wand,
  const double x,const double y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->chromaticity.green_primary.x=x;
  wand->images->chromaticity.green_primary.y=y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e I n d e x                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageIndex() set the current image to the position of the list
%  specified with the index parameter.
%
%  The format of the MagickSetImageIndex method is:
%
%      MagickBooleanType MagickSetImageIndex(MagickWand *wand,const long index)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o index: The scene number.
%
*/
WandExport MagickBooleanType MagickSetImageIndex(MagickWand *wand,
  const long index)
{
  Image
    *image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return(MagickFalse);
  image=GetImageFromList(wand->images,index);
  if (image == (Image *) NULL)
    {
      InheritException(&wand->exception,&wand->images->exception);
      return(MagickFalse);
    }
  wand->images=image;
  wand->iterator=MagickFalse;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e I n t e r l a c e S c h e m e                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageInterlaceScheme() sets the image compression.
%
%  The format of the MagickSetImageInterlaceScheme method is:
%
%      MagickBooleanType MagickSetImageInterlaceScheme(MagickWand *wand,
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
WandExport MagickBooleanType MagickSetImageInterlaceScheme(MagickWand *wand,
  const InterlaceType interlace_scheme)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->interlace=interlace_scheme;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e I t e r a t i o n s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageIterations() sets the image iterations.
%
%  The format of the MagickSetImageIterations method is:
%
%      MagickBooleanType MagickSetImageIterations(MagickWand *wand,
%        const unsigned long iterations)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o delay: The image delay in 1/100th of a second.
%
*/
WandExport MagickBooleanType MagickSetImageIterations(MagickWand *wand,
  const unsigned long iterations)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->iterations=iterations;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e M a t t e C o l o r                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageMatteColor() sets the image matte color.
%
%  The format of the MagickSetImageMatteColor method is:
%
%      MagickBooleanType MagickSetImageMatteColor(MagickWand *wand,
%        const PixelWand *matte)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o matte: The matte pixel wand.
%
*/
WandExport MagickBooleanType MagickSetImageMatteColor(MagickWand *wand,
  const PixelWand *matte)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelGetQuantumColor(matte,&wand->images->matte_color);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   M a g i c k S e t I m a g e O p t i o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageOption() associates one or options with a particular image
%  format (.e.g MagickSetImageOption(wand,"jpeg","perserve","yes").
%
%  The format of the MagickSetImageOption method is:
%
%      MagickBooleanType MagickSetImageOption(MagickWand *wand,
%        const char *format,const char *key,const char *value)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o format: The image format.
%
%    o key:  The key.
%
%    o value:  The value.
%
*/
WandExport MagickBooleanType MagickSetImageOption(MagickWand *wand,
  const char *format,const char *key,const char *value)
{
  char
    option[MaxTextExtent];

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  (void) FormatMagickString(option,MaxTextExtent,"%s:%s=%s",format,key,value);
  return(DefineImageOption(wand->image_info,option));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e P a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImagePage() sets the page geometry of the image.
%
%  The format of the MagickSetImagePage method is:
%
%      MagickBooleanType MagickSetImagePage(MagickWand *wand,
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
WandExport MagickBooleanType MagickSetImagePage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->page.width=width;
  wand->images->page.height=height;
  wand->images->page.x=x;
  wand->images->page.y=y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e P i x e l s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImagePixels() accepts pixel data and stores it in the image at the
%  location you specify.  The method returns MagickFalse on success otherwise
%  MagickTrue if an error is encountered.  The pixel data can be either char,
%  short int, int, long, float, or double in the order specified by map.
%
%  Suppose your want want to upload the first scanline of a 640x480 image from
%  character data in red-green-blue order:
%
%      MagickSetImagePixels(wand,0,0,0,640,1,"RGB",CharPixel,pixels);
%
%  The format of the MagickSetImagePixels method is:
%
%      MagickBooleanType MagickSetImagePixels(MagickWand *wand,
%        const long x,const long y,const unsigned long columns,
%        const unsigned long rows,const char *map,const StorageType storage,
%        const void *pixels)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x, y, columns, rows:  These values define the perimeter of a region
%      of pixels you want to define.
%
%    o map:  This string reflects the expected ordering of the pixel array.
%      It can be any combination or order of R = red, G = green, B = blue,
%      A = alpha (0 is transparent), O = opacity (0 is opaque), C = cyan,
%      Y = yellow, M = magenta, K = black, I = intensity (for grayscale),
%      P = pad.
%
%    o storage: Define the data type of the pixels.  Float and double types are
%      expected to be normalized [0..1] otherwise [0..QuantumRange].  Choose from
%      these types: CharPixel, ShortPixel, IntegerPixel, LongPixel, FloatPixel,
%      or DoublePixel.
%
%    o pixels: This array of values contain the pixel components as defined by
%      map and type.  You must preallocate this array where the expected
%      length varies depending on the values of width, height, map, and type.
%
%
*/
WandExport MagickBooleanType MagickSetImagePixels(MagickWand *wand,
  const long x,const long y,const unsigned long columns,
  const unsigned long rows,const char *map,const StorageType storage,
  const void *pixels)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=ImportImagePixels(wand->images,x,y,columns,rows,map,storage,pixels);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t P r o f i l e I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageProfile() adds a named profile to the magick wand.  If a
%  profile with the same name already exists, it is replaced.  This method
%  differs from the MagickProfileImage() method in that it does not apply any
%  CMS color profiles.
%
%  The format of the MagickSetImageProfile method is:
%
%      MagickBooleanType MagickSetImageProfile(MagickWand *wand,
%        const char *name,const void *profile,const unsigned long length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o name: Name of profile to add or remove: ICC, IPTC, or generic profile.
%
%    o profile: The profile.
%
%    o length: The length of the profile.
%
*/
WandExport MagickBooleanType MagickSetImageProfile(MagickWand *wand,
  const char *name,const void *profile,const unsigned long length)
{
  MagickBooleanType
    status;

  StringInfo
    *profile_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  profile_info=AcquireStringInfo((size_t) length);
  SetStringInfoDatum(profile_info,(unsigned char *) profile);
  status=SetImageProfile(wand->images,name,profile_info);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e P r o g r e s s M o n i t o r                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageProgressMonitor() sets the wand image progress monitor to the
%  specified method and returns the previous progress monitor if any.  The
%  progress monitor method looks like this:
%
%    MagickBooleanType MagickProgressMonitor(const char *text,
%      const MagickOffsetType offset,const MagickSizeType span,
%      void *client_data)
%
%  If the progress monitor returns MagickFalse, the current operation is
%  interrupted.
%
%  The format of the MagickSetImageProgressMonitor method is:
%
%      MagickProgressMonitor MagickSetImageProgressMonitor(MagickWand *wand
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
WandExport MagickProgressMonitor MagickSetImageProgressMonitor(MagickWand *wand,
  const MagickProgressMonitor progress_monitor,void *client_data)
{
  MagickProgressMonitor
    previous_monitor;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((MagickProgressMonitor) NULL);
    }
  previous_monitor=SetImageProgressMonitor(wand->images,
    progress_monitor,client_data);
  return(previous_monitor);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e R e d P r i m a r y                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageRedPrimary() sets the image chromaticity red primary point.
%
%  The format of the MagickSetImageRedPrimary method is:
%
%      MagickBooleanType MagickSetImageRedPrimary(MagickWand *wand,
%        const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The red primary x-point.
%
%    o y: The red primary y-point.
%
*/
WandExport MagickBooleanType MagickSetImageRedPrimary(MagickWand *wand,
  const double x,const double y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->chromaticity.red_primary.x=x;
  wand->images->chromaticity.red_primary.y=y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e R e n d e r i n g I n t e n t                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageRenderingIntent() sets the image rendering intent.
%
%  The format of the MagickSetImageRenderingIntent method is:
%
%      MagickBooleanType MagickSetImageRenderingIntent(MagickWand *wand,
%        const RenderingIntent rendering_intent)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o rendering_intent: The image rendering intent: UndefinedIntent,
%      SaturationIntent, PerceptualIntent, AbsoluteIntent, or RelativeIntent.
%
*/
WandExport MagickBooleanType MagickSetImageRenderingIntent(MagickWand *wand,
  const RenderingIntent rendering_intent)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->rendering_intent=rendering_intent;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e R e s o l u t i o n                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageResolution() sets the image resolution.
%
%  The format of the MagickSetImageResolution method is:
%
%      MagickBooleanType MagickSetImageResolution(MagickWand *wand,
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
WandExport MagickBooleanType MagickSetImageResolution(MagickWand *wand,
  const double x_resolution,const double y_resolution)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->x_resolution=x_resolution;
  wand->images->y_resolution=y_resolution;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e S c e n e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageScene() sets the image scene.
%
%  The format of the MagickSetImageScene method is:
%
%      MagickBooleanType MagickSetImageScene(MagickWand *wand,
%        const unsigned long scene)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o delay: The image scene number.
%
*/
WandExport MagickBooleanType MagickSetImageScene(MagickWand *wand,
  const unsigned long scene)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->scene=scene;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e T i c k s P e r S e c o n d                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageTicksPerSecond() sets the image ticks-per-second.
%
%  The format of the MagickSetImageTicksPerSecond method is:
%
%      MagickBooleanType MagickSetImageTicksPerSecond(MagickWand *wand,
%        const unsigned long ticks_per-second)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o ticks_per_second: The units to use for the image delay.
%
*/
WandExport MagickBooleanType MagickSetImageTicksPerSecond(MagickWand *wand,
  const unsigned long ticks_per_second)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->ticks_per_second=ticks_per_second;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e T y p e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageType() sets the image type.
%
%  The format of the MagickSetImageType method is:
%
%      MagickBooleanType MagickSetImageType(MagickWand *wand,
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
WandExport MagickBooleanType MagickSetImageType(MagickWand *wand,
  const ImageType image_type)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(SetImageType(wand->images,image_type));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e U n i t s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageUnits() sets the image units of resolution.
%
%  The format of the MagickSetImageUnits method is:
%
%      MagickBooleanType MagickSetImageUnits(MagickWand *wand,
%        const ResolutionType units)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o units: The image units of resolution : UndefinedResolution,
%      PixelsPerInchResolution, or PixelsPerCentimeterResolution.
%
*/
WandExport MagickBooleanType MagickSetImageUnits(MagickWand *wand,
  const ResolutionType units)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->units=units;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e V i r t u a l P i x e l M e t h o d           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageVirtualPixelMethod() sets the image virtual pixel method.
%
%  The format of the MagickSetImageVirtualPixelMethod method is:
%
%      MagickBooleanType MagickSetImageVirtualPixelMethod(MagickWand *wand,
%        const VirtualPixelMethod method)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o method: The image virtual pixel method : UndefinedVirtualPixelMethod,
%      ConstantVirtualPixelMethod,  EdgeVirtualPixelMethod,
%      MirrorVirtualPixelMethod, or TileVirtualPixelMethod.
%
*/
WandExport MagickBooleanType MagickSetImageVirtualPixelMethod(MagickWand *wand,
  const VirtualPixelMethod method)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  return(SetImageVirtualPixelMethod(wand->images,method));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S e t I m a g e W h i t e P o i n t                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSetImageWhitePoint() sets the image chromaticity white point.
%
%  The format of the MagickSetImageWhitePoint method is:
%
%      MagickBooleanType MagickSetImageWhitePoint(MagickWand *wand,
%        const double x,const double y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x: The white x-point.
%
%    o y: The white y-point.
%
%
*/
WandExport MagickBooleanType MagickSetImageWhitePoint(MagickWand *wand,
  const double x,const double y)
{
  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->chromaticity.white_point.x=x;
  wand->images->chromaticity.white_point.y=y;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S h a d o w I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickShadowImage() simulates an image shadow.
%
%  The format of the MagickShadowImage method is:
%
%      MagickBooleanType MagickShadowImage(MagickWand *wand,
%        const double radius,const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
*/
WandExport MagickBooleanType MagickShadowImage(MagickWand *wand,
  const double opacity,const double sigma,const long x,const long y)
{
  Image
    *shadow_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  shadow_image=ShadowImage(wand->images,opacity,sigma,x,y,&wand->exception);
  if (shadow_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,shadow_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S h a r p e n I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSharpenImage() sharpens an image.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, the radius should be larger than sigma.  Use a
%  radius of 0 and SharpenImage() selects a suitable radius for you.
%
%  The format of the MagickSharpenImage method is:
%
%      MagickBooleanType MagickSharpenImage(MagickWand *wand,
%        const double radius,const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
*/
WandExport MagickBooleanType MagickSharpenImage(MagickWand *wand,
  const double radius,const double sigma)
{
  Image
    *sharpen_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  sharpen_image=SharpenImage(wand->images,radius,sigma,&wand->exception);
  if (sharpen_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,sharpen_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S h a r p e n I m a g e C h a n n e l                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSharpenImageChannel() sharpens one or more image channels.  We
%  convolve the image cnannel with a gaussian operator of the given radius and
%  standard deviation (sigma).  For reasonable results, the radius should be
%  larger than sigma.  Use a radius of 0 and GaussinSharpenImageChannel()
%  selects a suitable radius for you.
%
%  The format of the MagickSharpenImageChannel method is:
%
%      MagickBooleanType MagickSharpenImageChannel(MagickWand *wand,
%        const ChannelType channel,const double radius,const double sigma)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o radius: The radius of the , in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the , in pixels.
%
*/
WandExport MagickBooleanType MagickSharpenImageChannel(MagickWand *wand,
  const ChannelType channel,const double radius,const double sigma)
{
  Image
    *sharp_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  sharp_image=SharpenImageChannel(wand->images,channel,radius,sigma,
    &wand->exception);
  if (sharp_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,sharp_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S h a v e I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickShaveImage() shaves pixels from the image edges.  It allocates the
%  memory necessary for the new Image structure and returns a pointer to the
%  new image.
%
%  The format of the MagickShaveImage method is:
%
%      MagickBooleanType MagickShaveImage(MagickWand *wand,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
%
*/
WandExport MagickBooleanType MagickShaveImage(MagickWand *wand,
  const unsigned long columns,const unsigned long rows)
{
  Image
    *shave_image;

  RectangleInfo
    shave_info;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  shave_info.width=columns;
  shave_info.height=rows;
  shave_info.x=0;
  shave_info.y=0;
  shave_image=ShaveImage(wand->images,&shave_info,&wand->exception);
  if (shave_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,shave_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S h e a r I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickShearImage() slides one edge of an image along the X or Y axis,
%  creating a parallelogram.  An X direction shear slides an edge along the X
%  axis, while a Y direction shear slides an edge along the Y axis.  The amount
%  of the shear is controlled by a shear angle.  For X direction shears, x_shear
%  is measured relative to the Y axis, and similarly, for Y direction shears
%  y_shear is measured relative to the X axis.  Empty triangles left over from
%  shearing the image are filled with the background color.
%
%  The format of the MagickShearImage method is:
%
%      MagickBooleanType MagickShearImage(MagickWand *wand,
%        const PixelWand *background,const double x_shear,onst double y_shear)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o background: The background pixel wand.
%
%    o x_shear: The number of degrees to shear the image.
%
%    o y_shear: The number of degrees to shear the image.
%
%
*/
WandExport MagickBooleanType MagickShearImage(MagickWand *wand,
  const PixelWand *background,const double x_shear,const double y_shear)
{
  Image
    *shear_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  PixelGetQuantumColor(background,&wand->images->background_color);
  shear_image=ShearImage(wand->images,x_shear,y_shear,&wand->exception);
  if (shear_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,shear_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S i g m o i d a l C o n t r a s t I m a g e                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSigmoidalContrastImage() adjusts the contrast of an image with a
%  non-linear sigmoidal contrast algorithm.  Increase the contrast of the image
%  using a sigmoidal transfer function without saturating highlights or
%  shadows.  Contrast indicates how much to increase the contrast (0 is none;
%  3 is typical; 20 is pushing it); mid-point indicates where midtones fall in
%  the resultant image (0 is white; 50% is middle-gray; 100% is black).  Set
%  sharpen to MagickTrue to increase the image contrast otherwise the contrast
%  is reduced.
%
%  The format of the MagickSigmoidalContrastImage method is:
%
%      MagickBooleanType MagickSigmoidalContrastImage(MagickWand *wand,
%        const MagickBooleanType sharpen,const double contrast,
%        const double midpoint)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o sharpen: Increase or decrease image contrast.
%
%    o contrast: control the "shoulder" of the contast curve.
%
%    o midpoint: control the "toe" of the contast curve.
%
*/
WandExport MagickBooleanType MagickSigmoidalContrastImage(MagickWand *wand,
  const MagickBooleanType sharpen,const double contrast,const double midpoint)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=SigmoidalContrastImage(wand->images,sharpen,contrast,midpoint);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S i g m o i d a l C o n t r a s t I m a g e C h a n n e l     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSigmoidalContrastImageChannel() adjusts the contrast of an image
%  channel with a non-linear sigmoidal contrast algorithm.  Increase the
%  contrast of the image using a sigmoidal transfer function without
%  saturating highlights or shadows.  Contrast indicates how much to increase
%  the contrast (0 is none; 3 is typical; 20 is pushing it); mid-point
%  indicates where midtones fall in the resultant image (0 is white; 50% is
%  middle-gray; 100% is black).  Set sharpen to MagickTrue to increase the
%  image contrast otherwise the contrast is reduced.
%
%  The format of the MagickSigmoidalContrastImageChannel method is:
%
%      MagickBooleanType MagickSigmoidalContrastImageChannel(MagickWand *wand,
%        const ChannelType channel,const MagickBooleanType sharpen,
%        const double alpha,const double beta)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to level: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
%
%    o sharpen: Increase or decrease image contrast.
%
%    o alpha: control the "shoulder" of the contast curve.
%
%    o beta: control the "toe" of the contast curve.
%
%
*/
WandExport MagickBooleanType MagickSigmoidalContrastImageChannel(
  MagickWand *wand,const ChannelType channel,const MagickBooleanType sharpen,
  const double alpha,const double beta)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=SigmoidalContrastImageChannel(wand->images,channel,sharpen,alpha,beta);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M a g i c k S o l a r i z e I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSolarizeImage() applies a special effect to the image, similar to the
%  effect achieved in a photo darkroom by selectively exposing areas of photo
%  sensitive paper to light.  Threshold ranges from 0 to QuantumRange and is a
%  measure of the extent of the solarization.
%
%  The format of the MagickSolarizeImage method is:
%
%      MagickBooleanType MagickSolarizeImage(MagickWand *wand,
%        const double threshold)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o threshold:  Define the extent of the solarization.
%
*/
WandExport MagickBooleanType MagickSolarizeImage(MagickWand *wand,
  const double threshold)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=SolarizeImage(wand->images,threshold);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S p l i c e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSpliceImage() splices a solid color intop the image.
%
%  The format of the MagickSpliceImage method is:
%
%      MagickBooleanType MagickSpliceImage(MagickWand *wand,
%        const unsigned long width,const unsigned long height,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o width: The region width.
%
%    o height: The region height.
%
%    o x: The region x offset.
%
%    o y: The region y offset.
%
%
*/
WandExport MagickBooleanType MagickSpliceImage(MagickWand *wand,
  const unsigned long width,const unsigned long height,const long x,
  const long y)
{
  Image
    *splice_image;

  RectangleInfo
    splice;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  splice.width=width;
  splice.height=height;
  splice.x=x;
  splice.y=y;
  splice_image=SpliceImage(wand->images,&splice,&wand->exception);
  if (splice_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,splice_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S p r e a d I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSpreadImage() is a special effects method that randomly displaces each
%  pixel in a block defined by the radius parameter.
%
%  The format of the MagickSpreadImage method is:
%
%      MagickBooleanType MagickSpreadImage(MagickWand *wand,const double radius)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o radius:  Choose a random pixel in a neighborhood of this extent.
%
*/
WandExport MagickBooleanType MagickSpreadImage(MagickWand *wand,
  const double radius)
{
  Image
    *spread_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  spread_image=SpreadImage(wand->images,radius,&wand->exception);
  if (spread_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,spread_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S t e g a n o I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSteganoImage() hides a digital watermark within the image.
%  Recover the hidden watermark later to prove that the authenticity of
%  an image.  Offset defines the start position within the image to hide
%  the watermark.
%
%  The format of the MagickSteganoImage method is:
%
%      MagickWand *MagickSteganoImage(MagickWand *wand,
%        const MagickWand *watermark_wand,const long offset)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o watermark_wand: The watermark wand.
%
%    o offset: Start hiding at this offset into the image.
%
*/
WandExport MagickWand *MagickSteganoImage(MagickWand *wand,
  const MagickWand *watermark_wand,const long offset)
{
  Image
    *stegano_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) ||
      (watermark_wand->images == (Image *) NULL))
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((MagickWand *) NULL);
    }
  wand->images->offset=offset;
  stegano_image=SteganoImage(wand->images,watermark_wand->images,
    &wand->exception);
  if (stegano_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,stegano_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S t e r e o I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickStereoImage() composites two images and produces a single image that
%  is the composite of a left and right image of a stereo pair
%
%  The format of the MagickStereoImage method is:
%
%      MagickWand *MagickStereoImage(MagickWand *wand,
%        const MagickWand *offset_wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o offset_wand: Another image wand.
%
*/
WandExport MagickWand *MagickStereoImage(MagickWand *wand,
  const MagickWand *offset_wand)
{
  Image
    *stereo_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) ||
      (offset_wand->images == (Image *) NULL))
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((MagickWand *) NULL);
    }
  stereo_image=StereoImage(wand->images,offset_wand->images,&wand->exception);
  if (stereo_image == (Image *) NULL)
    return((MagickWand *) NULL);
  return(CloneMagickWandFromImages(wand,stereo_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S t r i p I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickStripImage() strips an image of all profiles and comments.
%
%  The format of the MagickStripImage method is:
%
%      MagickBooleanType MagickStripImage(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport MagickBooleanType MagickStripImage(MagickWand *wand)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=StripImage(wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k S w i r l I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSwirlImage() swirls the pixels about the center of the image, where
%  degrees indicates the sweep of the arc through which each pixel is moved.
%  You get a more dramatic effect as the degrees move from 1 to 360.
%
%  The format of the MagickSwirlImage method is:
%
%      MagickBooleanType MagickSwirlImage(MagickWand *wand,const double degrees)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o degrees: Define the tightness of the swirling effect.
%
*/
WandExport MagickBooleanType MagickSwirlImage(MagickWand *wand,
  const double degrees)
{
  Image
    *swirl_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  swirl_image=SwirlImage(wand->images,degrees,&wand->exception);
  if (swirl_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,swirl_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k T e x t u r e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickTextureImage() repeatedly tiles the texture image across and down the
%  image canvas.
%
%  The format of the MagickTextureImage method is:
%
%      MagickWand *MagickTextureImage(MagickWand *wand,
%        const MagickWand *texture_wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o texture_wand: The texture wand
%
*/
WandExport MagickWand *MagickTextureImage(MagickWand *wand,
  const MagickWand *texture_wand)
{
  Image
    *texture_image;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if ((wand->images == (Image *) NULL) ||
      (texture_wand->images == (Image *) NULL))
    {
      (void) ThrowMagickException(&wand->exception,GetMagickModule(),WandError,
        "ContainsNoImages","`%s'",wand->name);
      return((MagickWand *) NULL);
    }
  texture_image=CloneImage(wand->images,0,0,MagickTrue,&wand->exception);
  if (texture_image == (Image *) NULL)
    return((MagickWand *) NULL);
  status=TextureImage(texture_image,texture_wand->images);
  if (status == MagickFalse)
    InheritException(&wand->exception,&texture_image->exception);
  return(CloneMagickWandFromImages(wand,texture_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k T h r e s h o l d I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickThresholdImage() changes the value of individual pixels based on
%  the intensity of each pixel compared to threshold.  The result is a
%  high-contrast, two color image.
%
%  The format of the MagickThresholdImage method is:
%
%      MagickBooleanType MagickThresholdImage(MagickWand *wand,
%        const double threshold)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o threshold: Define the threshold value.
%
*/
WandExport MagickBooleanType MagickThresholdImage(MagickWand *wand,
  const double threshold)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=BilevelImage(wand->images,threshold);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k T h r e s h o l d I m a g e C h a n n e l                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickThresholdImageChannel() changes the value of individual pixel
%  component based on the intensity of each pixel compared to threshold.  The
%  result is a high-contrast, two color image.
%
%  The format of the MagickThresholdImage method is:
%
%      MagickBooleanType MagickThresholdImageChannel(MagickWand *wand,
%        const ChannelType channel,const double threshold)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: The channel.
%
%    o threshold: Define the threshold value.
%
*/
WandExport MagickBooleanType MagickThresholdImageChannel(MagickWand *wand,
  const ChannelType channel,const double threshold)
{
  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  status=BilevelImageChannel(wand->images,channel,threshold);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k T h u m b n a i l I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickThumbnailImage()  changes the size of an image to the given dimensions
%  and removes any associated profiles.  The goal is to produce small low cost
%  thumbnail images suited for display on the Web.
%
%  The format of the MagickThumbnailImage method is:
%
%      MagickBooleanType MagickThumbnailImage(MagickWand *wand,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
*/
WandExport MagickBooleanType MagickThumbnailImage(MagickWand *wand,
  const unsigned long columns,const unsigned long rows)
{
  Image
    *thumbnail_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  thumbnail_image=ThumbnailImage(wand->images,columns,rows,&wand->exception);
  if (thumbnail_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,thumbnail_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k T i n t I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickTintImage() applies a color vector to each pixel in the image.  The
%  length of the vector is 0 for black and white and at its maximum for the
%  midtones.  The vector weighting function is
%  f(x)=(1-(4.0*((x-0.5)*(x-0.5)))).
%
%  The format of the MagickTintImage method is:
%
%      MagickBooleanType MagickTintImage(MagickWand *wand,
%        const PixelWand *tint,const PixelWand *opacity)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o tint: The tint pixel wand.
%
%    o opacity: The opacity pixel wand.
%
*/
WandExport MagickBooleanType MagickTintImage(MagickWand *wand,
  const PixelWand *tint,const PixelWand *opacity)
{
  char
    percent_opaque[MaxTextExtent];

  Image
    *tint_image;

  PixelPacket
    target;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  (void) FormatMagickString(percent_opaque,MaxTextExtent,"%g,%g,%g,%g",
    100.0*PixelGetRedQuantum(opacity)/QuantumRange,
    100.0*PixelGetGreenQuantum(opacity)/QuantumRange,
    100.0*PixelGetBlueQuantum(opacity)/QuantumRange,
    100.0*PixelGetOpacityQuantum(opacity)/QuantumRange);
  PixelGetQuantumColor(tint,&target);
  tint_image=TintImage(wand->images,percent_opaque,target,&wand->exception);
  if (tint_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,tint_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k T r a n s f o r m I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickTransformImage() is a convenience method that behaves like
%  MagickResizeImage() or MagickCropImage() but accepts scaling and/or cropping
%  information as a region geometry specification.  If the operation fails, the
%  original image handle is returned.
%
%  The format of the MagickTransformImage method is:
%
%      MagickWand *MagickTransformImage(MagickWand *wand,const char *crop,
%        const char *geometry)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o crop: A crop geometry string.  This geometry defines a subregion of the
%      image to crop.
%
%    o geometry: An image geometry string.  This geometry defines the final
%      size of the image.
%
*/
WandExport MagickWand *MagickTransformImage(MagickWand *wand,
  const char *crop,const char *geometry)
{
  Image
    *transform_image;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    return((MagickWand *) NULL);
  transform_image=CloneImage(wand->images,0,0,MagickTrue,&wand->exception);
  if (transform_image == (Image *) NULL)
    return((MagickWand *) NULL);
  status=TransformImage(&transform_image,crop,geometry);
  if (status == MagickFalse)
    InheritException(&wand->exception,&transform_image->exception);
  return(CloneMagickWandFromImages(wand,transform_image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k T r i m I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickTrimImage() remove edges that are the background color from the image.
%
%  The format of the MagickTrimImage method is:
%
%      MagickBooleanType MagickTrimImage(MagickWand *wand,const double fuzz)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o fuzz: By default target must match a particular pixel color
%      exactly.  However, in many cases two colors may differ by a small amount.
%      The fuzz member of image defines how much tolerance is acceptable to
%      consider two colors as the same.  For example, set fuzz to 10 and the
%      color red at intensities of 100 and 102 respectively are now interpreted
%      as the same color for the purposes of the floodfill.
%
*/
WandExport MagickBooleanType MagickTrimImage(MagickWand *wand,const double fuzz)
{
  Image
    *trim_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wand->images->fuzz=fuzz;
  trim_image=TrimImage(wand->images,&wand->exception);
  if (trim_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,trim_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k U n s h a r p M a s k I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickUnsharpMaskImage() sharpens an image.  We convolve the image with a
%  Gaussian operator of the given radius and standard deviation (sigma).
%  For reasonable results, radius should be larger than sigma.  Use a radius
%  of 0 and UnsharpMaskImage() selects a suitable radius for you.
%
%  The format of the MagickUnsharpMaskImage method is:
%
%      MagickBooleanType MagickUnsharpMaskImage(MagickWand *wand,
%        const double radius,const double sigma,const double amount,
%        const double threshold)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
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
*/
WandExport MagickBooleanType MagickUnsharpMaskImage(MagickWand *wand,
  const double radius,const double sigma,const double amount,
  const double threshold)
{
  Image
    *unsharp_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  unsharp_image=UnsharpMaskImage(wand->images,radius,sigma,amount,threshold,
    &wand->exception);
  if (unsharp_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,unsharp_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k U n s h a r p M a s k I m a g e C h a n n e l                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickUnsharpMaskImageChannel() sharpens one or more image channels.  We
%  convolve the image with a Gaussian operator of the given radius and standard
%  deviation (sigma).  For reasonable results, radius should be larger than
%  sigma.  Use a radius of 0 and UnsharpMaskImage() selects a suitable radius
%  for you.
%
%  The format of the MagickUnsharpMaskImageChannel method is:
%
%      MagickBooleanType MagickUnsharpMaskImageChannel(MagickWand *wand,
%        const ChannelType channel,const double radius,const double sigma,
%        const double amount,const double threshold)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o channel: Identify which channel to extract: RedChannel, GreenChannel,
%      BlueChannel, OpacityChannel, CyanChannel, MagentaChannel, YellowChannel,
%      BlackChannel, or IndexChannel.
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
*/
WandExport MagickBooleanType MagickUnsharpMaskImageChannel(MagickWand *wand,
  const ChannelType channel,const double radius,const double sigma,
  const double amount,const double threshold)
{
  Image
    *unsharp_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  unsharp_image=UnsharpMaskImageChannel(wand->images,channel,radius,sigma,amount,
    threshold,&wand->exception);
  if (unsharp_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,unsharp_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k W a v e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickWaveImage()  creates a "ripple" effect in the image by shifting
%  the pixels vertically along a sine wave whose amplitude and wavelength
%  is specified by the given parameters.
%
%  The format of the MagickWaveImage method is:
%
%      MagickBooleanType MagickWaveImage(MagickWand *wand,const double amplitude,
%        const double wave_length)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o amplitude, wave_length:  Define the amplitude and wave length of the
%      sine wave.
%
*/
WandExport MagickBooleanType MagickWaveImage(MagickWand *wand,
  const double amplitude,const double wave_length)
{
  Image
    *wave_image;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  wave_image=WaveImage(wand->images,amplitude,wave_length,&wand->exception);
  if (wave_image == (Image *) NULL)
    return(MagickFalse);
  ReplaceImageInList(&wand->images,wave_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k W h i t e T h r e s h o l d I m a g e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickWhiteThresholdImage() is like ThresholdImage() but  forces all pixels
%  above the threshold into white while leaving all pixels below the threshold
%  unchanged.
%
%  The format of the MagickWhiteThresholdImage method is:
%
%      MagickBooleanType MagickWhiteThresholdImage(MagickWand *wand,
%        const PixelWand *threshold)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o threshold: The pixel wand.
%
*/
WandExport MagickBooleanType MagickWhiteThresholdImage(MagickWand *wand,
  const PixelWand *threshold)
{
  char
    thresholds[MaxTextExtent];

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  (void) FormatMagickString(thresholds,MaxTextExtent,
    QuantumFormat "," QuantumFormat "," QuantumFormat "," QuantumFormat,
    PixelGetRedQuantum(threshold),PixelGetGreenQuantum(threshold),
    PixelGetBlueQuantum(threshold),PixelGetOpacityQuantum(threshold));
  status=WhiteThresholdImage(wand->images,thresholds);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k W r i t e I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickWriteImage() writes an image to the specified filename.  If the
%  filename parameter is NULL, the image is written to the filename set
%  by MagickReadImage() or MagickSetImageFilename().
%
%  The format of the MagickWriteImage method is:
%
%      MagickBooleanType MagickWriteImage(MagickWand *wand,
%        const char *filename)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o filename: The image filename.
%
%
*/
WandExport MagickBooleanType MagickWriteImage(MagickWand *wand,
  const char *filename)
{
  Image
    *image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  if (filename != (const char *) NULL)
    (void) CopyMagickString(wand->images->filename,filename,MaxTextExtent);
  image=CloneImage(wand->images,0,0,MagickTrue,&wand->exception);
  if (image == (Image *) NULL)
    return(MagickFalse);
  write_info=CloneImageInfo(wand->image_info);
  write_info->adjoin=MagickTrue;
  status=WriteImage(write_info,image);
  if (status == MagickFalse)
    InheritException(&wand->exception,&image->exception);
  image=DestroyImage(image);
  write_info=DestroyImageInfo(write_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k W r i t e I m a g e F i l e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickWriteImageFile() writes an image to an open file descriptor.
%
%  The format of the MagickWriteImageFile method is:
%
%      MagickBooleanType MagickWriteImageFile(MagickWand *wand,FILE *file)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o file: The file descriptor.
%
%
*/
WandExport MagickBooleanType MagickWriteImageFile(MagickWand *wand,FILE *file)
{
  Image
    *image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  assert(file != (FILE *) NULL);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  image=CloneImage(wand->images,0,0,MagickTrue,&wand->exception);
  if (image == (Image *) NULL)
    return(MagickFalse);
  write_info=CloneImageInfo(wand->image_info);
  SetImageInfoFile(write_info,file);
  write_info->adjoin=MagickTrue;
  status=WriteImage(write_info,image);
  write_info=DestroyImageInfo(write_info);
  if (status == MagickFalse)
    InheritException(&wand->exception,&image->exception);
  image=DestroyImage(image);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k W r i t e I m a g e s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickWriteImages() writes an image or image sequence.
%
%  The format of the MagickWriteImages method is:
%
%      MagickBooleanType MagickWriteImages(MagickWand *wand,
%        const char *filename,const MagickBooleanType adjoin)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o filename: The image filename.
%
%    o adjoin: join images into a single multi-image file.
%
*/
WandExport MagickBooleanType MagickWriteImages(MagickWand *wand,
  const char *filename,const MagickBooleanType adjoin)
{
  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  write_info=CloneImageInfo(wand->image_info);
  write_info->adjoin=adjoin;
  status=WriteImages(write_info,wand->images,filename,&wand->exception);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  write_info=DestroyImageInfo(write_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k W r i t e I m a g e s F i l e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickWriteImagesFile() writes an image sequence to an open file descriptor.
%
%  The format of the MagickWriteImagesFile method is:
%
%      MagickBooleanType MagickWriteImagesFile(MagickWand *wand,FILE *file)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o file: The file descriptor.
%
*/
WandExport MagickBooleanType MagickWriteImagesFile(MagickWand *wand,FILE *file)
{
  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  assert(wand != (MagickWand *) NULL);
  assert(wand->signature == MagickSignature);
  if (wand->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",wand->name);
  if (wand->images == (Image *) NULL)
    ThrowWandException(WandError,"ContainsNoImages",wand->name);
  write_info=CloneImageInfo(wand->image_info);
  SetImageInfoFile(write_info,file);
  write_info->adjoin=MagickTrue;
  status=WriteImages(write_info,wand->images,(const char *) NULL,
    &wand->exception);
  write_info=DestroyImageInfo(write_info);
  if (status == MagickFalse)
    InheritException(&wand->exception,&wand->images->exception);
  return(status);
}
