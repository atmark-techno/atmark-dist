/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               PPPP   RRRR    OOO   FFFFF  IIIII  L      EEEEE               %
%               P   P  R   R  O   O  F        I    L      E                   %
%               PPPP   RRRR   O   O  FFF      I    L      EEE                 %
%               P      R R    O   O  F        I    L      E                   %
%               P      R  R    OOO   F      IIIII  LLLLL  EEEEE               %
%                                                                             %
%                                                                             %
%                      ImageMagick Image Profile Methods                      %
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
#include "magick/color.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/profile.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/utility.h"
#if defined(HasLCMS)
#if defined(HAVE_LCMS_LCMS_H)
#include <lcms/lcms.h>
#else
#include "lcms.h"
#endif
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e P r o f i l e s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageProfiles() clones one or more image profiles.
%
%  The format of the CloneImageProfiles method is:
%
%      MagickBooleanType CloneImageProfiles(Image *image,
%        const Image *clone_image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o clone_image: The clone image.
%
%
*/
MagickExport MagickBooleanType CloneImageProfiles(Image *image,
  const Image *clone_image)
{
  const char
    *name;

  const StringInfo
    *profile;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(clone_image != (const Image *) NULL);
  assert(clone_image->signature == MagickSignature);
  if (clone_image->profiles == (SplayTreeInfo *) NULL)
    return(MagickTrue);
  ResetImageProfileIterator(clone_image);
  for (name=GetNextImageProfile(clone_image); name != (char *) NULL; )
  {
    profile=GetImageProfile(clone_image,name);
    if (profile != (StringInfo *) NULL)
      (void) SetImageProfile(image,name,profile);
    name=GetNextImageProfile(clone_image);
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e P r o f i l e s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageProfiles() releases memory associated with an image profile map.
%
%  The format of the DestroyProfiles method is:
%
%      void DestroyImageProfiles(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport void DestroyImageProfiles(Image *image)
{
  if (image->profiles != (SplayTreeInfo *) NULL)
    image->profiles=DestroySplayTree((SplayTreeInfo *) image->profiles);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e P r o f i l e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageProfile() gets a profile associated with an image by name.
%
%  The format of the GetImageProfile method is:
%
%      StringInfo *GetImageProfile(const Image *image,const char *name)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o name: The profile name.
%
*/
MagickExport StringInfo *GetImageProfile(const Image *image,const char *name)
{
  char
    key[MaxTextExtent];

  StringInfo
    *profile;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->profiles == (SplayTreeInfo *) NULL)
    return((StringInfo *) NULL);
  (void) CopyMagickString(key,name,MaxTextExtent);
  LocaleLower(key);
  profile=(StringInfo *)
    GetValueFromSplayTree((SplayTreeInfo *) image->profiles,key);
  return(profile);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t I m a g e P r o f i l e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextImageProfile() gets the next profile name for an image.
%
%  The format of the GetNextImageProfile method is:
%
%      char *GetNextImageProfile(const Image *image)
%
%  A description of each parameter follows:
%
%    o hash_info: The hash info.
%
*/
MagickExport char *GetNextImageProfile(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->profiles == (SplayTreeInfo *) NULL)
    return((char *) NULL);
  return((char *) GetNextKeyInSplayTree((SplayTreeInfo *) image->profiles));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P r o f i l e I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ProfileImage() adds or removes a ICC, IPTC, or generic profile from an
%  image.  If the profile is NULL, it is removed from the image otherwise
%  added.  Use a name of '*' and a profile of NULL to remove all profiles
%  from the image.
%
%  The format of the ProfileImage method is:
%
%      MagickBooleanType ProfileImage(Image *image,const char *name,
%        const void *datum,const unsigned long length,
%        const MagickBooleanType clone)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o name: Name of profile to add or remove: ICC, IPTC, or generic profile.
%
%    o datum: The profile data.
%
%    o length: The length of the profile.
%
%    o clone: should be MagickFalse.
%
%
*/

#if defined(HasLCMS)
#if defined(LCMS_VERSION) && (LCMS_VERSION > 1010)
static int LCMSErrorHandler(int severity,const char *context)
{
  (void) LogMagickEvent(TransformEvent,GetMagickModule(),"lcms: #%d, %s",
    severity,context != (char *) NULL ? context : "no context");
  return(MagickTrue);
}
#endif
#endif

MagickExport MagickBooleanType ProfileImage(Image *image,const char *name,
  const void *datum,const unsigned long length,const MagickBooleanType clone)
{
#define ThrowProfileException(severity,tag,context) \
{ \
  (void) cmsCloseProfile(source_profile); \
  (void) cmsCloseProfile(target_profile); \
  ThrowBinaryException(severity,tag,context); \
}

  MagickBooleanType
    status;

  StringInfo
    *profile;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(name != (const char *) NULL);
  if ((datum == (const void *) NULL) || (length == 0))
    {
      if (*name == '*')
        {
          DestroyImageProfiles(image);
          return(MagickTrue);
        }
      return(RemoveImageProfile(image,name));
    }
  /*
    Add a ICC, IPTC, or generic profile to the image.
  */
  profile=AcquireStringInfo((size_t) length);
  SetStringInfoDatum(profile,(unsigned char *) datum);
  if ((LocaleCompare("icc",name) == 0) || (LocaleCompare("icm",name) == 0))
    {
      StringInfo
        *icc_profile;

      icc_profile=GetImageProfile(image,"icc");
      if ((icc_profile != (StringInfo *) NULL) &&
          (CompareStringInfo(icc_profile,profile) == 0))
        {
          profile=DestroyStringInfo(profile);
          return(MagickTrue);
        }
#if !defined(HasLCMS)
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        MissingDelegateWarning,"LCMSLibraryIsNotAvailable","`%s'",
        image->filename);
#else
      if (icc_profile != (StringInfo *) NULL)
        {
          ColorspaceType
            source_colorspace,
            target_colorspace;

          cmsHPROFILE
            source_profile,
            target_profile;

          cmsHTRANSFORM
            transform;

          DWORD
            source_type,
            target_type;

          IndexPacket
            *indexes;

          int
            intent;

          long
            y;

          register long
            x;

          register PixelPacket
            *q;

          register unsigned short
            *p;

          size_t
            length,
            source_channels,
            target_channels;

          unsigned short
            *source_pixels,
            *target_pixels;

          /*
            Transform pixel colors as defined by the color profiles.
          */
#if defined(LCMS_VERSION) && (LCMS_VERSION > 1010)
          cmsSetErrorHandler(LCMSErrorHandler);
#else
          (void) cmsErrorAction(LCMS_ERROR_SHOW);
#endif
          source_profile=cmsOpenProfileFromMem(icc_profile->datum,(DWORD)
            icc_profile->length);
          target_profile=cmsOpenProfileFromMem(profile->datum,(DWORD)
            profile->length);
          if ((source_profile == (cmsHPROFILE) NULL) ||
              (target_profile == (cmsHPROFILE) NULL))
            ThrowBinaryException(ResourceLimitError,"UnableToManageColor",name);
          switch (cmsGetColorSpace(source_profile))
          {
            case icSigCmykData:
            {
              source_colorspace=CMYKColorspace;
              source_type=(DWORD) TYPE_CMYK_16;
              source_channels=4;
              break;
            }
            case icSigYCbCrData:
            {
              source_colorspace=YCbCrColorspace;
              source_type=(DWORD) TYPE_YCbCr_16;
              source_channels=3;
              break;
            }
            case icSigLabData:
            {
              source_colorspace=LABColorspace;
              source_type=(DWORD) TYPE_Lab_16;
              source_channels=3;
              break;
            }
            case icSigLuvData:
            {
              source_colorspace=YUVColorspace;
              source_type=(DWORD) TYPE_YUV_16;
              source_channels=3;
              break;
            }
            case icSigGrayData:
            {
              source_colorspace=GRAYColorspace;
              source_type=(DWORD) TYPE_GRAY_16;
              source_channels=1;
              break;
            }
            case icSigRgbData:
            {
              source_colorspace=RGBColorspace;
              source_type=(DWORD) TYPE_RGB_16;
              source_channels=3;
              break;
            }
            default:
            {
              source_colorspace=UndefinedColorspace;
              source_type=(DWORD) TYPE_RGB_16;
              source_channels=3;
              break;
            }
          }
          switch (cmsGetColorSpace(target_profile))
          {
            case icSigCmykData:
            {
              target_colorspace=CMYKColorspace;
              target_type=(DWORD) TYPE_CMYK_16;
              target_channels=4;
              break;
            }
            case icSigYCbCrData:
            {
              target_colorspace=YCbCrColorspace;
              target_type=(DWORD) TYPE_YCbCr_16;
              target_channels=3;
              break;
            }
            case icSigLabData:
            {
              target_colorspace=LABColorspace;
              target_type=(DWORD) TYPE_Lab_16;
              target_channels=3;
              break;
            }
            case icSigLuvData:
            {
              target_colorspace=YUVColorspace;
              target_type=(DWORD) TYPE_YUV_16;
              target_channels=3;
              break;
            }
            case icSigGrayData:
            {
              target_colorspace=GRAYColorspace;
              target_type=(DWORD) TYPE_GRAY_16;
              target_channels=1;
              break;
            }
            case icSigRgbData:
            {
              target_colorspace=RGBColorspace;
              target_type=(DWORD) TYPE_RGB_16;
              target_channels=3;
              break;
            }
            default:
            {
              target_colorspace=UndefinedColorspace;
              target_type=(DWORD) TYPE_RGB_16;
              target_channels=3;
              break;
            }
          }
          if (((source_colorspace == UndefinedColorspace) ||
               (target_colorspace == UndefinedColorspace)) ||
              ((source_colorspace == GRAYColorspace) &&
               (!IsGrayImage(image,&image->exception))) ||
              ((source_colorspace == CMYKColorspace) &&
               (image->colorspace != CMYKColorspace)) ||
              ((source_colorspace != GRAYColorspace) &&
               (source_colorspace != LABColorspace) &&
               (source_colorspace != CMYKColorspace) &&
               (image->colorspace != RGBColorspace)))
            ThrowProfileException(ImageError,"ColorspaceColorProfileMismatch",
              name);
          switch (image->rendering_intent)
          {
            case AbsoluteIntent: intent=INTENT_ABSOLUTE_COLORIMETRIC; break;
            case PerceptualIntent: intent=INTENT_PERCEPTUAL; break;
            case RelativeIntent: intent=INTENT_RELATIVE_COLORIMETRIC; break;
            case SaturationIntent: intent=INTENT_SATURATION; break;
            default: intent=INTENT_PERCEPTUAL; break;
          }
          transform=cmsCreateTransform(source_profile,source_type,
            target_profile,target_type,intent,cmsFLAGS_HIGHRESPRECALC);
          (void) cmsCloseProfile(source_profile);
          (void) cmsCloseProfile(target_profile);
          if (transform == (cmsHTRANSFORM) NULL)
            {
              cmsDeleteTransform(transform);
              ThrowBinaryException(ImageError,"UnableToCreateColorTransform",
                name);
            }
          /*
            Transform image as dictated by the source and target image profiles.
          */
          length=(size_t) image->columns*source_channels*sizeof(*source_pixels);
          source_pixels=(unsigned short *) AcquireMagickMemory(length);
          length=(size_t) image->columns*target_channels*sizeof(*target_pixels);
          target_pixels=(unsigned short *) AcquireMagickMemory(length);
          if ((source_pixels == (unsigned short *) NULL) ||
              (target_pixels == (unsigned short *) NULL))
            {
              cmsDeleteTransform(transform);
              ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
                image->filename);
            }
          image->storage_class=DirectClass;
          if (target_colorspace == CMYKColorspace)
            image->colorspace=target_colorspace;
          for (y=0; y < (long) image->rows; y++)
          {
            q=GetImagePixels(image,0,y,image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            p=source_pixels;
            indexes=GetIndexes(image);
            for (x=0; x < (long) image->columns; x++)
            {
              *p++=ScaleQuantumToShort(q->red);
              if (source_channels > 1)
                {
                  *p++=ScaleQuantumToShort(q->green);
                  *p++=ScaleQuantumToShort(q->blue);
                }
              if (source_channels > 3)
                *p++=ScaleQuantumToShort(indexes[x]);
              q++;
            }
            cmsDoTransform(transform,source_pixels,target_pixels,(unsigned int)
              image->columns);
            p=target_pixels;
            q-=image->columns;
            for (x=0; x < (long) image->columns; x++)
            {
              q->red=ScaleShortToQuantum(*p);
              p++;
              if (target_channels > 1)
                {
                  q->green=ScaleShortToQuantum(*p);
                  p++;
                  q->blue=ScaleShortToQuantum(*p);
                  p++;
                }
              if (target_channels > 3)
                {
                  indexes[x]=ScaleShortToQuantum(*p);
                  p++;
                }
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
          image->colorspace=target_colorspace;
          target_pixels=(unsigned short *)
            RelinquishMagickMemory(target_pixels);
          source_pixels=(unsigned short *)
            RelinquishMagickMemory(source_pixels);
          cmsDeleteTransform(transform);
        }
#endif
    }
  status=SetImageProfile(image,name,profile);
  profile=DestroyStringInfo(profile);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e m o v e I m a g e P r o f i l e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveImageProfile() removes a profile from the image-map by its name.
%
%  The format of the RemoveImageProfile method is:
%
%      MagickBooleanTyupe RemoveImageProfile(Image *image,const char *name)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o name: The profile name.
%
*/
MagickExport MagickBooleanType RemoveImageProfile(Image *image,
  const char *name)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->profiles == (SplayTreeInfo *) NULL)
    return(MagickFalse);
  if (LocaleCompare(name,"icc") == 0)
    {
      /*
        Continue to support deprecated color profile for now.
      */
      image->color_profile.length=0;
      image->color_profile.info=(unsigned char *) NULL;
    }
  if (LocaleCompare(name,"iptc") == 0)
    {
      /*
        Continue to support deprecated IPTC profile for now.
      */
      image->iptc_profile.length=0;
      image->iptc_profile.info=(unsigned char *) NULL;
    }
  return(RemoveNodeFromSplayTree((SplayTreeInfo *) image->profiles,name));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t P r o f i l e I t e r a t o r                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImageProfileIterator() resets the image profile iterator.  Use it in
%  conjunction with GetNextImageProfile() to iterate over all the profiles
%  associated with an image.
%
%  The format of the ResetImageProfileIterator method is:
%
%      ResetImageProfileIterator(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport void ResetImageProfileIterator(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->profiles == (SplayTreeInfo *) NULL)
    return;
  ResetSplayTreeIterator((SplayTreeInfo *) image->profiles);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e P r o f i l e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageProfile() adds a named profile to the image.  If a profile with the
%  same name already exists, it is replaced.  This method differs from the
%  ProfileImage() method in that it does not apply CMS color profiles.
%
%  The format of the SetImageProfile method is:
%
%      MagickBooleanType SetImageProfile(Image *image,const char *name,
%        const StringInfo *profile)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o name: The profile name.
%
%    o profile: A StringInfo structure that contains the named profile.
%
*/

static void *DestroyProfile(void *profile)
{
  profile=(void *) DestroyStringInfo((StringInfo *) profile);
  return(profile);
}

MagickExport MagickBooleanType SetImageProfile(Image *image,const char *name,
  const StringInfo *profile)
{
  char
    key[MaxTextExtent];

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->profiles == (SplayTreeInfo *) NULL)
    image->profiles=NewSplayTree(CompareSplayTreeString,RelinquishMagickMemory,
      DestroyProfile);
  (void) CopyMagickString(key,name,MaxTextExtent);
  LocaleLower(key);
  status=AddValueToSplayTree((SplayTreeInfo *) image->profiles,
    ConstantString(AcquireString(key)),CloneStringInfo(profile));
  if ((status != MagickFalse) &&
      ((LocaleCompare(name,"icc") == 0) || (LocaleCompare(name,"icm") == 0)))
    {
      const StringInfo
        *icc_profile;

      /*
        Continue to support deprecated color profile member.
      */
      icc_profile=GetImageProfile(image,name);
      if (icc_profile != (StringInfo *) NULL)
        {
          image->color_profile.length=icc_profile->length;
          image->color_profile.info=icc_profile->datum;
        }
    }
  if ((status != MagickFalse) &&
      ((LocaleCompare(name,"iptc") == 0) || (LocaleCompare(name,"8bim") == 0)))
    {
      const StringInfo
        *iptc_profile;

      /*
        Continue to support deprecated IPTC profile member.
      */
      iptc_profile=GetImageProfile(image,name);
      if (iptc_profile != (StringInfo *) NULL)
        {
          image->iptc_profile.length=iptc_profile->length;
          image->iptc_profile.info=iptc_profile->datum;
        }
    }
  return(status);
}
