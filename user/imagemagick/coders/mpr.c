/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            M   M  PPPP   RRRR                               %
%                            MM MM  P   P  R   R                              %
%                            M M M  PPPP   RRRR                               %
%                            M   M  P      R R                                %
%                            M   M  P      R  R                               %
%                                                                             %
%                                                                             %
%                  Read/Write the Magick Persistent Registry.                 %
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
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/registry.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteMPRImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d M P R I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadMPRImage() reads a Magick Persistent Registry image as a blob from
%  memory.  It allocates the memory necessary for the new Image structure and
%  returns a pointer to the new image.
%
%  The format of the ReadMPRImage method is:
%
%      Image *ReadMPRImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadMPRImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    *p;

  long
    id;

  RegistryType
    type;

  size_t
    length;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if (LocaleCompare(image_info->magick,"MPRI") != 0)
    return(GetImageFromMagickRegistry(image_info->filename,&id,exception));
  id=strtol(image_info->filename,&p,0);
  return((Image *) GetMagickRegistry(id,&type,&length,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M P R I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterMPRImage() adds attributes for the MPR image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterMPRImage method is:
%
%      RegisterMPRImage(void)
%
*/
ModuleExport void RegisterMPRImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("MPR");
  entry->decoder=(DecoderHandler *) ReadMPRImage;
  entry->encoder=(EncoderHandler *) WriteMPRImage;
  entry->adjoin=MagickFalse;
  entry->stealth=MagickTrue;
  entry->description=AcquireString("Magick Persistent Registry");
  entry->module=AcquireString("MPR");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("MPRI");
  entry->decoder=(DecoderHandler *) ReadMPRImage;
  entry->encoder=(EncoderHandler *) WriteMPRImage;
  entry->stealth=MagickTrue;
  entry->adjoin=MagickFalse;
  entry->stealth=MagickTrue;
  entry->description=AcquireString("Magick Persistent Registry");
  entry->module=AcquireString("MPRI");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r M P R I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterMPRImage() removes format registrations made by the
%  MPR module from the list of supported formats.
%
%  The format of the UnregisterMPRImage method is:
%
%      UnregisterMPRImage(void)
%
*/
ModuleExport void UnregisterMPRImage(void)
{
  (void) UnregisterMagickInfo("MPR");
  (void) UnregisterMagickInfo("MPRI");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e M P R I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteMPRImage() writes an image into the Magick Persistent Registry
%  image as a blob from memory.  It allocates the memory necessary for the
%  new Image structure and returns a pointer to the new image.
%
%  The format of the WriteMPRImage method is:
%
%      MagickBooleanType WriteMPRImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteMPRImage(const ImageInfo *image_info,Image *image)
{
  ExceptionInfo
    exception;

  Image
    *registry_image;

  long
    id;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  GetExceptionInfo(&exception);
  registry_image=GetImageFromMagickRegistry(image->filename,&id,&exception);
  DestroyExceptionInfo(&exception);
  if (registry_image != (Image *) NULL)
    {
      (void) DeleteMagickRegistry(id);
      registry_image=DestroyImage(registry_image);
    }
  id=SetMagickRegistry(ImageRegistryType,image,sizeof(Image),&image->exception);
  return((MagickBooleanType) (id >= 0));
}
