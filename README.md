# NormalTextureProcessor

## Description
This C++ project examines normal textures (i.e., textures for applying bumps to surfaces), checking them for validity, and cleaning them up or converting them as desired.

## Installation
Develop on Windows 10 on Visual Studio 2022 (double-click NormalTextureProcessor.sln, build Release version). You might be able to compile and run it elsewhere, as the program is purely command-line driven, no graphical user interface.

## Usage
This system has a few major functions: analyze normal textures for what type and how good a normal texture they are, clean up textures that have problems, and convert from one normal texture type to another. Currently the system is limited to reading in 8-bit PNG and TGA (Targa) textures. I may add 16-bit PNG support at some point.

To list out all the command-line options, simply don't put any:
```sh
NormalTextureProcessor.exe
```

### Test Suite

After building the Release version, you should be able to go into the **test_files** directory and double-click on **run_test_suite.bat**. As the README in that directory notes, running this test file will create various separate output directories where the results are put. You can look at **run_test_suite.bat** to see various options in use and look at the resulting files.

### Analysis

This list will be confusing, so here are some typical combinations. To analyze all PNG and TGA textures in the current directory:
```sh
NormalTextureProcessor.exe -a -v
```
The '-v' is optional. It means "verbose", giving you additional information during processing.

A report is generated. At the end of the report the various files are output in separate categories, such as "pristine", "nearly right", etc. These lists can be used to copy and paste the list of files into the program itself, e.g.:
```sh
NormalTextureProcessor.exe -a beehive_normal.png cakestand_n.png
```
By putting specific files on the command line, the program processes only them. This example doesn't do much, just analyzes the (already analyzed) set of files again. The point is that you can specify individual files for other operations, such as cleanup and conversion.

You can also feed in an input directory:
```sh
NormalTextureProcessor.exe -a -idir flora/materials/normal_textures
```

There are three main types of normal textures this program recognizes. They are:
* RGB "standard" normal textures: each color channel is interpreted as representing numbers that go from -1.0 to 1.0, for all channels.
* RGB normal, "Z-zero": as above, but the blue, Z, channel is instead interpreted as going from 0.0 to 1.0.
* heightfield texture: an elevation texture, usually a grayscale image, typically with black representing the lowest elevation and white the highest.

The first type, where Z goes from -1 to 1, is the type that is the most popular nowadays, from what I've seen. Usually this texture is applied to a surface, so that the normals on the texture are relative to the interpolated (shading) normal of the surface itself. So, for example, a curved surface made of polygons is rendered. At a given surface location (typically) the shading normal is interpolated from vertex normals, defining the normal (Z) direction of the surface, along with X and Y basis vectors. This is known as ["tangent space"](https://80.lv/articles/tutorial-types-of-normal-maps-common-problems/). The normal is retrieved from the normal texture and modifies the surface normal.

This first type has a few variants to it. One is that Y direction on the texture can be up or down. That is, the normal texture can be thought of as "X to the right, Y up" for how its normal is interpreted. A normal of, say, (0.2, 0.3, 0.933) can be thought of as pointing 0.2 to the right, 0.3 up. That is the OpenGL interpretation. The normal's Y direction could also be considered as pointing down, i.e., think of the upper left corner as the origin and Y pointing down. This is the DirectX interpretation. See [this page](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) for more information and examples, and search [this page](https://80.lv/articles/tutorial-types-of-normal-maps-common-problems/) for "invert" for a wider view. OpenGL and DirectX are two major different forms you'll run across, i.e., no one reverses the X direction normally. If you use the wrong type of texture in a renderer you will see artifacts such as objects looking like they're being lit from below, for example. The glTF file format requires the OpenGL style normal texture, USD assumes OpenGL but can use scale and bias settings to support DirectX. Currently the analysis program does not have code to tell OpenGL-style files from DirectX-style files. I have some ideas how to guess using statistics as to which format a normal texture is, but haven't examined the problem yet.

For this first type, all (8-bit) values map from value 0 through 255 to the range -1.0 to 1.0. However, if you think about putting normals on a surface, a value where Z is less than 0.0 makes little sense. It would define a normal pointing into the surface, but the surface itself is at best thought of as a displaced heightfield (faked by using a normal texture instead), so there should not be any overhangs or other geometry where the Z value could go negative. One of the checks the analysis mode "-a" does is checks if the Z value is ever lower than 0.0. If it does, this is flagged as a problem.

The second type of normal texture, what I call "Z-zero", maps the 0 to 255 8-bit blue channel value to the range 0.0 to 1.0, instead of -1.0 to 1.0. There then can be no negative values for Z, and the precision of the Z value can be one bit higher. This all sounds good, but this format is (in my experience) now rarely seen. If anyone understands this evolutionary step, let me know. The reality is that this format is not supported in glTF, though can be supported in USD by using bias and scale values. Again, see [this page](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) for examples and more information. It is difficult to simply look at a "Z-zero" texture and tell it apart from a normal texture of the (now-popular) first type. By analyzing the Z values of the input texture and seeing whether they are negative and, more importantly, whether the (X,Y,Z) triplet formed has a vector length of nearly 1.0, we can usually tell these two types apart. That is, the program tries both conversions, standard and Z-zero, and sees which of the two normals formed is approximately 1.0 in length. This determination is the major reason this program exists, as sometimes incorrect textures slip through into test suites.

Another problem sometimes seen with both types of normal texture is that just the X and Y values (ignoring Z) form a vector that is longer than 1.0. This can happen due to (poor) resizing or filtering of normal textures, slightly incorrect conversion formulas, or who knows what else. The analysis will note how many (X,Y) vectors are too long.

The first two types, "standard" and "Z-zero", map red and green to X and Y in the same way, with the possible reversal of Y for the DirectX style texture. Given that Z cannot be a negative value (for most applications), some applications forego saving a Z value channel at all. The Z value is simply derived by sqrt(1.0 - (X\*X + Y\*Y)). For example, the [LabPBR Material](https://shaderlabs.org/wiki/LabPBR_Material_Standard) used for Minecraft modding specifies the texture holding normals as X and Y in red and green (in DirectX format), ambient occlusion is stored in the blue channel, and alpha holds the heightfield texture. The analyze mode attempts to identify these types of textures by looking at the Z values and see if they act as normals or are independent values. These are called "XY-only" textures in the analysis report.

The third type of normal texture is a heightfield texture, often called a bump or height map. The format is usually a single channel (such as the red channel) or a grayscale image. Black is usually the lowest level, white the highest elevation. Analysis tries some basic testing to see if a texture may be a heightfield texture. Heightfield textures are often used to create normal textures, e.g., [GIMP has this functionality](https://docs.gimp.org/en/gimp-filter-normal-map.html). There is a scale (not stored in the image) that needs to be provided for the heightfield so that the conversion can be done. I have occasionally seen heightmap textures get intermingled with normal textures, so try to identify these.

### Cleanup and Conversion

In addition to analysis, this program cleans up and convert to different formats. Specify an output directory with '-odir _out_directory_name_'. The output file name will match the input file name, though always uses PNG for output. Be careful: if the input and output directories are the same or both are not specified (and so match), texture files will be written over in place.

Typical operations include:
* Clean up the normals in a texture. Use option '-oclean' to output only those files that need cleanup, '-oall' to output all files. Given a particular type of RGB texture, the program makes sure the normals are all properly normalized and that no Z values are negative.
  * If you want to maintain the sign of the Z value, use the '-allownegz' option.
* Convert between formats.
  * OpenGL to DirectX, standard to Z-zero, or vice versa. OpenGl-style is the default, use '-idx' to note that the input files are DirectX-style (does not affect heightfields), '-odx' to set that you want to output in DirectX-style.
  * To export to the Z-Zero format, where Z goes from 0 to 1 instead of -1 to 1, use '-ozzero'.
* Heightfields can be converted to RGB textures of any type, using the options above.
  * To force all input files to be treated as heightfields, use '-ihf'.
  * To specify how the input heightfield data is scaled, use '-hfs #', where you give a scale factor. The value is 5.0 by default.
  * By default heightfields are considered to repeat, meaning that along the edges heights "off the texture" are taken from the opposite edge. TBD: do the borders wrap or are they "doubled" or something else, such as maintaining slope. '-hborder'

## Algorithms

The key thing is to get the conversion right. Here's how to go from an 8-bit color channel value "rgb" to the floating point range -1.0 to 1.0 for "xyz":
```sh
x = ((float)r / 255.0f) * 2.0f - 1.0f;
y = ((float)g / 255.0f) * 2.0f - 1.0f;
z = ((float)b / 255.0f) * 2.0f - 1.0f;
```
In [UsdPreviewSurface](https://openusd.org/release/spec_usdpreviewsurface.html#texture-reader) terms, the 2.0f is the scale, the -1.0f is the bias.

If you want to go to the range 0.0 to 1.0, it's simpler still. Here's Z:
```sh
z = (float)(b) / 255.0f
```

If you want to compute Z from X and Y, it's:
```sh
z = sqrt(1.0f - (x * x + y * y));
```

Getting the rounding right for the return trip is important. Here's that:
```sh
r = (unsigned char)(((x + 1.0f)/2.0f)*255.0f + 0.5f);
g = (unsigned char)(((y + 1.0f)/2.0f)*255.0f + 0.5f);
b = (unsigned char)(((z + 1.0f)/2.0f)*255.0f + 0.5f);
```

See the code in the SCRATCHPAD section of NormalTextureProcessor.cpp for a roundtrip test and other procedures. Basically, rgb -> xyz -> rgb should give the same rgb's (this code does).

## Known Limitations

If you have two files with the same name, e.g., wood.tga and wood.png, both will be read in but only one output wood.png file (arbitrarily from one of the two input files) will be produced.

## Resources

Some resources I've found useful:
* [Normalmap Online](https://cpetry.github.io/NormalMap-Online/) - a web-based normal map creation tool, it takes heightfields and converts to normal textures. By default, Z is mapped from 0 to 1 (old style), but there is a "Z Range" option (that I helped add) that lets you choose -1 to 1. A bit confusing, but what you do is click on the "wavy rings" heightmap image itself. A file dialog comes up and you load your own image. You can then see the RGB texture formed, and the texture applied to a cube in the right. Mouse-drag to change the view of the cube. Lots of options, including true displacement, so you can see the difference between that and just normal textures.
* [USD Normals Texture Bias And Scale](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) - a set of test normal textures of different types in a USD file, along with copious documentation of how each cube is formed. I made this test.
* [UsdPreviewSurface](https://openusd.org/release/spec_usdpreviewsurface.html#texture-reader) - the USD specification's basic material. Search on "normal3f" to see more about how to specify it.
* [glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) - has much about normal textures.
* [GIMP conversion](https://docs.gimp.org/en/gimp-filter-normal-map.html) - how to make normal textures from heightfields in GIMP.

## License

MIT license. See the [LICENSE](https://github.com/erich666/NormalTextureProcessor/blob/main/LICENSE) file in this directory.

## Roadmap

Tasks:

- [ ] Figure out way (if any) to tell if a texture is OpenGL or DirectX oriented. I suspect there's some curvature analysis, perhaps converting into a heightfield, that could be done. For example, take differences between texels horizontally and vertically and see the correspondence - bumps are more likely than hyperboloids. Or maybe it's not possible (well, it certainly isn't if there's no Y variance).
- [ ] Convert from normal textures to heightfields. Not sure this is useful (nor how to do it, exactly), but might be worth adding, for visualization and analysis.
- [ ] Ignore pixels with alpha values of 0.
- [ ] Support 16-bit PNG files, input and output.