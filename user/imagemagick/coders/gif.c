/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                             GGGG  IIIII  FFFFF                              %
%                            G        I    F                                  %
%                            G  GG    I    FFF                                %
%                            G   G    I    F                                  %
%                             GGG   IIIII  F                                  %
%                                                                             %
%                                                                             %
%            Read/Write Compuserv Graphics Interchange Format.                %
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
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantize.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
  Forward declarations.
*/
static ssize_t
  ReadBlobBlock(Image *,unsigned char *);

static MagickBooleanType
  WriteGIFImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DecodeImage uncompresses an image via GIF-coding.
%
%  The format of the DecodeImage method is:
%
%      MagickBooleanType DecodeImage(Image *image,const long opacity)
%
%  A description of each parameter follows:
%
%    o image: The address of a structure of type Image.
%
%    o opacity:  The colormap index associated with the transparent
%      color.
%
%
*/
static MagickBooleanType DecodeImage(Image *image,const Quantum opacity)
{
#define MaxStackSize  4096
#define NullCode  (~0UL)

  IndexPacket
    index;

  long
    offset,
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register unsigned char
    *c;

  register unsigned long
    datum;

  short
    *prefix;

  ssize_t
    count;

  unsigned char
    *packet,
    *pixel_stack,
    *suffix,
    *top_stack;

  unsigned long
    available,
    bits,
    clear,
    code,
    code_mask,
    code_size,
    data_size,
    first,
    end_of_information,
    in_code,
    old_code,
    pass;

  /*
    Allocate decoder tables.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  packet=(unsigned char *) AcquireMagickMemory(256);
  prefix=(short *) AcquireMagickMemory(MaxStackSize*sizeof(*prefix));
  suffix=(unsigned char *) AcquireMagickMemory(MaxStackSize);
  pixel_stack=(unsigned char *) AcquireMagickMemory(MaxStackSize+1);
  if ((packet == (unsigned char *) NULL) ||
      (prefix == (short *) NULL) ||
      (suffix == (unsigned char *) NULL) ||
      (pixel_stack == (unsigned char *) NULL))
    {
      if (packet != (unsigned char *) NULL)
        packet=(unsigned char *) RelinquishMagickMemory(packet);
      if (prefix != (short *) NULL)
        prefix=(short *) RelinquishMagickMemory(prefix);
      if (suffix != (unsigned char *) NULL)
        suffix=(unsigned char *) RelinquishMagickMemory(suffix);
      if (pixel_stack != (unsigned char *) NULL)
        pixel_stack=(unsigned char *) RelinquishMagickMemory(pixel_stack);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  /*
    Initialize GIF data stream decoder.
  */
  data_size=(unsigned long) ReadBlobByte(image);
  if (data_size > 8)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
  clear=1UL << data_size;
  end_of_information=clear+1;
  available=clear+2;
  old_code=NullCode;
  code_size=data_size+1;
  code_mask=(1 << code_size)-1;
  for (code=0; code < clear; code++)
  {
    prefix[code]=0;
    suffix[code]=(unsigned char) code;
  }
  /*
    Decode GIF pixel stream.
  */
  datum=0;
  bits=0;
  c=0;
  count=0;
  first=0;
  offset=0;
  pass=0;
  top_stack=pixel_stack;
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,offset,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; )
    {
      if (top_stack == pixel_stack)
        {
          if (bits < code_size)
            {
              /*
                Load bytes until there is enough bits for a code.
              */
              if (count == 0)
                {
                  /*
                    Read a new data block.
                  */
                  count=ReadBlobBlock(image,packet);
                  if (count == 0)
                    break;
                  c=packet;
                }
              datum+=(unsigned long) (*c) << bits;
              bits+=8;
              c++;
              count--;
              continue;
            }
          /*
            Get the next code.
          */
          code=datum & code_mask;
          datum>>=code_size;
          bits-=code_size;
          /*
            Interpret the code
          */
          if ((code > available) || (code == end_of_information))
            break;
          if (code == clear)
            {
              /*
                Reset decoder.
              */
              code_size=data_size+1;
              code_mask=(1 << code_size)-1;
              available=clear+2;
              old_code=NullCode;
              continue;
            }
          if (old_code == NullCode)
            {
              *top_stack++=suffix[code];
              old_code=code;
              first=code;
              continue;
            }
          in_code=code;
          if (code >= available)
            {
              *top_stack++=(unsigned char) first;
              code=old_code;
            }
          while (code >= clear)
          {
            if ((top_stack-pixel_stack) >= MaxStackSize)
              break;
            *top_stack++=suffix[code];
            code=(unsigned long) prefix[code];
          }
          first=(unsigned long) suffix[code];
          /*
            Add a new string to the string table,
          */
          if ((top_stack-pixel_stack) >= MaxStackSize)
            break;
          if (available >= MaxStackSize)
            break;
          *top_stack++=(unsigned char) first;
          prefix[available]=(short) old_code;
          suffix[available]=(unsigned char) first;
          available++;
          if (((available & code_mask) == 0) && (available < MaxStackSize))
            {
              code_size++;
              code_mask+=available;
            }
          old_code=in_code;
        }
      /*
        Pop a pixel off the pixel stack.
      */
      top_stack--;
      index=ConstrainColormapIndex(image,*top_stack);
      indexes[x]=index;
      *q=image->colormap[index];
      q->opacity=(Quantum)
        (index == opacity ? TransparentOpacity : OpaqueOpacity);
      x++;
      q++;
    }
    if (image->interlace == NoInterlace)
      offset++;
    else
      switch (pass)
      {
        case 0:
        default:
        {
          offset+=8;
          if (offset >= (long) image->rows)
            {
              pass++;
              offset=4;
            }
          break;
        }
        case 1:
        {
          offset+=8;
          if (offset >= (long) image->rows)
            {
              pass++;
              offset=2;
            }
          break;
        }
        case 2:
        {
          offset+=4;
          if (offset >= (long) image->rows)
            {
              pass++;
              offset=1;
            }
          break;
        }
        case 3:
        {
          offset+=2;
          break;
        }
      }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if (x < (long) image->columns)
      break;
    if (image->previous == (Image *) NULL)
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(LoadImageTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
  }
  pixel_stack=(unsigned char *) RelinquishMagickMemory(pixel_stack);
  suffix=(unsigned char *) RelinquishMagickMemory(suffix);
  prefix=(short *) RelinquishMagickMemory(prefix);
  packet=(unsigned char *) RelinquishMagickMemory(packet);
  if (y < (long) image->rows)
    ThrowBinaryException(CorruptImageError,"CorruptImage",image->filename);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E n c o d e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeImage compresses an image via GIF-coding.
%
%  The format of the EncodeImage method is:
%
%      MagickBooleanType EncodeImage(const ImageInfo *image_info,Image *image,
%        const unsigned long data_size)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o image: The address of a structure of type Image.
%
%    o data_size:  The number of bits in the compressed packet.
%
%
*/
static MagickBooleanType EncodeImage(const ImageInfo *image_info,Image *image,
  const unsigned long data_size)
{
#define MaxCode(number_bits)  ((1UL << (number_bits))-1)
#define MaxHashTable  5003
#define MaxGIFBits  12UL
#define MaxGIFTable  (1UL << MaxGIFBits)
#define GIFOutputCode(code) \
{ \
  /*  \
    Emit a code. \
  */ \
  if (bits > 0) \
    datum|=(code) << bits; \
  else \
    datum=code; \
  bits+=number_bits; \
  while (bits >= 8) \
  { \
    /*  \
      Add a character to current packet. \
    */ \
    packet[length++]=(unsigned char) (datum & 0xff); \
    if (length >= 254) \
      { \
        (void) WriteBlobByte(image,(unsigned char) length); \
        (void) WriteBlob(image,length,packet); \
        length=0; \
      } \
    datum>>=8; \
    bits-=8; \
  } \
  if (free_code > max_code)  \
    { \
      number_bits++; \
      if (number_bits == MaxGIFBits) \
        max_code=MaxGIFTable; \
      else \
        max_code=MaxCode(number_bits); \
    } \
}

  IndexPacket
    index;

  long
    displacement,
    offset,
    k,
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  size_t
    length;

  short
    *hash_code,
    *hash_prefix,
    waiting_code;

  unsigned char
    *packet,
    *hash_suffix;

  unsigned long
    bits,
    clear_code,
    datum,
    end_of_information_code,
    free_code,
    max_code,
    next_pixel,
    number_bits,
    pass;

  /*
    Allocate encoder tables.
  */
  assert(image != (Image *) NULL);
  packet=(unsigned char *) AcquireMagickMemory(256);
  hash_code=(short *) AcquireMagickMemory(MaxHashTable*sizeof(*hash_code));
  hash_prefix=(short *) AcquireMagickMemory(MaxHashTable*sizeof(*hash_prefix));
  hash_suffix=(unsigned char *) AcquireMagickMemory(MaxHashTable);
  if ((packet == (unsigned char *) NULL) || (hash_code == (short *) NULL) ||
      (hash_prefix == (short *) NULL) ||
      (hash_suffix == (unsigned char *) NULL))
    {
      if (packet != (unsigned char *) NULL)
        packet=(unsigned char *) RelinquishMagickMemory(packet);
      if (hash_code != (short *) NULL)
        hash_code=(short *) RelinquishMagickMemory(hash_code);
      if (hash_prefix != (short *) NULL)
        hash_prefix=(short *) RelinquishMagickMemory(hash_prefix);
      if (hash_suffix != (unsigned char *) NULL)
        hash_suffix=(unsigned char *) RelinquishMagickMemory(hash_suffix);
      return(MagickFalse);
    }
  /*
    Initialize GIF encoder.
  */
  number_bits=data_size;
  max_code=MaxCode(number_bits);
  clear_code=((short) 1 << (data_size-1));
  end_of_information_code=clear_code+1;
  free_code=clear_code+2;
  length=0;
  datum=0;
  bits=0;
  for (i=0; i < MaxHashTable; i++)
    hash_code[i]=0;
  GIFOutputCode(clear_code);
  /*
    Encode pixels.
  */
  offset=0;
  pass=0;
  waiting_code=0;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,offset,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    if (y == 0)
      waiting_code=(short) (*indexes);
    for (x=(y == 0) ? 1 : 0; x < (long) image->columns; x++)
    {
      /*
        Probe hash table.
      */
      index=indexes[x] & 0xff;
      p++;
      k=(int) (index << (MaxGIFBits-8))+waiting_code;
      if (k >= MaxHashTable)
        k-=MaxHashTable;
      next_pixel=MagickFalse;
      displacement=1;
      if (hash_code[k] > 0)
        {
          if ((hash_prefix[k] == waiting_code) &&
              (hash_suffix[k] == (unsigned char) index))
            {
              waiting_code=hash_code[k];
              continue;
            }
          if (k != 0)
            displacement=MaxHashTable-k;
          for ( ; ; )
          {
            k-=displacement;
            if (k < 0)
              k+=MaxHashTable;
            if (hash_code[k] == 0)
              break;
            if ((hash_prefix[k] == waiting_code) &&
                (hash_suffix[k] == (unsigned char) index))
              {
                waiting_code=hash_code[k];
                next_pixel=MagickTrue;
                break;
              }
          }
          if (next_pixel == MagickTrue)
            continue;
        }
      GIFOutputCode((unsigned long) waiting_code);
      if (free_code < MaxGIFTable)
        {
          hash_code[k]=(short) free_code++;
          hash_prefix[k]=waiting_code;
          hash_suffix[k]=(unsigned char) index;
        }
      else
        {
          /*
            Fill the hash table with empty entries.
          */
          for (k=0; k < MaxHashTable; k++)
            hash_code[k]=0;
          /*
            Reset compressor and issue a clear code.
          */
          free_code=clear_code+2;
          GIFOutputCode(clear_code);
          number_bits=data_size;
          max_code=MaxCode(number_bits);
        }
      waiting_code=(short) index;
    }
    if (image_info->interlace == NoInterlace)
      offset++;
    else
      switch (pass)
      {
        case 0:
        default:
        {
          offset+=8;
          if (offset >= (long) image->rows)
            {
              pass++;
              offset=4;
            }
          break;
        }
        case 1:
        {
          offset+=8;
          if (offset >= (long) image->rows)
            {
              pass++;
              offset=2;
            }
          break;
        }
        case 2:
        {
          offset+=4;
          if (offset >= (long) image->rows)
            {
              pass++;
              offset=1;
            }
          break;
        }
        case 3:
        {
          offset+=2;
          break;
        }
      }
    if (image->previous == (Image *) NULL)
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(SaveImageTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
  }
  /*
    Flush out the buffered code.
  */
  GIFOutputCode((unsigned long) waiting_code);
  GIFOutputCode(end_of_information_code);
  if (bits > 0)
    {
      /*
        Add a character to current packet.
      */
      packet[length++]=(unsigned char) (datum & 0xff);
      if (length >= 254)
        {
          (void) WriteBlobByte(image,(unsigned char) length);
          (void) WriteBlob(image,length,packet);
          length=0;
        }
    }
  /*
    Flush accumulated data.
  */
  if (length > 0)
    {
      (void) WriteBlobByte(image,(unsigned char) length);
      (void) WriteBlob(image,length,packet);
    }
  /*
    Free encoder memory.
  */
  hash_suffix=(unsigned char *) RelinquishMagickMemory(hash_suffix);
  hash_prefix=(short *) RelinquishMagickMemory(hash_prefix);
  hash_code=(short *) RelinquishMagickMemory(hash_code);
  packet=(unsigned char *) RelinquishMagickMemory(packet);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s G I F                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGIF() returns MagickTrue if the image format type, identified by the
%  magick string, is GIF.
%
%  The format of the IsGIF method is:
%
%      MagickBooleanType IsGIF(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsGIF(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick,"GIF8",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b B l o c k                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobBlock() reads data from the image file and returns it.  The
%  amount of data is determined by first reading a count byte.  The number
%  or bytes read is returned.
%
%  The format of the ReadBlobBlock method is:
%
%      size_t ReadBlobBlock(Image *image,unsigned char *data)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o data:  Specifies an area to place the information requested from
%      the file.
%
%
*/
static ssize_t ReadBlobBlock(Image *image,unsigned char *data)
{
  ssize_t
    count;

  unsigned char
    block_count;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(data != (unsigned char *) NULL);
  count=ReadBlob(image,1,&block_count);
  if (count != 1)
    return(0);
  return(ReadBlob(image,(size_t) block_count,data));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d G I F I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadGIFImage() reads a Compuserve Graphics image file and returns it.
%  It allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadGIFImage method is:
%
%      Image *ReadGIFImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadGIFImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define BitSet(byte,bit)  (((byte) & (bit)) == (bit))
#define LSBFirstOrder(x,y)  (((y) << 8) | (x))

  Image
    *image;

  long
    opacity;

  MagickBooleanType
    status;

  RectangleInfo
    page;

  register long
    i;

  register unsigned char
    *p;

  ssize_t
    count;

  unsigned char
    background,
    c,
    flag,
    *global_colormap,
    header[MaxTextExtent],
    magick[12];

  unsigned long
    delay,
    dispose,
    global_colors,
    image_count,
    iterations;

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
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Determine if this is a GIF file.
  */
  count=ReadBlob(image,6,magick);
  if ((count != 6) || ((LocaleNCompare((char *) magick,"GIF87",5) != 0) &&
      (LocaleNCompare((char *) magick,"GIF89",5) != 0)))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  page.width=ReadBlobLSBShort(image);
  page.height=ReadBlobLSBShort(image);
  flag=(unsigned char) ReadBlobByte(image);
  background=(unsigned char) ReadBlobByte(image);
  c=(unsigned char) ReadBlobByte(image);  /* reserved */
  global_colors=1 << (((unsigned long) flag & 0x07)+1);
  global_colormap=(unsigned char *)
    AcquireMagickMemory((size_t) (3*Max(global_colors,256)));
  if (global_colormap == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  if (BitSet((int) flag,0x80) != 0)
    count=ReadBlob(image,(size_t) (3*global_colors),global_colormap);
  delay=0;
  dispose=0;
  iterations=1;
  opacity=(-1);
  image_count=0;
  for ( ; ; )
  {
    count=ReadBlob(image,1,&c);
    if (count != 1)
      break;
    if (c == (unsigned char) ';')
      break;  /* terminator */
    if (c == (unsigned char) '!')
      {
        /*
          GIF Extension block.
        */
        count=ReadBlob(image,1,&c);
        if (count != 1)
          ThrowReaderException(CorruptImageError,"UnableToReadExtensionBlock");
        switch (c)
        {
          case 0xf9:
          {
            /*
              Read graphics control extension.
            */
            while (ReadBlobBlock(image,header) != 0);
            dispose=(unsigned long) (header[0] >> 2);
            delay=(unsigned long) ((header[2] << 8) | header[1]);
            if ((int) (header[0] & 0x01) == 0x01)
              opacity=(long) header[3];
            break;
          }
          case 0xfe:
          {
            char
              *comments;

            /*
              Read comment extension.
            */
            comments=AcquireString((char *) NULL);
            for ( ; ; )
            {
              count=(ssize_t) ReadBlobBlock(image,header);
              if (count == 0)
                break;
              header[count]='\0';
              (void) ConcatenateString(&comments,(const char *) header);
            }
            (void) SetImageAttribute(image,"Comment",comments);
            comments=(char *) RelinquishMagickMemory(comments);
            break;
          }
          case 0xff:
          {
            MagickBooleanType
              loop;

            /*
              Read Netscape Loop extension.
            */
            loop=MagickFalse;
            if (ReadBlobBlock(image,header) != 0)
              loop=(MagickBooleanType)
                (LocaleNCompare((char *) header,"NETSCAPE2.0",11) == 0);
            while (ReadBlobBlock(image,header) != 0)
            if (loop != MagickFalse)
              iterations=(unsigned long) ((header[2] << 8) | header[1]);
            break;
          }
          default:
          {
            while (ReadBlobBlock(image,header) != 0);
            break;
          }
        }
      }
    if (c != (unsigned char) ',')
      continue;
    if (image_count != 0)
      {
        /*
          Allocate next image structure.
        */
        AllocateNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            global_colormap=(unsigned char *)
              RelinquishMagickMemory(global_colormap);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            status=image->progress_monitor(LoadImagesTag,TellBlob(image),
              GetBlobSize(image),image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
    image_count++;
    /*
      Read image attributes.
    */
    image->storage_class=PseudoClass;
    image->compression=LZWCompression;
    page.x=(long) ReadBlobLSBShort(image);
    page.y=(long) ReadBlobLSBShort(image);
    image->columns=ReadBlobLSBShort(image);
    image->rows=ReadBlobLSBShort(image);
    image->depth=8;
    flag=(unsigned char) ReadBlobByte(image);
    image->interlace=BitSet((int) flag,0x40) != 0 ? PlaneInterlace :
      NoInterlace;
    image->colors=BitSet((int) flag,0x80) == 0 ? global_colors :
      1UL << ((unsigned long) (flag & 0x07)+1);
    if (opacity >= (long) image->colors)
      image->colors=(unsigned long) opacity+1;
    image->page.width=page.width;
    image->page.height=page.height;
    image->page.y=page.y;
    image->page.x=page.x;
    image->delay=delay;
    image->ticks_per_second=100;
    image->dispose=(DisposeType) dispose;
    image->iterations=iterations;
    image->matte=(MagickBooleanType) (opacity >= 0);
    delay=0;
    dispose=0;
    iterations=1;
    if ((image->columns == 0) || (image->rows == 0))
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    /*
      Inititialize colormap.
    */
    if (AllocateImageColormap(image,image->colors) == MagickFalse)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    if (BitSet((int) flag,0x80) == 0)
      {
        /*
          Use global colormap.
        */
        p=global_colormap;
        for (i=0; i < (long) image->colors; i++)
        {
          image->colormap[i].red=ScaleCharToQuantum(*p++);
          image->colormap[i].green=ScaleCharToQuantum(*p++);
          image->colormap[i].blue=ScaleCharToQuantum(*p++);
        }
        image->background_color=
          image->colormap[Min((unsigned long) background,image->colors-1)];
      }
    else
      {
        unsigned char
          *colormap;

        /*
          Read local colormap.
        */
        colormap=(unsigned char *)
          AcquireMagickMemory((size_t) (3*image->colors));
        if (colormap == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        count=ReadBlob(image,(size_t) (3*image->colors),colormap);
        if (count != (ssize_t) (3*image->colors))
          ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
        p=colormap;
        for (i=0; i < (long) image->colors; i++)
        {
          image->colormap[i].red=ScaleCharToQuantum(*p++);
          image->colormap[i].green=ScaleCharToQuantum(*p++);
          image->colormap[i].blue=ScaleCharToQuantum(*p++);
        }
        colormap=(unsigned char *) RelinquishMagickMemory(colormap);
      }
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    /*
      Decode image.
    */
    status=DecodeImage(image,(Quantum) opacity);
    if ((image_info->ping == MagickFalse) && (status == MagickFalse))
      {
        global_colormap=(unsigned char *)
          RelinquishMagickMemory(global_colormap);
        ThrowReaderException(CorruptImageError,"CorruptImage");
      }
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    opacity=(-1);
  }
  global_colormap=(unsigned char *) RelinquishMagickMemory(global_colormap);
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r G I F I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterGIFImage() adds attributes for the GIF image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterGIFImage method is:
%
%      RegisterGIFImage(void)
%
*/
ModuleExport void RegisterGIFImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("GIF");
  entry->decoder=(DecoderHandler *) ReadGIFImage;
  entry->encoder=(EncoderHandler *) WriteGIFImage;
  entry->magick=(MagickHandler *) IsGIF;
  entry->description=AcquireString("CompuServe graphics interchange format");
  entry->module=AcquireString("GIF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("GIF87");
  entry->decoder=(DecoderHandler *) ReadGIFImage;
  entry->encoder=(EncoderHandler *) WriteGIFImage;
  entry->magick=(MagickHandler *) IsGIF;
  entry->adjoin=MagickFalse;
  entry->description=AcquireString("CompuServe graphics interchange format");
  entry->version=AcquireString("version 87a");
  entry->module=AcquireString("GIF");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r G I F I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterGIFImage() removes format registrations made by the
%  GIF module from the list of supported formats.
%
%  The format of the UnregisterGIFImage method is:
%
%      UnregisterGIFImage(void)
%
*/
ModuleExport void UnregisterGIFImage(void)
{
  (void) UnregisterMagickInfo("GIF");
  (void) UnregisterMagickInfo("GIF87");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e G I F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteGIFImage() writes an image to a file in the Compuserve Graphics
%  image format.
%
%  The format of the WriteGIFImage method is:
%
%      MagickBooleanType WriteGIFImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteGIFImage(const ImageInfo *image_info,Image *image)
{
  Image
    *next_image;

  int
    c;

  long
    j,
    opacity,
    y;

  ImageInfo
    *write_info;

  InterlaceType
    interlace;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  QuantizeInfo
    quantize_info;

  RectangleInfo
    page;

  register IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register long
    x;

  register long
    i;

  register unsigned char
    *q;

  size_t
    length;

  unsigned char
    *colormap,
    *global_colormap;

  unsigned long
    bits_per_pixel,
    delay;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  /*
    Determine image bounding box.
  */
  page.width=image->columns;
  page.height=image->rows;
  page.x=0;
  page.y=0;
  for (next_image=image; next_image != (Image *) NULL; )
  {
    page.x=next_image->page.x;
    page.y=next_image->page.y;
    if ((next_image->columns+page.x) > page.width)
      page.width=next_image->columns+page.x;
    if ((next_image->rows+page.y) > page.height)
      page.height=next_image->rows+page.y;
    next_image=GetNextImageInList(next_image);
  }
  /*
    Allocate colormap.
  */
  global_colormap=(unsigned char *) AcquireMagickMemory(768);
  colormap=(unsigned char *) AcquireMagickMemory(768);
  if ((global_colormap == (unsigned char *) NULL) ||
      (colormap == (unsigned char *) NULL))
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  for (i=0; i < 768; i++)
    colormap[i]=(unsigned char) 0;
  /*
    Write GIF header.
  */
  write_info=CloneImageInfo(image_info);
  if (LocaleCompare(write_info->magick,"GIF87") != 0)
    (void) WriteBlob(image,6,(unsigned char *) "GIF89a");
  else
    {
      (void) WriteBlob(image,6,(unsigned char *) "GIF87a");
      write_info->adjoin=MagickFalse;
    }
  page.x=image->page.x;
  page.y=image->page.y;
  if ((image->page.width != 0) && (image->page.height != 0))
    page=image->page;
  (void) WriteBlobLSBShort(image,(unsigned short) page.width);
  (void) WriteBlobLSBShort(image,(unsigned short) page.height);
  /*
    Write images to file.
  */
  interlace=write_info->interlace;
  if ((write_info->adjoin != MagickFalse) &&
      (GetNextImageInList(image) != (Image *) NULL))
    interlace=NoInterlace;
  scene=0;
  do
  {
    opacity=(-1);
    (void) SetImageColorspace(image,RGBColorspace);
    if ((image->storage_class == DirectClass) || (image->colors > 256))
      {
        /*
          GIF requires that the image is colormapped.
        */
        GetQuantizeInfo(&quantize_info);
        quantize_info.number_colors=256;
        quantize_info.dither=IsPaletteImage(image,&image->exception) ==
          MagickFalse ? MagickTrue : MagickFalse;
        (void) QuantizeImage(&quantize_info,image);
        if ((LocaleCompare(write_info->magick,"GIF87") != 0) &&
            (image->matte != MagickFalse))
          {
            MagickSizeType
              length;

            /*
              Set transparent pixel.
            */
            quantize_info.number_colors=255;
            (void) QuantizeImage(&quantize_info,image);
            opacity=(long) image->colors++;
            length=(size_t) image->colors*sizeof(*image->colormap);
            image->colormap=(PixelPacket *)
              ResizeMagickMemory(image->colormap,length);
            if (image->colormap == (PixelPacket *) NULL)
              {
                global_colormap=(unsigned char *)
                  RelinquishMagickMemory(global_colormap);
                colormap=(unsigned char *) RelinquishMagickMemory(colormap);
                write_info=DestroyImageInfo(write_info);
                ThrowWriterException(ResourceLimitError,
                  "MemoryAllocationFailed");
              }
            (void) QueryColorDatabase("#cccccc",image->colormap+opacity,
              &image->exception);
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetIndexes(image);
              for (x=0; x < (long) image->columns; x++)
              {
                if ((double) p->opacity >= (QuantumRange-image->fuzz))
                  indexes[x]=(IndexPacket) opacity;
                p++;
              }
              if (SyncImagePixels(image) == MagickFalse)
                break;
            }
          }
      }
    else
      if ((LocaleCompare(write_info->magick,"GIF87") != 0) &&
          (image->matte != MagickFalse))
        {
          /*
            Identify transparent pixel index.
          */
          for (y=0; y < (long) image->rows; y++)
          {
            p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
            if (p == (const PixelPacket *) NULL)
              break;
            indexes=GetIndexes(image);
            for (x=0; x < (long) image->columns; x++)
            {
              if ((double) p->opacity >= (QuantumRange-image->fuzz))
                {
                  opacity=(long) indexes[x];
                  break;
                }
              p++;
            }
            if (x < (long) image->columns)
              break;
          }
        }
    if (image->colormap == (PixelPacket *) NULL)
      break;
    for (bits_per_pixel=1; bits_per_pixel < 8; bits_per_pixel++)
      if ((1UL << bits_per_pixel) >= image->colors)
        break;
    q=colormap;
    for (i=0; i < (long) image->colors; i++)
    {
      *q++=ScaleQuantumToChar(image->colormap[i].red);
      *q++=ScaleQuantumToChar(image->colormap[i].green);
      *q++=ScaleQuantumToChar(image->colormap[i].blue);
    }
    for ( ; i < (long) (1UL << bits_per_pixel); i++)
    {
      *q++=(unsigned char) 0x0;
      *q++=(unsigned char) 0x0;
      *q++=(unsigned char) 0x0;
    }
    if ((GetPreviousImageInList(image) == (Image *) NULL) ||
        (write_info->adjoin == MagickFalse))
      {
        /*
          Write global colormap.
        */
        c=0x80;
        c|=(8-1) << 4;  /* color resolution */
        c|=(bits_per_pixel-1);   /* size of global colormap */
        (void) WriteBlobByte(image,(unsigned char) c);
        for (j=0; j < (long) image->colors; j++)
          if (ColorMatch(&image->background_color,image->colormap+j))
            break;
        (void) WriteBlobByte(image,(unsigned char)
          j == (long) image->colors ? 0 : j);  /* background color */
        (void) WriteBlobByte(image,(unsigned char) 0x00);  /* reserved */
        length=(size_t) (3*(1 << bits_per_pixel));
        (void) WriteBlob(image,length,colormap);
        for (j=0; j < 768; j++)
          global_colormap[j]=colormap[j];
      }
    if (LocaleCompare(write_info->magick,"GIF87") != 0)
      {
        /*
          Write graphics control extension.
        */
        (void) WriteBlobByte(image,(unsigned char) 0x21);
        (void) WriteBlobByte(image,(unsigned char) 0xf9);
        (void) WriteBlobByte(image,(unsigned char) 0x04);
        c=image->dispose << 2;
        if (opacity >= 0)
          c|=0x01;
        (void) WriteBlobByte(image,(unsigned char) c);
        delay=100*image->delay/Max(image->ticks_per_second,1);
        (void) WriteBlobLSBShort(image,(unsigned short) delay);
        (void) WriteBlobByte(image,(unsigned char)
          (opacity >= 0 ? opacity : 0));
        (void) WriteBlobByte(image,(unsigned char) 0x00);
        if ((LocaleCompare(write_info->magick,"GIF87") != 0) &&
            (GetImageAttribute(image,"Comment") != (ImageAttribute *) NULL))
          {
            const ImageAttribute
              *attribute;

            register char
              *p;

            size_t
              count;

            /*
              Write Comment extension.
            */
            (void) WriteBlobByte(image,(unsigned char) 0x21);
            (void) WriteBlobByte(image,(unsigned char) 0xfe);
            attribute=GetImageAttribute(image,"Comment");
            p=attribute->value;
            while (strlen(p) != 0)
            {
              count=Min(strlen(p),255);
              (void) WriteBlobByte(image,(unsigned char) count);
              for (i=0; i < (long) count; i++)
                (void) WriteBlobByte(image,(unsigned char) *p++);
            }
            (void) WriteBlobByte(image,(unsigned char) 0x00);
          }
        if ((GetPreviousImageInList(image) == (Image *) NULL) &&
            (GetNextImageInList(image) != (Image *) NULL) &&
            (image->iterations != 1))
          {
            /*
              Write Netscape Loop extension.
            */
            (void) WriteBlobByte(image,(unsigned char) 0x21);
            (void) WriteBlobByte(image,(unsigned char) 0xff);
            (void) WriteBlobByte(image,(unsigned char) 0x0b);
            (void) WriteBlob(image,11,(unsigned char *) "NETSCAPE2.0");
            (void) WriteBlobByte(image,(unsigned char) 0x03);
            (void) WriteBlobByte(image,(unsigned char) 0x01);
            (void) WriteBlobLSBShort(image,(unsigned short) image->iterations);
            (void) WriteBlobByte(image,(unsigned char) 0x00);
          }
      }
    (void) WriteBlobByte(image,',');  /* image separator */
    /*
      Write the image header.
    */
    page.x=image->page.x;
    page.y=image->page.y;
    if ((image->page.width != 0) && (image->page.height != 0))
      page=image->page;
    (void) WriteBlobLSBShort(image,(unsigned short) (page.x < 0 ? 0 : page.x));
    (void) WriteBlobLSBShort(image,(unsigned short) (page.y < 0 ? 0 : page.y));
    (void) WriteBlobLSBShort(image,(unsigned short) image->columns);
    (void) WriteBlobLSBShort(image,(unsigned short) image->rows);
    c=0x00;
    if (interlace != NoInterlace)
      c|=0x40;  /* pixel data is interlaced */
    for (j=0; j < (long) (3*image->colors); j++)
      if (colormap[j] != global_colormap[j])
        break;
    if (j == (long) (3*image->colors))
      (void) WriteBlobByte(image,(unsigned char) c);
    else
      {
        c|=0x80;
        c|=(bits_per_pixel-1);   /* size of local colormap */
        (void) WriteBlobByte(image,(unsigned char) c);
        length=(size_t) (3*(1UL << bits_per_pixel));
        (void) WriteBlob(image,length,colormap);
      }
    /*
      Write the image data.
    */
    c=(int) Max(bits_per_pixel,2);
    (void) WriteBlobByte(image,(unsigned char) c);
    status=EncodeImage(write_info,image,Max(bits_per_pixel,2)+1);
    if (status == MagickFalse)
      {
        global_colormap=(unsigned char *)
          RelinquishMagickMemory(global_colormap);
        colormap=(unsigned char *) RelinquishMagickMemory(colormap);
        write_info=DestroyImageInfo(write_info);
        ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
      }
    (void) WriteBlobByte(image,(unsigned char) 0x00);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        status=image->progress_monitor(SaveImagesTag,scene,
          GetImageListLength(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
    scene++;
  } while (write_info->adjoin != MagickFalse);
  (void) WriteBlobByte(image,';'); /* terminator */
  global_colormap=(unsigned char *) RelinquishMagickMemory(global_colormap);
  colormap=(unsigned char *) RelinquishMagickMemory(colormap);
  write_info=DestroyImageInfo(write_info);
  CloseBlob(image);
  return(MagickTrue);
}
