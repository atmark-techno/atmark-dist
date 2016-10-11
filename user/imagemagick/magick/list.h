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

  ImageMagick image list methods.
*/
#ifndef _MAGICK_LIST_H
#define _MAGICK_LIST_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern MagickExport Image
  *CloneImageList(const Image *,ExceptionInfo *),
  *DestroyImageList(Image *),
  *GetFirstImageInList(const Image *),
  *GetImageFromList(const Image *,const long),
  *GetLastImageInList(const Image *),
  *GetNextImageInList(const Image *),
  *GetPreviousImageInList(const Image *),
  **ImageListToArray(const Image *,ExceptionInfo *),
  *NewImageList(void),
  *RemoveImageFromList(Image **),
  *RemoveLastImageFromList(Image **),
  *RemoveFirstImageFromList(Image **),
  *SplitImageList(Image *),
  *SyncNextImageInList(const Image *);

extern MagickExport long
  GetImageIndexInList(const Image *);

extern MagickExport unsigned long
  GetImageListLength(const Image *);

extern MagickExport void
  AppendImageToList(Image **,const Image *),
  DeleteImageFromList(Image **),
  InsertImageInList(Image **,Image *),
  PrependImageToList(Image **,Image *),
  ReplaceImageInList(Image **,Image *),
  ReverseImageList(Image **),
  SpliceImageIntoList(Image **,const unsigned long,const Image *),
  SyncImageList(Image *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
