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

  ImageMagick private image colorspace methods.
*/
#ifndef _MAGICK_COLORSPACE_PRIVATE_H
#define _MAGICK_COLORSPACE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static inline void RGBtoCMYK(MagickPixelPacket *pixel)
{
  MagickRealType
    black,
    cyan,
    magenta,
    yellow;
                                                                                
  cyan=QuantumRange-pixel->red;
  magenta=QuantumRange-pixel->green;
  yellow=QuantumRange-pixel->blue;
  black=cyan < magenta ? Min(cyan,yellow) : Min(magenta,yellow);
  pixel->colorspace=CMYKColorspace;
  pixel->red=cyan;
  pixel->green=magenta;
  pixel->blue=yellow;
  pixel->index=black;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
