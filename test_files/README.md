# Normal Map test files

Execute **run_test_suite.bat** on Windows to analyze and convert/clean-up the texture test files in this directory and subdirectories. Running this test file will create various separate output directories where the results are put.

## Directories
* Standard - standard RGB -> XYZ, where Z is -1 to 1 range
* XYtextures - RG -> XY, and Z is something else (like ambient occlusion, e.g., LabPBR)
* ZZero - RGB -> XYZ, but Z goes from 0 to 1 instead of -1 to 1
* Heightfields - heightfield test images
* NoneOfTheAbove - other test images of various sorts

Each directory's contents:

### Standard

**lava_flow_n.png** - a file with almost no bumps, all nearly the same value. File is from the [jg-rtx](https://github.com/jasonjgardner/jg-rtx) resource pack for Minecraft, license [CC-NC-BY-SA](https://github.com/jasonjgardner/jg-rtx/blob/main/LICENSE).

![lava_flow_n.png](./Standard/lava_flow_n.png "lava_flow_n.png")

**ntp_heightfield_gimp.png** - conversion of the **ntp_heightfield.png** heightfield to a normals texture with [GIMP](https://www.gimp.org/) using [these instructions](https://docs.gimp.org/en/gimp-filter-normal-map.html).

![ntp_heightfield_gimp.png](./Standard/ntp_heightfield_gimp.png "ntp_heightfield_gimp.png")

**ntp_heightfield_gimp_5scale.png** - conversion of the **ntp_heightfield.png** heightfield to a normals texture with [GIMP](https://www.gimp.org/), changing the scale to 5, using [these instructions](https://docs.gimp.org/en/gimp-filter-normal-map.html).

![ntp_heightfield_gimp_5scale.png](./Standard/ntp_heightfield_gimp_5scale.png "ntp_heightfield_gimp_5scale.png")

**ntp_heightfield_normalmap_online.png** - conversion of the **ntp_heightfield.png** heightfield to a normals texture with [Normalmap Online](https://cpetry.github.io/NormalMap-Online/), setting "Z-Range -1 to +1".

![ntp_heightfield_normalmap_online.png](./Standard/ntp_heightfield_normalmap_online.png "ntp_heightfield_normalmap_online.png")

**ntp_resize_128.png** - the result file **ntp_heightfield_normalmap_online.png** resized to 128x128 using IrfanView. Doing a naive resize causes normals to no longer be normalized, see results in directory \_output_Heatmap.

![ntp_resize_128.png](./Standard/ntp_resize_128.png "ntp_resize_128.png")

**ntp_resize_512.png** - the result file **ntp_heightfield_normalmap_online.png** resized to 512x512 using IrfanView. Doing a naive resize causes normals to no longer be normalized, see results in directory \_output_Heatmap.

![ntp_resize_512.png](./Standard/ntp_resize_512.png "ntp_resize_512.png")

**r_normal_map.png** - letter R squiggles (see Heightfields directory) in standard form. Not quite correct, making it a good test. From the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

![r_normal_map.png](./Standard/r_normal_map.png "r_normal_map.png")

**r_normal_map_reversed_y.png** - letter R in standard form, DirectX orientation. Not quite correct, making it a good test. From the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

![r_normal_map_reversed_y.png](./Standard/r_normal_map_reversed_y.png "r_normal_map_reversed_y.png")

**squiggles_gimp.png** - squiggles (see Heightfields directory) converted with [GIMP](https://www.gimp.org/) using [these instructions](https://docs.gimp.org/en/gimp-filter-normal-map.html). GIMP evidently uses the DirectX style for output by default, though has a "Flip Y" and other options to adjust the style.

![squiggles_gimp.png](./Standard/squiggles_gimp.png "squiggles_gimp.png")

**squiggles_normalmap_online_zneg.png** - converted using [NormalMap Online](https://cpetry.github.io/NormalMap-Online/), with the "Z Range -1 to +1" box checked.

![squiggles_normalmap_online_zneg.png](./Standard/squiggles_normalmap_online_zneg.png "squiggles_normalmap_online_zneg.png")

**squiggles_ntp_quick.png** - converted by an earlier version of the NormalTextureProcessor.exe's heightfield converter, but without adjusting for roundtripping. Some pixels have one-level differences with **squiggles_ntp_roundtrip.png**. Use a "diff" program such as Beyond Compare to see the differences between the two. Note if you read this file in and convert it, the result should be the same as **squiggles_ntp_roundtrip.png**.

![squiggles_ntp_quick.png](./Standard/squiggles_ntp_quick.png "squiggles_ntp_quick.png")

**squiggles_ntp_roundtrip.png** - converted by NormalTextureProcessor.exe's heightfield converter using roundtripping. If you try to clean this file, the output file should be identical, since all the texels are perfectly roundtrippable (see the **Algorithms** section on the main page).

![squiggles_ntp_roundtrip.png](./Standard/squiggles_ntp_roundtrip.png "squiggles_ntp_roundtrip.png")

**targa_r_normal_map.tga** - (no image shown) same as r_normal_map.png**, the letter R in standard form. Test that Targa input works. Converted from the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

### XYtextures

**acacia_door_bottom_n.png** - the blue channel is ambient occlusion, [LabPBR format](https://shaderlabs.org/wiki/LabPBR_Material_Standard). File is from the [jg-rtx](https://github.com/jasonjgardner/jg-rtx) resource pack for Minecraft, license [CC-NC-BY-SA](https://github.com/jasonjgardner/jg-rtx/blob/main/LICENSE).

![acacia_door_bottom_n.png](./XYtextures/acacia_door_bottom_n.png "acacia_door_bottom_n.png")

### ZZero

**r_normal_map_reversed_x_0_bias_z.png** - letter R in Z-Zero form, and the X axis is reversed (non-standard, just meant as a test). Not quite correct, making it a good test. From the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

![r_normal_map_reversed_x_0_bias_z.png](./ZZero/r_normal_map_reversed_x_0_bias_z.png "r_normal_map_reversed_x_0_bias_z.png")

**squiggles_normalmap_online_zzero.png** - converted using [NormalMap Online](https://cpetry.github.io/NormalMap-Online/).

![squiggles_normalmap_online_zzero.png](./ZZero/squiggles_normalmap_online_zzero.png "squiggles_normalmap_online_zzero.png")

### Heightfields

**grayscale_wood_floor.png** - grayscale wood floor sample, non-tiling.

![grayscale_wood_floor.png](./Heightfields/grayscale_wood_floor.png "grayscale_wood_floor.png")

**ntp_heightfield.png** - text and brushes heightfield, giving a wide variety of slopes 

![ntp_heightfield.png](./Heightfields/ntp_heightfield.png "ntp_heightfield.png")

**r_bump_map.png** - grayscale letter R. From the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

![r_bump_map.png](./Heightfields/r_bump_map.png "r_bump_map.png")

**squiggles.png** - brushstrokes of similar slopes. Note there are strokes that touch the top edge, causing conversion glitches due to wrap-around; using '-hborder' would be a more sensible conversion, which is done as a separate test.

![squiggles.png](./Heightfields/squiggles.png "squiggles.png")

### NoneOfTheAbove

**black.tga** - (no image shown) all black, as a test. Non-power-of-two non-square size.

**cyan.png** - solid cyan, as a test.

![cyan.png](./NoneOfTheAbove/cyan.png "cyan.png")

**fade.png** - fade from blue to green to red, as a test.

![fade.png](./NoneOfTheAbove/fade.png "cyan.png")

**gray.png** - a neutral gray, as a test. Non-power-of-two non-square size.

![gray.png](./NoneOfTheAbove/gray.png "gray.png")

**white.tga** - (no image shown) all white, as a test.

**wood_floor.png** - colored wood floor sample, non-tiling. Currently this file gets misinterpreted as being an XY-only texture, because the Z's are clearly not used properly and the X and Y values, due to the coloration, mostly form vectors that have a length less than 1.0.

![wood_floor.png](./NoneOfTheAbove/wood_floor.png "wood_floor.png")

**yellow.png** - solid yellow, as a test.

![yellow.png](./NoneOfTheAbove/yellow.png "yellow.png")

## License

For images, the license is [CC-NC-BY-SA](https://github.com/erich666/NormalTextureProcessor/test_files/LICENSE) and created by Eric Haines, unless noted otherwise above.