@rem run_test_suite.bat - run to test the NormalTextureProcessor. Assumes the release version of the program has been compiled and is in the normal place.
set NTP_EXE="..\x64\Release\NormalTextureProcessor.exe"

@rem run everything, as a start
@rem we could force everything to be considered a heightfield for this next test by using -ihf, but it turns out these all pass anyway
%NTP_EXE% -idir Standard -odir _output_Standard -oall -a -v
%NTP_EXE% -idir ZZero -odir _output_ZZero -oall -a -v
%NTP_EXE% -idir XYtextures -odir _output_XYtextures -oall -a -v
%NTP_EXE% -idir Heightfields -odir _output_Heightfields -oall -a -v
@rem how are various non-normal, non-bump files interpreted?
%NTP_EXE% -idir NoneOfTheAbove -odir _output_NoneOfTheAbove -oall -a -v

@rem Other tests follow, mostly unit tests to exercise and demonstrate the various options

@rem the 0 to 1 file produced by NormalMap Online is so funky we have to force it to be read in as Z-zero, using -izzero. Also output as -ozzero, making this a cleanup sort of pass
%NTP_EXE% -idir ZZero -odir _output_ZZero_stay_zzero -oclean -a -v -izzero -ozzero squiggles_normalmap_online_zzero.png

@rem another way is to set the tolerance percentage higher, '-etol 8', which increases the tolerance for bad normals (and other bad data), so letting this one be classified as Z-zero
%NTP_EXE% -idir ZZero -odir _output_ZZero_find_zzero -oclean -a -v -etol 8 squiggles_normalmap_online_zzero.png

@rem make the wood bumpmap have more extreme bumps and have borders instead of being tiling (repeating)
%NTP_EXE% -idir Heightfields -odir _output_Heightfields_scale -oall -a -v -hfs 15 grayscale_wood_floor.png -hborder

@rem convert the squiggles heightfield to have a border, which makes more sense since it's not meant to be tiling
%NTP_EXE% -idir Heightfields -odir _output_Heightfields_border -oall -a -v squiggles.png -hborder

@rem force colored wood floor to be considered a heightfield
%NTP_EXE% -idir NoneOfTheAbove -odir _output_NoneOfTheAbove_force -oall -a -v -ihf wood_floor.png

@rem GIMP gives DirectX-style by default. Convert to OpenGL-style, and force to be standard Z, just to test the -izneg flag
%NTP_EXE% -idir Standard -odir _output_Standard_OpenGL -oall -a -v -izneg -idx squiggles_gimp.png

@rem convert the r_normal_map.png OpenGL-style file to DirectX style. Note there is no '-iogl' input option needed, since OpenGL format is the default.
%NTP_EXE% -idir Standard -odir _output_Standard_DirectX -oall -a -v -izneg -odx r_normal_map.png

@rem export to CSV, showing only errors
%NTP_EXE% -idir Standard -odir _output_CSV -a -v squiggles_normalmap_online_zneg.png -csve -izneg
%NTP_EXE% -idir Standard -odir _output_CSV -a -v ntp_heightfield_normalmap_online.png -csve -izneg

@rem export to CSV, dumping all pixels
%NTP_EXE% -idir NoneOfTheAbove -odir _output_CSV -a -v fade.png -csv -ixy

@rem make heatmaps, tell program that files are of a given format
%NTP_EXE% -idir Standard -odir _output_Heatmap -oall -izneg -a -v -oheatmap
%NTP_EXE% -idir XYtextures -odir _output_Heatmap -oall -ixy -a -v -oheatmap
%NTP_EXE% -idir ZZero -odir _output_Heatmap -oall -izzero -a -v -oheatmap

@rem TODO: Flag currently not tested here, due to a lack of bad test data: -allownegz -etolxy
