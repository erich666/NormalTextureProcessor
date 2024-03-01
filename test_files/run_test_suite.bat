@rem run_test_suite.bat - run to test the NormalTextureProcessor. Assumes the release version of the program has been compiled and is in the normal place.
set NTP_EXE="..\x64\Release\NormalTextureProcessor.exe"

@rem run everything, as a start
@rem we could force everything to be considered a heightfield for this next test by using -ihf, but it turns out these all pass anyway
%NTP_EXE% -idir Heightfields -odir output_Heightfields -oall -a -v
%NTP_EXE% -idir Standard -odir output_Standard -oall -a -v
@rem note we force reading in as an XY texture here with -ixy, as some files have garbage data for some texels, throwing off the analysis
%NTP_EXE% -idir XYtextures -odir output_XYtextures -oall -a -v -ixy
%NTP_EXE% -idir ZZero -odir output_ZZero -oall -a -v -izzero

@rem other tests
@rem maintain Z-zero format on output
%NTP_EXE% -idir ZZero -odir output_ZZero_stay_zzero -oall -a -v -izzero -ozzero

@rem TODO: test in place (replacing existing file with itself), test -idx -ihs and other options, test being in a directory and not specifying a directory