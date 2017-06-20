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

  ImageMagick splay tree methods.
*/
#ifndef _MAGICK_SPLAY_H
#define _MAGICK_SPLAY_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _SplayTreeInfo
  SplayTreeInfo;

extern MagickExport MagickBooleanType
  AddValueToSplayTree(SplayTreeInfo *,const void *,const void *),
  RemoveNodeByValueFromSplayTree(SplayTreeInfo *,const void *),
  RemoveNodeFromSplayTree(SplayTreeInfo *,const void *);

extern MagickExport int
  CompareSplayTreeString(const void *,const void *),
  CompareSplayTreeStringInfo(const void *,const void *);

extern MagickExport SplayTreeInfo
  *DestroySplayTree(SplayTreeInfo *),
  *NewSplayTree(int (*)(const void *,const void *),void *(*)(void *),
    void *(*)(void *));

extern MagickExport unsigned long
  GetNumberOfNodesInSplayTree(const SplayTreeInfo *);

extern MagickExport void
  *GetNextKeyInSplayTree(SplayTreeInfo *),
  *GetNextValueInSplayTree(SplayTreeInfo *),
  *GetValueFromSplayTree(SplayTreeInfo *,const void *),
  ResetSplayTreeIterator(SplayTreeInfo *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif