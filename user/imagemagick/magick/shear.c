/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      SSSSS  H   H  EEEEE   AAA    RRRR                      %
%                      SS     H   H  E      A   A   R   R                     %
%                       SSS   HHHHH  EEE    AAAAA   RRRR                      %
%                         SS  H   H  E      A   A   R R                       %
%                      SSSSS  H   H  EEEEE  A   A   R  R                      %
%                                                                             %
%                                                                             %
%            Methods to Shear or Rotate an Image by an Arbitrary Angle        %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                  July 1992                                  %
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
%  RotateImage(), XShearImage(), and YShearImage() is based on the paper
%  "A Fast Algorithm for General Raster Rotatation" by Alan W. Paeth,
%  Graphics Interface '86 (Vancouver).  RotateImage is adapted from a similar
%  method based on the Paeth paper written by Michael Halle of the Spatial
%  Imaging Group, MIT Media Lab.
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/memory_.h"
#include "magick/list.h"
#include "magick/monitor.h"
#include "magick/shear.h"
#include "magick/transform.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     A f f i n e T r a n s f o r m I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AffineTransformImage() transforms an image as dictated by the affine matrix.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the AffineTransformImage method is:
%
%      Image *AffineTransformImage(const Image *image,AffineMatrix *affine,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o affine: The affine transform.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *AffineTransformImage(const Image *image,
  const AffineMatrix *affine,ExceptionInfo *exception)
{
  AffineMatrix
    transform;

  Image
    *affine_image;

  PointInfo
    extent[4],
    min,
    max,
    point;

  register long
    i;

  /*
    Determine bounding box.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(affine != (AffineMatrix *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  extent[0].x=0.0;
  extent[0].y=0.0;
  extent[1].x=(double) image->columns;
  extent[1].y=0.0;
  extent[2].x=(double) image->columns;
  extent[2].y=(double) image->rows;
  extent[3].x=0.0;
  extent[3].y=(double) image->rows;
  for (i=0; i < 4; i++)
  {
    point=extent[i];
    extent[i].x=(double) (point.x*affine->sx+point.y*affine->ry+affine->tx);
    extent[i].y=(double) (point.x*affine->rx+point.y*affine->sy+affine->ty);
  }
  min=extent[0];
  max=extent[0];
  for (i=1; i < 4; i++)
  {
    if (min.x > extent[i].x)
      min.x=extent[i].x;
    if (min.y > extent[i].y)
      min.y=extent[i].y;
    if (max.x < extent[i].x)
      max.x=extent[i].x;
    if (max.y < extent[i].y)
      max.y=extent[i].y;
  }
  /*
    Affine transform image.
  */
  affine_image=CloneImage(image,(unsigned long) (max.x-min.x+0.5),
    (unsigned long) (max.y-min.y+0.5),MagickTrue,exception);
  if (affine_image == (Image *) NULL)
    return((Image *) NULL);
  affine_image->background_color.opacity=TransparentOpacity;
  SetImageBackgroundColor(affine_image);
  transform.sx=affine->sx;
  transform.rx=affine->rx;
  transform.ry=affine->ry;
  transform.sy=affine->sy;
  transform.tx=min.x;
  transform.ty=min.y;
  (void) DrawAffineImage(affine_image,image,&transform);
  return(affine_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C r o p T o F i t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CropToFitImage() crops the sheared image as determined by the bounding box
%  as defined by width and height and shearing angles.
%
%  The format of the CropToFitImage method is:
%
%      Image *CropToFitImage(Image **image,const MagickRealType x_shear,
%        const MagickRealType x_shear,const MagickRealType width,
%        const MagickRealType height,const MagickBooleanType rotate,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o x_shear, y_shear, width, height: Defines a region of the image to crop.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static inline void CropToFitImage(Image **image,const MagickRealType x_shear,
  const MagickRealType y_shear,const MagickRealType width,
  const MagickRealType height,const MagickBooleanType rotate,
  ExceptionInfo *exception)
{
  Image
    *crop_image;

  PointInfo
    extent[4],
    min,
    max;

  RectangleInfo
    geometry,
    page;

  register long
    i;

  /*
    Calculate the rotated image size.
  */
  extent[0].x=(double) (-width/2.0);
  extent[0].y=(double) (-height/2.0);
  extent[1].x=(double) width/2.0;
  extent[1].y=(double) (-height/2.0);
  extent[2].x=(double) (-width/2.0);
  extent[2].y=(double) height/2.0;
  extent[3].x=(double) width/2.0;
  extent[3].y=(double) height/2.0;
  for (i=0; i < 4; i++)
  {
    extent[i].x+=x_shear*extent[i].y;
    extent[i].y+=y_shear*extent[i].x;
    if (rotate != MagickFalse)
      extent[i].x+=x_shear*extent[i].y;
    extent[i].x+=(double) (*image)->columns/2.0;
    extent[i].y+=(double) (*image)->rows/2.0;
  }
  min=extent[0];
  max=extent[0];
  for (i=1; i < 4; i++)
  {
    if (min.x > extent[i].x)
      min.x=extent[i].x;
    if (min.y > extent[i].y)
      min.y=extent[i].y;
    if (max.x < extent[i].x)
      max.x=extent[i].x;
    if (max.y < extent[i].y)
      max.y=extent[i].y;
  }
  geometry.x=(long) (min.x+0.5);
  geometry.y=(long) (min.y+0.5);
  geometry.width=(unsigned long) (max.x-min.x+0.5);
  geometry.height=(unsigned long) (max.y-min.y+0.5);
  page=(*image)->page;
  (void) ParseAbsoluteGeometry("0x0+0+0",&(*image)->page);
  crop_image=CropImage(*image,&geometry,exception);
  (*image)->page=page;
  if (crop_image != (Image *) NULL)
    {
      crop_image->page=page;
      *image=DestroyImage(*image);
      *image=crop_image;
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n t e g r a l R o t a t e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IntegralRotateImage()  rotates the image an integral of 90 degrees.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the rotated image.
%
%  The format of the IntegralRotateImage method is:
%
%      Image *IntegralRotateImage(const Image *image,unsigned long rotations,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o rotations: Specifies the number of 90 degree rotations.
%
%
*/
static Image *IntegralRotateImage(const Image *image,unsigned long rotations,
  ExceptionInfo *exception)
{
#define RotateImageTag  "Rotate/Image"

  Image
    *rotate_image;

  long
    y;

  MagickBooleanType
    status;

  RectangleInfo
    page;

  register IndexPacket
    *indexes,
    *rotate_indexes;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize rotated image attributes.
  */
  assert(image != (Image *) NULL);
  page=image->page;
  rotations%=4;
  if ((rotations == 1) || (rotations == 3))
    rotate_image=CloneImage(image,image->rows,image->columns,MagickTrue,
      exception);
  else
    rotate_image=CloneImage(image,image->columns,image->rows,MagickTrue,
      exception);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Integral rotate the image.
  */
  switch (rotations)
  {
    case 0:
    {
      /*
        Rotate 0 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,0,y,rotate_image->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        (void) CopyMagickMemory(q,p,(size_t) image->columns*sizeof(*q));
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        if ((indexes != (IndexPacket *) NULL) &&
            (rotate_indexes != (IndexPacket *) NULL))
          (void) CopyMagickMemory(rotate_indexes,indexes,(size_t)
            image->columns*sizeof(*rotate_indexes));
        if (SyncImagePixels(rotate_image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RotateImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      break;
    }
    case 1:
    {
      /*
        Rotate 90 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,(long) (image->rows-y-1),0,1,
          rotate_image->rows);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        (void) CopyMagickMemory(q,p,(size_t) image->columns*sizeof(*q));
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        if ((indexes != (IndexPacket *) NULL) &&
            (rotate_indexes != (IndexPacket *) NULL))
          (void) CopyMagickMemory(rotate_indexes,indexes,(size_t)
            image->columns*sizeof(*rotate_indexes));
        if (SyncImagePixels(rotate_image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RotateImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      Swap(page.width,page.height);
      Swap(page.x,page.y);
      if (page.width != 0)
        page.x=(long) (page.width-rotate_image->columns-page.x);
      break;
    }
    case 2:
    {
      /*
        Rotate 180 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,0,(long) (image->rows-y-1),
          image->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        q+=image->columns;
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        if ((indexes != (IndexPacket *) NULL) &&
            (rotate_indexes != (IndexPacket *) NULL))
          for (x=0; x < (long) image->columns; x++)
            rotate_indexes[image->columns-x-1]=indexes[x];
        for (x=0; x < (long) image->columns; x++)
          *--q=(*p++);
        if (SyncImagePixels(rotate_image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RotateImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      if (page.width != 0)
        page.x=(long) (page.width-rotate_image->columns-page.x);
      if (page.height != 0)
        page.y=(long) (page.height-rotate_image->rows-page.y);
      break;
    }
    case 3:
    {
      /*
        Rotate 270 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,y,0,1,rotate_image->rows);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        q+=image->columns;
        for (x=0; x < (long) image->columns; x++)
          *--q=(*p++);
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        if ((indexes != (IndexPacket *) NULL) &&
            (rotate_indexes != (IndexPacket *) NULL))
          for (x=0; x < (long) image->columns; x++)
            rotate_indexes[image->columns-x-1]=indexes[x];
        if (SyncImagePixels(rotate_image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(RotateImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      Swap(page.width,page.height);
      Swap(page.x,page.y);
      if (page.height != 0)
        page.y=(long) (page.height-rotate_image->rows-page.y);
      break;
    }
  }
  rotate_image->page=page;
  return(rotate_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S h e a r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  XShearImage() shears the image in the X direction with a shear angle of
%  'degrees'.  Positive angles shear counter-clockwise (right-hand rule), and
%  negative angles shear clockwise.  Angles are measured relative to a vertical
%  Y-axis.  X shears will widen an image creating 'empty' triangles on the left
%  and right sides of the source image.
%
%  The format of the XShearImage method is:
%
%      void XShearImage(Image *image,const MagickRealType degrees,
%        const unsigned long width,const unsigned long height,
%        const long x_offset,long y_offset)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o degrees: A MagickRealType representing the shearing angle along the X
%      axis.
%
%    o width, height, x_offset, y_offset: Defines a region of the image
%      to shear.
%
*/

static inline MagickRealType Blend_(const MagickRealType p,
  const MagickRealType alpha,const MagickRealType q,const MagickRealType beta)
{
  return((1.0-QuantumScale*alpha)*p+(1.0-QuantumScale*beta)*q);
}

static inline void MagickCompositeBlend(const PixelPacket *p,
  const MagickRealType alpha,const PixelPacket *q,const MagickRealType beta,
  const MagickRealType area,PixelPacket *composite)
{
  MagickRealType
    gamma;

  if ((alpha == TransparentOpacity) && (beta == TransparentOpacity))
    {
      *composite=(*p);
      return;
    }
  gamma=RoundToUnity((1.0-QuantumScale*(QuantumRange-(1.0-area)*
    (QuantumRange-alpha)))+(1.0-QuantumScale*(QuantumRange-area*
    (QuantumRange-beta))));
  composite->opacity=RoundToQuantum(QuantumRange*(1.0-gamma));
  gamma=1.0/(fabs(gamma) <= MagickEpsilon ? 1.0 : gamma);
  composite->red=RoundToQuantum(gamma*Blend_((MagickRealType) p->red,
    QuantumRange-(1.0-area)*(QuantumRange-alpha),(MagickRealType) q->red,
    QuantumRange-area*(QuantumRange-beta)));
  composite->green=RoundToQuantum(gamma*Blend_((MagickRealType) p->green,
    QuantumRange-(1.0-area)*(QuantumRange-alpha),(MagickRealType) q->green,
    QuantumRange-area*(QuantumRange-beta)));
  composite->blue=RoundToQuantum(gamma*Blend_((MagickRealType) p->blue,
    QuantumRange-(1.0-area)*(QuantumRange-alpha),(MagickRealType) q->blue,
    QuantumRange-area*(QuantumRange-beta)));
}

static inline void XShearImage(Image *image,const MagickRealType degrees,
  const unsigned long width,const unsigned long height,const long x_offset,
  long y_offset)
{
#define XShearImageTag  "XShear/Image"

  enum {LEFT, RIGHT}
    direction;

  long
    step,
    y;

  MagickBooleanType
    status;

  MagickRealType
    area,
    displacement;

  PixelPacket
    pixel;

  register long
    i;

  register PixelPacket
    *p,
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  y_offset--;
  for (y=0; y < (long) height; y++)
  {
    y_offset++;
    displacement=degrees*(MagickRealType) (y-height/2.0);
    if (displacement == 0.0)
      continue;
    if (displacement > 0.0)
      direction=RIGHT;
    else
      {
        displacement*=(-1.0);
        direction=LEFT;
      }
    step=(long) floor((double) displacement);
    area=(MagickRealType) (displacement-step);
    step++;
    pixel=image->background_color;
    switch (direction)
    {
      case LEFT:
      {
        /*
          Transfer pixels left-to-right.
        */
        if (step > x_offset)
          break;
        p=GetImagePixels(image,0,y_offset,image->columns,1);
        if (p == (PixelPacket *) NULL)
          break;
        p+=x_offset;
        q=p-step;
        for (i=0; i < (long) width; i++)
        {
          if ((x_offset+i) < step)
            {
              pixel=(*++p);
              q++;
              continue;
            }
          MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,p,
            (MagickRealType) p->opacity,area,q);
          q++;
          pixel=(*p++);
        }
        MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,
          &image->background_color,(MagickRealType)
          image->background_color.opacity,area,q);
        q++;
        for (i=0; i < (step-1); i++)
          *q++=image->background_color;
        break;
      }
      case RIGHT:
      {
        /*
          Transfer pixels right-to-left.
        */
        p=GetImagePixels(image,0,y_offset,image->columns,1);
        if (p == (PixelPacket *) NULL)
          break;
        p+=x_offset+width;
        q=p+step;
        for (i=0; i < (long) width; i++)
        {
          p--;
          q--;
          if ((unsigned long) (x_offset+width+step-i) >= image->columns)
            continue;
          MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,p,
            (MagickRealType) p->opacity,area,q);
          pixel=(*p);
        }
        q--;
        MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,
          &image->background_color,(MagickRealType)
          image->background_color.opacity,area,q);
        for (i=0; i < (step-1); i++)
          *--q=image->background_color;
        break;
      }
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,height) != MagickFalse))
      {
        status=image->progress_monitor(XShearImageTag,y,height,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   Y S h e a r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  YShearImage shears the image in the Y direction with a shear angle of
%  'degrees'.  Positive angles shear counter-clockwise (right-hand rule), and
%  negative angles shear clockwise.  Angles are measured relative to a
%  horizontal X-axis.  Y shears will increase the height of an image creating
%  'empty' triangles on the top and bottom of the source image.
%
%  The format of the YShearImage method is:
%
%      void YShearImage(Image *image,const MagickRealType degrees,
%        const unsigned long width,const unsigned long height,long x_offset,
%        const long y_offset)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o degrees: A MagickRealType representing the shearing angle along the Y
%      axis.
%
%    o width, height, x_offset, y_offset: Defines a region of the image
%      to shear.
%
%
*/
static inline void YShearImage(Image *image,const MagickRealType degrees,
  const unsigned long width,const unsigned long height,long x_offset,
  const long y_offset)
{
#define YShearImageTag  "YShear/Image"

  enum {UP, DOWN}
    direction;

  long
    step,
    y;

  MagickBooleanType
    status;

  MagickRealType
    area,
    displacement;

  register PixelPacket
    *p,
    *q;

  register long
    i;

  PixelPacket
    pixel;

  assert(image != (Image *) NULL);
  x_offset--;
  for (y=0; y < (long) width; y++)
  {
    x_offset++;
    displacement=degrees*(MagickRealType) (y-width/2.0);
    if (displacement == 0.0)
      continue;
    if (displacement > 0.0)
      direction=DOWN;
    else
      {
        displacement*=(-1.0);
        direction=UP;
      }
    step=(long) floor((double) displacement);
    area=(MagickRealType) (displacement-step);
    step++;
    pixel=image->background_color;
    switch (direction)
    {
      case UP:
      {
        /*
          Transfer pixels top-to-bottom.
        */
        if (step > y_offset)
          break;
        p=GetImagePixels(image,x_offset,0,1,image->rows);
        if (p == (PixelPacket *) NULL)
          break;
        p+=y_offset;
        q=p-step;
        for (i=0; i < (long) height; i++)
        {
          if ((y_offset+i) < step)
            {
              pixel=(*++p);
              q++;
              continue;
            }
          MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,p,
            (MagickRealType) p->opacity,area,q);
          q++;
          pixel=(*p++);
        }
        MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,
          &image->background_color,(MagickRealType)
          image->background_color.opacity,area,q);
        q++;
        for (i=0; i < (step-1); i++)
          *q++=image->background_color;
        break;
      }
      case DOWN:
      {
        /*
          Transfer pixels bottom-to-top.
        */
        p=GetImagePixels(image,x_offset,0,1,image->rows);
        if (p == (PixelPacket *) NULL)
          break;
        p+=y_offset+height;
        q=p+step;
        for (i=0; i < (long) height; i++)
        {
          p--;
          q--;
          if ((unsigned long) (y_offset+height+step-i) >= image->rows)
            continue;
          MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,p,
            (MagickRealType) p->opacity,area,q);
          pixel=(*p);
        }
        q--;
        MagickCompositeBlend(&pixel,(MagickRealType) pixel.opacity,
          &image->background_color,(MagickRealType)
          image->background_color.opacity,area,q);
        for (i=0; i < (step-1); i++)
          *--q=image->background_color;
        break;
      }
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,width) != MagickFalse))
      {
        status=image->progress_monitor(XShearImageTag,y,width,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R o t a t e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RotateImage() creates a new image that is a rotated copy of an existing
%  one.  Positive angles rotate counter-clockwise (right-hand rule), while
%  negative angles rotate clockwise.  Rotated images are usually larger than
%  the originals and have 'empty' triangular corners.  X axis.  Empty
%  triangles left over from shearing the image are filled with the background
%  color defined by member 'background_color' of the image.  RotateImage
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  RotateImage() is based on the paper "A Fast Algorithm for General
%  Raster Rotatation" by Alan W. Paeth.  RotateImage is adapted from a similar
%  method based on the Paeth paper written by Michael Halle of the Spatial
%  Imaging Group, MIT Media Lab.
%
%  The format of the RotateImage method is:
%
%      Image *RotateImage(const Image *image,const double degrees,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o degrees: Specifies the number of degrees to rotate the image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *RotateImage(const Image *image,const double degrees,
  ExceptionInfo *exception)
{
  Image
    *integral_image,
    *rotate_image;

  long
    x_offset,
    y_offset;

  MagickRealType
    angle;

  PointInfo
    shear;

  RectangleInfo
    border_info;

  unsigned long
    height,
    rotations,
    width,
    y_width;

  /*
    Adjust rotation angle.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  angle=degrees;
  while (angle < -45.0)
    angle+=360.0;
  for (rotations=0; angle > 45.0; rotations++)
    angle-=90.0;
  rotations%=4;
  /*
    Calculate shear equations.
  */
  integral_image=IntegralRotateImage(image,rotations,exception);
  if (integral_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  shear.x=(-tan((double) DegreesToRadians(angle)/2.0));
  shear.y=sin((double) DegreesToRadians(angle));
  if ((shear.x == 0.0) && (shear.y == 0.0))
    return(integral_image);
  integral_image->storage_class=DirectClass;
  if (integral_image->matte == MagickFalse)
    SetImageOpacity(integral_image,OpaqueOpacity);
  /*
    Compute image size.
  */
  width=image->columns;
  height=image->rows;
  if ((rotations == 1) || (rotations == 3))
    {
      width=image->rows;
      height=image->columns;
    }
  x_offset=(long) (fabs(2.25*(double) height*shear.y)+0.5);
  y_width=(unsigned long) (fabs((double) height*shear.x)+width+0.5);
  y_offset=(long) (fabs((double) y_width*shear.y)+0.5);
  /*
    Surround image with a border.
  */
  integral_image->border_color=integral_image->background_color;
  border_info.width=(unsigned long) x_offset;
  border_info.height=(unsigned long) y_offset;
  rotate_image=BorderImage(integral_image,&border_info,exception);
  integral_image=DestroyImage(integral_image);
  if (rotate_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Rotate the image.
  */
  XShearImage(rotate_image,shear.x,width,height,x_offset,
    ((long) rotate_image->rows-height)/2);
  YShearImage(rotate_image,shear.y,y_width,height,
    ((long) rotate_image->columns-y_width)/2,y_offset);
  XShearImage(rotate_image,shear.x,y_width,rotate_image->rows,
    ((long) rotate_image->columns-y_width)/2,0);
  CropToFitImage(&rotate_image,shear.x,shear.y,(MagickRealType) width,
    (MagickRealType) height,MagickTrue,exception);
  rotate_image->page.width=0;
  rotate_image->page.height=0;
  return(rotate_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S h e a r I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShearImage() creates a new image that is a shear_image copy of an existing
%  one.  Shearing slides one edge of an image along the X or Y axis, creating
%  a parallelogram.  An X direction shear slides an edge along the X axis,
%  while a Y direction shear slides an edge along the Y axis.  The amount of
%  the shear is controlled by a shear angle.  For X direction shears, x_shear
%  is measured relative to the Y axis, and similarly, for Y direction shears
%  y_shear is measured relative to the X axis.  Empty triangles left over from
%  shearing the image are filled with the background color defined by member
%  'background_color' of the image..  ShearImage() allocates the memory
%  necessary for the new Image structure and returns a pointer to the new image.
%
%  ShearImage() is based on the paper "A Fast Algorithm for General Raster
%  Rotatation" by Alan W. Paeth.
%
%  The format of the ShearImage method is:
%
%      Image *ShearImage(const Image *image,const double x_shear,
%        const double y_shear,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o x_shear, y_shear: Specifies the number of degrees to shear the image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ShearImage(const Image *image,const double x_shear,
  const double y_shear,ExceptionInfo *exception)
{
  Image
    *integral_image,
    *shear_image;

  long
    x_offset,
    y_offset;

  PointInfo
    shear;

  RectangleInfo
    border_info;

  unsigned long
    y_width;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((x_shear != 0.0) && (fmod(x_shear,90.0) == 0.0))
    ThrowImageException(ImageError,"AngleIsDiscontinuous");
  if ((y_shear != 0.0) && (fmod(y_shear,90.0) == 0.0))
    ThrowImageException(ImageError,"AngleIsDiscontinuous");
  /*
    Initialize shear angle.
  */
  integral_image=CloneImage(image,0,0,MagickTrue,exception);
  if (integral_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  shear.x=(-tan(DegreesToRadians(x_shear)));
  shear.y=tan(DegreesToRadians(y_shear));
  if ((shear.x == 0.0) && (shear.y == 0.0))
    return(integral_image);
  integral_image->storage_class=DirectClass;
  if (integral_image->matte == MagickFalse)
    SetImageOpacity(integral_image,OpaqueOpacity);
  /*
    Compute image size.
  */
  x_offset=(long) (fabs((double) image->rows*shear.x)+0.5);
  y_width=(unsigned long)
    (fabs((double) image->rows*shear.x)+image->columns+0.5);
  y_offset=(long) (fabs((double) y_width*shear.y)+0.5);
  /*
    Surround image with border.
  */
  integral_image->border_color=integral_image->background_color;
  border_info.width=(unsigned long) x_offset;
  border_info.height=(unsigned long) y_offset;
  shear_image=BorderImage(integral_image,&border_info,exception);
  if (shear_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  integral_image=DestroyImage(integral_image);
  /*
    Shear the image.
  */
  if (shear_image->matte == MagickFalse)
    SetImageOpacity(shear_image,OpaqueOpacity);
  XShearImage(shear_image,shear.x,image->columns,image->rows,x_offset,
    ((long) shear_image->rows-image->rows)/2);
  YShearImage(shear_image,shear.y,y_width,image->rows,
    ((long) shear_image->columns-y_width)/2,y_offset);
  CropToFitImage(&shear_image,shear.x,shear.y,(MagickRealType) image->columns,
    (MagickRealType) image->rows,MagickFalse,exception);
  shear_image->page.width=0;
  shear_image->page.height=0;
  return(shear_image);
}
