// NormalTextureProcessor.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Program to read in normal textures and analyze or clean up/convert them to other normal texture forms.

#include <iostream>
#include <string>
#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "readtga.h"

#define	VERSION_STRING	L"1.00"

static int gErrorCount = 0;
static int gWarningCount = 0;

static wchar_t gErrorString[1000];
// 1000 errors of 100 characters each - sounds sufficient. Sloppy, I know.
#define CONCAT_ERROR_LENGTH	(1000*100)
static wchar_t gConcatErrorString[CONCAT_ERROR_LENGTH];

#define UNKNOWN_FILE_EXTENSION 0 
#define	PNG_EXTENSION_FOUND 1
#define	TGA_EXTENSION_FOUND	2
#define JPG_EXTENSION_FOUND 3
#define BMP_EXTENSION_FOUND	4

#define IMAGE_BUMP_TYPE			0x3
#define IMAGE_IS_NORMAL_MAP		1
#define IMAGE_IS_HEIGHT_MAP		2
#define IMAGE_IS_NOT_BUMP_MAP	3
#define IMAGE_BUMP_REAL			0x4

#define	CHANNEL_RED		0
#define	CHANNEL_GREEN	1
#define	CHANNEL_BLUE	2

#define IMAGE_ALL_SAME						0x0001
#define IMAGE_R_CHANNEL_ALL_SAME			0x0002
#define IMAGE_G_CHANNEL_ALL_SAME			0x0004
#define IMAGE_B_CHANNEL_ALL_SAME			0x0008
#define IMAGE_GRAYSCALE						0x0010
#define IMAGE_VALID_NORMALS_FULL			0x0020
#define IMAGE_ALMOST_VALID_NORMALS_FULL		0x0040
#define IMAGE_ALMOST2_VALID_NORMALS_FULL	0x0080
#define IMAGE_VALID_ZVAL_NONNEG				0x0100
#define IMAGE_VALID_NORMALS_ZERO			0x0200
#define IMAGE_ALMOST_VALID_NORMALS_ZERO		0x0400
#define IMAGE_ALMOST2_VALID_NORMALS_ZERO	0x0800
#define	IMAGE_VALID_NORMALS_XY				0x1000

#define IMAGE_TYPE_NORMAL_FULL		1
#define IMAGE_TYPE_NORMAL_FULL_SAME	2
#define IMAGE_TYPE_NORMAL_ZERO		3
#define IMAGE_TYPE_NORMAL_XY_ONLY	4
#define IMAGE_TYPE_HEIGHTFIELD		5


// for command line parsing
#define INC_AND_TEST_ARG_INDEX( loc )	\
	argLoc++;				\
	if (argLoc == argc) {	\
		printHelp();		\
		return 1;			\
	}


#ifdef SHOW_PIXEL_VALUE
#define PRINT_PIXEL(s,t,ch,c,r) { unsigned char *p = &(t).image_data[(ch)*((t).width*(r)+(c))]; printf ("%s %d %d %d\n",(s),(int)p[0],(int)p[1],(int)p[2]);}
#else
#define PRINT_PIXEL(s,t,ch,c,r)
#endif

#define MAX_PATH_AND_FILE (2*MAX_PATH)

// Normal 0-255 RGB values to XYZ set of floats
#define CONVERT_RGB_TO_Z_FULL(r,g,b, x,y,z)	\
	x = ((float)(r) / 255.0f) * 2.0f - 1.0f;	\
	y = ((float)(g) / 255.0f) * 2.0f - 1.0f;	\
	z = ((float)(b) / 255.0f) * 2.0f - 1.0f;

// convert Z to 0 to 1 range instead of -1 to 1, the norm nowadays
#define CONVERT_RGB_TO_Z_ZERO(r,g,b, x,y,z)	\
	x = ((float)(r) / 255.0f) * 2.0f - 1.0f;	\
	y = ((float)(g) / 255.0f) * 2.0f - 1.0f;	\
	z = (float)(b) / 255.0f;

#define CONVERT_CHANNEL_TO_FULL(chan, val)	\
	val = ((float)(chan) / 255.0f) * 2.0f - 1.0f;

#define CONVERT_CHANNEL_TO_ZERO(chan, val)	\
	val = (float)(chan) / 255.0f;

#define CONVERT_Z_FULL_TO_RGB(x,y,z, r,g,b)	\
	r = (unsigned char)((((x) + 1.0f)/2.0f)*255.0f + 0.5f);	\
	g = (unsigned char)((((y) + 1.0f)/2.0f)*255.0f + 0.5f);	\
	b = (unsigned char)((((z) + 1.0f)/2.0f)*255.0f + 0.5f);

#define CONVERT_Z_ZERO_TO_RGB(x,y,z, r,g,b)	\
	r = (unsigned char)((((x) + 1.0f)/2.0f)*255.0f + 0.5f);	\
	g = (unsigned char)((((y) + 1.0f)/2.0f)*255.0f + 0.5f);	\
	b = (unsigned char)((z)*255.0f + 0.5f);

#define CONVERT_FULL_TO_CHANNEL(val, chan)	\
	chan = (unsigned char)((((val) + 1.0f)/2.0f)*255.0f + 0.5f);

#define CONVERT_ZERO_TO_CHANNEL(val, chan)	\
	chan = (unsigned char)((val)*255.0f + 0.5f);

#define COMPUTE_Z_FROM_XY(x,y, z)	\
	z = sqrt(1.0f - ((x) * (x) + (y) * (y)));

// maximum difference in length from 1.0 for normals when saving in [0,255] numbers,
// when Z value is computed from X and Y
#define	MAX_NORMAL_LENGTH_DIFFERENCE	0.00387f
#define	MAX_Z_DIFFERENCE				0.00392f

#define VECTOR_LENGTH(x,y,z)	\
	sqrt(x*x + y*y + z*z)

struct Options {
	bool analyze;
	std::wstring inputDirectory;
	bool inputOpenGL;
	bool inputDirectX;
	bool outputAll;
	bool outputClean;
	std::wstring outputDirectory;
	bool outputZzeroToOne;
	bool outputOpenGL;
	bool outputDirectX;
	float heightfieldScale;
	bool wrapHeightfield;
	bool verbose;
};

Options gOptions;

std::wstring gListUnknown;
std::wstring gListStandard;
std::wstring gListStandardDirty;
std::wstring gListStandardSame;
std::wstring gListZZero;
std::wstring gListZZeroDirty;
std::wstring gListXYonly;
std::wstring gListHeightfield;


//-------------------------------------------------------------------------
void printHelp();

static bool processImageFile(wchar_t* inputFile, int fileType);

static void reportReadError(int rc, const wchar_t* filename);
static void saveErrorForEnd();

void convertHeightfieldToXYZ(progimage_info* dst, progimage_info* src, float heightfieldScale, bool y_flip);
void cleanAndCopyNormalTexture(progimage_info* dst, progimage_info* src, bool must_clean, bool output_zzero, bool y_flip);

bool removeFileType(wchar_t* name);
int isImageFile(wchar_t* name);
bool dirExists(const wchar_t* path);
bool createDir(const wchar_t* path);

int wmain(int argc, wchar_t* argv[])
{
	// TODOTODO experiment: try all possible X and Y values in a quadrant, compute Z. How close is the triplet formed to 1.0 length?

	// Result: Average diff from length of 1.0f: 0.00129662, maximum length difference detected is 0.00386751 and
	//   maximum z difference detected is 0.00391376 for the 51040 valid pixel values
	// Fun fact: 51040 combinations out of 256*256 of red and green combinations are valid, else red and green's defined length is > 1.0.
	// So 77.88% of all r,g combinations [0..255],[0..255] are valid.
//#define SCRATCHPAD
#ifdef SCRATCHPAD
	// This is just a place to check on such things, such as valid r,g combinations and how much various values vary in floating point.
	int row, col;
	int r, g, b;
	float x, y, z, znew, len;
	float absdiffsum = 0.0f;
	float maxdiff = -1.0f;
	float zmaxdiff = -1.0f;
	int validcount = 0;
	for (row = 0; row < 256; row++) {
		for (col = 0; col < 256; col++) {
			// really just getting the x and y here:
			CONVERT_RGB_TO_Z_FULL(col, row, 255, x, y, z);

			// is X and Y valid, forming a vector <= 1.0 in length?
			if ((x * x + y * y) <= 1.0f) {
				// find the proper z value that normalizes this vector
				COMPUTE_Z_FROM_XY(x, y, z);
				validcount++;
				// convert back to rgb
				CONVERT_Z_FULL_TO_RGB(x, y, z, r, g, b);
				// and then to xyz again (really just need to convert Z in both cases)
				CONVERT_RGB_TO_Z_FULL(r, g, b, x, y, znew);
				// now, how far off from 1.0 is the xyz vector length?
				len = VECTOR_LENGTH(x, y, znew);
				float absdiff = abs(1.0f - len);
				absdiffsum += absdiff;
				if (absdiff > maxdiff) {
					maxdiff = absdiff;
				}
				float zabsdiff = abs(z - znew);
				if (zabsdiff > zmaxdiff) {
					zmaxdiff = zabsdiff;
				}
				// std::wcout << absdiff << "\n";
				// last little reality check: does it round-trip?
				unsigned char r1, g1, b1;
				CONVERT_Z_FULL_TO_RGB(x, y, z, r1, g1, b1);
				// this passes with flying colors
				assert(r == r1 && g == g1 && b == b1);
				if (r != r1 || g != g1 || b != b1) {
					std::cerr << "ERROR: something weird's going on, the round trip test failed.\n" << std::flush;
				}
			}
		}
	}
	std::wcout << "Average diff from length of 1.0f: " << absdiffsum / validcount << ", maximum length difference detected is " << maxdiff << " and maximum z difference detected is " << zmaxdiff << " for the " << validcount << " valid pixel values\n";
#endif

	gConcatErrorString[0] = 0;

	int argLoc = 1;

	gOptions.analyze = false;
	gOptions.inputOpenGL = false;
	gOptions.inputDirectX = false;
	gOptions.outputAll = false;
	gOptions.outputClean = false;
	gOptions.outputZzeroToOne = false;
	gOptions.outputOpenGL = false;
	gOptions.outputDirectX = false;
	gOptions.heightfieldScale = 0.5f;
	gOptions.wrapHeightfield = false;
	gOptions.verbose = false;

	std::vector<std::wstring> fileList;

	while (argLoc < argc)
	{
		if (wcscmp(argv[argLoc], L"-a") == 0)
		{
			gOptions.analyze = true;
		}
		else if (wcscmp(argv[argLoc], L"-idir") == 0)
		{
			// input directory
			INC_AND_TEST_ARG_INDEX(argLoc);
			gOptions.inputDirectory = argv[argLoc];
		}
		else if (wcscmp(argv[argLoc], L"-iogl") == 0)
		{
			gOptions.inputOpenGL = true;
		}
		else if (wcscmp(argv[argLoc], L"-idx") == 0)
		{
			gOptions.inputDirectX = true;
		}
		else if (wcscmp(argv[argLoc], L"-oall") == 0)
		{
			gOptions.outputAll = true;
		}
		else if (wcscmp(argv[argLoc], L"-oclean") == 0)
		{
			gOptions.outputClean = true;
		}
		else if (wcscmp(argv[argLoc], L"-odir") == 0)
		{
			// output directory
			INC_AND_TEST_ARG_INDEX(argLoc);
			gOptions.outputDirectory = argv[argLoc];
		}
		else if (wcscmp(argv[argLoc], L"-ozzero") == 0)
		{
			gOptions.outputZzeroToOne = true;
		}
		else if (wcscmp(argv[argLoc], L"-oogl") == 0)
		{
			gOptions.outputOpenGL = true;
		}
		else if (wcscmp(argv[argLoc], L"-odx") == 0)
		{
			gOptions.outputDirectX = true;
		}
		else if (wcscmp(argv[argLoc], L"-hfs") == 0)
		{
			// heightfield scale - 0.5 by default
			INC_AND_TEST_ARG_INDEX(argLoc);
			swscanf_s(argv[argLoc], L"%f", &gOptions.heightfieldScale);
		}
		else if (wcscmp(argv[argLoc], L"-hwrap") == 0)
		{
			// TODO; and should there be a "continue slope" option for the border, too?
			gOptions.wrapHeightfield = true;
		}
		else if (wcscmp(argv[argLoc], L"-v") == 0)
		{
			gOptions.verbose = true;
		}
		else
		{
			// assume it's a file name, but let's check first if the user thinks it's an option
			if (argv[argLoc][0] == '-') {
				// unknown option
				printHelp();
				return 1;
			}
			fileList.push_back(argv[argLoc]);
		}
		argLoc++;
	}

	if (gOptions.verbose) {
		std::wcout << "NormalTextureProcessor version " << VERSION_STRING << "/n" << std::flush;
	}

	// check validity of options only if we're actually outputing
	if (gOptions.outputAll || gOptions.outputClean) {
		if (gOptions.inputOpenGL && gOptions.inputDirectX) {
			std::wcerr << "ERROR: You can pick only one input type, either -iogl for OpenGL-style (Y up) or -idx for DirectX-style (Y down). Aborting.\n" << std::flush;
			printHelp();
			return 1;
		}
		if (gOptions.outputOpenGL && gOptions.outputDirectX) {
			std::wcerr << "ERROR: You can pick only one output type, either -oogl for OpenGL-style (Y up) or -odx for DirectX-style (Y down). Aborting.\n" << std::flush;
			printHelp();
			return 1;
		}
		if (!(gOptions.inputOpenGL || gOptions.inputDirectX) && (gOptions.outputOpenGL || gOptions.outputDirectX)) {
			std::wcerr << "ERROR: Since output style is specified, you must pick at least one input style, either -iogl for OpenGL-style (Y up) or -idx for DirectX-style (Y down). Aborting.\n" << std::flush;
			printHelp();
			return 1;
		}
		if ((gOptions.inputOpenGL || gOptions.inputDirectX) && !(gOptions.outputOpenGL || gOptions.outputDirectX)) {
			std::wcerr << "ERROR: Since input style is specified, you must pick at least one output style, either -oogl for OpenGL-style (Y up) or -odx for DirectX-style (Y down). Aborting.\n" << std::flush;
			printHelp();
			return 1;
		}
	}

	int pos;
	// Does input directory exist?
	if (gOptions.inputDirectory.size() > 0) {
		// not the current directory, so check
		if (!dirExists(gOptions.inputDirectory.c_str())) {
			std::wcerr << "ERROR: input directory " << gOptions.inputDirectory.c_str() << " does not exist.\n" << std::flush;
			printHelp();
			return 1;
		}
	}
	// Does output directory exist?
	if (gOptions.outputDirectory.size() > 0) {
		// not the current directory, so check
		if (!dirExists(gOptions.outputDirectory.c_str())) {
			// try to create output directory
			if (!createDir(gOptions.outputDirectory.c_str())) {
				std::wcerr << "ERROR: output directory " << gOptions.outputDirectory.c_str() << " does not exist and cannot be created.\n" << std::flush;
				printHelp();
				return 1;
			}
		}
		// add a "/" to the output directory path if not there; do it now
		pos = (int)gOptions.outputDirectory.size() - 1;
		// TODOTODO - test with "\" at end of input directory
		if (gOptions.outputDirectory[pos] != '/' && gOptions.outputDirectory[pos] != '\\') {
			gOptions.outputDirectory.push_back('/');
		}
	}

	// look through files and process each one.
	int filesFound = 0;
	int filesProcessed = 0;
	HANDLE hFind;
	WIN32_FIND_DATA ffd;

	wchar_t fileSearch[MAX_PATH];
	wcscpy_s(fileSearch, MAX_PATH, gOptions.inputDirectory.c_str());
	if (gOptions.inputDirectory.size() > 0) {
		// input directory exists, so make sure there's a "\" or "/" at the end of this path
		pos = (int)gOptions.inputDirectory.size() - 1;
		// TODOTODO - test with "\" at end of input directory
		if (gOptions.inputDirectory[pos] != '/' && gOptions.inputDirectory[pos] != '\\') {
			wcscat_s(fileSearch, MAX_PATH, L"/");
		}
	}
	// files to search on. We'll later filter by .png and .tga
	wcscat_s(fileSearch, MAX_PATH, L"*");
	hFind = FindFirstFile(fileSearch, &ffd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		// go through all the files in the blocks directory
		do {
			int fileType = isImageFile(ffd.cFileName);
			if (fileType != UNKNOWN_FILE_EXTENSION) {
				// Seems to be an image file. Can we process it?
				if (fileType == PNG_EXTENSION_FOUND || fileType == TGA_EXTENSION_FOUND) {
					filesFound++;
					filesProcessed += (processImageFile(ffd.cFileName, fileType) ? 1 : 0);
				}
				else {
					std::wcerr << "WARNING: file " << ffd.cFileName << " is not an image file type currently supported in this program. Convert to PNG or TGA to process it.\n" << std::flush;
				}
			}
		} while (FindNextFile(hFind, &ffd) != 0);

		FindClose(hFind);
	}

	if (gErrorCount) {
		std::wcerr << "\nERROR SUMMARY:\n" << gConcatErrorString << "\n";
		std::wcerr << "Error count: " << gErrorCount << " error";
		if (gErrorCount == 1) {
			std::wcerr << " was";
		}
		else {
			std::wcerr << "s were";
		}
		std::wcerr << " generated.\n" << std::flush;
	}

	if (gOptions.analyze) {
		// summary lists of different file types
		std::wcout << "==============================\nFile types found, by category:\n";
		bool output_any = false;
		if (gListStandardSame.size() > 0) {
			output_any = true;
			std::wcout << "  Standard normal textures that have no bumps, all texels are the same value:" << gListStandardSame << "\n";
		}
		if (gListStandard.size() > 0) {
			output_any = true;
			std::wcout << "  Standard normal textures:" << gListStandard << "\n";
			if (gListStandardDirty.size() > 0) {
				output_any = true;
				std::wcout << "    Standard normal textures that are not as expected:" << gListStandardDirty << "\n";
			}
		}
		if (gListZZero.size() > 0) {
			output_any = true;
			std::wcout << "  Z-Zero normal textures:" << gListZZero << "\n";
			if (gListZZeroDirty.size() > 0) {
				output_any = true;
				std::wcout << "    Z-Zero normal textures that are not as expected:" << gListZZeroDirty << "\n";
			}
		}
		if (gListXYonly.size() > 0) {
			output_any = true;
			std::wcout << "  XY-only normal textures:" << gListXYonly << "\n";
		}
		if (gListHeightfield.size() > 0) {
			output_any = true;
			std::wcout << "  Heightfield textures:" << gListHeightfield << "\n";
		}
		if (gListUnknown.size() > 0) {
			output_any = true;
			std::wcout << "  Unknown textures:" << gListUnknown << "\n";
		}
		if (!output_any) {
			std::wcout << "  (No PNG or TGA files found to analyze)\n";
		}
	}

	std::wcout << "\nNormalTextureProcessor summary: " << filesFound << " PNG/TGA files discovered and " << filesProcessed << " processed.\n" << std::flush;
	return 0;
}

// return true if successfully read in and processed in some way
static bool processImageFile(wchar_t* inputFile, int fileType)
{
	// read input file
	bool imageProcessed = true;
	progimage_info imageInfo;
	int rc = readImage(&imageInfo, inputFile, LCT_RGB, fileType);
	if (rc != 0)
	{
		// file not found
		reportReadError(rc, inputFile);
		return false;
	}

	// All the things to test:
	// Are all the pixels the same color? If so, no normals data, no matter what format.
	//   Is this same color the "blue" of 127/127/255? Note it's a "proper" normal map format, but no real data.
	// Is each separate channel entirely the same? If only the red channel has data (or green, or blue), then
	// that might be a bump map - assume so.
	// Are the pixel values a 0 to 1 Z or -1 to 1 Z? How close to each (how normalized are the directions)?
	// Are the r and g values combined form a vector > 1.0f in length without even including b's?

	int row, col, channel;
	int image_field_bits = 0x0;
	float x, y, z, zcalc;
	unsigned char* src_data = &imageInfo.image_data[0];
	// various counters
	int pixels_match = 0;
	int rgb_match[3];
	int grayscale = 0;
	int normalizable_xy = 0;
	int xy_outside_bounds = 0;
	int z_outside_bounds = 0;
	int z_outside_zero_bounds = 0;
	int normal_length_zneg[3];
	normal_length_zneg[0] = normal_length_zneg[1] = normal_length_zneg[2] = 0;
	int normal_zval_nonnegative = 0;
	int normal_length_zzero[3];
	normal_length_zzero[0] = normal_length_zzero[1] = normal_length_zzero[2] = 0;
	float min_z = 999.0f;
	int zmaxabsdiff = 0;
	int zmaxabsdiff_zero = 0;
	unsigned char first_pixel[3];
	for (channel = 0; channel < 3; channel++) {
		first_pixel[channel] = src_data[channel];
		rgb_match[channel] = 0;
	}
	int image_size = imageInfo.width * imageInfo.height;
	for (row = 0; row < imageInfo.height; row++)
	{
		for (col = 0; col < imageInfo.width; col++)
		{
			// does this pixel match the first pixel? (i.e. are all pixels the same?)
			if (src_data[0] == first_pixel[0] && src_data[1] == first_pixel[1] && src_data[2] == first_pixel[2]) {
				pixels_match++;
			}
			// does the channel match the first channel value?
			for (channel = 0; channel < 3; channel++) {
				if (src_data[channel] == first_pixel[channel]) {
					rgb_match[channel]++;
				}
			}
			// are all three channels the same? (grayscale)
			if (src_data[0] == src_data[1] && src_data[1] == src_data[2]) {
				grayscale++;
			}

			// Try conversion to XYZ's. Which if any make sense?
			CONVERT_RGB_TO_Z_FULL(src_data[0], src_data[1], src_data[2], x, y, z);

			// Check if the range of z values is non-negative. It may just be the normals are not normalized?
			if (z > -MAX_NORMAL_LENGTH_DIFFERENCE) {
				normal_zval_nonnegative++;
			}

			// lowest z value found (for -1 to 1 conversion)
			if (min_z > z) {
				min_z = z;
			}

			// find the proper z value
			float xy_len = (x * x + y * y);
			// is x,y "short enough" to properly represent a normalized vector?
			if (sqrt(xy_len) <= 1.0f + MAX_NORMAL_LENGTH_DIFFERENCE) {
				// it's short enough in X and Y.
				normalizable_xy++;

				// determine what z really should be for this x,y pair
				if (xy_len <= 1.0f) {
					COMPUTE_Z_FROM_XY(x, y, zcalc);
				}
				else {
					// x,y's length is a tiny bit above 1.0 in length, but still not unreasonable.
					// Set z to 0.0
					zcalc = 0.0f;
				}
				// is the z converted pixel level off by only one?
				unsigned char bcalc;
				CONVERT_FULL_TO_CHANNEL(zcalc, bcalc);
				int zabsdiff = (int)(abs(src_data[2] - bcalc));
				if (zabsdiff == 0) {
					normal_length_zneg[0]++;
				}
				if (zabsdiff == 1) {
					normal_length_zneg[1]++;
				}
				if (zabsdiff == 2) {
					normal_length_zneg[2]++;
				}
				if (zabsdiff > 2) {
					z_outside_bounds++;
				}
				if (zmaxabsdiff < zabsdiff) {
					zmaxabsdiff = zabsdiff;
				}

				// try same thing with Z 0-1 range
				CONVERT_ZERO_TO_CHANNEL(zcalc, bcalc);
				zabsdiff = (int)(abs(src_data[2] - bcalc));
				if (zabsdiff == 0) {
					normal_length_zzero[0]++;
				}
				if (zabsdiff == 1) {
					normal_length_zzero[1]++;
				}
				if (zabsdiff == 2) {
					normal_length_zzero[2]++;
				}
				if (zabsdiff > 2) {
					z_outside_zero_bounds++;
				}
				if (zmaxabsdiff_zero < zabsdiff) {
					zmaxabsdiff_zero = zabsdiff;
				}
			}
			else {
				xy_outside_bounds++;	// just a place to put a break and see what's what
			}

			// next texel
			src_data += 3;
		}
	}

	// At this point we can do some analysis and see what sort of normal map we have
	if (pixels_match == image_size) {
		image_field_bits |= IMAGE_ALL_SAME;
	}
	for (channel = 0; channel < 3; channel++) {
		if (rgb_match[channel] == image_size) {
			image_field_bits |= (IMAGE_R_CHANNEL_ALL_SAME << channel);
		}
	}
	if (grayscale == image_size) {
		image_field_bits |= IMAGE_GRAYSCALE;
	}
	if (normal_length_zneg[0] == image_size) {
		image_field_bits |= IMAGE_VALID_NORMALS_FULL;
		assert(zmaxabsdiff == 0);	// another way to test
	}
	if (normal_length_zneg[1] + normal_length_zneg[0] == image_size) {
		image_field_bits |= IMAGE_ALMOST_VALID_NORMALS_FULL;
	}
	if (normal_length_zneg[2] + normal_length_zneg[1] + normal_length_zneg[0] == image_size) {
		image_field_bits |= IMAGE_ALMOST2_VALID_NORMALS_FULL;
	}
	if (normal_zval_nonnegative == image_size) {
		image_field_bits |= IMAGE_VALID_ZVAL_NONNEG;
	}
	if (normal_length_zzero[0] == image_size) {
		image_field_bits |= IMAGE_VALID_NORMALS_ZERO;
		assert(zmaxabsdiff_zero == 0);	// another way to test
	}
	if (normal_length_zzero[1] + normal_length_zzero[0] == image_size) {
		image_field_bits |= IMAGE_ALMOST_VALID_NORMALS_ZERO;
	}
	if (normal_length_zzero[2] + normal_length_zzero[1] + normal_length_zzero[0] == image_size) {
		image_field_bits |= IMAGE_ALMOST2_VALID_NORMALS_ZERO;
	}
	if (normalizable_xy == image_size) {
		image_field_bits |= IMAGE_VALID_NORMALS_XY;
	}

	int image_type = 0;
	int must_clean = false;
	// What sort of normal texture is this?
	if (image_field_bits & (IMAGE_VALID_NORMALS_FULL | IMAGE_ALMOST_VALID_NORMALS_FULL | IMAGE_ALMOST2_VALID_NORMALS_FULL)) {
		image_type = IMAGE_TYPE_NORMAL_FULL;
		if (image_field_bits & IMAGE_ALL_SAME) {
			image_type = IMAGE_TYPE_NORMAL_FULL_SAME;
			if (gOptions.analyze) {
				std::wcout << "Image file '" << inputFile << "' is a normal texture, but all the values are the same (normals do not vary).\n";
			}
		}
		else {
			if (gOptions.analyze) {
				std::wcout << "Image file '" << inputFile << "' is a normal texture.\n";
			}
			if (!(image_field_bits & IMAGE_VALID_NORMALS_FULL)) {
				must_clean = true;
				if (image_field_bits & IMAGE_ALMOST_VALID_NORMALS_FULL) {
					if (gOptions.analyze) {
						std::wcout << "  The image does not have exactly the z-values expected, but these are within one of the expected value. " << 100.0f * (float)normal_length_zneg[0] / (float)image_size << " percent are as expected.\n";
					}
				}
				else {
					assert(image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_FULL);
					if (gOptions.analyze) {
						std::wcout << "  The image does not have exactly the z-values expected, but these are within two of the expected value.\n";
						std::wcout << "  " << 100.0f * (float)normal_length_zneg[0] / (float)image_size << " percent are as expected, " << 100.0f * (float)normal_length_zneg[1] / (float)image_size << " percent are just one away, and " << 100.0f * (float)normal_length_zneg[2] / (float)image_size << " percent are two away.\n";
					}
				}
			}
		}
		// mostly a reality check - should be caught by the NORMALS_FULL testing.
		if (!(image_field_bits & IMAGE_VALID_ZVAL_NONNEG)) {
			if (gOptions.analyze) {
				std::wcout << "  But, some Z values were found to be negative. " << 100.0f * (float)normal_zval_nonnegative / (float)image_size << " percent were not negative. The most negative Z value found was " << min_z << "\n";
			}
		}
	}
	else if (image_field_bits & (IMAGE_VALID_NORMALS_ZERO | IMAGE_ALMOST_VALID_NORMALS_ZERO | IMAGE_ALMOST2_VALID_NORMALS_ZERO)) {
		image_type = IMAGE_TYPE_NORMAL_ZERO;
		if (image_field_bits & IMAGE_ALL_SAME) {
			// really, we should never reach here - the similar code above should flag first.
			if (gOptions.analyze) {
				std::wcout << "Image file '" << inputFile << "' is a normal texture with Z ranging from 0.0 to 1.0, but all the values are the same (normals do not vary).\n";
			}
			assert(0);
		}
		else {
			if (gOptions.analyze) {
				std::wcout << "Image file '" << inputFile << "' is a normal texture with Z ranging from 0.0 to 1.0.\n";
			}
			if (!(image_field_bits & IMAGE_VALID_NORMALS_ZERO)) {
				must_clean = true;
				if (image_field_bits & IMAGE_ALMOST_VALID_NORMALS_ZERO) {
					if (gOptions.analyze) {
						std::wcout << "  The image does not have exactly the z-values expected, but these are within one of the expected value. " << 100.0f * (float)normal_length_zzero[0] / (float)image_size << " percent are precise as expected.\n";
					}
				}
				else {
					assert(image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_ZERO);
					if (gOptions.analyze) {
						std::wcout << "  The image does not have exactly the z-values expected, but these are within two of the expected value.\n";
						std::wcout << "  " << 100.0f * (float)normal_length_zzero[0] / (float)image_size << " percent are as expected, " << 100.0f * (float)normal_length_zzero[1] / (float)image_size << " percent are just one away, and " << 100.0f * (float)normal_length_zzero[2] / (float)image_size << " percent are two away.\n";
					}
				}
			}
		}
	}
	else if (image_field_bits & IMAGE_GRAYSCALE) {
		image_type = IMAGE_TYPE_HEIGHTFIELD;
		if (image_field_bits & IMAGE_ALL_SAME) {
			if (gOptions.analyze) {
				std::wcout << "Image file '" << inputFile << "' is likely a heightfield (grayscale) texture, but all the values are the same (normals do not vary).\n";
			}
		}
		else {
			if (gOptions.analyze) {
				std::wcout << "Image file '" << inputFile << "' is likely a heightfield (grayscale) texture.\n";
			}
		}
	}
	else {
		// We're really not sure at this point. Try some "close enough" tests. Could get more refined here, e.g., set error bounds.
		must_clean = true;
		// which has fewer values outside the bounds?
		if (z_outside_bounds < z_outside_zero_bounds) {
			// how much did the z value vary from expected?
			if (zmaxabsdiff > 2) {
				image_type = IMAGE_TYPE_NORMAL_XY_ONLY;
				if (gOptions.analyze) {
					std::wcout << "Image file '" << inputFile << "' is probably an XY-only normal texture,\n  with at least " << 100.0f * (float)z_outside_bounds / (float)image_size << " percent of the texel Z values being more than two from being properly normalized.\n";
					std::wcout << "  The z value was found to be as far off as " << zmaxabsdiff << " in expected value,\n  and was greater than a difference of two for " << 100.0f * (float)z_outside_bounds / (float)image_size << " percent of the texels.\n";
				}
			}
			else {
				image_type = IMAGE_TYPE_NORMAL_FULL;
				if (image_field_bits & IMAGE_VALID_ZVAL_NONNEG) {
					if (gOptions.analyze) {
						std::wcout << "Image file '" << inputFile << "' may be a normal texture, with " << 100.0f * (float)z_outside_bounds / (float)image_size << " percent of the texel Z values being more than two from being properly normalized.\n";
					}
				}
				else {
					if (gOptions.analyze) {
						std::wcout << "Image file '" << inputFile << "' might be a normal texture, with " << 100.0f * (float)normal_zval_nonnegative / (float)image_size << " percent of the Z values being non-negative.\n";
					}
				}
			}
		}
		else {
			// how much did the z value vary from expected?
			if (zmaxabsdiff_zero > 2) {
				image_type = IMAGE_TYPE_NORMAL_XY_ONLY;
				if (gOptions.analyze) {
					std::wcout << "Image file '" << inputFile << "' is probably an XY-only normal texture,\n  with at least " << 100.0f * (float)z_outside_zero_bounds / (float)image_size << " percent of the texel Z values being more than two from being properly normalized.\n";
					std::wcout << "  The z value was found to be as far off as " << zmaxabsdiff_zero << " in expected value,\n  and was greater than a difference of two for " << 100.0f * (float)z_outside_bounds / (float)image_size << " percent of the texels.\n";
				}
			}
			else {
				image_type = IMAGE_TYPE_NORMAL_ZERO;
				if (gOptions.analyze) {
					std::wcout << "Image file '" << inputFile << "' may be a normal texture with Z ranging from 0.0 to 1.0.\n";
				}
			}
		}

		// were the X and Y coordinates reasonable or not?
		if (!(image_field_bits & IMAGE_VALID_NORMALS_XY)) {
			if (gOptions.analyze) {
				std::wcout << "  The X and Y coordinates properly form vectors of length 1.0 or less for " << 100.0f * (float)normalizable_xy / (float)image_size << " percent of the texels.\n";
			}
		}
	}

	// additional info
	if (!(image_field_bits & IMAGE_ALL_SAME)) {
		if (image_field_bits & IMAGE_R_CHANNEL_ALL_SAME) {
			if (gOptions.analyze) {
				std::wcout << "  The red channel values in file '" << inputFile << "' all have the same value: " << first_pixel[0] << "\n";
			}
		}
		if (image_field_bits & IMAGE_G_CHANNEL_ALL_SAME) {
			if (gOptions.analyze) {
				std::wcout << "  The green channel values in file '" << inputFile << "' all have the same value: " << first_pixel[1] << "\n";
			}
		}
		if (image_field_bits & IMAGE_B_CHANNEL_ALL_SAME) {
			if (gOptions.analyze) {
				std::wcout << "  The blue channel values in file '" << inputFile << "' all have the same value: " << first_pixel[2] << "\n";
			}
		}
	}
	if (gOptions.analyze) {
		// whitespace between analyses, and flush
		std::wcout << "\n" << std::flush;

		// Add to proper summary list.
		std::wstring if_string = inputFile;
		switch (image_type) {
		case 0x0:
			gListUnknown += L" " + if_string;
			break;
		case IMAGE_TYPE_NORMAL_FULL:
			gListStandard += L" " + if_string;
			if (must_clean) {
				gListStandardDirty += L" " + if_string;
			}
			break;
		case IMAGE_TYPE_NORMAL_FULL_SAME:
			gListStandardSame += L" " + if_string;
			break;
		case IMAGE_TYPE_NORMAL_ZERO:
			gListZZero += L" " + if_string;
			if (must_clean) {
				gListZZeroDirty += L" " + if_string;
			}
			break;
		case IMAGE_TYPE_NORMAL_XY_ONLY:
			gListXYonly += L" " + if_string;
			break;
		case IMAGE_TYPE_HEIGHTFIELD:
			gListHeightfield += L" " + if_string;
			break;
		}
	}

	// Now see if we want to output the file
	if ((image_type > 0) && (gOptions.outputAll || (must_clean && gOptions.outputClean))) {
		// Output all or output clean file
		
		// Do we flip the Y axis on output? Do only if parity doesn't match
		bool y_flip = ((gOptions.inputDirectX && !gOptions.outputOpenGL) || (gOptions.inputOpenGL && !gOptions.outputDirectX));

		// make output image info
		progimage_info destination;
		destination.width = imageInfo.width;
		destination.height = imageInfo.height;
		destination.image_data.resize(destination.width * destination.height * 3 * sizeof(unsigned char), 0x0);

		if (image_type == IMAGE_TYPE_HEIGHTFIELD) {
			// For heightfields, we pay attention to just the output format specified.
			y_flip = gOptions.outputDirectX;

			// convert from heightfield
			if (gOptions.verbose) {
				std::wcout << ">> Converting heightfield to normal texture.\n" << std::flush;
			}
			convertHeightfieldToXYZ(&destination, &imageInfo, gOptions.heightfieldScale, y_flip);
		} else {
			// input is a normal texture of some type, so output it, cleaning as needed, flipping y if needed.
			if (gOptions.verbose) {
				std::wcout << ">> Readying output normal texture.\n" << std::flush;
			}
			cleanAndCopyNormalTexture(&destination, &imageInfo, must_clean, gOptions.outputZzeroToOne, y_flip);
		}

		// The outputFile has the same file name as the inputFile, possibly different path.
		// Walk through inputFile and get just the file bit
		//wchar_t* fileName;

		//wchar_t outputFile[MAX_PATH];
		//wcscpy_s(outputFile, MAX_PATH, gOptions.outputDirectory.c_str());

		//rc = writepng(&destination, 3, outputFile);
		//if (rc != 0)
		//{
		//	reportReadError(rc, outputFile);
		//	// quit
		//	return 1;
		//}
		//writepng_cleanup(&destination);
		//if (gOptions.verbose) {
		//	std::wcout << "New texture '" << outputFile << "' created.\n" << std::flush;
		//}
	}

	// clean up input image info.
	readImage_cleanup(1, &imageInfo);
	return imageProcessed;
}

void printHelp()
{
	std::wcerr << "NormalTextureProcessor version " << VERSION_STRING << "\n";
	std::wcerr << "usage: NormalTextureProcessor [-options] [file1.png file2.png...]\n";
	std::wcerr << "  -a - analyze texture files read in and output report.\n";
	std::wcerr << "  -idir path - give a relative or absolute path for what directory of files to read in.\n";
	std::wcerr << "  -iogl - assume input files use OpenGL-style (Y up) mapping for the green channel.\n";
	std::wcerr << "  -idx - assume input files use DirectX-style (Y down) mapping for the green channel.\n";
	std::wcerr << "  -oall - output all texture files read in, as possible.\n";
	std::wcerr << "  -oclean - clean and output all texture files that are not considered perfect.\n";
	std::wcerr << "  -odir path - give a relative or absolute path for what directory to output files.\n";
	std::wcerr << "  -ozzero - output files so that their blue channel value maps Z from 0 to 1 instead of -1 to 1.\n";
	std::wcerr << "  -oogl - output files using the OpenGL-style (Y up) mapping for the green channel.\n";
	std::wcerr << "  -odx - output files using the DirectX-style (Y down) mapping for the green channel.\n";
	std::wcerr << "  -hfs # - for heightfields, specify output scale.\n";
	std::wcerr << "  -hwrap - treat heightfields as wrapping around (repeating) when output to a normal texture.\n";
	std::wcerr << "  -v - verbose, explain everything going on. Default: display only warnings and errors.\n";
	std::wcerr << std::flush;
}

//====================== statics ==========================

static void reportReadError(int rc, const wchar_t* filename)
{
	switch (rc) {
	case 1:
		wsprintf(gErrorString, L"***** ERROR [%s] is not a PNG file: incorrect signature.\n", filename);
		break;
	case 2:
		wsprintf(gErrorString, L"***** ERROR [%s] has bad IHDR (libpng longjmp).\n", filename);
		break;
	case 4:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - insufficient memory.\n", filename);
		break;
	case 57:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - invalid CRC. Try saving this PNG using some program, e.g., IrfanView.\n", filename);
		break;
	case 63:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - chunk too long.\n", filename);
		break;
	case 78:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - file not found or could not be read.\n", filename);
		break;
	case 79:
		wsprintf(gErrorString, L"***** ERROR [%s] write failed - directory not found. Please create the directory.\n", filename);
		break;
	case 83:
		wsprintf(gErrorString, L"***** ERROR [%s] allocation failed. Image file is too large for your system to handle?\n", filename);
		break;
	case 102:
		wsprintf(gErrorString, L"***** ERROR [%s] - could not read Targa TGA file header.\n", filename);
		break;
	case 103:
		wsprintf(gErrorString, L"***** ERROR [%s] - could not read Targa TGA file data.\n", filename);
		break;
	case 104:
		wsprintf(gErrorString, L"***** ERROR [%s] - unsupported Targa TGA file type.\n", filename);
		break;
	case 999:
		wsprintf(gErrorString, L"***** ERROR [%s] - unknown image file type.\n", filename);
		break;
	default:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - unknown readpng_init() or targa error.\n", filename);
		break;
	}
	saveErrorForEnd();
	gErrorCount++;

	if (rc != 78 && rc != 79 && rc < 100) {
		wsprintf(gErrorString, L"Often this means the PNG file has some small bit of information that TileMaker cannot\n  handle. You might be able to fix this error by opening this PNG file in\n  Irfanview or other viewer and then saving it again. This has been known to clear\n  out any irregularity that TileMaker's somewhat-fragile PNG reader dies on.\n");
		saveErrorForEnd();
	}
}

static void saveErrorForEnd()
{
	wprintf(gErrorString);
	wcscat_s(gConcatErrorString, CONCAT_ERROR_LENGTH, L"  ");
	wcscat_s(gConcatErrorString, CONCAT_ERROR_LENGTH, gErrorString);
}

//================================ Image Manipulation ====================================

// assumes 3 channels are input
void convertHeightfieldToXYZ(progimage_info* dst, progimage_info* src, float heightfieldScale, bool y_flip)
{
	int row, col;
	// create temporary grayscale image of input and work off of that, putting the result in the output dst file
	progimage_info* phf = allocateGrayscaleImage(src);
	copyOneChannel(phf, CHANNEL_RED, src, LCT_RGB);
	unsigned char* phf_data = &phf->image_data[0];
	unsigned char* dst_data = &dst->image_data[0];

	for (row = 0; row < phf->height; row++)
	{
		// TODOTODO - right now we always wrap around edges, grabbing the texel off the other edge. Other options possible.
		int trow = (row + phf->height - 1) % phf->height;
		int brow = (row + phf->height + 1) % phf->height;
		for (col = 0; col < phf->width; col++)
		{
			int lcol = (col + phf->width - 1) % phf->width;
			int rcol = (col + phf->width + 1) % phf->width;
			// Won't swear to this conversion being quite right. From Real-Time Rendering, p. 214 referencing an article:
			// Eberly, David, "Reconstructing a Height Field from a Normal Map," Geometric Tools blog, May 3, 2006.
			// https://www.geometrictools.com/Documentation/ReconstructHeightFromNormals.pdf
			float x = heightfieldScale * (phf_data[row * phf->width + lcol] - phf_data[row * phf->width + rcol]) / 255.0f;
			float y = heightfieldScale * (phf_data[trow * phf->width + col] - phf_data[brow * phf->width + col]) / 255.0f;
			if (!y_flip) {
				// Right, this is confusing, I admit it. The point here is that the y-flip is normally done by default
				// by the code above. So if the DirectX style is needed, it's already done. This is for OpenGL style.
				// TODOTODO verify!
				y = -y;
			}
			float length = (float)sqrt(x * x + y * y + 1.0f);
			// Basically, map from XYZ [-1,1] to RGB. Make sure it's normalized.
			*dst_data++ = (unsigned char)(((x / length + 1.0f) / 2.0f) * 255.0f + 0.5f);
			*dst_data++ = (unsigned char)(((y / length + 1.0f) / 2.0f) * 255.0f + 0.5f);
			*dst_data++ = (unsigned char)(((1.0f / length + 1.0f) / 2.0f) * 255.0f + 0.5f);
		}
	}
}

void cleanAndCopyNormalTexture(progimage_info* dst, progimage_info* src, bool must_clean, bool output_zzero, bool y_flip)
{
	int row, col;
	float x, y, z;
	unsigned char* src_data = &src->image_data[0];
	unsigned char* dst_data = &dst->image_data[0];

	for (row = 0; row < src->height; row++)
	{
		for (col = 0; col < src->width; col++)
		{
			// One thing we always do is compute the Z value and convert it properly to the blue channel,
			// even if the data is clean. In this way we can entirely ignore the incoming Z channel and whether
			// it's standard or Z-zero. All we care about is how to output it.
			CONVERT_CHANNEL_TO_FULL(src_data[0], x);
			CONVERT_CHANNEL_TO_FULL(src_data[1], y);

			// Clean up x,y if needed, then finally convert back to r,g,b.
			// Really, we could always do the code below as a sanity check, but I like to think this saves a few microseconds overall.
			if (must_clean) {
				// are X and Y invalid, forming a vector > 1.0 in length?
				// TODOTODO check that this code gets hit.
				float xy_len2 = x * x + y * y;
				if (xy_len2 > 1.0f) {
					// rescale x and y to fit (what else could we possibly do?) and set z - done!
					float xy_length = sqrt(xy_len2);
					x /= xy_length;
					y /= xy_length;
					// yes, the normal has to point sideways (what else could it be? The normal's too big), and we're done computing Z
					z = 0.0f;
				}
				else {
					// all's well with XY length, so compute Z from it
					COMPUTE_Z_FROM_XY(x, y, z);
				}
			}
			else {
				// no cleaning, just compute Z
				assert(x * x + y * y <= 1.0f);

				COMPUTE_Z_FROM_XY(x, y, z);
			}

			// y-flip, if needed
			if (y_flip) {
				y = -y;
			}

			// and save; Z is guaranteed to not be negative because we computed it
			if (output_zzero) {
				// convert back to rgb
				CONVERT_Z_ZERO_TO_RGB(x, y, z, dst_data[0], dst_data[1], dst_data[2]);
			}
			else {
				// convert back to rgb
				CONVERT_Z_FULL_TO_RGB(x, y, z, dst_data[0], dst_data[1], dst_data[2]);
			}

			// next texel
			dst_data += 3;
			src_data += 3;
		}
	}
}


bool removeFileType(wchar_t* name)
{
	wchar_t* loc = wcsrchr(name, L'.');
	if (loc != NULL)
	{
		// remove .png suffix
		*loc = 0x0;
		return true;
	}
	return false;
}

int isImageFile(wchar_t* name)
{
	// check for .png suffix - note test is case insensitive
	int len = (int)wcslen(name);
	if (len > 4) {
		if (_wcsicmp(&name[len - 4], L".png") == 0)
		{
			return PNG_EXTENSION_FOUND;
		}
		else if (_wcsicmp(&name[len - 4], L".tga") == 0)
		{
			return TGA_EXTENSION_FOUND;
		}
		else if (_wcsicmp(&name[len - 4], L".jpg") == 0)
		{
			return JPG_EXTENSION_FOUND;
		}
		else if (_wcsicmp(&name[len - 4], L".bmp") == 0)
		{
			return BMP_EXTENSION_FOUND;
		}
	}
	return UNKNOWN_FILE_EXTENSION;
}

// from https://stackoverflow.com/questions/8233842/how-to-check-if-directory-exist-using-c-and-winapi
bool dirExists(const wchar_t* path)
{
	DWORD ftyp = GetFileAttributesW(path);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

bool createDir(const wchar_t* path)
{
	if (CreateDirectory(path, NULL)) {
		return true;
	}
	else {
		DWORD err = GetLastError();
		if (err == ERROR_ALREADY_EXISTS) {
			return true;
		}
	}
	return false;
}