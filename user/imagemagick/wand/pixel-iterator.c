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
%            IIIII  TTTTT EEEEE  RRRR    AAA   TTTTT   OOO   RRRR             %
%              I      T   E      R   R  A   A    T    O   O  R   R            %
%              I      T   EEE    RRRR   AAAAA    T    O   O  RRRR             %
%              I      T   E      R R    A   A    T    O   O  R R              %
%            IIIII    T   EEEEE  R  R   A   A    T     OOO   R  R             %
%                                                                             %
%                                                                             %
%                   ImageMagick Image Pixel Iterator Methods                  %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                March 2003                                   %
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
#include "wand/pixel-iterator.h"
#include "wand/pixel-wand.h"
#include "wand/wand.h"
#include "magick/ImageMagick.h"

/*
  Define declarations.
*/
#define PixelIteratorId  "PixelIterator"

/*
  Typedef declarations.
*/
struct _PixelIterator
{
  unsigned long
    id;

  char
    name[MaxTextExtent];

  ExceptionInfo
    exception;

  ViewInfo
    *view;

  RectangleInfo
    region;

  long
    y;

  PixelWand
    **pixel_wand;

  MagickBooleanType
    debug;

  unsigned long
    signature;
};

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l e a r P i x e l I t e r a t o r                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClearPixelIterator() clear resources associated with a PixelIterator.
%
%  The format of the ClearPixelIterator method is:
%
%      PixelIterator *ClearPixelIterator(PixelIterator *iterator)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%
*/
WandExport void ClearPixelIterator(PixelIterator *iterator)
{
  assert(iterator != (const PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  CloseCacheView(iterator->view);
  iterator->pixel_wand=DestroyPixelWands(iterator->pixel_wand,
    iterator->region.width);
  DestroyExceptionInfo(&iterator->exception);
  GetExceptionInfo(&iterator->exception);
  iterator->pixel_wand=NewPixelWands(iterator->region.width);
  iterator->y=(-1);
  iterator->debug=IsEventLogging();
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y P i x e l I t e r a t o r                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyPixelIterator() deallocates resources associated with a PixelIterator.
%
%  The format of the DestroyPixelIterator method is:
%
%      PixelIterator *DestroyPixelIterator(PixelIterator *iterator)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%
*/
WandExport PixelIterator *DestroyPixelIterator(PixelIterator *iterator)
{
  assert(iterator != (const PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  CloseCacheView(iterator->view);
  iterator->pixel_wand=DestroyPixelWands(iterator->pixel_wand,
    iterator->region.width);
  DestroyExceptionInfo(&iterator->exception);
  iterator->signature=(~MagickSignature);
  RelinquishWandId(iterator->id);
  iterator=(PixelIterator *) RelinquishMagickMemory(iterator);
  return(iterator);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P i x e l I t e r a t o r                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPixelIterator() returns MagickTrue if the iterator is verified as a pixel
%  iterator.
%
%  The format of the IsPixelIterator method is:
%
%      MagickBooleanType IsPixelIterator(const PixelIterator *iterator)
%
%  A description of each parameter follows:
%
%    o iterator: The magick iterator.
%
*/
MagickExport MagickBooleanType IsPixelIterator(const PixelIterator *iterator)
{
  size_t
    length;

  if (iterator == (const PixelIterator *) NULL)
    return(MagickFalse);
  if (iterator->signature != MagickSignature)
    return(MagickFalse);
  length=strlen(PixelIteratorId);
  if (LocaleNCompare(iterator->name,PixelIteratorId,length) != 0)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N e w P i x e l I t e r a t o r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewPixelIterator() returns a new pixel iterator.
%
%  The format of the NewPixelIterator method is:
%
%      PixelIterator NewPixelIterator(MagickWand *wand)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
*/
WandExport PixelIterator *NewPixelIterator(MagickWand *wand)
{
  const char
    *quantum;

  Image
    *image;

  PixelIterator
    *iterator;

  unsigned long
    depth;

  ViewInfo
    *view;

  depth=QuantumDepth;
  quantum=GetMagickQuantumDepth(&depth);
  if (depth != QuantumDepth)
    ThrowWandFatalException(WandError,"QuantumDepthMismatch",quantum);
  assert(wand != (MagickWand *) NULL);
  image=GetImageFromMagickWand(wand);
  if (image == (Image *) NULL)
    return((PixelIterator *) NULL);
  view=OpenCacheView(image);
  if (view == (ViewInfo *) NULL)
    return((PixelIterator *) NULL);
  iterator=(PixelIterator *) AcquireMagickMemory(sizeof(*iterator));
  if (iterator == (PixelIterator *) NULL)
    ThrowWandFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  (void) ResetMagickMemory(iterator,0,sizeof(*iterator));
  iterator->id=AcquireWandId();
  (void) FormatMagickString(iterator->name,MaxTextExtent,"%s-%lu",
    PixelIteratorId,iterator->id);
  GetExceptionInfo(&iterator->exception);
  iterator->view=view;
  SetGeometry(image,&iterator->region);
  iterator->pixel_wand=NewPixelWands(iterator->region.width);
  iterator->y=(-1);
  iterator->debug=IsEventLogging();
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  iterator->signature=MagickSignature;
  return(iterator);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l C l e a r I t e r a t o r E x c e p t i o n                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelClearIteratorException() clear any exceptions associated with the
%  iterator.
%
%  The format of the PixelClearIteratorException method is:
%
%      MagickBooleanType PixelClearIteratorException(PixelIterator *wand)
%
%  A description of each parameter follows:
%
%    o wand: The pixel wand.
%
*/
WandExport MagickBooleanType PixelClearIteratorException(
  PixelIterator *iterator)
{
  assert(iterator != (PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  return(SetExceptionInfo(&iterator->exception,UndefinedException));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N e w P i x e l R e g i o n I t e r a t o r                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewPixelRegionIterator() returns a new pixel iterator.
%
%  The format of the NewPixelRegionIterator method is:
%
%      PixelIterator NewPixelRegionIterator(MagickWand *wand,const long x,
%        const long y,const unsigned long columns,const unsigned long rows,
%        const MagickBooleanType modify)
%
%  A description of each parameter follows:
%
%    o wand: The magick wand.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
WandExport PixelIterator *NewPixelRegionIterator(MagickWand *wand,const long x,
  const long y,const unsigned long columns,const unsigned long rows)
{
  const char
    *quantum;

  Image
    *image;

  PixelIterator
    *iterator;

  unsigned long
    depth;

  ViewInfo
    *view;

  assert(wand != (MagickWand *) NULL);
  depth=QuantumDepth;
  quantum=GetMagickQuantumDepth(&depth);
  if (depth != QuantumDepth)
    ThrowWandFatalException(WandError,"QuantumDepthMismatch",quantum);
  if ((columns == 0) || (rows == 0))
    ThrowWandFatalException(WandError,"ZeroRegionSize",quantum);
  image=GetImageFromMagickWand(wand);
  if (image == (Image *) NULL)
    return((PixelIterator *) NULL);
  view=OpenCacheView(image);
  if (view == (ViewInfo *) NULL)
    return((PixelIterator *) NULL);
  iterator=(PixelIterator *) AcquireMagickMemory(sizeof(*iterator));
  if (iterator == (PixelIterator *) NULL)
    ThrowWandFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  (void) ResetMagickMemory(iterator,0,sizeof(*iterator));
  iterator->id=AcquireWandId();
  (void) FormatMagickString(iterator->name,MaxTextExtent,"%s-%lu",
    PixelIteratorId,iterator->id);
  GetExceptionInfo(&iterator->exception);
  iterator->view=view;
  SetGeometry(image,&iterator->region);
  iterator->region.x=x;
  iterator->region.y=y;
  iterator->region.width=columns;
  iterator->region.height=rows;
  iterator->pixel_wand=NewPixelWands(iterator->region.width);
  iterator->y=(-1);
  iterator->debug=IsEventLogging();
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  iterator->signature=MagickSignature;
  return(iterator);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l G e t I t e r a t o r E x c e p t i o n                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelGetIteratorException() returns the severity, reason, and description of
%  any error that occurs when using other methods in this API.
%
%  The format of the PixelGetIteratorException method is:
%
%      char *PixelGetIteratorException(const Pixeliterator *iterator,
%        ExceptionType *severity)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%    o severity: The severity of the error is returned here.
%
*/

WandExport char *PixelIteratorGetException(const PixelIterator *iterator,
  ExceptionType *severity)
{
  return(PixelGetIteratorException(iterator,severity));
}

WandExport char *PixelGetIteratorException(const PixelIterator *iterator,
  ExceptionType *severity)
{
  char
    *description;

  assert(iterator != (const PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  assert(severity != (ExceptionType *) NULL);
  *severity=iterator->exception.severity;
  description=(char *) AcquireMagickMemory(2*MaxTextExtent);
  if (description == (char *) NULL)
    ThrowWandFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  *description='\0';
  if (iterator->exception.reason != (char *) NULL)
    (void) CopyMagickString(description,GetLocaleExceptionMessage(
      iterator->exception.severity,iterator->exception.reason),MaxTextExtent);
  if (iterator->exception.description != (char *) NULL)
    {
      (void) ConcatenateMagickString(description," (",MaxTextExtent);
      (void) ConcatenateMagickString(description,GetLocaleExceptionMessage(
        iterator->exception.severity,iterator->exception.description),
        MaxTextExtent);
      (void) ConcatenateMagickString(description,")",MaxTextExtent);
    }
  return(description);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l G e t N e x t I t e r a t o r R o w                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelGetNextIteratorRow() returns the next row as an array of pixel wands
%  from the pixel iterator.
%
%  The format of the PixelGetNextRow method is:
%
%      PixelWand **PixelGetNextRow(PixelIterator *iterator,
%        unsigned long *number_wands)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%    o number_wands: The number of pixel wands.
%
*/

WandExport PixelWand **PixelGetNextRow(PixelIterator *iterator)
{
  unsigned long
    number_wands;

  return(PixelGetNextIteratorRow(iterator,&number_wands));
}

WandExport PixelWand **PixelGetNextIteratorRow(PixelIterator *iterator,
  unsigned long *number_wands)
{
  IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register long
    x;

  assert(iterator != (PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  *number_wands=0;
  iterator->y++;
  if (PixelSetIteratorRow(iterator,iterator->y) == MagickFalse)
    return((PixelWand **) NULL);
  p=AcquireCacheView(iterator->view,iterator->region.x,iterator->region.y+
    iterator->y,iterator->region.width,1,&iterator->exception);
  if (p == (const PixelPacket *) NULL)
    return((PixelWand **) NULL);
  indexes=GetCacheViewIndexes(iterator->view);
  for (x=0; x < (long) iterator->region.width; x++)
  {
    PixelSetQuantumColor(iterator->pixel_wand[x],p);
    if (iterator->view->image->colorspace == CMYKColorspace)
      PixelSetBlackQuantum(iterator->pixel_wand[x],indexes[x]);
    else
      if (iterator->view->image->storage_class == PseudoClass)
        PixelSetIndex(iterator->pixel_wand[x],indexes[x]);
    p++;
  }
  *number_wands=iterator->region.width;
  return(iterator->pixel_wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l G e t P r e v i o u s I t e r a t o r R o w                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelGetPreviousIteratorRow() returns the previous row as an array of pixel
%  wands from the pixel iterator.
%
%  The format of the PixelGetPreviousRow method is:
%
%      PixelWand **PixelGetPreviousRow(PixelIterator *iterator,
%        unsigned long *number_wands)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%    o number_wands: The number of pixel wands.
%
*/

WandExport PixelWand **PixelGetPreviousRow(PixelIterator *iterator)
{
  unsigned long
    number_wands;

  return(PixelGetPreviousIteratorRow(iterator,&number_wands));
}

WandExport PixelWand **PixelGetPreviousIteratorRow(PixelIterator *iterator,
  unsigned long *number_wands)
{
  IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register long
    x;

  assert(iterator != (PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  *number_wands=0;
  iterator->y--;
  if (PixelSetIteratorRow(iterator,iterator->y) == MagickFalse)
    return((PixelWand **) NULL);
  p=AcquireCacheView(iterator->view,iterator->region.x,iterator->region.y+
    iterator->y,iterator->region.width,1,&iterator->exception);
  if (p == (const PixelPacket *) NULL)
    return((PixelWand **) NULL);
  indexes=GetCacheViewIndexes(iterator->view);
  for (x=0; x < (long) iterator->region.width; x++)
  {
    PixelSetQuantumColor(iterator->pixel_wand[x],p);
    if (iterator->view->image->colorspace == CMYKColorspace)
      PixelSetBlackQuantum(iterator->pixel_wand[x],indexes[x]);
    else
      if (iterator->view->image->storage_class == PseudoClass)
        PixelSetIndex(iterator->pixel_wand[x],indexes[x]);
    p++;
  }
  *number_wands=iterator->region.width;
  return(iterator->pixel_wand);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l R e s e t I t e r a t o r                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelResetIterator() resets the pixel iterator.  Use it in conjunction
%  with PixelGetNextPixel() to iterate over all the pixels in a pixel
%  container.
%
%  The format of the PixelResetIterator method is:
%
%      void PixelResetIterator(PixelIterator *iterator)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
*/
WandExport void PixelResetIterator(PixelIterator *iterator)
{
  assert(iterator != (PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  iterator->y=(-1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l S e t F i r s t I t e r a t o r R o w                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelSetFirstIteratorRow() sets the pixel iterator to the first pixel row.
%
%  The format of the PixelSetFirstIteratorRow method is:
%
%      void PixelSetFirstIteratorRow(PixelIterator *iterator)
%
%  A description of each parameter follows:
%
%    o iterator: The magick iterator.
%
*/
WandExport void PixelSetFirstIteratorRow(PixelIterator *iterator)
{
  assert(iterator != (PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  iterator->y=iterator->region.y-1;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l S e t I t e r a t o r R o w                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelSetIteratorRow() set the pixel iterator row.
%
%  The format of the PixelSetIteratorRow method is:
%
%      MagickBooleanType PixelSetIteratorRow(PixelIterator *iterator,
%        const long row)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%
*/
WandExport MagickBooleanType PixelSetIteratorRow(PixelIterator *iterator,
  const long row)
{
  assert(iterator != (const PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  if ((row < 0) || (row >= (long) iterator->region.height))
    return(MagickFalse);
  iterator->y=row;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l S e t L a s t I t e r a t o r R o w                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelSetLastIteratorRow() sets the pixel iterator to the last pixel row.
%
%  The format of the PixelSetLastIteratorRow method is:
%
%      void PixelSetLastIteratorRow(PixelIterator *iterator)
%
%  A description of each parameter follows:
%
%    o iterator: The magick iterator.
%
*/
WandExport void PixelSetLastIteratorRow(PixelIterator *iterator)
{
  assert(iterator != (PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  iterator->y=(long) iterator->region.height;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i x e l S y n c I t e r a t o r                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PixelSyncIterator() syncs the pixel iterator.
%
%  The format of the PixelSyncIterator method is:
%
%      MagickBooleanType PixelSyncIterator(PixelIterator *iterator)
%
%  A description of each parameter follows:
%
%    o iterator: The pixel iterator.
%
%
*/
WandExport MagickBooleanType PixelSyncIterator(PixelIterator *iterator)
{
  IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *p;

  assert(iterator != (const PixelIterator *) NULL);
  assert(iterator->signature == MagickSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  p=GetCacheView(iterator->view,iterator->region.x,iterator->region.y+
    iterator->y,iterator->region.width,1);
  if (p == (PixelPacket *) NULL)
    {
      InheritException(&iterator->exception,&iterator->view->image->exception);
      return(MagickFalse);
    }
  indexes=GetCacheViewIndexes(iterator->view);
  for (x=0; x < (long) iterator->region.width; x++)
  {
    PixelGetQuantumColor(iterator->pixel_wand[x],p);
    if (iterator->view->image->colorspace == CMYKColorspace)
      indexes[x]=PixelGetBlackQuantum(iterator->pixel_wand[x]);
    else
      if (iterator->view->image->storage_class == PseudoClass)
        indexes[x]=PixelGetIndex(iterator->pixel_wand[x]);
    p++;
  }
  if (SyncCacheView(iterator->view) == MagickFalse)
    {
      InheritException(&iterator->exception,&iterator->view->image->exception);
      return(MagickFalse);
    }
  return(MagickTrue);
}
