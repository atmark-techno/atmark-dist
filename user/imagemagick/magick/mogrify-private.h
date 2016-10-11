/*
  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Image private mogrify command methods.
*/
#ifndef _MAGICK_MOGRIFY_PRIVATE_H
#define _MAGICK_MOGRIFY_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define MaxImageStackDepth  32
#define MogrifyImageStack(image,advance,fire) \
if ((j <= i) && (i < argc)) \
  { \
    if ((image) == (Image *) NULL) \
      status&=MogrifyImageInfo(image_info,(int) (i-j+1),(const char **) \
        argv+j,exception); \
    else \
      if ((fire) != MagickFalse) \
        { \
          status&=MogrifyImages(image_info,(int) (i-j+1),(const char **) \
            argv+j,&(image),exception); \
          if ((advance) != MagickFalse) \
            j=i+1; \
          pend=MagickFalse; \
        } \
  }

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
