/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   OOO   PPPP   TTTTT  IIIII   OOO   N   N                   %
%                  O   O  P   P    T      I    O   O  NN  N                   %
%                  O   O  PPPP     T      I    O   O  N N N                   %
%                  O   O  P        T      I    O   O  N  NN                   %
%                   OOO   P        T    IIIII   OOO   N   N                   %
%                                                                             %
%                                                                             %
%                        ImageMagick Option Methods                           %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 March 2000                                  %
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
#include "magick/studio.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/compare.h"
#include "magick/constitute.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/fx.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/montage.h"
#include "magick/option.h"
#include "magick/resource_.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"

/*
  ImageMagick values.
*/
static const OptionInfo
  AlignOptions[] =
  {
    { "Undefined", (long) UndefinedAlign },
    { "Center", (long) CenterAlign },
    { "End", (long) RightAlign },
    { "Left", (long) LeftAlign },
    { "Middle", (long) CenterAlign },
    { "Right", (long) RightAlign },
    { "Start", (long) LeftAlign },
    { (char *) NULL, (long) UndefinedAlign }
  },
  BooleanOptions[] =
  {
    { "False", 0L },
    { "True", 1L },
    { "0", 0L },
    { "1", 1L },
    { (char *) NULL, 0L }
  },
  ChannelOptions[] =
  {
    { "Undefined", (long) UndefinedChannel },
    { "All", (long) AllChannels },
    { "Alpha", (long) OpacityChannel },
    { "Black", (long) BlackChannel },
    { "Blue", (long) BlueChannel },
    { "Cyan", (long) CyanChannel },
    { "Default", (long) AllChannels &~ (long) OpacityChannel },
    { "Gray", (long) GrayChannel },
    { "Green", (long) GreenChannel },
    { "Index", (long) IndexChannel },
    { "Magenta", (long) MagentaChannel },
    { "Matte", (long) OpacityChannel },
    { "Opacity", (long) OpacityChannel },
    { "Red", (long) RedChannel },
    { "Yellow", (long) YellowChannel },
    { (char *) NULL, (long) UndefinedChannel }
  },
  ClassOptions[] =
  {
    { "Undefined", (long) UndefinedClass },
    { "DirectClass", (long) DirectClass },
    { "PseudoClass", (long) PseudoClass },
    { (char *) NULL, (long) UndefinedClass }
  },
  ClipPathOptions[] =
  {
    { "Undefined", (long) UndefinedPathUnits },
    { "ObjectBoundingBox", (long) ObjectBoundingBox },
    { "UserSpace", (long) UserSpace },
    { "UserSpaceOnUse", (long) UserSpaceOnUse },
    { (char *) NULL, (long) UndefinedPathUnits }
  },
  CommandOptions[] =
  {
    { "-{0}", 0L },
    { "-{1}", 1L },
    { "-{2}", 2L },
    { "-adjoin", 0L },
    { "-affine", 1L },
    { "-annotate", 2L },
    { "-antialias", 0L },
    { "-append", 0L },
    { "-authenticate", 1L },
    { "-average", 0L },
    { "-background", 1L },
    { "-bias", 1L },
    { "-black-threshold", 1L },
    { "-blend", 1L },
    { "-blue-primary", 1L },
    { "-blur", 1L },
    { "-border", 1L },
    { "-bordercolor", 1L },
    { "-box", 1L },
    { "-cache", 1L },
    { "-channel", 1L },
    { "-charcoal", 1L },
    { "-chop", 1L },
    { "-clip-path", 1L },
    { "-clone", 1L },
    { "-coalesce", 0L },
    { "-colorize", 1L },
    { "-colors", 1L },
    { "-colorspace", 1L },
    { "-combine", 0L },
    { "-comment", 1L },
    { "-compose", 1L },
    { "-composite", 0L },
    { "-compress", 1L },
    { "-contrast", 0L },
    { "-convolve", 1L },
    { "-crop", 1L },
    { "-cycle", 1L },
    { "-deconstruct", 0L },
    { "-debug", 1L },
    { "-define", 1L },
    { "+define", 1L },
    { "-delay", 1L },
    { "-delete", 1L },
    { "-density", 1L },
    { "-depth", 1L },
    { "-despeckle", 0L },
    { "-display", 1L },
    { "-dispose", 1L },
    { "-dither", 0L },
    { "-draw", 1L },
    { "-edge", 1L },
    { "-emboss", 1L },
    { "-encoding", 1L },
    { "-endian", 1L },
    { "-enhance", 0L },
    { "-equalize", 0L },
    { "-evaluate", 2L },
    { "-extent", 1L },
    { "-extract", 1L },
    { "-family", 1L },
    { "-fill", 1L },
    { "-filter", 1L },
    { "-flatten", 0L },
    { "-flip", 0L },
    { "-flop", 0L },
    { "-floodfill", 2L },
    { "-family", 1L },
    { "-font", 1L },
    { "-format", 1L },
    { "-frame", 1L },
    { "-fuzz", 1L },
    { "-fx", 1L },
    { "-gamma", 1L },
    { "-gaussian", 1L },
    { "-geometry", 1L },
    { "-green-primary", 1L },
    { "-gravity", 1L },
    { "-help", 0L },
    { "-identify", 0L },
    { "-implode", 1L },
    { "-insert", 1L },
    { "-intent", 1L },
    { "-interlace", 1L },
    { "-label", 1L },
    { "-lat", 1L },
    { "-level", 1L },
    { "-limit", 2L },
    { "-linewidth", 1L },
    { "-list", 1L },
    { "-log", 1L },
    { "-loop", 1L },
    { "-map", 1L },
    { "-mask", 1L },
    { "-matte", 0L },
    { "-mattecolor", 1L },
    { "-median", 1L },
    { "-mode", 1L },
    { "-modulate", 1L },
    { "-monochrome", 0L },
    { "-morph", 1L },
    { "-mosaic", 0L },
    { "-motion-blur", 1L },
    { "-negate", 0L },
    { "-normalize", 0L },
    { "-noise", 1L },
    { "+noise", 1L },
    { "-opaque", 1L },
    { "-ordered-dither", 1L },
    { "-orient", 1L },
    { "-page", 1L },
    { "-paint", 1L },
    { "-pen", 1L },
    { "-ping", 0L },
    { "-pointsize", 1L },
    { "-posterize", 1L },
    { "-preview", 1L },
    { "-profile", 1L },
    { "+profile", 1L },
    { "-process", 1L },
    { "-quality", 1L },
    { "-radial-blur", 1L },
    { "-raise", 1L },
    { "-random-threshold", 1L },
    { "-region", 1L },
    { "-raise", 1L },
    { "-red-primary", 1L },
    { "-render", 0L },
    { "-repage", 1L },
    { "-resample", 1L },
    { "-resize", 1L },
    { "-roll", 1L },
    { "-rotate", 1L },
    { "-sample", 1L },
    { "-sampling-factor", 1L },
    { "-scale", 1L },
    { "-scene", 1L },
    { "-seed", 1L },
    { "-segment", 1L },
    { "-separate", 0L },
    { "+set", 1L },
    { "-set", 2L },
    { "-shade", 1L },
    { "-shadow", 1L },
    { "-sharpen", 1L },
    { "-shave", 1L },
    { "-shear", 1L },
    { "-size", 1L },
    { "-snaps", 1L },
    { "-solarize", 1L },
    { "-splice", 1L },
    { "-spread", 1L },
    { "-stegano", 1L },
    { "-stroke", 1L },
    { "-strokewidth", 1L },
    { "-stretch", 1L },
    { "-strip", 0L },
    { "-style", 1L },
    { "-support", 1L },
    { "-swap", 1L },
    { "-swirl", 1L },
    { "-texture", 1L },
    { "-threshold", 1L },
    { "-thumbnail", 1L },
    { "-tile", 1L },
    { "-tint", 1L },
    { "-transparent", 1L },
    { "-treedepth", 1L },
    { "-trim", 0L },
    { "-type", 1L },
    { "-undercolor", 1L },
    { "-units", 1L },
    { "-unsharp", 1L },
    { "-update", 1L },
    { "-verbose", 0L },
    { "-version", 0L },
    { "-view", 1L },
    { "-virtual-pixel", 1L },
    { "-watermark", 1L },
    { "-wave", 1L },
    { "-weight", 1L },
    { "-window", 1L },
    { "-window_group", 1L },
    { "-white-point", 1L },
    { "-white-threshold", 1L },
    { "-write", 1L },
    { (char *) NULL, (long) 0L }
  },
  CompositeOptions[] =
  {
    { "Undefined", (long) UndefinedCompositeOp },
    { "Add", (long) AddCompositeOp },
    { "Atop", (long) SrcAtopCompositeOp },
    { "Blend", (long) BlendCompositeOp },
    { "Bumpmap", (long) BumpmapCompositeOp },
    { "Clear", (long) ClearCompositeOp },
    { "ColorBurn", (long) ColorBurnCompositeOp },
    { "ColorDodge", (long) ColorDodgeCompositeOp },
    { "Colorize", (long) ColorizeCompositeOp },
    { "CopyBlack", (long) CopyBlackCompositeOp },
    { "CopyBlue", (long) CopyBlueCompositeOp },
    { "CopyCyan", (long) CopyCyanCompositeOp },
    { "CopyGreen", (long) CopyGreenCompositeOp },
    { "Copy", (long) CopyCompositeOp },
    { "CopyMagenta", (long) CopyMagentaCompositeOp },
    { "CopyOpacity", (long) CopyOpacityCompositeOp },
    { "CopyRed", (long) CopyRedCompositeOp },
    { "CopyYellow", (long) CopyYellowCompositeOp },
    { "Darken", (long) DarkenCompositeOp },
    { "Dst", (long) DstCompositeOp },
    { "Difference", (long) DifferenceCompositeOp },
    { "Displace", (long) DisplaceCompositeOp },
    { "Dissolve", (long) DissolveCompositeOp },
    { "DstAtop", (long) DstAtopCompositeOp },
    { "DstIn", (long) DstInCompositeOp },
    { "DstOut", (long) DstOutCompositeOp },
    { "DstOver", (long) DstOverCompositeOp },
    { "Dst", (long) DstCompositeOp },
    { "Exclusion", (long) ExclusionCompositeOp },
    { "HardLight", (long) HardLightCompositeOp },
    { "Hue", (long) HueCompositeOp },
    { "In", (long) InCompositeOp },
    { "Lighten", (long) LightenCompositeOp },
    { "Luminize", (long) LuminizeCompositeOp },
    { "Minus", (long) MinusCompositeOp },
    { "Modulate", (long) ModulateCompositeOp },
    { "Multiply", (long) MultiplyCompositeOp },
    { "None", (long) NoCompositeOp },
    { "Out", (long) SrcOutCompositeOp },
    { "Overlay", (long) OverlayCompositeOp },
    { "Over", (long) OverCompositeOp },
    { "Plus", (long) PlusCompositeOp },
    { "Replace", (long) ReplaceCompositeOp },
    { "Saturate", (long) SaturateCompositeOp },
    { "Screen", (long) ScreenCompositeOp },
    { "SoftLight", (long) SoftLightCompositeOp },
    { "Src", (long) SrcCompositeOp },
    { "SrcAtop", (long) SrcAtopCompositeOp },
    { "SrcIn", (long) SrcInCompositeOp },
    { "SrcOut", (long) SrcOutCompositeOp },
    { "SrcOver", (long) SrcOverCompositeOp },
    { "Src", (long) SrcCompositeOp },
    { "Subtract", (long) SubtractCompositeOp },
    { "Threshold", (long) ThresholdCompositeOp },
    { "Xor", (long) XorCompositeOp },
    { (char *) NULL, (long) UndefinedCompositeOp }
  },
  CompressionOptions[] =
  {
    { "Undefined", (long) UndefinedCompression },
    { "BZip", (long) BZipCompression },
    { "Fax", (long) FaxCompression },
    { "Group4", (long) Group4Compression },
    { "JPEG", (long) JPEGCompression },
    { "JPEG2000", (long) JPEG2000Compression },
    { "Lossless", (long) LosslessJPEGCompression },
    { "LosslessJPEG", (long) LosslessJPEGCompression },
    { "LZW", (long) LZWCompression },
    { "None", (long) NoCompression },
    { "RLE", (long) RLECompression },
    { "Zip", (long) ZipCompression },
    { "RunlengthEncoded", (long) RLECompression },
    { (char *) NULL, (long) UndefinedCompression }
  },
  ColorspaceOptions[] =
  {
    { "Undefined", (long) UndefinedColorspace },
    { "CMYK", (long) CMYKColorspace },
    { "Gray", (long) GRAYColorspace },
    { "HSB", (long) HSBColorspace },
    { "HSL", (long) HSLColorspace },
    { "HWB", (long) HWBColorspace },
    { "LAB", (long) LABColorspace },
    { "Log", (long) LogColorspace },
    { "OHTA", (long) OHTAColorspace },
    { "Rec601Luma", (long) Rec601LumaColorspace },
    { "Rec709Luma", (long) Rec709LumaColorspace },
    { "RGB", (long) RGBColorspace },
    { "sRGB", (long) sRGBColorspace },
    { "Transparent", (long) TransparentColorspace },
    { "XYZ", (long) XYZColorspace },
    { "YCbCr", (long) YCbCrColorspace },
    { "YCC", (long) YCCColorspace },
    { "YIQ", (long) YIQColorspace },
    { "YPbPr", (long) YPbPrColorspace },
    { "YUV", (long) YUVColorspace },
    { (char *) NULL, (long) UndefinedColorspace }
  },
  DecorationOptions[] =
  {
    { "Undefined", (long) UndefinedDecoration },
    { "LineThrough", (long) LineThroughDecoration },
    { "None", (long) NoDecoration },
    { "Overline", (long) OverlineDecoration },
    { "Underline", (long) UnderlineDecoration },
    { (char *) NULL, (long) UndefinedDecoration }
  },
  DisposeOptions[] =
  {
    { "Background", (long) BackgroundDispose },
    { "None", (long) NoneDispose },
    { "Previous", (long) PreviousDispose },
    { "Undefined", (long) UndefinedDispose },
    { "0", (long) UndefinedDispose },
    { "1", (long) NoneDispose },
    { "2", (long) BackgroundDispose },
    { "3", (long) PreviousDispose },
    { (char *) NULL, (long) UndefinedDispose }
  },
  EndianOptions[] =
  {
    { "Undefined", (long) UndefinedEndian },
    { "LSB", (long) LSBEndian },
    { "MSB", (long) MSBEndian },
    { (char *) NULL, (long) UndefinedEndian }
  },
  EvaluateOptions[] =
  {
    { "Undefined", (long) UndefinedEvaluateOperator },
    { "Add", (long) AddEvaluateOperator },
    { "And", (long) AndEvaluateOperator },
    { "Divide", (long) DivideEvaluateOperator },
    { "LeftShift", (long) LeftShiftEvaluateOperator },
    { "Max", (long) MaxEvaluateOperator },
    { "Min", (long) MinEvaluateOperator },
    { "Multiply", (long) MultiplyEvaluateOperator },
    { "Or", (long) OrEvaluateOperator },
    { "RightShift", (long) RightShiftEvaluateOperator },
    { "Set", (long) SetEvaluateOperator },
    { "Subtract", (long) SubtractEvaluateOperator },
    { "Xor", (long) XorEvaluateOperator },
    { (char *) NULL, (long) UndefinedEvaluateOperator }
  },
  FillRuleOptions[] =
  {
    { "Undefined", (long) UndefinedRule },
    { "Evenodd", (long) EvenOddRule },
    { "NonZero", (long) NonZeroRule },
    { (char *) NULL, (long) UndefinedRule }
  },
  FilterOptions[] =
  {
    { "Undefined", (long) UndefinedFilter },
    { "Bessel", (long) BesselFilter },
    { "Blackman", (long) BlackmanFilter },
    { "Box", (long) BoxFilter },
    { "Catrom", (long) CatromFilter },
    { "Cubic", (long) CubicFilter },
    { "Gaussian", (long) GaussianFilter },
    { "Hamming", (long) HammingFilter },
    { "Hanning", (long) HanningFilter },
    { "Hermite", (long) HermiteFilter },
    { "Lanczos", (long) LanczosFilter },
    { "Mitchell", (long) MitchellFilter },
    { "Point", (long) PointFilter },
    { "Quadratic", (long) QuadraticFilter },
    { "Sinc", (long) SincFilter },
    { "Triangle", (long) TriangleFilter },
    { (char *) NULL, (long) UndefinedFilter }
  },
  GravityOptions[] =
  {
    { "Undefined", (long) UndefinedGravity },
    { "None", (long) UndefinedGravity },
    { "Center", (long) CenterGravity },
    { "East", (long) EastGravity },
    { "Forget", (long) ForgetGravity },
    { "NorthEast", (long) NorthEastGravity },
    { "North", (long) NorthGravity },
    { "NorthWest", (long) NorthWestGravity },
    { "SouthEast", (long) SouthEastGravity },
    { "South", (long) SouthGravity },
    { "SouthWest", (long) SouthWestGravity },
    { "West", (long) WestGravity },
    { "Static", (long) StaticGravity },
    { (char *) NULL, UndefinedGravity }
  },
  ImageOptions[] =
  {
    { "Undefined", (long) UndefinedType },
    { "Bilevel", (long) BilevelType },
    { "ColorSeparation", (long) ColorSeparationType },
    { "ColorSeparationMatte", (long) ColorSeparationMatteType },
    { "Grayscale", (long) GrayscaleType },
    { "GrayscaleMatte", (long) GrayscaleMatteType },
    { "Optimize", (long) OptimizeType },
    { "PaletteMatte", (long) PaletteMatteType },
    { "Palette", (long) PaletteType },
    { "TrueColorMatte", (long) TrueColorMatteType },
    { "TrueColor", (long) TrueColorType },
    { (char *) NULL, (long) UndefinedType }
  },
  IntentOptions[] =
  {
    { "Undefined", (long) UndefinedIntent },
    { "Absolute", (long) AbsoluteIntent },
    { "Perceptual", (long) PerceptualIntent },
    { "Relative", (long) RelativeIntent },
    { "Saturation", (long) SaturationIntent },
    { (char *) NULL, (long) UndefinedIntent }
  },
  InterlaceOptions[] =
  {
    { "Undefined", (long) UndefinedInterlace },
    { "Line", (long) LineInterlace },
    { "None", (long) NoInterlace },
    { "Plane", (long) PlaneInterlace },
    { "Partition", (long) PartitionInterlace },
    { (char *) NULL, (long) UndefinedInterlace }
  },
  LineCapOptions[] =
  {
    { "Undefined", (long) UndefinedCap },
    { "Butt", (long) ButtCap },
    { "Round", (long) RoundCap },
    { "Square", (long) SquareCap },
    { (char *) NULL, (long) UndefinedCap }
  },
  LineJoinOptions[] =
  {
    { "Undefined", (long) UndefinedJoin },
    { "Bevel", (long) BevelJoin },
    { "Miter", (long) MiterJoin },
    { "Round", (long) RoundJoin },
    { (char *) NULL, (long) UndefinedJoin }
  },
  ListOptions[] =
  {
    { "Coder", 1L },
    { "Color", 1L },
    { "Configure", 1L },
    { "Delegate", 1L },
    { "Format", 1L },
    { "Locale", 1L },
    { "Log", 1L },
    { "Magic", 1L },
    { "Module", 1L },
    { "Resource", 1L },
    { "Type", 1L },
    { (char *) NULL, 0L }
  },
  LogEventOptions[] =
  {
    { "Undefined", (long) UndefinedEvents },
    { "All", (long) (AllEvents &~ TraceEvent) },
    { "Annotate", (long) AnnotateEvent },
    { "Blob", (long) BlobEvent },
    { "Cache", (long) CacheEvent },
    { "Coder", (long) CoderEvent },
    { "Configure", (long) ConfigureEvent },
    { "Deprecate", (long) DeprecateEvent },
    { "Draw", (long) DrawEvent },
    { "Exception", (long) ExceptionEvent },
    { "Locale", (long) LocaleEvent },
    { "Module", (long) ModuleEvent },
    { "None", (long) NoEvents },
    { "Resource", (long) ResourceEvent },
    { "Trace", (long) TraceEvent },
    { "Transform", (long) TransformEvent },
    { "User", (long) UserEvent },
    { "Wand", (long) WandEvent },
    { "X11", (long) X11Event },
    { (char *) NULL, (long) UndefinedEvents }
  },
  MetricOptions[] =
  {
    { "Undefined", (long) UndefinedMetric },
    { "MAE", (long) MeanAbsoluteErrorMetric },
    { "MSE", (long) MeanSquaredErrorMetric },
    { "PAE", (long) PeakAbsoluteErrorMetric },
    { "PSNR", (long) PeakSignalToNoiseRatioMetric },
    { "RMSE", (long) RootMeanSquaredErrorMetric },
    { (char *) NULL, (long) UndefinedMetric }
  },
  MethodOptions[] =
  {
    { "Undefined", (long) UndefinedMethod },
    { "FillToBorder", (long) FillToBorderMethod },
    { "Floodfill", (long) FloodfillMethod },
    { "Point", (long) PointMethod },
    { "Replace", (long) ReplaceMethod },
    { "Reset", (long) ResetMethod },
    { (char *) NULL, (long) UndefinedMethod }
  },
  ModeOptions[] =
  {
    { "Undefined", (long) UndefinedMode },
    { "Concatenate", (long) ConcatenateMode },
    { "Frame", (long) FrameMode },
    { "Unframe", (long) UnframeMode },
    { (char *) NULL, (long) UndefinedMode }
  },
  MogrifyOptions[] =
  {
    { "append", MagickTrue },
    { "average", MagickTrue },
    { "coalesce", MagickTrue },
    { "combine", MagickTrue },
    { "composite", MagickTrue },
    { "crop", MagickTrue },
    { "deconstruct", MagickTrue },
    { "delete", MagickTrue },
    { "flatten", MagickTrue },
    { "fx", MagickTrue },
    { "insert", MagickTrue },
    { "map", MagickTrue },
    { "morph", MagickTrue },
    { "mosaic", MagickTrue },
    { "process", MagickTrue },
    { "scene", MagickTrue },
    { "swap", MagickTrue },
    { "write", MagickTrue },
    { (char *) NULL, MagickFalse }
  },
  NoiseOptions[] =
  {
    { "Undefined", (long) UndefinedNoise },
    { "Gaussian", (long) (long) GaussianNoise },
    { "Impulse", (long) ImpulseNoise },
    { "Laplacian", (long) LaplacianNoise },
    { "Multiplicative", (long) MultiplicativeGaussianNoise },
    { "Poisson", (long) PoissonNoise },
    { "Uniform", (long) UniformNoise },
    { (char *) NULL, (long) UndefinedNoise }
  },
  OrientationOptions[] =
  {
    { "Undefined", (long) UndefinedOrientation },
    { "TopLeft", (long) TopLeftOrientation },
    { "TopRight", (long) TopRightOrientation },
    { "BottomRight", (long) BottomRightOrientation },
    { "BottomLeft", (long) BottomLeftOrientation },
    { "LeftTop", (long) LeftTopOrientation },
    { "RightTop", (long) RightTopOrientation },
    { "RightBottom", (long) RightBottomOrientation },
    { "LeftBottom", (long) LeftBottomOrientation }
  },
  PreviewOptions[] =
  {
    { "Undefined", (long) UndefinedPreview },
    { "AddNoise", (long) AddNoisePreview },
    { "Blur", (long) BlurPreview },
    { "Brightness", (long) BrightnessPreview },
    { "Charcoal", (long) CharcoalDrawingPreview },
    { "Despeckle", (long) DespecklePreview },
    { "Dull", (long) DullPreview },
    { "EdgeDetect", (long) EdgeDetectPreview },
    { "Gamma", (long) GammaPreview },
    { "Grayscale", (long) GrayscalePreview },
    { "Hue", (long) HuePreview },
    { "Implode", (long) ImplodePreview },
    { "JPEG", (long) JPEGPreview },
    { "OilPaint", (long) OilPaintPreview },
    { "Quantize", (long) QuantizePreview },
    { "Raise", (long) RaisePreview },
    { "ReduceNoise", (long) ReduceNoisePreview },
    { "Roll", (long) RollPreview },
    { "Rotate", (long) RotatePreview },
    { "Saturation", (long) SaturationPreview },
    { "Segment", (long) SegmentPreview },
    { "Shade", (long) ShadePreview },
    { "Sharpen", (long) SharpenPreview },
    { "Shear", (long) ShearPreview },
    { "Solarize", (long) SolarizePreview },
    { "Spiff", (long) SpiffPreview },
    { "Spread", (long) SpreadPreview },
    { "Swirl", (long) SwirlPreview },
    { "Threshold", (long) ThresholdPreview },
    { "Wave", (long) WavePreview },
    { (char *) NULL, (long) UndefinedPreview }
  },
  PrimitiveOptions[] =
  {
    { "Undefined", (long) UndefinedPrimitive },
    { "Arc", (long) ArcPrimitive },
    { "Bezier", (long) BezierPrimitive },
    { "Circle", (long) CirclePrimitive },
    { "Color", (long) ColorPrimitive },
    { "Ellipse", (long) EllipsePrimitive },
    { "Image", (long) ImagePrimitive },
    { "Line", (long) LinePrimitive },
    { "Matte", (long) MattePrimitive },
    { "Path", (long) PathPrimitive },
    { "Point", (long) PointPrimitive },
    { "Polygon", (long) PolygonPrimitive },
    { "Polyline", (long) PolylinePrimitive },
    { "Rectangle", (long) RectanglePrimitive },
    { "roundRectangle", (long) RoundRectanglePrimitive },
    { "Text", (long) TextPrimitive },
    { (char *) NULL, (long) UndefinedPrimitive }
  },
  ResolutionOptions[] =
  {
    { "Undefined", (long) UndefinedResolution },
    { "PixelsPerInch", (long) PixelsPerInchResolution },
    { "PixelsPerCentimeter", (long) PixelsPerCentimeterResolution },
    { (char *) NULL, (long) UndefinedResolution }
  },
  ResourceOptions[] =
  {
    { "Undefined", (long) UndefinedResource },
    { "Area", (long) AreaResource },
    { "Disk", (long) DiskResource },
    { "File", (long) FileResource },
    { "Map", (long) MapResource },
    { "Memory", (long) MemoryResource },
    { (char *) NULL, (long) UndefinedResource }
  },
  StorageOptions[] =
  {
    { "Undefined", (long) UndefinedPixel },
    { "Char", (long) CharPixel },
    { "Double", (long) DoublePixel },
    { "Float", (long) FloatPixel },
    { "Integer", (long) IntegerPixel },
    { "Long", (long) LongPixel },
    { "Quantum", (long) QuantumPixel },
    { "Short", (long) ShortPixel },
    { (char *) NULL, (long) UndefinedResource }
  },
  StretchOptions[] =
  {
    { "Undefined", (long) UndefinedStretch },
    { "Any", (long) AnyStretch },
    { "Condensed", (long) CondensedStretch },
    { "Expanded", (long) ExpandedStretch },
    { "ExtraCondensed", (long) ExtraCondensedStretch },
    { "ExtraExpanded", (long) ExtraExpandedStretch },
    { "Normal", (long) NormalStretch },
    { "SemiCondensed", (long) SemiCondensedStretch },
    { "SemiExpanded", (long) SemiExpandedStretch },
    { "UltraCondensed", (long) UltraCondensedStretch },
    { "UltraExpanded", (long) UltraExpandedStretch },
    { (char *) NULL, (long) UndefinedStretch }
  },
  StyleOptions[] =
  {
    { "Undefined", (long) UndefinedStyle },
    { "Any", (long) AnyStyle },
    { "Italic", (long) ItalicStyle },
    { "Normal", (long) NormalStyle },
    { "Oblique", (long) ObliqueStyle },
    { (char *) NULL, (long) UndefinedStyle }
  },
  VirtualPixelOptions[] =
  {
    { "Undefined", (long) UndefinedVirtualPixelMethod },
    { "Background", (long) BackgroundVirtualPixelMethod },
    { "Constant", (long) BackgroundVirtualPixelMethod },
    { "Edge", (long) EdgeVirtualPixelMethod },
    { "Mirror", (long) MirrorVirtualPixelMethod },
    { "Tile", (long) TileVirtualPixelMethod },
    { "Transparent", (long) TransparentVirtualPixelMethod },
    { (char *) NULL, (long) UndefinedVirtualPixelMethod }
  };

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e O p t i o n s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageOptions() clones one or more image options.
%
%  The format of the CloneImageOptions method is:
%
%      MagickBooleanType CloneImageOptions(ImageInfo *image_info,
%        const ImageInfo *clone_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o clone_info: The clone image info.
%
%
*/
MagickExport MagickBooleanType CloneImageOptions(ImageInfo *image_info,
  const ImageInfo *clone_info)
{
  const char
    *key,
    *value;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(clone_info != (const ImageInfo *) NULL);
  assert(clone_info->signature == MagickSignature);
  if (clone_info->options == (void *) NULL)
    return(MagickTrue);
  ResetImageOptionIterator(clone_info);
  for (key=GetNextImageOption(clone_info); key != (char *) NULL; )
  {
    value=GetImageOption(clone_info,key);
    if (value != (const char *) NULL)
      (void) SetImageOption(image_info,key,value);
    key=GetNextImageOption(clone_info);
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e f i n e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DefineImageOption() associates a key/value pair with an image option.
%
%  The format of the DefineImageOption method is:
%
%      MagickBooleanType DefineImageOption(ImageInfo *image_info,
%        const char *option)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o option: The image option.
%
%
*/
MagickExport MagickBooleanType DefineImageOption(ImageInfo *image_info,
  const char *option)
{
  char
    key[MaxTextExtent],
    value[MaxTextExtent];

  register char
    *p;

  assert(image_info != (ImageInfo *) NULL);
  assert(option != (const char *) NULL);
  (void) CopyMagickString(key,option,MaxTextExtent);
  for (p=key; *p != '\0'; p++)
    if (*p == '=')
      break;
  *value='\0';
  if (*p == '=')
    (void) CopyMagickString(value,p+1,MaxTextExtent);
  *p='\0';
  return(SetImageOption(image_info,key,value));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteImageOption() deletes an key from the image map.
%
%  The format of the DeleteImageOption method is:
%
%      MagickBooleanType DeleteImageOption(ImageInfo *image_info,
%        const char *key)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o option: The image option.
%
*/
MagickExport MagickBooleanType DeleteImageOption(ImageInfo *image_info,
  const char *option)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return(MagickFalse);
  return(RemoveNodeFromSplayTree((SplayTreeInfo *) image_info->options,option));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e O p t i o n s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageOptions() releases memory associated with image option values.
%
%  The format of the DestroyDefines method is:
%
%      void DestroyImageOptions(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%
*/
MagickExport void DestroyImageOptions(ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options != (void *) NULL)
    image_info->options=DestroySplayTree((SplayTreeInfo *) image_info->options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e O p t i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageOption() gets a value associated with an image option.
%
%  The format of the GetImageOption method is:
%
%      const char *GetImageOption(const ImageInfo *image_info,
%        const char *key)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o key: The key.
%
*/
MagickExport const char *GetImageOption(const ImageInfo *image_info,
  const char *key)
{
  const char
    *option;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return((const char *) NULL);
  option=(const char *)
    GetValueFromSplayTree((SplayTreeInfo *) image_info->options,key);
  return(option);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k O p t i o n s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickOptions() returns a list of values.
%
%  The format of the GetMagickOptions method is:
%
%      const char **GetMagickOptions(const MagickOption value)
%
%  A description of each parameter follows:
%
%    o value: The value.
%
%
*/

static const OptionInfo *GetOptionInfo(const MagickOption value)
{
  switch (value)
  {
    case MagickAlignOptions: return(AlignOptions);
    case MagickBooleanOptions: return(BooleanOptions);
    case MagickChannelOptions: return(ChannelOptions);
    case MagickClassOptions: return(ClassOptions);
    case MagickClipPathOptions: return(ClipPathOptions);
    case MagickColorspaceOptions: return(ColorspaceOptions);
    case MagickCommandOptions: return(CommandOptions);
    case MagickCompositeOptions: return(CompositeOptions);
    case MagickCompressionOptions: return(CompressionOptions);
    case MagickDecorationOptions: return(DecorationOptions);
    case MagickDisposeOptions: return(DisposeOptions);
    case MagickEndianOptions: return(EndianOptions);
    case MagickEvaluateOptions: return(EvaluateOptions);
    case MagickFillRuleOptions: return(FillRuleOptions);
    case MagickFilterOptions: return(FilterOptions);
    case MagickGravityOptions: return(GravityOptions);
    case MagickImageOptions: return(ImageOptions);
    case MagickIntentOptions: return(IntentOptions);
    case MagickInterlaceOptions: return(InterlaceOptions);
    case MagickLineCapOptions: return(LineCapOptions);
    case MagickLineJoinOptions: return(LineJoinOptions);
    case MagickListOptions: return(ListOptions);
    case MagickLogEventOptions: return(LogEventOptions);
    case MagickMetricOptions: return(MetricOptions);
    case MagickMethodOptions: return(MethodOptions);
    case MagickModeOptions: return(ModeOptions);
    case MagickMogrifyOptions: return(MogrifyOptions);
    case MagickNoiseOptions: return(NoiseOptions);
    case MagickOrientationOptions: return(OrientationOptions);
    case MagickPreviewOptions: return(PreviewOptions);
    case MagickPrimitiveOptions: return(PrimitiveOptions);
    case MagickResolutionOptions: return(ResolutionOptions);
    case MagickResourceOptions: return(ResourceOptions);
    case MagickStorageOptions: return(StorageOptions);
    case MagickStretchOptions: return(StretchOptions);
    case MagickStyleOptions: return(StyleOptions);
    case MagickVirtualPixelOptions: return(VirtualPixelOptions);
    default: break;
  }
  return((const OptionInfo *) NULL);
}

MagickExport char **GetMagickOptions(const MagickOption value)
{
  char
    **values;

  const OptionInfo
    *option_info;

  register long
    i;

  option_info=GetOptionInfo(value);
  if (option_info == (const OptionInfo *) NULL)
    return((char **) NULL);
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++);
  values=(char **) AcquireMagickMemory((size_t) (i+1)*sizeof(*values));
  if (values == (char **) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"UnableToAcquireString",
      option_info[0].mnemonic);
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++)
    values[i]=AcquireString(option_info[i].mnemonic);
  values[i]=(char *) NULL;
  return(values);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t I m a g e O p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextImageOption() gets the next image option value.
%
%  The format of the GetNextImageOption method is:
%
%      char *GetNextImageOption(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
*/
MagickExport char *GetNextImageOption(const ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return((char *) NULL);
  return((char *) GetNextKeyInSplayTree((SplayTreeInfo *) image_info->options));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     I s M a g i c k O p t i o n                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMagickOption() returns MagickTrue if the option begins with a - or +.
%
%  The format of the IsMagickOption method is:
%
%      MagickBooleanType IsMagickOption(const char *option)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType IsMagickOption(const char *option)
{
  assert(option != (const char *) NULL);
  if ((*option != '-') && (*option != '+'))
    return(MagickFalse);
  if (strlen(option) == 1)
    return(MagickFalse);
  if (isalpha((int) (unsigned char) *(option+1)) == 0)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a g i c k O p t i o n T o M n e m o n i c                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickOptionToMnemonic() returns an enumerated value as a mnemonic.
%
%  The format of the MagickOptionToMnemonic method is:
%
%      const char *MagickOptionToMnemonic(const MagickOption value,
%        const long type)
%
%  A description of each parameter follows:
%
%    o values: The values.
%
%    o type: one or more values separated by commas.
%
%
*/
MagickExport const char *MagickOptionToMnemonic(const MagickOption value,
  const long type)
{
  const OptionInfo
    *option_info;

  register long
    i;

  option_info=GetOptionInfo(value);
  if (option_info == (const OptionInfo *) NULL)
    return((const char *) NULL);
  for (i=0; option_info[i].mnemonic != (const char *) NULL; i++)
    if (type == option_info[i].type)
      break;
  return(option_info[i].mnemonic);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e C h a n n e l O p t i o n                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseChannelOption() parses a string and returns an enumerated channel
%  type(s).
%
%  The format of the ParseChannelOption method is:
%
%      long ParseChannelOption(const char *channels)
%
%  A description of each parameter follows:
%
%    o options: One or more values separated by commas.
%
%
*/
MagickExport long ParseChannelOption(const char *channels)
{
  long
    channel;

  register long
    i;

  channel=ParseMagickOption(MagickChannelOptions,MagickTrue,channels);
  if (channel >= 0)
    return(channel);
  channel=0;
  for (i=0; i < (long) strlen(channels); i++)
  {
    switch (channels[i])
    {
      case 'A':
      case 'a':
      {
        channel|=OpacityChannel;
        break;
      }
      case 'B':
      case 'b':
      {
        channel|=BlueChannel;
        break;
      }
      case 'C':
      case 'c':
      {
        channel|=CyanChannel;
        break;
      }
      case 'g':
      case 'G':
      {
        channel|=GreenChannel;
        break;
      }
      case 'I':
      case 'i':
      {
        channel|=IndexChannel;
        break;
      }
      case 'K':
      case 'k':
      {
        channel|=BlackChannel;
        break;
      }
      case 'M':
      case 'm':
      {
        channel|=MagentaChannel;
        break;
      }
      case 'o':
      case 'O':
      {
        channel|=OpacityChannel;
        break;
      }
      case 'R':
      case 'r':
      {
        channel|=RedChannel;
        break;
      }
      case 'Y':
      case 'y':
      {
        channel|=YellowChannel;
        break;
      }
      default:
        return(-1);
    }
  }
  return(channel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e M a g i c k O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseMagickOption() parses a string and returns an enumerated option type(s).
%
%  The format of the ParseMagickOption method is:
%
%      long ParseMagickOption(const MagickOption option,
%        const MagickBooleanType list,const char *options)
%
%  A description of each parameter follows:
%
%    o option: The option.
%
%    o list: A option other than zero permits more than one option separated by
%      commas.
%
%    o options: One or more options separated by commas.
%
%
*/
MagickExport long ParseMagickOption(const MagickOption option,
  const MagickBooleanType list,const char *options)
{
  char
    *q,
    token[MaxTextExtent];

  const OptionInfo
    *option_info;

  long
    option_types;

  MagickBooleanType
    negate;

  register char
    *p;

  register long
    i;

  option_info=GetOptionInfo(option);
  if (option_info == (const OptionInfo *) NULL)
    return(-1);
  option_types=0;
  for (q=(char *) options; *q != '\0'; )
  {
    GetMagickToken(q,&q,token);
    if ((*token == '-') || (*token == '+'))
      GetMagickToken(q,&q,token+1);
    negate=(*token == '!') ? MagickTrue : MagickFalse;
    if (negate != MagickFalse)
      GetMagickToken(q,&q,token);
    p=strchr(token+1,'-');
    if (p != (char *) NULL)
      (void) CopyMagickString(p,p+1,MaxTextExtent);
    p=strchr(token+1,'_');
    if (p != (char *) NULL)
      (void) CopyMagickString(p,p+1,MaxTextExtent);
    for (i=0; option_info[i].mnemonic != (char *) NULL; i++)
      if (LocaleCompare(token,option_info[i].mnemonic) == 0)
        {
          if (*token == '!')
            option_types=option_types &~ option_info[i].type;
          else
            option_types=option_types | option_info[i].type;
          break;
        }
    if (option_info[i].mnemonic == (char *) NULL)
      return(-1);
    if (list == MagickFalse)
      break;
    GetMagickToken(q,&q,token);
    if (*token != ',')
      break;
  }
  return(option_types);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e m o v e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RemoveImageOption() removes an option from the image map.
%
%  The format of the RemoveImageOption method is:
%
%      MagickBooleanType RemoveImageOption(ImageInfo *image_info,
%        const char *option)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o option: The image option.
%
*/
MagickExport MagickBooleanType RemoveImageOption(ImageInfo *image_info,
  const char *option)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return(MagickFalse);
  return(RemoveNodeFromSplayTree((SplayTreeInfo *) image_info->options,option));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e O p t i o n I t e r a t o r                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImageOptionIterator() resets the image_info values iterator.  Use it
%  in conjunction with GetNextImageOption() to iterate over all the values
%  associated with an image option.
%
%  The format of the ResetImageOptionIterator method is:
%
%      ResetImageOptionIterator(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
*/
MagickExport void ResetImageOptionIterator(const ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    return;
  ResetSplayTreeIterator((SplayTreeInfo *) image_info->options);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e O p t i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageOption() associates an value with an image option.
%
%  The format of the SetImageOption method is:
%
%      MagickBooleanType SetImageOption(ImageInfo *image_info,
%        const char *option,const char *value)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o option: The image option.
%
%    o values: The image option values.
%
*/
MagickExport MagickBooleanType SetImageOption(ImageInfo *image_info,
  const char *option,const char *value)
{
  MagickBooleanType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->options == (void *) NULL)
    image_info->options=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,RelinquishMagickMemory);
  status=AddValueToSplayTree((SplayTreeInfo *) image_info->options,
    ConstantString(AcquireString(option)),ConstantString(AcquireString(value)));
  return(status);
}
