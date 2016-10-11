  "I swear by my life and my love of it that I will never live for the sake of
   another man, nor ask another man to live for mine"

                    John Galt in "Atlas Shrugged", by Ayn Rand


AUTHOR

  The author is magick-users@imagemagick.org.  This software is NOT shareware.
  However, I am interested in who might be using it.  Please consider sending
	me a picture postcard of the area where you live.  Send postcards to

    ImageMagick Studio LLC
    P.O. Box 40
    Landenberg, PA  19350
    USA

  I'm also interested in receiving currency or stamps from around the world
  for my collection.


AVAILABILITY

  ImageMagick is available as

    ftp://ftp.imagemagick.org/pub/ImageMagick/ImageMagick-6.2.4-4.tar.gz
    ftp://ftp.imagemagick.net/pub/ImageMagick/ImageMagick-6.2.4-4.tar.gz

  ImageMagick client executables are available for some platforms. See

    ftp://ftp.imagemagick.org/pub/ImageMagick/binaries
    ftp://ftp.imagemagick.org/pub/ImageMagick/linux
    ftp://ftp.imagemagick.org/pub/ImageMagick/windows
    ftp://ftp.imagemagick.org/pub/ImageMagick/mac
    ftp://ftp.imagemagick.org/pub/ImageMagick/vms

  I want ImageMagick to be of high quality, so if you encounter a problem I
  will investigate.  However, be sure you are using the most recent version
  from

    ftp://ftp.imagemagick.org/pub/ImageMagick

  before submitting any bug reports or suggestions.  Report any problems via
  the web-based reporting facility at

    http://studio.imagemagick.org/mailman/listinfo/magick-bugs

WWW

  The official ImageMagick WWW page is

    http://www.imagemagick.org/
    http://www.imagemagick.net/

  To use display as your external image viewer, edit the global mail-cap file
  or your personal mail-cap file .mailrc (located at your home directory) and
  put this entry:

    image/*; display %s


MAILING LIST

  There is a mailing list for discussions and bug reports about
  ImageMagick.  To subscribe send the message

    subscribe

  to one of

    magick-users-request@imagemagick.org
    magick-developers-request@imagemagick.org
    magick-bugs-request@imagemagick.org
    magick-announce-request@imagemagick.org

  You will receive a welcome message which tells you how to post messages to
  the list.


CVS

  ImageMagick is currently under development.  It may be retrieved via CVS
  using the following procedure:

  Use

    export CVSROOT=":pserver:anonymous@cvs.imagemagick.org:/ImageMagick"

  or

    setenv CVSROOT=":pserver:anonymous@cvs.imagemagick.org:/ImageMagick"

  to set CVSROOT in the environment (depending on your shell), or prepend
  -d followed by the CVS root to every command. For example

    cvs -d ":pserver:anonymous@cvs ...

  For brevity the following examples assume that CVSROOT is set in the
  environment.

    cvs login
    [ enter "anonymous" ]

    cvs co ImageMagick

  If you would like to retrieve the (huge) Windows source package use

    cvs co ImageMagick-NT

  If you  would like to retrieve *everything* associated with ImageMagick
  (useful or not) use

    cvs co ImageMagick-World


DOCUMENTATION

  Open the file index.html in a web browser, or refer to the ImageMagick(1)
  manual page. Also read the ImageMagick frequently asked questions in the file
  www/FAQ.html.


INSTALLATION

  ImageMagick may be compiled from source code for virtually any modern Unix
  system (including Linux and MacOS X), Microsoft Windows, MacOS9, and VMS.
  Installation instructions may be found in the following files (or their HTML
	equivalents):

    o Unix:

       Install-unix.txt

    o Microsoft Windows:

      Install-windows.txt

    o MacOS 9 (for MacOS X follow the Unix procedure):

      Install-mac.txt:

    o VMS:

      Install-vms.txt


MAGICK DELEGATES

  To further enhance the capabilities of ImageMagick, you may want to get these
  programs or libraries. Note that the Windows source package (equivalent to
  CVS module "ImageMagick-NT") includes (and builds) all of the library-based
  packages listed here.

  o ImageMagick requires the BZLIB library from

        http://sources.redhat.com/bzip2/

    or

        ftp://sources.redhat.com/pub/bzip2/

    to read and write BZip compressed MIFF images.

  o ImageMagick requires ralcgm from

        http://www.agocg.ac.uk/train/cgm/ralcgm.htm

    to read the Computer Image Metafile (CGM) image format. You also need
    Ghostscript (see below).

  o ImageMagick requires 'dcraw' from

        http://www.cybercom.net/~dcoffin/dcraw/

    to read raw images from digital cameras.  Try

        convert crw:image image.png

  o ImageMagick requires fig2dev from

        ftp://ftp.x.org/contrib/applications/drawing_tools/transfig

    to read the Fig image format.

  o ImageMagick requires the FreeType software, version 2.0 or above, available
    as

         http://freetype.sourceforge.net

    to annotate with TrueType and Postscript Type 1 fonts. Note that enabling
    TT_CONFIG_OPTION_BYTECODE_INTERPRETER in FreeType's
    include/freetype/config/ftoption.h will produce better glyph renderings but
    may violate an Apple patent.

  o ImageMagick requires GhostPCL software (version 1.40 recommended)
    available from

        http://www.artifex.com/downloads/

    to read the PCL documents.

  o ImageMagick requires Ghostscript software (version 8.10 recommended)
    available from

        http://www.cs.wisc.edu/~ghost/

    to read the Postscript or the Portable Document format. Ghostscript is used
    to annotate an image when the FreeType library is not used, or an X server
    is not available. See the FreeType library above for another means to
    annotate an image. Note, Ghostscript must support the ppmraw device (type
    gs -h to verify). If Ghostscript is unavailable, the Display Postscript
    X11 extension is used to rasterize a Postscript document (assuming you
    define HasDPS and DPS is available). The DPS extension is less robust than
    Ghostscript in that it will only rasterize one page of a multi-page
    document.

    Ghostscript (release 7.0 and later) may optionally install a library
    (libgs). If this library is installed, ImageMagick may be configured to use
    it. Note that Ghostscript provides its own modified version of libjpeg and
    that symbols from this libjpeg may be confused with symbols with the
    stand-alone libjpeg. If conflicts cause JPEG to fail (JPEG returns an error
    regarding expected structure sizes), it may be necessary to use
    Ghostscript's copy of libjpeg for ImageMagick, and all delegate libraries
    which depend on libjpeg, or convince Ghostscript to build against an
    unmodified installed JPEG library (and loose compatibility with some
    Postscript files).

  o ImageMagick requires hp2xx available from

         http://www.gnu.org/software/hp2xx/hp2xx.html

    to read the HP-GL image format. Note that HPGL is a plotter file format. HP
    printers usually accept PCL format rather than HPGL format.

  o ImageMagick requires the LCMS library available from

         http://www.littlecms.com/

    to perform ICC CMS color management.

  o ImageMagick requires gnuplot available via anonymous FTP as

         ftp://ftp.dartmouth.edu/pub/gnuplot/gnuplot-3.7.tar.gz

    to read GNUPLOT plot files (with extension gplt).

  o ImageMagick requires html2ps available from

         http://www.tdb.uu.se/~jan/html2ps.html

    to rasterize HTML files.

  o ImageMagick requires the JBIG-Kit software available via HTTP from

         http://www.cl.cam.ac.uk/~xml25/jbigkit/

    or via anonymous FTP as

         ftp://ftp.informatik.uni-erlangen.de/pub/doc/ISO/JBIG/

    to read the JBIG image format.

  o ImageMagick requires the Independent JPEG Group's software available via
    anonymous FTP as

         ftp://ftp.uu.net/graphics/jpeg/jpegsrc.v6b.tar.gz

    to read the JPEG v1 image format.

    Apply this JPEG patch to Independent JPEG Group's source distribution if
    you want to read lossless jpeg-encoded DICOM (medical) images:

         ftp://ftp.imagemagick.org/pub/ImageMagick/delegates/ljpeg-6b.tar.gz

    Use of lossless JPEG is not encouraged. Unless you have a requirement to
    read lossless jpeg-encoded DICOM images, please disregard the patch.

  o ImageMagick requires the JasPer Project's Jasper library version 1.701.0
    available via http from

         http://www.ece.uvic.ca/~mdadams/jasper/

    to read and write the JPEG-2000 format.

  o ImageMagick requires the EXIF library available via http from

         http://sourceforge.net/projects/libexif

    to describe tags produced by most digital camera.

  o ImageMagick requires the MPEG utilities from the MPEG Software Simulation
    Group, which are available via anonymous FTP as

         ftp://ftp.mpeg.org/pub/mpeg/mssg/mpeg2vidcodec_v12.tar.gz

    to read or write the MPEG image format.

  o ImageMagick requires the PNG library, version 1.0 or above, from

         http://www.libpng.org/pub/png/pngcode.html

    to read the PNG image format.

  o ImageMagick requires ra_ppm from Greg Ward's Radiance software available
    from

         http://radsite.lbl.gov/radiance/HOME.html

    to read the Radiance image format.

  o ImageMagick requires rawtorle from the Utah Raster Toolkit available via
    anonymous FTP as

         ftp://ftp.cs.utah.edu/pub/dept/OLD/pub/urt-3.1b.tar.Z

    to write the RLE image format.

  o ImageMagick requires scanimage from

         http://www.sane-project.org/ 

    to import an image from a scanner device.

  o ImageMagick requires Sam Leffler's TIFF software available via anonymous
    FTP at

         ftp://ftp.remotesensing.org/libtiff/

    or via HTTP at

         http://www.remotesensing.org/libtiff/

    to read the TIFF image format. It in turn optionally requires the JPEG and
    ZLIB libraries.

  o ImageMagick requires libwmf 0.2.5 (or later) from

         http://sourceforge.net/projects/wvware/

    to render files in the Windows Meta File (WMF) metafile format (16-bit WMF
    files only, not 32-bit "EMF"). This is the format commonly used for Windows
    clipart (available on CD at your local computer or technical book store).
    WMF support requires the FreeType 2 library in order to render TrueType and
    Postscript fonts.

    While ImageMagick uses the libwmflite (parser) component of the
    libwmf package which does not depend on any special libraries,
    the libwmf package as a whole depends on FreeType 2 and either the
    xmlsoft libxml, or expat libraries. Since ImageMagick already uses
    libxml (for reading SVG and to retrieve files via HTTP or FTP),
    it is recommended that the options '--without-expat --with-xml'
    be supplied to libwmf's configure script.

    ImageMagick's WMF renderer provides some of the finest WMF rendering
    available due its use of antialiased drawing algorithms.  You may select a
    background color or texture image to render on.  For example, "-background
    '#ffffffff'" renders on a transparent background while "-texture
    plasma:fractal" renders on a fractal image.

    A free set of Microsoft Windows fonts may be retrieved from
    "http://sourceforge.net/projects/corefonts/".

  o ImageMagick requires the FlashPIX library version 1.2.0 from the Digital
    Imaging Group in order to support the FlashPIX format. The FlashPIX library
    may be obtained from ImageMagick anonymous CVS by checking out the 'fpx'
    module, or retrieving the file libfpx-1.2.0.9.tar.gz from the ftp
    directory.

         ftp://ftp.imagemagick.org/pub/ImageMagick/delegates/

  o ImageMagick requires an X server for the 'display', 'animate', and 'import'
    commands to work properly. Unix systems usually provide an X server as
    part of their standard installation.

    A free X server for Microsoft Windows is available from

         http://sources.redhat.com/win32-x11/

    The Cygwin port of XFree86 may also be used. It is available from

         http://www.cygwin.com/xfree/

    There is a nearly free X server available for Windows and Macintosh at

         http://www.microimages.com/freestuf/mix/

  o ImageMagick requires libxml available from

         http://xmlsoft.org/

    to read the SVG image format and to retrieve files from over a network via
    FTP and HTTP.

  o ImageMagick requires the ZLIB library from

         http://www.gzip.org/zlib/

    to read or write the PNG or Zip compressed MIFF images.

  o ImageMagick requires a background texture for the TILE format and for the
    -texture option of montage(1).  You can use your own or get samples from

         http://the-tech.mit.edu/KPT/
