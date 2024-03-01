# Normal Map test files

Execute **run_test_suite.bat** on Windows to analyze and convert/clean-up the texture test files in this directory and subdirectories. Running this test file will create various separate output directories where the results are put.

## Directories
* Heightfields - heightfield test images
* Standard - standard RGB -> XYZ, where Z is -1 to 1 range
* XYtextures - RG -> XY, and Z is something else (like ambient occlusion, e.g., LabPBR)
* ZZero - RGB -> XYZ, but Z goes from 0 to 1 instead of -1 to 1

Each directory's contents:

### Heightfields

black.tga - all black, as a test. Non-power-of-two non-square size.

cyan.png - solid cyan, as a test.

gray.png - a neutral gray, as a test. Non-power-of-two non-square size.

r_bump_map.png - grayscale letter R. From the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

white.tga - all white, as a test.

wood_floor.png - colored wood floor sample, non-tiling. 

### Standard

lava_flow_n.png - a file with no bumps, all the same value. File is from the [jg-rtx](https://github.com/jasonjgardner/jg-rtx) resource pack for Minecraft, license [CC-NC-BY-SA](https://github.com/jasonjgardner/jg-rtx/blob/main/LICENSE).

r_normal_map.png - letter R in standard form. Not quite correct, making it a good test. From the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

r_normal_map_reversed_y.png - letter R in standard form, DirectX orientation. Not quite correct, making it a good test. From the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

targa_r_normal_map.tga - same as r_normal_map.png, the letter R in standard form. Test that Targa input works. Converted from the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

### XYtextures

acacia_door_bottom_n.png - the blue channel is ambient occlusion, [LabPBR format](https://shaderlabs.org/wiki/LabPBR_Material_Standard). File is from the [jg-rtx](https://github.com/jasonjgardner/jg-rtx) resource pack for Minecraft, license [CC-NC-BY-SA](https://github.com/jasonjgardner/jg-rtx/blob/main/LICENSE).

### ZZero

r_normal_map_reversed_x_0_bias_z.png - letter R in Z-Zero form, and the X axis is reversed (non-standard, just meant as a test). Not quite correct, making it a good test. From the [USD Assets Working Group](https://github.com/usd-wg/assets/tree/main/test_assets/NormalsTextureBiasAndScale) Normals Texture Bias And Scale test, license CC-NC-BY-SA.

## License

For images, license is [CC-NC-BY-SA](https://github.com/erich666/NormalTextureProcessor/test_files/LICENSE) unless noted otherwise above.