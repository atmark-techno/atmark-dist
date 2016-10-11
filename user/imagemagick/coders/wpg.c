/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                            W   W  PPPP    GGGG                              %
%                            W   W  P   P  G                                  %
%                            W W W  PPPP   G GGG                              %
%                            WW WW  P      G   G                              %
%                            W   W  P       GGG                               %
%                                                                             %
%                                                                             %
%                       Read WordPerfect Image Format.                        %
%                                                                             %
%                              Software Design                                %
%                              Jaroslav Fojtik                                %
%                                 June 2000                                   %
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
#include "magick/color-private.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/cache.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magic.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/resource_.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s W P G                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsWPG()() returns MagickTrue if the image format type, identified by the
%  magick string, is WPG.
%
%  The format of the IsWPG method is:
%
%      MagickBooleanType IsWPG(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static MagickBooleanType IsWPG(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"\377WPC",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}


static void Rd_WP_DWORD(Image *image,unsigned long *d)
{
  unsigned char
    b;

  b=ReadBlobByte(image);
  *d=b;
  if (b < 0xFF)
    return;
  b=ReadBlobByte(image);
  *d=(unsigned long) b;
  b=ReadBlobByte(image);
  *d+=(unsigned long) b*256l;
  if (*d < 0x8000)
    return;
  *d=(*d & 0x7FFF) << 16;
  b=ReadBlobByte(image);
  *d+=(unsigned long) b;
  b=ReadBlobByte(image);
  *d+=(unsigned long) b*256l;
  return;
}

static void InsertRow(unsigned char *p,long y,Image *image, int bpp)
{
  int
    bit;

  long
    x;

  register PixelPacket
    *q;

  IndexPacket
    index;

  register IndexPacket
    *indexes;

  switch (bpp)
    {
    case 1:  /* Convert bitmap scanline. */
      {
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < ((long) image->columns-7); x+=8)
          {
            for (bit=0; bit < 8; bit++)
              {
                index=((*p) & (0x80 >> bit) ? 0x01 : 0x00);
                indexes[x+bit]=index;
                *q++=image->colormap[index];
              }
            p++;
          }
        if ((image->columns % 8) != 0)
          {
            for (bit=0; bit < (long) (image->columns % 8); bit++)
              {
                index=((*p) & (0x80 >> bit) ? 0x01 : 0x00);
                indexes[x+bit]=index;
                *q++=image->colormap[index];
              }
            p++;
          }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        break;
      }
    case 2:  /* Convert PseudoColor scanline. */
      {
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < ((long) image->columns-1); x+=2)
          {
            index=ConstrainColormapIndex(image,(*p >> 6) & 0x3);
            indexes[x]=index;
            *q++=image->colormap[index];
            index=ConstrainColormapIndex(image,(*p >> 4) & 0x3);
            indexes[x]=index;
            *q++=image->colormap[index];
            index=ConstrainColormapIndex(image,(*p >> 2) & 0x3);
            indexes[x]=index;
            *q++=image->colormap[index];
            index=ConstrainColormapIndex(image,(*p) & 0x3);
            indexes[x+1]=index;
            *q++=image->colormap[index];
            p++;
          }
        if ((image->columns % 4) != 0)
          {
            index=ConstrainColormapIndex(image,(*p >> 6) & 0x3);
            indexes[x]=index;
            *q++=image->colormap[index];
            if ((image->columns % 4) >= 1)

              {
                index=ConstrainColormapIndex(image,(*p >> 4) & 0x3);
                indexes[x]=index;
                *q++=image->colormap[index];
                if ((image->columns % 4) >= 2)

                  {
                    index=ConstrainColormapIndex(image,(*p >> 2) & 0x3);
                    indexes[x]=index;
                    *q++=image->colormap[index];
                  }
              }
            p++;
          }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        break;
      }

    case 4:  /* Convert PseudoColor scanline. */
      {
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < ((long) image->columns-1); x+=2)
          {
            index=ConstrainColormapIndex(image,(*p >> 4) & 0x0f);
            indexes[x]=index;
            *q++=image->colormap[index];
            index=ConstrainColormapIndex(image,(*p) & 0x0f);
            indexes[x+1]=index;
            *q++=image->colormap[index];
            p++;
          }
        if ((image->columns % 2) != 0)
          {
            index=ConstrainColormapIndex(image,(*p >> 4) & 0x0f);
            indexes[x]=index;
            *q++=image->colormap[index];
            p++;
          }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        break;
      }
    case 8: /* Convert PseudoColor scanline. */
      {
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL) break;
        indexes=GetIndexes(image);

        for (x=0; x < (long) image->columns; x++)
          {
            index=ConstrainColormapIndex(image,*p);
            indexes[x]=index;
            *q++=image->colormap[index];
            p++;
          }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      break;

    case 24:     /*  Convert DirectColor scanline.  */
      q=SetImagePixels(image,0,y,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      for (x=0; x < (long) image->columns; x++)
        {
          q->red=ScaleCharToQuantum(*p++);
          q->green=ScaleCharToQuantum(*p++);
          q->blue=ScaleCharToQuantum(*p++);
          q++;
        }
      if (SyncImagePixels(image) == MagickFalse)
        break;
      break;
    }
}

#define InsertByte(b) \
{ \
  BImgBuff[x]=b; \
  x++; \
  if((long) x>=ldblk) \
  { \
    InsertRow(BImgBuff,(long) y,image,bpp); \
    x=0; \
    y++; \
    } \
}
static int UnpackWPGRaster(Image *image,int bpp)
{
  long
    ldblk,
    y;

  register long
    i,
    x;

  unsigned char
    bbuf,
    *BImgBuff,
    RunCount;

  x=0;
  y=0;

  ldblk=(long) ((bpp*image->columns+7)/8);
  BImgBuff=(unsigned char *) AcquireMagickMemory(ldblk);
  if(BImgBuff==NULL) return(-2);

  while (y < (long) image->rows)
  {
      bbuf=ReadBlobByte(image);

      /*
        if not readed byte ??????
        {
          delete Raster;
          Raster=NULL;
          return(-2);
        }
      */

      RunCount=bbuf & 0x7F;
      if(bbuf & 0x80)
        {
          if(RunCount)  /* repeat next byte runcount * */
            {
              bbuf=ReadBlobByte(image);
              for(i=0;i<(int) RunCount;i++) InsertByte(bbuf);
            }
          else {  /* read next byte as RunCount; repeat 0xFF runcount* */
            RunCount=ReadBlobByte(image);
            for(i=0;i<(int) RunCount;i++) InsertByte(0xFF);
          }
        }
      else {
        if(RunCount)   /* next runcount byte are readed directly */
          {
            for(i=0;i < (int) RunCount;i++)
              {
                bbuf=ReadBlobByte(image);
                InsertByte(bbuf);
              }
          }
        else {  /* repeat previous line runcount* */
          RunCount=ReadBlobByte(image);
          if(x) {    /* attempt to duplicate row from x position: */
            /* I do not know what to do here */
            BImgBuff=(unsigned char *) RelinquishMagickMemory(BImgBuff);
            return(-3);
          }
          for(i=0;i < (int) RunCount;i++)
            {
              x=0;
              y++;    /* Here I need to duplicate previous row RUNCOUNT* */
              if(y<2) continue;
              if(y>(long) image->rows)
                {
                  BImgBuff=(unsigned char *) RelinquishMagickMemory(BImgBuff);
                  return(-4);
                }
              InsertRow(BImgBuff,y-1,image,bpp);
            }
        }
      }
    }
  BImgBuff=(unsigned char *) RelinquishMagickMemory(BImgBuff);
  return(0);
}

static int UnpackWPG2Raster(Image *image,int bpp)
{
  char
    SampleSize=1;

  register long
    i,
    x;

  long
    ldblk,
    y;

  unsigned char
    bbuf,
    *BImgBuff,
    RunCount,
    SampleBuffer[8];

  x=0;
  y=0;
  ldblk=(long) ((bpp*image->columns+7)/8);
  BImgBuff=(unsigned char *) AcquireMagickMemory(ldblk);
  if(BImgBuff==NULL)
    return(-2);

  while(y<(long) image->rows)
    {
      bbuf=ReadBlobByte(image);

      switch(bbuf)
        {
        case 0x7D:
          SampleSize=ReadBlobByte(image);  /* DSZ */
          if (SampleSize > 8)
            return(-2);
          if (SampleSize < 1)
            return(-2);
          break;
        case 0x7E:
          fprintf(stderr,"\nUnsupported WPG token XOR, please report!");
          break;
        case 0x7F:
          RunCount=ReadBlobByte(image);   /* BLK */
          for(i=0; i < (long) SampleSize*((long)RunCount+1); i++)
          {
            InsertByte(0);
          }
          break;
        case 0xFD:
          RunCount=ReadBlobByte(image);   /* EXT */
          for (i=0; i <= (int) RunCount; i++)
            for (bbuf=0; bbuf < SampleSize; bbuf++)
              InsertByte(SampleBuffer[bbuf]);
          break;
        case 0xFE:
          RunCount=ReadBlobByte(image);  /* RST */
          if(x!=0)
            {
              fprintf(stderr,
                "\nUnsupported WPG2 unaligned token RST x=%ld, please report!\n"
                  ,x);
              return(-3);
            }
          {
            /* duplicate the previous row RunCount x */
           for(i=0;i<=RunCount;i++)
              {
              x=0;
              InsertRow(BImgBuff,(long) ( y< (long) image->rows ? y :
                image->rows-1),image,bpp);
              y++;
              if(y<2) continue;
              if(y> (long) image->rows) return(-4);
              }
          }
          break;
        case 0xFF:
          RunCount=ReadBlobByte(image);  /* WHT */
          for (i=0; i< (int) SampleSize*((int) RunCount+1); i++)
          {
            InsertByte(0xFF);
          }
          break;
        default:
          RunCount=bbuf & 0x7F;
          if (bbuf & 0x80)  /* REP */
            {
              for(i=0;i<(int) SampleSize;i++)
                SampleBuffer[i]=ReadBlobByte(image);
              for(i=0;i<=(int)RunCount;i++)
                for(bbuf=0;bbuf<SampleSize;bbuf++)
                  InsertByte(SampleBuffer[bbuf]);
            }
          else {  /* NRP */
            for(i=0;i<(int) SampleSize*((int)RunCount+1);i++)
              {
                bbuf=ReadBlobByte(image);
                InsertByte(bbuf);
              }
          }
        }

    }
  BImgBuff=(unsigned char *) RelinquishMagickMemory(BImgBuff);
  return(0);
}

unsigned LoadWPG2Flags(Image *image,char Precision,float *Angle)
{
const unsigned char TPR=1,TRN=2,SKW=4,SCL=8,ROT=0x10,OID=0x20,LCK=0x80;
long  x;
unsigned DenX;
unsigned Flags;
float CTM[3][3];        /* current transform matrix (currently ignored) */
                                                                                
 ResetMagickMemory(CTM,0,sizeof(CTM));     /* CTM.erase();CTM.resize(3,3) */
 CTM[0][0]=1;
 CTM[1][1]=1;
 CTM[2][2]=1;
                                                                                
 Flags=ReadBlobLSBShort(image);
 if(Flags & LCK) x=ReadBlobLSBLong(image);  /* Edit lock */
 if(Flags & OID)
  {
  if(Precision==0)
    {x=ReadBlobLSBShort(image);}  /* ObjectID */
  else
    {x=ReadBlobLSBLong(image);}  /* ObjectID (Double precision). */
  }
 if(Flags & ROT)
  {
  x=ReadBlobLSBLong(image);  /* Rot Angle. */
  if(Angle) *Angle=x/65536.0;
  }
 if(Flags & (ROT|SCL))
  {
  x=ReadBlobLSBLong(image); /* Sx*cos(). */
  CTM[0][0]=x;
  x=ReadBlobLSBLong(image); /* Sy*cos(). */
  CTM[1][1]=x;
  }
 if(Flags & (ROT|SKW))
  {
  x=ReadBlobLSBLong(image);       /* Kx*sin(). */
  CTM[1][0]=x;
  x=ReadBlobLSBLong(image);       /* Ky*sin(). */
  CTM[0][1]=x;
  }
 if(Flags & TRN)
  {
  x=ReadBlobLSBLong(image); DenX=ReadBlobLSBLong(image);  /* Tx */
  CTM[0][2]=(float)x + ((x >= 0)?1:-1)*(float)DenX/0x10000;
  x=ReadBlobLSBLong(image); DenX=ReadBlobLSBLong(image);  /* Ty */
  CTM[1][2]=(float)x + ((x >= 0)?1:-1)*(float)DenX/0x10000;
  }
 if(Flags & TPR)
  {
  x=ReadBlobLSBLong(image);  /* Px. */
  CTM[2][0]=x;
  x=ReadBlobLSBLong(image);  /* Py. */
  CTM[2][1]=x;
  }
 return(Flags);
}

static Image *ExtractPostscript(Image *image,const ImageInfo *image_info,
  MagickOffsetType PS_Offset,long PS_Size,ExceptionInfo *exception)
{
  const MagicInfo
    *magic_info;

  FILE
    *file;

  ImageInfo
    *clone_info;

  int
    c,
    unique_file;

  Image
    *image2;

  register int
    i;

  unsigned char
    magick[2*MaxTextExtent];

  if ((clone_info=CloneImageInfo(image_info)) == NULL)
    return(GetFirstImageInList(image));
  SetImageInfoBlob(clone_info,(void *) NULL,0);
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource((char *) clone_info->filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"wb");
  if ((unique_file == -1) || (file == NULL))
    goto FINISH;
  (void) SeekBlob(image,PS_Offset,SEEK_SET);
  for (i=0; i < PS_Size; i++)
  {
    c=ReadBlobByte(image);
    if (i <  (long) (2*MaxTextExtent))
      magick[i]=c;
    (void) fputc(c,file);
  }
  (void) fclose(file);
  magic_info=GetMagicInfo(magick,2*MaxTextExtent,exception);
  if ((magic_info == (const MagicInfo *) NULL) ||
      (GetMagicName(magic_info) == (char *) NULL) ||
      (exception->severity != UndefinedException))
    goto FINISH_UNL;
  (void) CopyMagickString(clone_info->magick,GetMagicName(magic_info),
    MaxTextExtent);
  image2=ReadImage(clone_info,exception);
  if (image2 == (Image *) NULL)
    goto FINISH_UNL;
  CopyMagickString(image2->filename,image->filename,MaxTextExtent);
  CopyMagickString(image2->magick_filename,image->magick_filename,
    MaxTextExtent);
  CopyMagickString(image2->magick,image->magick,MaxTextExtent);
  image2->depth=image->depth;
  DestroyBlob(image2);
  image2->blob=ReferenceBlob(image->blob);
  if ((image->rows == 0) || (image->columns == 0))
    DeleteImageFromList(&image);
  AppendImageToList(&image,image2);

 FINISH_UNL:
  (void) RelinquishUniqueFileResource(clone_info->filename);/* */
 FINISH:
  clone_info=DestroyImageInfo(clone_info);
  return(GetFirstImageInList(image));
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d W P G I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadWPGImage() reads an WPG X image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadWPGImage method is:
%
%    Image *ReadWPGImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadWPGImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  typedef struct
  {
    unsigned char
      red,
      blue,
      green;
  } ColorPalette;

  typedef struct
  {
    unsigned long FileId;
    MagickOffsetType DataOffset;
    unsigned int ProductType;
    unsigned int FileType;
    unsigned char MajorVersion;
    unsigned char MinorVersion;
    unsigned int EncryptKey;
    unsigned int Reserved;
  } WPGHeader;

  typedef struct
  {
    unsigned char  RecType;
    unsigned long   RecordLength;
  } WPGRecord;

  typedef struct
  {
    unsigned char  Class;
    unsigned char    RecType;
    unsigned long   Extension;
    unsigned long   RecordLength;
  } WPG2Record;

  typedef struct
  {
    unsigned  HorizontalUnits;
    unsigned  VerticalUnits;
    unsigned char PosSizePrecision;
  } WPG2Start;

  typedef struct
  {
    unsigned int Width;
    unsigned int Heigth;
    unsigned int Depth;
    unsigned int HorzRes;
    unsigned int VertRes;
  } WPGBitmapType1;

  typedef struct
  {
    unsigned int Width;
    unsigned int Heigth;
    unsigned char Depth;
    unsigned char Compression;
  } WPG2BitmapType1;

  typedef struct
  {
    unsigned int RotAngle;
    unsigned int LowLeftX;
    unsigned int LowLeftY;
    unsigned int UpRightX;
    unsigned int UpRightY;
    unsigned int Width;
    unsigned int Heigth;
    unsigned int Depth;
    unsigned int HorzRes;
    unsigned int VertRes;
  } WPGBitmapType2;

  typedef struct
  {
    unsigned int StartIndex;
    unsigned int NumOfEntries;
  } WPGColorMapRec;

  typedef struct {
    unsigned long PS_unknown1;
    unsigned int PS_unknown2;
    unsigned int PS_unknown3;
  } WPGPSl1Record;

  Image
    *image,
    *p;

  long
    scene;

  static const ColorPalette
    WPG1Palette[256]=
    {
      {  0,   0,   0}, {  0,   0, 168}, {  0, 168,   0}, {  0, 168, 168},
      {168,   0,   0}, {168,   0, 168}, {168,  84,   0}, {168, 168, 168},
      { 84,  84,  84}, { 84,  84, 252}, { 84, 252,  84}, { 84, 252, 252},
      {252,  84,  84}, {252,  84, 252}, {252, 252,  84}, {252, 252, 252},
      {  0,   0,   0}, { 20,  20,  20}, { 32,  32,  32}, { 44,  44,  44},
      { 56,  56,  56}, { 68,  68,  68}, { 80,  80,  80}, { 96,  96,  96},
      {112, 112, 112}, {128, 128, 128}, {144, 144, 144}, {160, 160, 160},
      {180, 180, 180}, {200, 200, 200}, {224, 224, 224}, {252, 252, 252},
      {  0,   0, 252}, { 64,   0, 252}, {124,   0, 252}, {188,   0, 252},
      {252,   0, 252}, {252,   0, 188}, {252,   0, 124}, {252,   0,  64},
      {252,   0,   0}, {252,  64,   0}, {252, 124,   0}, {252, 188,   0},
      {252, 252,   0}, {188, 252,   0}, {124, 252,   0}, { 64, 252,   0},
      {  0, 252,   0}, {  0, 252,  64}, {  0, 252, 124}, {  0, 252, 188},
      {  0, 252, 252}, {  0, 188, 252}, {  0, 124, 252}, {  0,  64, 252},
      {124, 124, 252}, {156, 124, 252}, {188, 124, 252}, {220, 124, 252},
      {252, 124, 252}, {252, 124, 220}, {252, 124, 188}, {252, 124, 156},
      {252, 124, 124}, {252, 156, 124}, {252, 188, 124}, {252, 220, 124},
      {252, 252, 124}, {220, 252, 124}, {188, 252, 124}, {156, 252, 124},
      {124, 252, 124}, {124, 252, 156}, {124, 252, 188}, {124, 252, 220},
      {124, 252, 252}, {124, 220, 252}, {124, 188, 252}, {124, 156, 252},
      {180, 180, 252}, {196, 180, 252}, {216, 180, 252}, {232, 180, 252},
      {252, 180, 252}, {252, 180, 232}, {252, 180, 216}, {252, 180, 196},
      {252, 180, 180}, {252, 196, 180}, {252, 216, 180}, {252, 232, 180},
      {252, 252, 180}, {232, 252, 180}, {216, 252, 180}, {196, 252, 180},
      {180, 220, 180}, {180, 252, 196}, {180, 252, 216}, {180, 252, 232},
      {180, 252, 252}, {180, 232, 252}, {180, 216, 252}, {180, 196, 252},
      {  0,   0, 112}, { 28,   0, 112}, { 56,   0, 112}, { 84,   0, 112},
      {112,   0, 112}, {112,   0,  84}, {112,   0,  56}, {112,   0,  28},
      {112,   0,   0}, {112,  28,   0}, {112,  56,   0}, {112,  84,   0},
      {112, 112,   0}, { 84, 112,   0}, { 56, 112,   0}, { 28, 112,   0},
      {  0, 112,   0}, {  0, 112,  28}, {  0, 112,  56}, {  0, 112,  84},
      {  0, 112, 112}, {  0,  84, 112}, {  0,  56, 112}, {  0,  28, 112},
      { 56,  56, 112}, { 68,  56, 112}, { 84,  56, 112}, { 96,  56, 112},
      {112,  56, 112}, {112,  56,  96}, {112,  56,  84}, {112,  56,  68},
      {112,  56,  56}, {112,  68,  56}, {112,  84,  56}, {112,  96,  56},
      {112, 112,  56}, { 96, 112,  56}, { 84, 112,  56}, { 68, 112,  56},
      { 56, 112,  56}, { 56, 112,  69}, { 56, 112,  84}, { 56, 112,  96},
      { 56, 112, 112}, { 56,  96, 112}, { 56,  84, 112}, { 56,  68, 112},
      { 80,  80, 112}, { 88,  80, 112}, { 96,  80, 112}, {104,  80, 112},
      {112,  80, 112}, {112,  80, 104}, {112,  80,  96}, {112,  80,  88},
      {112,  80,  80}, {112,  88,  80}, {112,  96,  80}, {112, 104,  80},
      {112, 112,  80}, {104, 112,  80}, { 96, 112,  80}, { 88, 112,  80},
      { 80, 112,  80}, { 80, 112,  88}, { 80, 112,  96}, { 80, 112, 104},
      { 80, 112, 112}, { 80, 114, 112}, { 80,  96, 112}, { 80,  88, 112},
      {  0,   0,  64}, { 16,   0,  64}, { 32,   0,  64}, { 48,   0,  64},
      { 64,   0,  64}, { 64,   0,  48}, { 64,   0,  32}, { 64,   0,  16},
      { 64,   0,   0}, { 64,  16,   0}, { 64,  32,   0}, { 64,  48,   0},
      { 64,  64,   0}, { 48,  64,   0}, { 32,  64,   0}, { 16,  64,   0},
      {  0,  64,   0}, {  0,  64,  16}, {  0,  64,  32}, {  0,  64,  48},
      {  0,  64,  64}, {  0,  48,  64}, {  0,  32,  64}, {  0,  16,  64},
      { 32,  32,  64}, { 40,  32,  64}, { 48,  32,  64}, { 56,  32,  64},
      { 64,  32,  64}, { 64,  32,  56}, { 64,  32,  48}, { 64,  32,  40},
      { 64,  32,  32}, { 64,  40,  32}, { 64,  48,  32}, { 64,  56,  32},
      { 64,  64,  32}, { 56,  64,  32}, { 48,  64,  32}, { 40,  64,  32},
      { 32,  64,  32}, { 32,  64,  40}, { 32,  64,  48}, { 32,  64,  56},
      { 32,  64,  64}, { 32,  56,  64}, { 32,  48,  64}, { 32,  40,  64},
      { 44,  44,  64}, { 48,  44,  64}, { 52,  44,  64}, { 60,  44,  64},
      { 64,  44,  64}, { 64,  44,  60}, { 64,  44,  52}, { 64,  44,  48},
      { 64,  44,  44}, { 64,  48,  44}, { 64,  52,  44}, { 64,  60,  44},
      { 64,  64,  44}, { 60,  64,  44}, { 52,  64,  44}, { 48,  64,  44},
      { 44,  64,  44}, { 44,  64,  48}, { 44,  64,  52}, { 44,  64,  60},
      { 44,  64,  64}, { 44,  60,  64}, { 44,  55,  64}, { 44,  48,  64},
      {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
      {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0}
    };

  MagickBooleanType
    status;

  WPGHeader
    Header;

  WPGRecord
    Rec;

  WPG2Record
    Rec2;

  WPG2Start
    StartWPG;

  WPGBitmapType1
    BitmapHeader1;

  WPG2BitmapType1
    Bitmap2Header1;

  WPGBitmapType2
    BitmapHeader2;

  WPGColorMapRec
    WPG_Palette;

  int
    i,
    bpp,
    WPG2Flags;

  long
    ldblk;

  ssize_t
    count;

  unsigned char
    *BImgBuff;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  image->depth=8;
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Read WPG image.
  */
  Header.FileId=ReadBlobLSBLong(image);
  Header.DataOffset=(MagickOffsetType) ReadBlobLSBLong(image);
  Header.ProductType=ReadBlobLSBShort(image);
  Header.FileType=ReadBlobLSBShort(image);
  Header.MajorVersion=ReadBlobByte(image);
  Header.MinorVersion=ReadBlobByte(image);
  Header.EncryptKey=ReadBlobLSBShort(image);
  Header.Reserved=ReadBlobLSBShort(image);

  if (Header.FileId!=0x435057FF || (Header.ProductType>>8)!=0x16)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  if (Header.EncryptKey!=0)
    ThrowReaderException(CoderError,"EncryptedWPGImageFileNotSupported");

  image->colors = 0;
  bpp=0;

  switch(Header.FileType)
    {
    case 1:     /*WPG level 1*/
      while(!EOFBlob(image)) /* object parser loop */
        {
          (void) SeekBlob(image,Header.DataOffset,SEEK_SET);
          if(EOFBlob(image) != MagickFalse)
            break;

          Rec.RecType=(i=ReadBlobByte(image));
          if(i==EOF)
            break;
          Rd_WP_DWORD(image,&Rec.RecordLength);
          if(EOFBlob(image) != MagickFalse)
            break;

          Header.DataOffset=TellBlob(image)+Rec.RecordLength;

          switch(Rec.RecType)
            {
            case 0x0B: /* bitmap type 1 */
              BitmapHeader1.Width=ReadBlobLSBShort(image);
              BitmapHeader1.Heigth=ReadBlobLSBShort(image);
              BitmapHeader1.Depth=ReadBlobLSBShort(image);
              BitmapHeader1.HorzRes=ReadBlobLSBShort(image);
              BitmapHeader1.VertRes=ReadBlobLSBShort(image);

              if(BitmapHeader1.HorzRes && BitmapHeader1.VertRes)
                {
                  image->units=PixelsPerCentimeterResolution;
                  image->x_resolution=BitmapHeader1.HorzRes/470.0;
                  image->y_resolution=BitmapHeader1.VertRes/470.0;
                }
              image->columns=BitmapHeader1.Width;
              image->rows=BitmapHeader1.Heigth;
              bpp=BitmapHeader1.Depth;

              goto UnpackRaster;

            case 0x0E:  /*Color palette */
              WPG_Palette.StartIndex=ReadBlobLSBShort(image);
              WPG_Palette.NumOfEntries=ReadBlobLSBShort(image);

              image->colors=WPG_Palette.NumOfEntries;
              if (AllocateImageColormap(image,image->colors) == MagickFalse)
                goto NoMemory;
              for (i=WPG_Palette.StartIndex;
                   i < (int)WPG_Palette.NumOfEntries; i++)
                {
                  image->colormap[i].red=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  image->colormap[i].green=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  image->colormap[i].blue=
                    ScaleCharToQuantum(ReadBlobByte(image));
                }
              break;

            case 0x11:  /* Start PS l1 */
              if (Rec.RecordLength > 8)
                image=ExtractPostscript(image,image_info,TellBlob(image)+8,
                  (long) Rec.RecordLength-8,exception);
              break;

            case 0x14:  /* bitmap type 2 */
              BitmapHeader2.RotAngle=ReadBlobLSBShort(image);
              BitmapHeader2.LowLeftX=ReadBlobLSBShort(image);
              BitmapHeader2.LowLeftY=ReadBlobLSBShort(image);
              BitmapHeader2.UpRightX=ReadBlobLSBShort(image);
              BitmapHeader2.UpRightY=ReadBlobLSBShort(image);
              BitmapHeader2.Width=ReadBlobLSBShort(image);
              BitmapHeader2.Heigth=ReadBlobLSBShort(image);
              BitmapHeader2.Depth=ReadBlobLSBShort(image);
              BitmapHeader2.HorzRes=ReadBlobLSBShort(image);
              BitmapHeader2.VertRes=ReadBlobLSBShort(image);

              image->units=PixelsPerCentimeterResolution;
              image->page.width=(unsigned long)
                ((BitmapHeader2.LowLeftX-BitmapHeader2.UpRightX)/470.0);
              image->page.height=(unsigned long)
                ((BitmapHeader2.LowLeftX-BitmapHeader2.UpRightY)/470.0);
              image->page.x=(int) (BitmapHeader2.LowLeftX/470.0);
              image->page.y=(int) (BitmapHeader2.LowLeftX/470.0);
              if(BitmapHeader2.HorzRes && BitmapHeader2.VertRes)
                {
                  image->x_resolution=BitmapHeader2.HorzRes/470.0;
                  image->y_resolution=BitmapHeader2.VertRes/470.0;
                }
              image->columns=BitmapHeader2.Width;
              image->rows=BitmapHeader2.Heigth;
              bpp=BitmapHeader2.Depth;

            UnpackRaster:
              if (image->colors == 0 && bpp!=24)
                {
                  image->colors=1 << bpp;
                  if (AllocateImageColormap(image,image->colors) == MagickFalse)
                    {
                    NoMemory:
                      ThrowReaderException(ResourceLimitError,
                        "MemoryAllocationFailed");
                    }
                  for (i=0; (i < (long) image->colors) && (i < 256) ;i++)
                  {
                    image->colormap[i].red=
                      ScaleCharToQuantum(WPG1Palette[i].red);
                    image->colormap[i].green=
                      ScaleCharToQuantum(WPG1Palette[i].green);
                    image->colormap[i].blue=
                      ScaleCharToQuantum(WPG1Palette[i].blue);
                  }
                }
              else {
                if (bpp < 24)
                  if( image->colors<(1UL<<bpp) && bpp!=24 )
                    image->colormap=(PixelPacket *) ResizeMagickMemory(
                      image->colormap,(1<<bpp)*sizeof(PixelPacket));
              }

              if (bpp == 1)
                {
                  if(image->colormap[0].red==0 &&
                     image->colormap[0].green==0 &&
                     image->colormap[0].blue==0 &&
                     image->colormap[1].red==0 &&
                     image->colormap[1].green==0 &&
                     image->colormap[1].blue==0)
                    {  /*fix crippled monochrome palette*/
                      image->colormap[1].red =
                        image->colormap[1].green =
                        image->colormap[1].blue = QuantumRange;
                    }
                }

              if(UnpackWPGRaster(image,bpp) < 0)
                /* The raster cannot be unpacked */
                {
                DecompressionFailed:
                  ThrowReaderException(CoderError,"UnableToDecompressImage");
                }

              /* Allocate next image structure. */
              AllocateNextImage(image_info,image);
              image->depth=8;
              if (GetNextImageInList(image) == (Image *) NULL)
                goto Finish;
              image=SyncNextImageInList(image);
              image->columns=image->rows=0;
              image->colors=0;
              break;

            case 0x1B:  /*Postscript l2*/
              if (Rec.RecordLength > 0x3C)
                image=ExtractPostscript(image,image_info,TellBlob(image)+0x3C,
                  (long) Rec.RecordLength-0x3C,exception);
              break;
            }
        }
      break;

    case 2:  /*WPG level 2*/
      ResetMagickMemory(&StartWPG,0,sizeof(StartWPG));
      while(!EOFBlob(image)) /* object parser loop */

        {
          (void) SeekBlob(image,Header.DataOffset,SEEK_SET);
          if(EOFBlob(image) != MagickFalse)
            break;

          Rec2.Class=(i=ReadBlobByte(image));
          if(i==EOF)
            break;
          Rec2.RecType=(i=ReadBlobByte(image));
          if(i==EOF)
            break;
          Rd_WP_DWORD(image,&Rec2.Extension);
          Rd_WP_DWORD(image,&Rec2.RecordLength);
          if(EOFBlob(image) != MagickFalse)
            break;

          Header.DataOffset=TellBlob(image)+Rec2.RecordLength;

          switch(Rec2.RecType)
            {
            case 1:
              StartWPG.HorizontalUnits=ReadBlobLSBShort(image);
              StartWPG.VerticalUnits=ReadBlobLSBShort(image);
              StartWPG.PosSizePrecision=ReadBlobByte(image);
              break;
            case 0x0C:    /*Color palette */
              WPG_Palette.StartIndex=ReadBlobLSBShort(image);
              WPG_Palette.NumOfEntries=ReadBlobLSBShort(image);

              image->colors=WPG_Palette.NumOfEntries;
              if (AllocateImageColormap(image,image->colors) == MagickFalse)
                ThrowReaderException(ResourceLimitError,
                  "MemoryAllocationFailed");
              for (i=WPG_Palette.StartIndex;
                   i < (int)WPG_Palette.NumOfEntries; i++)
                {
                  image->colormap[i].red=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  image->colormap[i].green=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  image->colormap[i].blue=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  (void) ReadBlobByte(image);   /*Opacity??*/
                }
              break;
            case 0x0E:
              Bitmap2Header1.Width=ReadBlobLSBShort(image);
              Bitmap2Header1.Heigth=ReadBlobLSBShort(image);
              Bitmap2Header1.Depth=ReadBlobByte(image);
              Bitmap2Header1.Compression=ReadBlobByte(image);

              if(Bitmap2Header1.Compression>1)
                continue; /*Unknown compression method */
              switch(Bitmap2Header1.Depth)
                {
                case 1:
                  bpp=1;
                  break;
                case 2:
                  bpp=2;
                  break;
                case 3:
                  bpp=4;
                  break;
                case 4:
                  bpp=8;
                  break;
                case 8:
                  bpp=24;
                  break;
                default:
                  continue;  /*Ignore raster with unknown depth*/
                }
              image->columns=Bitmap2Header1.Width;
              image->rows=Bitmap2Header1.Heigth;

              if (image->colors == 0 && bpp!=24)
                {
                  image->colors=1 << bpp;
                  if (AllocateImageColormap(image,image->colors) == MagickFalse)
                    goto NoMemory;
                }
              else {
                if (bpp < 24)
                  if( image->colors<(1UL<<bpp) && bpp!=24 )
                    image->colormap=(PixelPacket *) ResizeMagickMemory(
                      image->colormap,(1<<bpp)*sizeof(PixelPacket));
              }


              switch(Bitmap2Header1.Compression)
                {
                case 0:    /*Uncompressed raster*/
                  {
                    ldblk=(long) ((bpp*image->columns+7)/8);
                    if((BImgBuff=(unsigned char *)
                        AcquireMagickMemory(ldblk))==NULL)
                      goto NoMemory;

                    for(i=0;i<(long) image->rows;i++)
                      {
                        count=ReadBlob(image,ldblk,BImgBuff);
                        InsertRow(BImgBuff,i,image,bpp);
                      }
                    if(BImgBuff)
                      BImgBuff=(unsigned char *) RelinquishMagickMemory(BImgBuff);
                    break;
                  }
                case 1:    /*RLE for WPG2 */
                  {
                    if( UnpackWPG2Raster(image,bpp) < 0)
                      goto DecompressionFailed;
                    break;
                  }
                }

              /* Allocate next image structure. */
              AllocateNextImage(image_info,image);
              image->depth=8;
              if (GetNextImageInList(image) == (Image *) NULL)
                goto Finish;
              image=SyncNextImageInList(image);
              image->columns=image->rows=0;
              image->colors=0;
              break;

            case 0x12:  /* Postscript WPG2*/
              i=ReadBlobLSBShort(image);
              if ((long) Rec2.RecordLength > i)
                image=ExtractPostscript(image,image_info,TellBlob(image)+i,
                  (long) (Rec2.RecordLength-i-2),exception);
              break;
            case 0x1B:          /*bitmap rectangle*/
              WPG2Flags = LoadWPG2Flags(image,StartWPG.PosSizePrecision,NULL);
              break;
            }
        }

      break;

    default:
      ThrowReaderException(CoderError,"UnsupportedLevel");
  }

 Finish:
  CloseBlob(image);

  /*
    Rewind list, removing any empty images while rewinding.
  */
  for (p=image; p != (Image *) NULL; p=GetPreviousImageInList(p))
  {
    if ((p->rows == 0) || (p->columns == 0))
      DeleteImageFromList(&p);
    image=p;
  }
  scene=0;
  for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
    p->scene=scene++;
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r W P G I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterWPGImage() adds attributes for the WPG image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterWPGImage method is:
%
%      RegisterWPGImage(void)
%
*/
ModuleExport void RegisterWPGImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("WPG");
  entry->decoder=(DecoderHandler *) ReadWPGImage;
  entry->magick=(MagickHandler *) IsWPG;
  entry->description=AcquireString("Word Perfect Graphics");
  entry->module=AcquireString("WPG");
  entry->seekable_stream=MagickTrue;
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r W P G I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterWPGImage() removes format registrations made by the
%  WPG module from the list of supported formats.
%
%  The format of the UnregisterWPGImage method is:
%
%      UnregisterWPGImage(void)
%
*/
ModuleExport void UnregisterWPGImage(void)
{
  (void) UnregisterMagickInfo("WPG");
}
