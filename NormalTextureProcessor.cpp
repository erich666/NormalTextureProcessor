// NormalTextureProcessor.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Program to read in normals textures and analyze or clean up/convert them to other normals texture forms.

#include <iostream>
#include <fstream>
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
#define	IMAGE_ALMOST_VALID_NORMALS_XY		0x2000
#define	IMAGE_ALMOST2_VALID_NORMALS_XY		0x4000

#define IMAGE_TYPE_NORMAL_FULL		1
#define IMAGE_TYPE_NORMAL_ZERO		2
#define IMAGE_TYPE_NORMAL_XY_ONLY	3
#define IMAGE_TYPE_HEIGHTFIELD		4


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

// for direct output or setting a value
#define CONVERT_CHANNEL_FULL_DIRECT(chan)	\
	(((float)(chan) / 255.0f) * 2.0f - 1.0f)

#define CONVERT_CHANNEL_ZERO_DIRECT(chan)	\
	((float)(chan) / 255.0f)

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

#define COMPUTE_GRAY_FROM_RGB(r,g,b, gray)	\
    gray = (unsigned char)((0.2126f * ((float)r / 255.0f) + 0.7152f * ((float)g / 255.0f) + 0.0722f * ((float)b / 255.0f)) * 255.0f + 0.5f);


// maximum difference in length from 1.0 for normals when saving in [0,255] numbers,
// when Z value is computed from X and Y
#define	MAX_NORMAL_LENGTH_DIFFERENCE	0.00387f
#define	MAX_Z_DIFFERENCE				0.00392f

#define VECTOR_LENGTH(x,y,z)	\
	sqrt(x*x + y*y + z*z)

struct Options {
	bool analyze;
	std::wstring inputDirectory;
	bool inputZzeroToOne;
	bool inputZnegOneToOne;
	bool inputXYonly;
	bool inputDirectX;
	bool inputHeightfield;
	bool outputAll;
	bool outputClean;
	std::wstring outputDirectory;
	bool outputZzeroToOne;
	bool outputDirectX;
	bool outputHeatmap;
	bool allowNegativeZ;
	float heightfieldScale;
	bool borderHeightfield;
	bool verbose;
	bool csvAll;
	bool csvErrors;
	float errorTolerance;
	float errorToleranceXY;
	//bool roundtrip;
};

Options gOptions;

int gFilesFound = 0;
int gFilesOutput = 0;

std::wstring gListUnknown;
std::wstring gListStandard;
std::wstring gListStandardDirty;
std::wstring gListStandardSame;
std::wstring gListZZero;
std::wstring gListZZeroDirty;
std::wstring gListXYonly;
std::wstring gListHeightfield;
std::wstring gListAllSame;


//-------------------------------------------------------------------------
void printHelp();

int filterAndProcessImageFile(wchar_t* inputFile);
bool processImageFile(wchar_t* inputFile, int fileType);
bool XYanalysis(int image_field_bits, int image_size, int* normalizable_xy, int xy_outside_bounds);
bool Zanalysis(int image_type, int image_field_bits, int image_size, int* normal_length_zneg, float minnormallength, float maxnormallength, int normal_zval_nonnegative, float min_z, int z_outside_bounds, int zmaxabsdiff);

void reportReadError(int rc, const wchar_t* filename);
void saveErrorForEnd();

//bool examineForDirectXStyle(progimage_info* src);
void convertHeightfieldToXYZ(progimage_info* dst, progimage_info* src, float heightfieldScale, bool input_grayscale, bool y_flip, bool clamp_border);
void cleanAndCopyNormalTexture(progimage_info* dst, progimage_info* src, int image_type, bool output_zzero, bool y_flip, bool heatmap, bool reason_xy);

bool removeFileType(wchar_t* name);
int isImageFile(wchar_t* name);
bool dirExists(const wchar_t* path);
bool createDir(const wchar_t* path);

int wmain(int argc, wchar_t* argv[])
{
	// experiment: try all possible X and Y values in a quadrant, compute Z. How close is the triplet formed to 1.0 length?

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
					std::wcerr << "ERROR: something weird's going on, the round trip test failed.\n" << std::flush;
				}
			}
		}
	}
	std::wcout << "Average diff from length of 1.0f: " << absdiffsum / validcount << ", maximum length difference detected is " << maxdiff << " and maximum z difference detected is " << zmaxdiff << " for the " << validcount << " valid pixel values\n";
#endif

	gConcatErrorString[0] = 0;

	int argLoc = 1;

	gOptions.analyze = false;
	gOptions.inputHeightfield = false;
	gOptions.inputXYonly = false;
	gOptions.inputDirectX = false;
	gOptions.outputAll = false;
	gOptions.outputClean = false;
	gOptions.outputZzeroToOne = false;
	gOptions.outputDirectX = false;
	gOptions.outputHeatmap = false;
	gOptions.allowNegativeZ = false;
	gOptions.heightfieldScale = 5.0f;
	gOptions.borderHeightfield = false;
	gOptions.verbose = false;
	gOptions.csvAll = false;
	gOptions.csvErrors = false;
	gOptions.errorTolerance = 0.02f;
	gOptions.errorToleranceXY = 0.10f;
	//gOptions.roundtrip = false;

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
		else if (wcscmp(argv[argLoc], L"-izzero") == 0)
		{
			gOptions.inputZzeroToOne = true;
		}
		else if (wcscmp(argv[argLoc], L"-izneg") == 0)
		{
			gOptions.inputZnegOneToOne = true;
		}
		else if (wcscmp(argv[argLoc], L"-ixy") == 0)
		{
			gOptions.inputXYonly = true;
		}
		else if (wcscmp(argv[argLoc], L"-idx") == 0)
		{
			gOptions.inputDirectX = true;
		}
		else if (wcscmp(argv[argLoc], L"-ihf") == 0)
		{
			gOptions.inputHeightfield = true;
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
		else if (wcscmp(argv[argLoc], L"-odx") == 0)
		{
			gOptions.outputDirectX = true;
		}
		else if (wcscmp(argv[argLoc], L"-oheatmap") == 0)
		{
			gOptions.outputHeatmap = true;
		}
		else if (wcscmp(argv[argLoc], L"-allownegz") == 0)
		{
			gOptions.allowNegativeZ = true;
		}
		else if (wcscmp(argv[argLoc], L"-hfs") == 0)
		{
			// heightfield scale float value
			INC_AND_TEST_ARG_INDEX(argLoc);
			swscanf_s(argv[argLoc], L"%f", &gOptions.heightfieldScale);
		}
		else if (wcscmp(argv[argLoc], L"-hborder") == 0)
		{
			// TODO: should there also be a "continue slope" option for the border, too?
			// Basically, if on the border, take the difference and double it (since we'll be using the texel itself for one offset).
			gOptions.borderHeightfield = true;
		}
		else if (wcscmp(argv[argLoc], L"-csv") == 0)
		{
			gOptions.csvAll = true;
		}
		else if (wcscmp(argv[argLoc], L"-csve") == 0)
		{
			gOptions.csvErrors = true;
		}
		else if (wcscmp(argv[argLoc], L"-etol") == 0)
		{
			// heightfield scale float value
			INC_AND_TEST_ARG_INDEX(argLoc);
			swscanf_s(argv[argLoc], L"%f", &gOptions.errorTolerance);
			if (gOptions.errorTolerance < 0.0f || gOptions.errorTolerance > 100.0f) {
				std::wcerr << "ERROR: error tolerance percentage must be between 0 and 100.\n";
				printHelp();
				return 1;
			}
			// turn percent tolerance into 0-1 range value
			gOptions.errorTolerance *= 0.01f;
		}
		else if (wcscmp(argv[argLoc], L"-etolxy") == 0)
		{
			// heightfield scale float value
			INC_AND_TEST_ARG_INDEX(argLoc);
			swscanf_s(argv[argLoc], L"%f", &gOptions.errorToleranceXY);
			if (gOptions.errorToleranceXY < 0.0f || gOptions.errorToleranceXY > 1.0f) {
				std::wcerr << "ERROR: error tolerance XY magnitude must be between 0.0 and 1.0.\n";
				printHelp();
				return 1;
			}
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
				std::wcerr << "ERROR: unknown option '" << argv[argLoc] << "'.\n";
				printHelp();
				return 1;
			}
			fileList.push_back(argv[argLoc]);
		}
		argLoc++;
	}

	if (gOptions.verbose) {
		std::wcout << "NormalTextureProcessor version " << VERSION_STRING << "\n\n" << std::flush;
	}

	// check validity of options only if we're actually outputing
	if (!(gOptions.outputAll || gOptions.outputClean || gOptions.csvAll || gOptions.csvErrors)) {
		// No output mode chosen, so these options won't do anything
		if (gOptions.inputHeightfield || gOptions.inputDirectX || gOptions.borderHeightfield ||
			gOptions.outputDirectory.size() > 0 ||
				gOptions.outputDirectX || gOptions.outputZzeroToOne || gOptions.outputHeatmap) {
			std::wcerr << "WARNING: options associated with output are set, but -oall or -oclean are not set. One of these two must be chosen.\n" << std::flush;
		}
	}

	if (gOptions.inputHeightfield && gOptions.inputDirectX) {
		std::wcerr << "WARNING: -ihf says to assume input is a heightfield, but -idx says it's DirectX style; latter option is ignored.\n" << std::flush;
		gOptions.inputDirectX = false;
	}

	if ((gOptions.inputZzeroToOne ? 1 : 0) + (gOptions.inputZnegOneToOne ? 1 : 0) + (gOptions.inputXYonly ? 1 : 0) + (gOptions.inputHeightfield ? 1 : 0) > 1) {
		std::wcerr << "ERROR: more than one input specifier of [-izneg | -izzero | -ixy | -ihf] is given; choose at most one.\n" << std::flush;
		printHelp();
		return 1;
 	}

	if (gOptions.inputHeightfield && gOptions.inputXYonly) {
		std::wcerr << "ERROR: input specifiers -ihf and -xy cannot both be set.\n" << std::flush;
		printHelp();
		return 1;
	}

	if ((gOptions.csvAll || gOptions.csvErrors) && ((gOptions.inputZzeroToOne ? 1 : 0) + (gOptions.inputZnegOneToOne ? 1 : 0) + (gOptions.inputXYonly ? 1 : 0) != 1)) {
		std::wcerr << "ERROR: for CSV analysis output you must specify the input type [-izneg | -izzero | -ixy].\n" << std::flush;
		printHelp();
		return 1;
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
		if (gOptions.outputDirectory[pos] != '/' && gOptions.outputDirectory[pos] != '\\') {
			gOptions.outputDirectory.push_back('/');
		}
	}

	// look through files and process each one.
	HANDLE hFind;
	WIN32_FIND_DATA ffd;

	// add "/" to end of directory if not already there
	if (gOptions.inputDirectory.size() > 0) {
		// input directory exists, so make sure there's a "\" or "/" at the end of this path
		pos = (int)gOptions.inputDirectory.size() - 1;
		if (gOptions.inputDirectory[pos] != '/' && gOptions.inputDirectory[pos] != '\\') {
			gOptions.inputDirectory.push_back('/');
		}
	}
	wchar_t fileSearch[MAX_PATH];
	wcscpy_s(fileSearch, MAX_PATH, gOptions.inputDirectory.c_str());

	// files to search on. We'll later filter by .png and .tga
	if (fileList.size() > 0) {
		bool bad_file = false;
		for (int i = 0; i < fileList.size(); i++) {
			wcscpy_s(fileSearch, MAX_PATH, fileList[i].c_str());
			if (filterAndProcessImageFile(fileSearch) == UNKNOWN_FILE_EXTENSION) {
				// file not an image file - note error
				std::wcerr << "Error: file '" << fileSearch << "' is not an image file.\n";
				bad_file = true;
			}
		}
		if (bad_file) {
			printHelp();
		}
	}
	else {
		wcscat_s(fileSearch, MAX_PATH, L"*");
		hFind = FindFirstFile(fileSearch, &ffd);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			// go through all the files in the blocks directory
			do {
				filterAndProcessImageFile(ffd.cFileName);
			} while (FindNextFile(hFind, &ffd) != 0);

			FindClose(hFind);
		}
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
			std::wcout << "  Standard normals textures that have no bumps, all texels are the same value:" << gListStandardSame << "\n";
		}
		if (gListStandard.size() > 0) {
			output_any = true;
			std::wcout << "  Standard normals textures:" << gListStandard << "\n";
			if (gListStandardDirty.size() > 0) {
				output_any = true;
				std::wcout << "    Standard normals textures that are not perfectly normalized:" << gListStandardDirty << "\n";
			}
		}
		if (gListZZero.size() > 0) {
			output_any = true;
			std::wcout << "  Z-Zero normals textures:" << gListZZero << "\n";
			if (gListZZeroDirty.size() > 0) {
				output_any = true;
				std::wcout << "    Z-Zero normals textures that are not perfectly normalized:" << gListZZeroDirty << "\n";
			}
		}
		if (gListXYonly.size() > 0) {
			output_any = true;
			std::wcout << "  XY-only normals textures:" << gListXYonly << "\n";
		}
		if (gListHeightfield.size() > 0) {
			output_any = true;
			std::wcout << "  Heightfield textures:" << gListHeightfield << "\n";
		}
		if (gListUnknown.size() > 0) {
			output_any = true;
			std::wcout << "  Unknown textures:" << gListUnknown << "\n";
		}
		if (gListAllSame.size() > 0) {
			output_any = true;
			std::wcout << "  Textures with no bumps:" << gListAllSame << "\n";
		}
		if (!output_any) {
			if (gOptions.inputDirectory.size() > 0) {
				std::wcout << "  No PNG or TGA files found to analyze in directory '" << gOptions.inputDirectory << "'\n";
			}
			else {
				std::wcout << "  No PNG or TGA files found to analyze in current directory\n";
			}
		}
	}

	std::wcout << "\nNormalTextureProcessor summary: " << gFilesFound << " PNG/TGA file" << (gFilesFound == 1 ? "" : "s") << " discovered and " << gFilesOutput << " output.\n" << std::flush;
	return 0;
}

int filterAndProcessImageFile(wchar_t* inputFile)
{
	int file_type = isImageFile(inputFile);
	if (file_type != UNKNOWN_FILE_EXTENSION) {
		// Seems to be an image file. Can we process it?
		if (file_type == PNG_EXTENSION_FOUND || file_type == TGA_EXTENSION_FOUND) {
			gFilesFound++;
			gFilesOutput += (processImageFile(inputFile, file_type) ? 1 : 0);
			return file_type;
		}
		else {
			std::wcerr << "WARNING: file " << inputFile << " is not an image file type currently supported in this program. Convert to PNG or TGA to process it.\n" << std::flush;
		}
	}
	return UNKNOWN_FILE_EXTENSION;
}

// return true if successfully read in and output
bool processImageFile(wchar_t* inputFile, int file_type)
{
	bool rc_output = false;
	wchar_t inputPathAndFile[MAX_PATH];
	// note that inputDirectory already has a "/" at the end, due to manipulation at the start of the program
	wcscpy_s(inputPathAndFile, MAX_PATH, gOptions.inputDirectory.c_str());
	wcscat_s(inputPathAndFile, MAX_PATH, inputFile);

	// read input file
	progimage_info imageInfo;
	int rc = readImage(&imageInfo, inputPathAndFile, LCT_RGB, file_type);
	if (rc != 0)
	{
		// file not found
		reportReadError(rc, inputPathAndFile);
		return rc_output;
	}

	// see how many bits etc. the input PNG has (if it's a PNG);
	int bitdepth = 8;
	LodePNGColorType colortype = LCT_RGB;
	if (file_type == PNG_EXTENSION_FOUND) {
		std::vector<unsigned char> buffer;
		if (lodepng::load_file(buffer, inputPathAndFile) == 0) {
			unsigned w, h;
			lodepng::State state;
			lodepng_inspect(&w, &h, &state, &buffer[0], buffer.size());
			bitdepth = state.info_png.color.bitdepth;
			colortype = state.info_png.color.colortype;
		}
		else {
			// what? We just loaded the file earlier
			std::wcerr << "ERROR: file " << inputFile << "' could not be read to get its header information.\n" << std::flush;
			return 0;
		}
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
	float x, y, z, zzero, zcalc;
	x = 0; y = 0; z = 0; zzero = 0;	// initialized mostly to make the compiler happy; these do get used in some analysis messages.
	unsigned char* src_data = &imageInfo.image_data[0];
	// various counters
	int pixels_match = 0;
	int rgb_match[3];
	int grayscale = 0;
	int normalizable_xy[3];
	normalizable_xy[0] = normalizable_xy[1] = normalizable_xy[2] = 0;
	int xy_outside_bounds = 0;
	int z_outside_bounds = 0;
	int z_outside_zzero_bounds = 0;
	int normal_length_zneg[3];
	normal_length_zneg[0] = normal_length_zneg[1] = normal_length_zneg[2] = 0;
	int normal_zval_nonnegative = 0;
	int normal_length_zzero[3];
	normal_length_zzero[0] = normal_length_zzero[1] = normal_length_zzero[2] = 0;
	float min_z = 999.0f;
	float zzero_min_z = 999.0f;
	int zmaxabsdiff = 0;
	int zmaxabsdiff_zero = 0;
	float xymaxlength = 0.0f;
	float maxnormallength = 0.0f;
	float minnormallength = 2.0f;
	float zzero_maxnormallength = 0.0f;
	float zzero_minnormallength = 2.0f;
	double xsum, ysum, zsum, zzerosum;
	xsum = ysum = zsum = zzerosum = 0.0;
	int chan_min[3];
	int chan_max[3];
	int gray_min = 256*256;
	int gray_max = -1;
	unsigned char first_pixel[3];
	for (channel = 0; channel < 3; channel++) {
		first_pixel[channel] = src_data[channel];
		rgb_match[channel] = 0;
		chan_min[channel] = 256 * 256;
		chan_max[channel] = -1;
	}
	int image_size = imageInfo.width * imageInfo.height;

	if (image_size <= 0) {
		std::wcerr << "WARNING: file '" << inputFile << "' has no data so is not processed further.\n";
		return true;
	}

	// if CSV desired, open file
	std::wfstream csvOutput;
	if (gOptions.csvAll || gOptions.csvErrors) {
		wchar_t csvOutputPathAndFile[MAX_PATH];
		// note that outputDirectory already has a "/" at the end, due to manipulation at the start of the program
		wcscpy_s(csvOutputPathAndFile, MAX_PATH, gOptions.outputDirectory.c_str());

		// We always use PNG as output, so change the .TGA suffix if necouted be.
		wchar_t csvFileName[MAX_PATH];
		wcscpy_s(csvFileName, MAX_PATH, inputFile);
		// change suffix to CSV
		size_t csv_suffix_pos = wcslen(csvFileName) - 4;
		csvFileName[csv_suffix_pos] = 0;
		wcscat_s(csvFileName, MAX_PATH, L".csv");

		// add the file name to the output directory
		wcscat_s(csvOutputPathAndFile, MAX_PATH, csvFileName);
		csvOutput.open(csvOutputPathAndFile, std::fstream::out);

		if (csvOutput.fail()) {
			std::wcerr << "ERROR: could not open file '" << csvOutputPathAndFile << "' for writing, failbit " << csvOutput.fail() << ".\n";
			return false;
		}

		if (gOptions.verbose) {
			std::wcout << "New CSV file '" << csvOutputPathAndFile << "' created.\n\n" << std::flush;
		}
		csvOutput << "Column, Row, r, g, b, X, Y, Z, Z-zero, XY length, XYZ length, XYZ-zero length, XY > 1, Z < 0, " <<
			(!gOptions.inputZzeroToOne ? "XYZ != 1\n" : "XYZ - zero != 1\n");
	}

	// note these are reset each iteration only when writing to a CSV file, as these are used only for CSV output
	char err_outside_xy = ' ';
	char err_z_neg_is_negative = ' ';
	char err_outside_z = ' ';
	char err_outside_z_zero = ' ';
	int gray = 0;	// initialized to make the compiler happy
	//int ignore_normal_texel_count = 0;

	int b_less_than_255 = 0;
	for (row = 0; row < imageInfo.height; row++)
	{
		for (col = 0; col < imageInfo.width; col++)
		{
			// TODO: should probably ignore texels that are all-black or all-white as far as
			// analysis goes. We could assume such texels are in unused parts of the texture.
			// If we go this route, the XYZ statistics get messier.
			//bool black_or_white = false;
			//if ((src_data[0] == 0 && src_data[1] == 0 && src_data[2] == 0) ||
			//	(src_data[0] == 255 && src_data[1] == 255 && src_data[2] == 255)) {
			//	// note how many black and white
			//	ignore_normal_texel_count++;
			//	black_or_white = true;
			//}

			// Does this pixel match the first pixel? (i.e. are all pixels the same?)
			if (src_data[0] == first_pixel[0] && src_data[1] == first_pixel[1] && src_data[2] == first_pixel[2]) {
				pixels_match++;
			}
			// channel by channel, does the channel match the first channel value?
			for (channel = 0; channel < 3; channel++) {
				if (src_data[channel] == first_pixel[channel]) {
					rgb_match[channel]++;
				}
				// min max
				if (chan_min[channel] > src_data[channel]) {
					chan_min[channel] = src_data[channel];
				}
				if (chan_max[channel] < src_data[channel]) {
					chan_max[channel] = src_data[channel];
				}
			}
			if (src_data[2] != 255) {
				b_less_than_255++;
			}

			// heightfield height range
			COMPUTE_GRAY_FROM_RGB(src_data[0], src_data[1], src_data[2], gray);
			if (gray_min > gray) {
				gray_min = gray;
			}
			if (gray_max < gray) {
				gray_max = gray;
			}

			// are all three channel values the same? (grayscale)
			if (src_data[0] == src_data[1] && src_data[1] == src_data[2]) {
				grayscale++;
			}

			//if (!black_or_white) {
			// Try conversion to XYZ's. Which if any make sense?
			CONVERT_RGB_TO_Z_FULL(src_data[0], src_data[1], src_data[2], x, y, z);
			CONVERT_CHANNEL_TO_ZERO(src_data[2], zzero);
			xsum += x;
			ysum += y;
			zsum += z;
			zzerosum += zzero;

			// compute normal's length from values and compare
			float normal_length = VECTOR_LENGTH(x, y, z);
			if (maxnormallength < normal_length) {
				maxnormallength = normal_length;
			}
			if (minnormallength > normal_length) {
				minnormallength = normal_length;
			}

			// Check if the range of z values is non-negative. It may just be the normals are not normalized?
			if (z > -MAX_NORMAL_LENGTH_DIFFERENCE) {
				normal_zval_nonnegative++;
				if (z < 0.0f && !gOptions.allowNegativeZ) {
					err_z_neg_is_negative = '1';
				}
			}
			else {
				if (!gOptions.allowNegativeZ) {
					err_z_neg_is_negative = 'X';
				}
			}

			// lowest z value found (for -1 to 1 conversion)
			if (min_z > z) {
				min_z = z;
			}
			if (zzero_min_z > zzero) {
				zzero_min_z = zzero;
			}

			float zzero_normal_length = VECTOR_LENGTH(x, y, zzero);
			if (zzero_maxnormallength < zzero_normal_length) {
				zzero_maxnormallength = zzero_normal_length;
			}
			if (zzero_minnormallength > zzero_normal_length) {
				zzero_minnormallength = zzero_normal_length;
			}

			// find the proper z value
			float xy_len = sqrt(x * x + y * y);
			if (xymaxlength < xy_len) {
				xymaxlength = xy_len;
			}
			// is x,y "short enough" to properly represent a normalized vector?
			if (xy_len <= 1.0f + 2.0f * MAX_NORMAL_LENGTH_DIFFERENCE) {
				// it's (mostly) short enough in X and Y.

				// determine what z really should be for this x,y pair
				if (xy_len <= 1.0f) {
					// xy_len is definitely valid
					normalizable_xy[0]++;
					COMPUTE_Z_FROM_XY(x, y, zcalc);
				}
				else {
					// x,y's length is a tiny bit above 1.0 in length, but still not unreasonable.
					// Set z to 0.0
					zcalc = 0.0f;
					if (xy_len <= 1.0f + MAX_NORMAL_LENGTH_DIFFERENCE) {
						// it's one texel level difference
						normalizable_xy[1]++;
						err_outside_xy = '1';
					}
					else {
						// it's a two-texel level difference
						normalizable_xy[2]++;
						err_outside_xy = '2';
					}
				}
				// is the correctly z converted pixel level off by only one, compared to the original blue channel value?
				unsigned char bcalc;
				CONVERT_FULL_TO_CHANNEL(zcalc, bcalc);
				int zabsdiff = (int)(abs(src_data[2] - bcalc));
				if (zabsdiff == 0) {
					normal_length_zneg[0]++;
				}
				if (zabsdiff == 1) {
					normal_length_zneg[1]++;
					err_outside_z = '1';
				}
				if (zabsdiff == 2) {
					normal_length_zneg[2]++;
					err_outside_z = '2';
				}
				if (zabsdiff > 2) {
					// computed z is considerably different from input z
					z_outside_bounds++;
					err_outside_z = 'X';
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
					err_outside_z_zero = '1';
				}
				if (zabsdiff == 2) {
					normal_length_zzero[2]++;
					err_outside_z_zero = '2';
				}
				if (zabsdiff > 2) {
					z_outside_zzero_bounds++;
					err_outside_z_zero = 'X';
				}
				if (zmaxabsdiff_zero < zabsdiff) {
					zmaxabsdiff_zero = zabsdiff;
				}
			}
			else {
				xy_outside_bounds++;	// just a place to put a break and see what's what
				err_outside_xy = 'X';
			}

			if (gOptions.csvAll ||
				(gOptions.csvErrors && (err_z_neg_is_negative != ' ' || (!gOptions.inputZzeroToOne && (err_outside_z != ' ')) || (!gOptions.inputZnegOneToOne && (err_outside_z_zero != ' ')) || err_outside_xy != ' '))) {
				//csvOutput << "Column, Row, r, g, b, X, Y, Z, Z-zero, XY length, XYZ length, XYZ-zero length, XY > 1, Z < 0, XYZ != 1, XYZ-zero != 1\n";
				csvOutput << col << ", " << row << ", " << src_data[0] << ", " << src_data[1] << ", " << src_data[2] << ", "
					<< x << ", " << y << ", " << z << ", " << zzero << ", " << xy_len << ", " << normal_length << ", " << zzero_normal_length << ", "
					<< err_outside_xy << ", " << err_z_neg_is_negative << ", " << (!gOptions.inputZzeroToOne ? err_outside_z : err_outside_z_zero) << "\n";
				err_outside_xy = ' ';
				err_z_neg_is_negative = ' ';
				err_outside_z = ' ';
				err_outside_z_zero = ' ';
			}

			// next texel
			src_data += 3;
		}
	}
	if (gOptions.csvAll || gOptions.csvErrors) {
		csvOutput.close();
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
	if (gOptions.allowNegativeZ || (normal_zval_nonnegative == image_size)) {
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
	if (normalizable_xy[0] == image_size) {
		image_field_bits |= IMAGE_VALID_NORMALS_XY;
	}
	if (normalizable_xy[1] + normalizable_xy[0] == image_size) {
		image_field_bits |= IMAGE_ALMOST_VALID_NORMALS_XY;
	}
	if (normalizable_xy[2] + normalizable_xy[1] + normalizable_xy[0] == image_size) {
		image_field_bits |= IMAGE_ALMOST2_VALID_NORMALS_XY;
	}

	int image_type = 0;
	bool must_clean = false;
	bool reason_xy = false;
	// What sort of normals texture is this?
	if (gOptions.inputZnegOneToOne) {
		image_type = IMAGE_TYPE_NORMAL_FULL;
		// no analysis done, so assume the file needs cleaning
		must_clean = true;
		if (gOptions.analyze) {
			std::wcout << "Image file '" << inputFile << "' forced to be a standard normals texture.\n";
		}
	}
	else if (gOptions.inputZzeroToOne) {
		image_type = IMAGE_TYPE_NORMAL_ZERO;
		// no analysis done, so assume the file needs cleaning
		must_clean = true;
		if (gOptions.analyze) {
			std::wcout << "Image file '" << inputFile << "' forced to be a Z-zero normals texture.\n";
		}
	}
	else if (gOptions.inputXYonly) {
		image_type = IMAGE_TYPE_NORMAL_XY_ONLY;
		// no analysis done, so assume the file needs cleaning
		must_clean = true;
		if (gOptions.analyze) {
			std::wcout << "Image file '" << inputFile << "' forced to be a normals texture with only X and Y input.\n";
		}
	}
	else if (gOptions.inputHeightfield) {
		image_type = IMAGE_TYPE_HEIGHTFIELD;
		if (gOptions.analyze) {
			std::wcout << "Image file '" << inputFile << "' forced to be considered as a grayscale heightfield.\n";
		}
	}
	// Nothing forced, so analyze enough to establish an image type
	else if ((image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_FULL) && (image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_XY)) {
		image_type = IMAGE_TYPE_NORMAL_FULL;
		if (gOptions.analyze) {
			std::wcout << "Image file '" << inputFile << "' is a standard normals texture.\n";
		}
	}
	else if ((image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_ZERO) && (image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_XY)) {
		image_type = IMAGE_TYPE_NORMAL_ZERO;
		if (gOptions.analyze) {
			std::wcout << "Image file '" << inputFile << "' is a Z-zero normals texture, with Z ranging from 0.0 to 1.0.\n";
		}
	}
	else if (image_field_bits & IMAGE_GRAYSCALE) {
		image_type = IMAGE_TYPE_HEIGHTFIELD;
		if (gOptions.analyze) {
			std::wcout << "Image file '" << inputFile << "' is a heightfield (grayscale) texture.\n";
		}
	}
	else {
		// We're really not sure at this point. Try some "close enough" tests. Could get more refined here.
		must_clean = true;
		// All the same but (for example) X and Y out of bounds?
		if (image_field_bits & IMAGE_ALL_SAME) {
			image_type = IMAGE_TYPE_HEIGHTFIELD;
			if (gOptions.analyze) {
				std::wcout << "Image file '" << inputFile << "' is perhaps a heightfield (or nothing - all texels are the same).\n";
			}
		}
		// Were the X and Y coordinates reasonable or not?
		//else if (!(image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_XY)) {
		// If less than 95% of XY's were reasonable, consider the thing likely to be a heightfield
		else if ((normalizable_xy[2] + normalizable_xy[1] + normalizable_xy[0]) < (1.0f - gOptions.errorTolerance) * image_size ||
			// OR, if the average of the X values > 0.02 or Y value > 0.02, it seems like a weirdly biased normal map
			(abs((float)(xsum / (double)image_size)) > gOptions.errorToleranceXY || abs((float)(ysum / (double)image_size)) > gOptions.errorToleranceXY)) {

			// the X and Y values are out of range, so assume it's a heightfield
			image_type = IMAGE_TYPE_HEIGHTFIELD;
			if (gOptions.analyze) {
				std::wcout << "Image file '" << inputFile << "' is probably a heightfield texture.\n";
				// why
				if ((normalizable_xy[2] + normalizable_xy[1] + normalizable_xy[0]) < (1.0f - gOptions.errorTolerance) * image_size) {
					std::wcout << "  Reason: there are many XY pairs that have a vector length notably longer than 1.0; " << 100.0f * (image_size - (normalizable_xy[2] + normalizable_xy[1] + normalizable_xy[0])) / (float)image_size << "%.\n";
				}
				if (abs((float)(xsum / (double)image_size)) > gOptions.errorToleranceXY) {
					std::wcout << "  Reason: the average value of X " << (float)(xsum / (double)image_size) << " is larger in magnitude than the XY error tolerance of " << gOptions.errorToleranceXY << ".\n";
				}
				if (abs((float)(ysum / (double)image_size)) > gOptions.errorToleranceXY) {
					std::wcout << "  Reason: the average value of Y " << (float)(ysum / (double)image_size) << " is larger in magnitude than the XY error tolerance of " << gOptions.errorToleranceXY << ".\n";
				}
			}
		}
		else {
			// Which interpretation of Z - zero or negative 1 minimum - gives a smaller difference in normal lengths computed?
			// Alternate test: Which has fewer values outside the bounds?
			if (z_outside_bounds < z_outside_zzero_bounds) {
			// Whichever it is, the input file could be a heightfield or a file with XY only. Right now we favor XY only, but that could be changed or modified with options.
			//if (maxnormallength - minnormallength <= zzero_maxnormallength - zzero_minnormallength) {
				// More likely Z -1 to 1
				// how much did the z value vary from expected?
				// stricter: if (zmaxabsdiff > 2) {
				if (z_outside_bounds > gOptions.errorTolerance * image_size) {
					image_type = IMAGE_TYPE_NORMAL_XY_ONLY;
					reason_xy = true;
					if (gOptions.analyze) {
						std::wcout << "Image file '" << inputFile << "' is probably an XY-only normals texture.\n";
						std::wcout << "  Reason: the percentage of stored Z -1 to 1 values that do not make normalized XYZ vectors is " << 100.0f * (float)z_outside_bounds/(float)image_size << "%,\n    larger than the error tolerance of " << 100.0f * gOptions.errorTolerance << "%.\n";
						std::wcout << "    The Z values were found to be as far off as " << zmaxabsdiff << " levels in expected value.\n";
					}
				}
				else {
					image_type = IMAGE_TYPE_NORMAL_FULL;
					if (image_field_bits & IMAGE_VALID_ZVAL_NONNEG) {
						if (gOptions.analyze) {
							std::wcout << "Image file '" << inputFile << "' may be a standard normals texture.\n";
							std::wcout << "  Reason: no Z -1 to 1 range values are negative.\n";
						}
					}
					else {
						// has negative Z values
						if (gOptions.analyze) {
							std::wcout << "Image file '" << inputFile << "' might be a standard normals texture.\n";
							std::wcout << "  Reason: some Z -1 to 1 range values are negative.\n";
						}
					}
					if (gOptions.analyze) {
						std::wcout << "  Reason: the percentage of stored Z -1 to 1 values that do not make normalized XYZ vectors is " << 100.0f * (float)z_outside_bounds / (float)image_size << "%,\n    smaller than the error tolerance of " << 100.0f * gOptions.errorTolerance << "%.\n";
					}
				}
				if (gOptions.analyze) {
					std::wcout << "    " << 100.0f * (float)(image_size - z_outside_bounds) / (float)image_size << "% of Z's are valid for range -1 to 1, " << 100.0f * (float)(image_size - z_outside_zzero_bounds) / (float)image_size << "% are valid for range 0 to 1.\n";
				}
			}
			else {
				// More likely Z 0 to 1
				// how much did the z value vary from expected?
				// stricter: zmaxabsdiff_zero > 2
				if (z_outside_zzero_bounds > gOptions.errorTolerance * image_size) {
					image_type = IMAGE_TYPE_NORMAL_XY_ONLY;
					reason_xy = true;
					if (gOptions.analyze) {
						std::wcout << "Image file '" << inputFile << "' is probably an XY-only normals texture.\n";
						std::wcout << "  Reason: the percentage of stored Z 0 to 1 values that do not make normalized XYZ vectors is " << 100.0f * (float)z_outside_zzero_bounds / (float)image_size << "%,\n    larger than the error tolerance of " << 100.0f * gOptions.errorTolerance << "%.\n";
						std::wcout << "    The Z values were found to be as far off as " << zmaxabsdiff_zero << " levels in expected value.\n";
					}
				}
				else {
					image_type = IMAGE_TYPE_NORMAL_ZERO;
					if (gOptions.analyze) {
						std::wcout << "Image file '" << inputFile << "' may be a Z-zero normals texture, with Z ranging from 0.0 to 1.0.\n";
						std::wcout << "  Reason: the percentage of stored Z 0 to 1 values that do not make normalized XYZ vectors is " << 100.0f * (float)z_outside_zzero_bounds / (float)image_size << "%,\n    lower than the error tolerance of " << 100.0f * gOptions.errorTolerance << "%.\n";
					}
				}
				if (gOptions.analyze) {
					std::wcout << "    " << 100.0f * (float)(image_size - z_outside_zzero_bounds) / (float)image_size << "% of Z's are valid for range 0 to 1, " << 100.0f * (float)(image_size - z_outside_bounds) / (float)image_size << "% are valid for range -1 to 1.\n";
				}
			}
		}
	}
	assert(image_type);

	// warn about color depth mismatch
	if (colortype == LCT_RGB || colortype == LCT_RGBA) {
		if (bitdepth > 8) {
			std::wcout << "  WARNING: input file has " << bitdepth << " bits of precision per channel, but this program currently outputs only 8 bits per channel.\n";
		}
		else if (bitdepth < 8) {
			std::wcout << "  WARNING: input file has " << bitdepth << " bits of precision per channel, but this program currently uses 8 bits per channel. Conversion may be off.\n";
		}
	}

	// additional info for if particular channels are constant
	if (!(image_field_bits & IMAGE_ALL_SAME)) {
		if (image_field_bits & IMAGE_R_CHANNEL_ALL_SAME) {
			if (gOptions.analyze) {
				std::wcout << "  The red channel values in file '" << inputFile << "' all have the same value: " << first_pixel[0] << ".\n";
			}
		}
		if (image_field_bits & IMAGE_G_CHANNEL_ALL_SAME) {
			if (gOptions.analyze) {
				std::wcout << "  The green channel values in file '" << inputFile << "' all have the same value: " << first_pixel[1] << ".\n";
			}
		}
		if (image_field_bits & IMAGE_B_CHANNEL_ALL_SAME) {
			if (gOptions.analyze) {
				std::wcout << "  The blue channel values in file '" << inputFile << "' all have the same value: " << first_pixel[2] << ".\n";
			}
		} else if (b_less_than_255 < (float)image_size * gOptions.errorTolerance) {
			if (gOptions.analyze) {
				std::wcout << "  Only " << 100.0f * (float)b_less_than_255 / (float)image_size << "% (" << b_less_than_255 << " texels) of the blue channel values are less than 255.\n";
			}
		}
	}

	if (gOptions.analyze) {
		// try to examine X and Y fluctuations to determine if this is OpenGL or DirectX style.
		// Sadly, this idea doesn't seem to work - it's a coin-flip.
		//if (image_type != IMAGE_TYPE_HEIGHTFIELD) {
		//	examineForDirectXStyle(&imageInfo);
		//}

		if (image_type == IMAGE_TYPE_NORMAL_FULL || image_type == IMAGE_TYPE_NORMAL_ZERO) {
			if (image_field_bits & IMAGE_ALL_SAME) {
				std::wcout << "  All values in the texture are the same (normals do not vary): r = " << first_pixel[0] << ", g = " << first_pixel[1] << ", b = " << first_pixel[2] << "; X = " << x << ", Y = " << y << ", Z = " << z << "\n";
			}
		}
		else if (image_type == IMAGE_TYPE_NORMAL_XY_ONLY) {
			if (image_field_bits & IMAGE_ALL_SAME) {
				std::wcout << "  All values in the texture are the same (normals do not vary): r = " << first_pixel[0] << ", g = " << first_pixel[1] << ", b = " << first_pixel[2] << "; X = " << x << ", Y = " << y << ", Z = " << zzero << "\n";
			}
			else if ((image_field_bits & IMAGE_R_CHANNEL_ALL_SAME) && (image_field_bits & IMAGE_G_CHANNEL_ALL_SAME)) {
				std::wcout << "  All XY values in the texture are the same (normals do not vary): r = " << first_pixel[0] << ", g = " << first_pixel[1] << "; X = " << x << ", Y = " << y << "\n";
			}
		}

		// full analysis for various types
		switch (image_type) {
		case 0x0:
			// currently should never reach here
			assert(0);
			break;
		case IMAGE_TYPE_NORMAL_FULL:
			must_clean |= XYanalysis(image_field_bits, image_size, normalizable_xy, xy_outside_bounds);
			must_clean |= Zanalysis(image_type, image_field_bits, image_size, normal_length_zneg, minnormallength, maxnormallength, normal_zval_nonnegative, min_z, z_outside_bounds, zmaxabsdiff);

			std::wcout << "  Average XYZ values, assuming Z -1 to 1: X " << (float)(xsum / (double)image_size) << ", Y " << (float)(ysum / (double)image_size) << ", Z " << (float)(zsum / (double)image_size) << "\n";
			std::wcout << "  XYZ values ranges, assuming Z -1 to 1:\n    X " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[0]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[0])
				<< "\n    Y " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[1]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[1])
				<< "\n    Z " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[2]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[2]) << "\n";
			std::wcout << "  Color channels range: r " << chan_min[0] << " to " << chan_max[0] << "; g " << chan_min[1] << " to " << chan_max[1] << "; b " << chan_min[2] << " to " << chan_max[2] << ".\n";
			break;
		case IMAGE_TYPE_NORMAL_ZERO:
			must_clean |= XYanalysis(image_field_bits, image_size, normalizable_xy, xy_outside_bounds);
			must_clean |= Zanalysis(image_type, image_field_bits, image_size, normal_length_zzero, zzero_minnormallength, zzero_maxnormallength, 0, zzero_min_z, z_outside_zzero_bounds, zmaxabsdiff_zero);

			std::wcout << "  Average XYZ values, assuming Z 0 to 1: X " << (float)(xsum / (double)image_size) << ", Y " << (float)(ysum / (double)image_size) << ", Z " << (float)(zzerosum / (double)image_size) << "\n";
			std::wcout << "  XYZ values ranges, assuming Z 0 to 1:\n    X " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[0]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[0])
				<< "\n    Y " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[1]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[1])
				<< "\n    Z " << CONVERT_CHANNEL_ZERO_DIRECT(chan_min[2]) << " to " << CONVERT_CHANNEL_ZERO_DIRECT(chan_max[2]) << "\n";
			std::wcout << "  Color channels range: r " << chan_min[0] << " to " << chan_max[0] << "; g " << chan_min[1] << " to " << chan_max[1] << "; b " << chan_min[2] << " to " << chan_max[2] << ".\n";
			break;
		case IMAGE_TYPE_NORMAL_XY_ONLY:
			// no Zanalysis, since with XY we always derive Z precisely
			// If a reason for categorizing the XY was already given, don't repeat here
			if (!reason_xy) {
				std::wcout << "  Note: For the input Z's, assuming Z -1 to 1, " << 100.0f * (float)z_outside_bounds / (float)image_size << "% of the texel Z values\n    are more than 2 levels from being properly normalized.\n";
				std::wcout << "    The Z values were found to be as far off as " << zmaxabsdiff << " levels in expected value.\n";
			}
			std::wcout << "  Average input Z channel values, if mapped from -1 to 1: " << (float)(zsum / (double)image_size) << "\n";
			std::wcout << "    and Z would range from " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[2]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[2]) << "\n";

			must_clean |= XYanalysis(image_field_bits, image_size, normalizable_xy, xy_outside_bounds);
			std::wcout << "  Average XY values: X " << (float)(xsum / (double)image_size) << ", Y " << (float)(ysum / (double)image_size) << "\n";
			std::wcout << "  X and Y values ranges:\n    X " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[0]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[0])
				<< "\n    Y " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[1]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[1]) << "\n";
			std::wcout << "  Color channels range: r " << chan_min[0] << " to " << chan_max[0] << "; g " << chan_min[1] << " to " << chan_max[1] << "; b " << chan_min[2] << " to " << chan_max[2] << ".\n";
			break;
		case IMAGE_TYPE_HEIGHTFIELD:
			if (image_field_bits & IMAGE_ALL_SAME) {
				if (image_field_bits & IMAGE_GRAYSCALE) {
					std::wcout << "  All values are the same (heights do not vary): r = " << first_pixel[0] << ", g = " << first_pixel[1] << ", b = " << first_pixel[2] << ".\n";
				}
				else {
					// gray computed earlier, same for all
					std::wcout << "  All values are the same (heights do not vary): gray level is " << gray << " from r = " << first_pixel[0] << ", g = " << first_pixel[1] << ", b = " << first_pixel[2] << ".\n";
				}
			}
			else {
				std::wcout << "  Gray-level values vary between " << gray_min << " and " << gray_max << ".\n";
				if (!(image_field_bits & IMAGE_GRAYSCALE)) {
					std::wcout << "  Color channels vary: r " << chan_min[0] << " to " << chan_max[0] << "; g " << chan_min[1] << " to " << chan_max[1] << "; b " << chan_min[2] << " to " << chan_max[2] << ".\n";
				}
			}
			// if must_clean is set, then we're not sure this is a heightfield texture, so show X and Y's
			if (must_clean) {
				XYanalysis(image_field_bits, image_size, normalizable_xy, xy_outside_bounds);
				std::wcout << "  Average XY values: X " << (float)(xsum / (double)image_size) << ", Y " << (float)(ysum / (double)image_size) << "\n";
				std::wcout << "  X and Y values ranges:\n    X " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[0]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[0])
					<< "\n    Y " << CONVERT_CHANNEL_FULL_DIRECT(chan_min[1]) << " to " << CONVERT_CHANNEL_FULL_DIRECT(chan_max[1]) << "\n";
			}
			break;
		}

		// whitespace between analyses, and flush
		std::wcout << "\n" << std::flush;

		// Add to proper summary list. We don't add the directory (could do that with inputPathAndFile).
		std::wstring if_string = inputFile;
		switch (image_type) {
		case 0x0:
			gListUnknown += L" " + if_string;
			// currently should never reach here
			assert(0);
			break;
		case IMAGE_TYPE_NORMAL_FULL:
			gListStandard += L" " + if_string;
			if (must_clean) {
				gListStandardDirty += L" " + if_string;
			}
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

		// special list: texture has no bumps, no matter what it is
		if (image_field_bits & IMAGE_ALL_SAME) {
			gListAllSame += L" " + if_string;
		}
	}

	// Now see if we want to output the file
	if ((image_type > 0) &&
		(gOptions.outputAll ||
			((must_clean||(image_type == IMAGE_TYPE_HEIGHTFIELD)) && gOptions.outputClean))) {
		// Output all or output only files that need to be cleaned

		// Do we flip the Y axis on output? Do only if parity doesn't match
		bool y_flip = (gOptions.inputDirectX != gOptions.outputDirectX);

		// make output image info
		progimage_info destination;
		destination.width = imageInfo.width;
		destination.height = imageInfo.height;
		destination.image_data.resize(destination.width * destination.height * 3 * sizeof(unsigned char), 0x0);

		bool perform_output = true;
		if (image_type == IMAGE_TYPE_HEIGHTFIELD) {
			if (gOptions.outputHeatmap) {
				std::wcout << "WARNING: input image '" << inputFile << "' is categorized as a heightfield, so heatmap output is meaningless and so is not done.\n\n" << std::flush;
				perform_output = false;
			}
			else {
				// For heightfields, we pay attention to just the output format specified.
				y_flip = gOptions.outputDirectX;

				// convert from heightfield
				if (gOptions.verbose) {
					std::wcout << ">> Converting heightfield to normals texture.\n" << std::flush;
				}
				convertHeightfieldToXYZ(&destination, &imageInfo, gOptions.heightfieldScale, (image_field_bits & IMAGE_GRAYSCALE) ? true : false, y_flip, gOptions.borderHeightfield);
			}
		}
		else {
			// input is a normals texture of some type, so output it, cleaning as needed, flipping y if needed.
			if (gOptions.verbose) {
				std::wcout << ">> Readying output " << (gOptions.outputHeatmap ? "heatmap" : "normals") << " texture.\n" << std::flush;
			}
			cleanAndCopyNormalTexture(&destination, &imageInfo, image_type, gOptions.outputZzeroToOne, y_flip, gOptions.outputHeatmap, reason_xy);
		}

		if (perform_output) {
			// The outputFile has the same file name as the inputFile, possibly different path.

			wchar_t outputPathAndFile[MAX_PATH];
			// note that outputDirectory already has a "/" at the end, due to manipulation at the start of the program
			wcscpy_s(outputPathAndFile, MAX_PATH, gOptions.outputDirectory.c_str());

			// We always use PNG as output, so change the .TGA suffix if need be.
			wchar_t fileName[MAX_PATH];
			wcscpy_s(fileName, MAX_PATH, inputFile);
			size_t suffix_pos = wcslen(fileName) - 4;
			if (_wcsicmp(&fileName[suffix_pos], L".tga") == 0) {
				assert(file_type == TGA_EXTENSION_FOUND);
				// change to _TGA.PNG
				fileName[suffix_pos] = 0;
				wcscat_s(fileName, MAX_PATH, L"_tga.png");
			}

			// add the file name to the output directory
			wcscat_s(outputPathAndFile, MAX_PATH, fileName);

			rc = writepng(&destination, 3, outputPathAndFile);
			if (rc != 0)
			{
				reportReadError(rc, outputPathAndFile);
				// quit
				return rc_output;
			}
			if (gOptions.verbose) {
				std::wcout << "  New texture '" << outputPathAndFile << "' created.\n";
				// Note what's been done
				if (gOptions.outputDirectX) {
					std::wcout << "    Output to DirectX (Y down) format.\n";
				}
				else if (gOptions.inputDirectX && !gOptions.outputDirectX) {
					std::wcout << "    Converting from DirectX to OpenGL format.\n";
				}
				if (gOptions.outputZzeroToOne) {
					std::wcout << "    Outputting Z to the range 0.0 to 1.0.\n";
				}
				if (image_type == IMAGE_TYPE_HEIGHTFIELD) {
					std::wcout << "    Converted to a heightfield using a scale factor of " << gOptions.heightfieldScale <<
						" and " << (gOptions.borderHeightfield ? "border" : "tiling") << " at the edges.\n";
				}
				else {
					if (!(image_field_bits |= IMAGE_VALID_NORMALS_XY)) {
						std::wcout << "    Cleanup: X,Y pairs were renormalized to be a length of 1.0 or less.\n";
					}
					if (((image_type == IMAGE_TYPE_NORMAL_FULL) && !(image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_FULL)) ||
						((image_type == IMAGE_TYPE_NORMAL_ZERO) && !(image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_ZERO))) {
						std::wcout << "    Cleanup: Normals were renormalized to be the correct length.\n";
					}
					else if (((image_type == IMAGE_TYPE_NORMAL_FULL) && !(image_field_bits & IMAGE_VALID_NORMALS_FULL)) ||
						((image_type == IMAGE_TYPE_NORMAL_ZERO) && !(image_field_bits & IMAGE_VALID_NORMALS_ZERO))) {
						std::wcout << "    Cleanup: Normals were cleaned up to roundtrippable triplets.\n";
					}

					if ((image_type == IMAGE_TYPE_NORMAL_FULL) && !(image_field_bits & IMAGE_VALID_ZVAL_NONNEG)) {
						std::wcout << "    Cleanup: Z values were adjusted to not be negative.\n";
					}
				}
				std::wcout << "\n" << std::flush;
			}
			rc_output = true;
		}
		writepng_cleanup(&destination);
	}

	// clean up input image info.
	readImage_cleanup(1, &imageInfo);
	return rc_output;
}

bool XYanalysis(int image_field_bits, int image_size, int* normalizable_xy, int xy_outside_bounds)
{
	bool must_clean = false;
	if (image_field_bits & IMAGE_VALID_NORMALS_XY) {
		assert(xy_outside_bounds == 0);
		std::wcout << "  The X and Y coordinates always form vectors that are length 1.0 or less.\n";
	}
	else if (image_field_bits & IMAGE_ALMOST_VALID_NORMALS_XY) {
		assert(xy_outside_bounds == 0);
		must_clean = true;
		std::wcout << "  The X and Y coordinates form vectors 1 levels longer than 1.0 for " << 100.0f * (float)normalizable_xy[1] / (float)image_size << "% (" << normalizable_xy[1] << " texels) of the texels.\n";
	}
	else if (image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_XY) {
		assert(xy_outside_bounds == 0);
		must_clean = true;
		std::wcout << "  The X and Y coordinates form vectors 2 levels longer than 1.0 for " << 100.0f * (float)normalizable_xy[2] / (float)image_size << "% of the texels,\n    and 1 level longer for " << 100.0f * (float)normalizable_xy[1] / (float)image_size << "%, total (" << normalizable_xy[1]+ normalizable_xy[2] << " texels).\n";
	}
	else {
		must_clean = true;
		std::wcout << "  *Problem: the X and Y coordinates form vectors considerably longer than 1.0 for " << 100.0f * (float)xy_outside_bounds / (float)image_size << "% of the texels,\n    only 2 levels longer for " << 100.0f * (float)normalizable_xy[2] / (float)image_size << "%, and just 1 level longer for " << 100.0f * (float)normalizable_xy[1] / (float)image_size << "%.\n";
	}

	return must_clean;
}

bool Zanalysis(int image_type, int image_field_bits, int image_size, int *normal_length_zneg, float minnormallength, float maxnormallength, int normal_zval_nonnegative, float min_z, int z_outside_bounds, int zmaxabsdiff)
{
	bool must_clean = false;
	if (image_type == IMAGE_TYPE_NORMAL_FULL) {
		if (image_field_bits & IMAGE_VALID_NORMALS_FULL) {
			std::wcout << "  The texture has all perfect, roundtrippable z-values.\n";
		}
		else {
			must_clean = true;
			if (image_field_bits & IMAGE_ALMOST_VALID_NORMALS_FULL) {
				std::wcout << "  Some texels do not have perfect z-values,\n    but all Z's are within 1 level of the expected values.\n  " << 100.0f * (float)normal_length_zneg[0] / (float)image_size << "% of the XYZ normals are perfectly normalized.\n";
			}
			else if (image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_FULL) {
				std::wcout << "  Some texels do not have perfect z-values,\n    but all Z's are within 2 levels of the expected values.\n";
				std::wcout << "  " << 100.0f * (float)normal_length_zneg[0] / (float)image_size << "% of the XYZ normals are perfectly normalized,\n    "
					<< 100.0f * (float)normal_length_zneg[1] / (float)image_size << "% are just 1 level away, and\n    "
					<< 100.0f * (float)normal_length_zneg[2] / (float)image_size << "% are 2 levels away.\n";
			}
			else {
				// some Z values were way out of range
				std::wcout << "  *Problem: the Z values (assuming range -1 to 1) were found to be as far off as " << zmaxabsdiff << " levels in expected value.\n";
				std::wcout << "  " << 100.0f * (float)z_outside_bounds / (float)image_size << "% of the texel Z values are more than two from being properly normalized.\n";
				std::wcout << "  " << 100.0f * (float)normal_length_zneg[0] / (float)image_size << "% of the XYZ normals are perfectly normalized,\n    "
					<< 100.0f * (float)normal_length_zneg[1] / (float)image_size << "% are just 1 level away, and\n    "
					<< 100.0f * (float)normal_length_zneg[2] / (float)image_size << "% are 2 levels away.\n";
			}
		}

		std::wcout << "  Normal lengths found in original data, assuming Z -1 to 1:\n    minimum " << minnormallength << " and maximum " << maxnormallength << "\n";
		if (!(image_field_bits & IMAGE_VALID_ZVAL_NONNEG)) {
			if (!gOptions.allowNegativeZ) {
				must_clean = true;
			}
			std::wcout << "  *Problem: some Z values were found to be negative, " << 100.0f * (float)(image_size-normal_zval_nonnegative) / (float)image_size << "% (" << image_size - normal_zval_nonnegative << " texels).\n";
		}
		std::wcout << "  The lowest Z value found was " << min_z << "\n";
	}
	else {
		assert(image_type == IMAGE_TYPE_NORMAL_ZERO);
		if (image_field_bits & IMAGE_VALID_NORMALS_ZERO) {
			std::wcout << "  The texture has all perfect, roundtrippable z-values.\n";
		}
		else {
			must_clean = true;
			if (image_field_bits & IMAGE_ALMOST_VALID_NORMALS_ZERO) {
				std::wcout << "  Some texels do not have perfect z-values,\n    but all Z's are within 1 level of the expected values.\n  " << 100.0f * (float)normal_length_zneg[0] / (float)image_size << "% of the XYZ normals are perfectly normalized.\n";
			}
			else if (image_field_bits & IMAGE_ALMOST2_VALID_NORMALS_ZERO) {
				std::wcout << "  Some texels do not have perfect z-values,\n    but all Z's are within 2 levels of the expected values.\n";
				std::wcout << "  " << 100.0f * (float)normal_length_zneg[0] / (float)image_size << "% of the XYZ normals are perfectly normalized,\n    " 
					<< 100.0f * (float)normal_length_zneg[1] / (float)image_size << "% are just 1 level away, and\n    " 
					<< 100.0f * (float)normal_length_zneg[2] / (float)image_size << "% are 2 levels away.\n";
			}
			else {
				// some Z values were way out of range
				std::wcout << "  *Problem: the stored z values (assuming range 0 to 1) were found to be as far off as " << zmaxabsdiff << " levels in expected value.\n";
				std::wcout << "  " << 100.0f * (float)z_outside_bounds / (float)image_size << "% (" << z_outside_bounds << " texels) of the texel Z values are more than two from being properly normalized.\n";
				std::wcout << "  " << 100.0f * (float)normal_length_zneg[0] / (float)image_size << "% of the XYZ normals are perfectly normalized,\n    "
					<< 100.0f * (float)normal_length_zneg[1] / (float)image_size << "% are just 1 level away, and\n    " 
					<< 100.0f * (float)normal_length_zneg[2] / (float)image_size << "% are 2 levels away.\n";
			}
		}

		std::wcout << "  Normal lengths found in original data, assuming Z 0 to 1:\n    minimum " << minnormallength << " and maximum " << maxnormallength << "\n";
		std::wcout << "  The lowest Z value found was " << min_z << "\n";
	}
	return must_clean;
}

void printHelp()
{
	if (!gOptions.verbose) {
		// was already output if verbose is on.
		std::wcerr << "NormalTextureProcessor version " << VERSION_STRING << "\n";
	}
	std::wcerr << "usage: NormalTextureProcessor [-options] [file1.png file2.png...]\n";
	std::wcerr << "  -a - analyze texture files read in and output report.\n";
	std::wcerr << "  -idir path - give a relative or absolute path for what directory of files to read in.\n";
	std::wcerr << "  -izneg - assume all input files are normals textures with Z's between -1 and 1.\n";
	std::wcerr << "  -izzero - assume all input files are normals textures with Z's between 0 and 1.\n";
	std::wcerr << "  -ixy - assume all input files have just X and Y values (Z channel is something else).\n";
	std::wcerr << "  -idx - assume normals texture input files use DirectX-style (Y down) mapping for the green channel.\n";
	std::wcerr << "  -ihf - assume all input files are heightfields (converted to grayscale).\n";
	std::wcerr << "  -oall - output all texture files read in, as possible.\n";
	std::wcerr << "  -oclean - clean and output all texture files that are not considered perfect.\n";
	std::wcerr << "  -odir path - give a relative or absolute path for what directory to output files.\n";
	std::wcerr << "  -ozzero - output files so that their blue channel value maps Z from 0 to 1 instead of -1 to 1.\n";
	std::wcerr << "  -allownegz - do not flag problems with Z being negative and maintain Z's sign on output.\n";
	std::wcerr << "  -odx - output files using the DirectX-style (Y down) mapping for the green channel.\n";
	std::wcerr << "  -oheatmap - output red for XY > 1.0, green for non-matching Z's, blue for negative Z's.\n";
	std::wcerr << "  -hfs # - for heightfields, specify output scale.\n";
	std::wcerr << "  -hborder - treat heightfields as having a border (instead of wrapping around) when converting to a normals texture.\n";
	std::wcerr << "  -etol # - when classifying a texture, what percentage of texels can fail. Default tolerance is 2%.\n";
	std::wcerr << "  -etolxy # - when classifying a texture, how much the X and Y component averages can differ from 0. Default 0.10.\n";
	std::wcerr << "  -csv - output all texel values and possible conversions to a comma-separated value file (for a spreadsheet).\n";
	std::wcerr << "  -csve - output only texel values far out of range to a comma-separated value file (for a spreadsheet).\n";
	std::wcerr << "  -v - verbose, explain everything going on. Default: display only warnings and errors.\n";
	std::wcerr << std::flush;
}

void reportReadError(int rc, const wchar_t* filename)
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

void saveErrorForEnd()
{
	wprintf(gErrorString);
	wcscat_s(gConcatErrorString, CONCAT_ERROR_LENGTH, L"  ");
	wcscat_s(gConcatErrorString, CONCAT_ERROR_LENGTH, gErrorString);
}

//================================ Image Manipulation ====================================

// My theory was that if we looked for peaks/valleys vs. "cusps" (peak in one direction, valley in another)
// that there would be more peaks/valleys for OpenGL-style, cusps for DirectX style.
// Imagine a simple bump with a single peak in an OpenGL-style texture. That peak would look like a cusp
// if we interpreted the texture as a DirectX texture.
// Unfortunately, this doesn't work.
// TODO: another idea is to unscramble the normal map to get a height map. In that process I can imagine
// that the range of elevations is lower for the correct style than for the opposite style.
//bool examineForDirectXStyle(progimage_info* src)
//{
//	int row, col;
//	unsigned char* src_data = &src->image_data[0];
//	bool clamp_border = false;
//
//	float* gridx = (float*)malloc(src->height * src->width * sizeof(float));
//	float* gridy = (float*)malloc(src->height * src->width * sizeof(float));
//	int index = 0;
//
//	// populate grid with converted values
//	for (row = 0; row < src->height; row++)
//	{
//		for (col = 0; col < src->width; col++)
//		{
//			CONVERT_CHANNEL_TO_FULL(src_data[0], gridx[index]);
//			CONVERT_CHANNEL_TO_FULL(src_data[1], gridy[index]);
//
//			// next texel
//			src_data += 3;
//			index++;
//		}
//	}
//
//	// now look at values: is a texel a peak, valley, or cusp?
//	int parabola = 0;
//	int hyperbola = 0;
//	for (row = 0; row < src->height; row++)
//	{
//		// Ensure value is not negative by adding height to trow
//		int trow = (row + src->height - 1) % src->height;
//		int brow = (row + 1) % src->height;
//		if (clamp_border) {
//			// undo the wraps if we want to clamp to the border
//			if (trow >= src->height - 1)
//				trow = 0;
//			if (brow == 0)
//				brow = src->height - 1;
//		}
//		for (col = 0; col < src->width; col++)
//		{
//			bool xpeak = false;
//			bool xvalley = false;
//			bool ypeak = false;
//			bool yvalley = false;
//			int lcol = (col + src->width - 1) % src->width;
//			int rcol = (col + 1) % src->width;
//			if (clamp_border) {
//				// undo the wraps if we want to clamp to the border
//				if (lcol >= src->width - 1)
//					lcol = 0;
//				if (rcol == 0)
//					rcol = src->width - 1;
//			}
//			if (gridx[row * src->width + col] > gridx[row * src->width + lcol] &&
//				gridx[row * src->width + col] > gridx[row * src->width + rcol]) {
//				xpeak = true;
//			}
//			else if (gridx[row * src->width + col] < gridx[row * src->width + lcol] &&
//				gridx[row * src->width + col] < gridx[row * src->width + rcol]) {
//				xvalley = true;
//			}
//			if (gridy[row * src->width + col] > gridy[trow * src->width + col] &&
//				gridy[row * src->width + col] > gridy[brow * src->width + col]) {
//				ypeak = true;
//			}
//			else if (gridy[row * src->width + col] < gridy[trow * src->width + col] &&
//				gridy[row * src->width + col] < gridy[brow * src->width + col]) {
//				yvalley = true;
//			}
//
//			// did we get a parabola or hyperbola?
//			if ((xpeak && ypeak) || (xvalley && yvalley)) {
//				parabola++;
//			}
//			else if ((xpeak && yvalley) || (xvalley && ypeak)) {
//				hyperbola++;
//			}
//
//			// next texel
//			index++;
//		}
//	}
//	std::wcout << "  I think it's a " << ((parabola >= hyperbola) ? "OpenGL-style" : "DirectX-style") << " normals texture because parabola = " << parabola << " and hyperbola = " << hyperbola << "\n";
//	free(gridx);
//	free(gridy);
//	return parabola >= hyperbola;
//}

// assumes 3 channels are input
void convertHeightfieldToXYZ(progimage_info* dst, progimage_info* src, float heightfieldScale, bool input_grayscale, bool y_flip, bool clamp_border)
{
	int row, col;
	// create temporary grayscale image of input and work off of that, putting the result in the output dst file
	progimage_info* phf = allocateGrayscaleImage(src);
	// A minor efficiency thing. If we know the incoming image is grayscale, we can just grab the red channel and be done.
	if (input_grayscale) {
		copyOneChannel(phf, CHANNEL_RED, src, LCT_RGB);
	}
	else {
		convertToGrayscale(phf, src, LCT_RGB);
	}
	unsigned char* phf_data = &phf->image_data[0];
	unsigned char* dst_data = &dst->image_data[0];

	for (row = 0; row < phf->height; row++)
	{
		// Ensure value is not negative by adding height to trow
		int trow = (row + phf->height - 1) % phf->height;
		int brow = (row + 1) % phf->height;
		if (clamp_border) {
			// undo the wraps if we want to clamp to the border
			if (trow >= phf->height - 1)
				trow = 0;
			if (brow == 0)
				brow = phf->height - 1;
		}
		for (col = 0; col < phf->width; col++)
		{
			int lcol = (col + phf->width - 1) % phf->width;
			int rcol = (col + 1) % phf->width;
			if (clamp_border) {
				// undo the wraps if we want to clamp to the border
				if (lcol >= phf->width - 1)
					lcol = 0;
				if (rcol == 0)
					rcol = phf->width - 1;
			}
			// From Real-Time Rendering, p. 214 referencing an article:
			// Eberly, David, "Reconstructing a Height Field from a Normal Map," Geometric Tools blog, May 3, 2006.
			// https://www.geometrictools.com/Documentation/ReconstructHeightFromNormals.pdf
			// Basically, take the height to the left and height to the right and use as the slope for X, etc.
			// Note how the height value actually at the texel is not used.
			// So, the maximum difference computed for X and Y is -1 to 1, scaled by the heightfieldScale value.
			float x = heightfieldScale * (phf_data[row * phf->width + lcol] - phf_data[row * phf->width + rcol]) / 255.0f;
			float y = heightfieldScale * (phf_data[trow * phf->width + col] - phf_data[brow * phf->width + col]) / 255.0f;
			if (!y_flip) {
				// Right, this is confusing, I admit it. The point here is that the y-flip is normally done by default
				// by the code above. So if the DirectX style is needed, it's already done. This is for OpenGL style.
				// The reason it's DirectX-style by default is due to the nature of the equations being done from the upper
				// left corner: trow < brow, just like lcol < rcol
				y = -y;
			}
			// assume Z value is 1.0 and use this length to normalize the normal
			float length = (float)sqrt(x * x + y * y + 1.0f);
			// normalize and map from XYZ [-1,1] to RGB
			// Always roundtrip the data. What's this mean? Right now we have an XYZ we've produced above. What can happen
			// is that, if we convert to RGB, then convert back to XYZ, normalize XYZ, then convert to RGB again, the second RGB will
			// be slightly different in Z value.
			//if (gOptions.roundtrip) {
				unsigned char r = *dst_data++ = (unsigned char)(((x / length + 1.0f) / 2.0f) * 255.0f + 0.5f);
				unsigned char g = *dst_data++ = (unsigned char)(((y / length + 1.0f) / 2.0f) * 255.0f + 0.5f);
				// To make a triplet round-tripable, you need to compute b from the r and g -> X and Y values, not X and Y themselves.
				CONVERT_CHANNEL_TO_FULL(r, x);
				CONVERT_CHANNEL_TO_FULL(g, y);
				float z;
				COMPUTE_Z_FROM_XY(x, y, z);
				if (gOptions.outputZzeroToOne) {
					CONVERT_ZERO_TO_CHANNEL(z, *dst_data++);
				}
				else {
					CONVERT_FULL_TO_CHANNEL(z, *dst_data++);
				}
			//} else {
			//	// could use CONVERT_FULL_TO_CHANNEL, but I think it's clearer to write it out here
			//	*dst_data++ = (unsigned char)(((x / length + 1.0f) / 2.0f) * 255.0f + 0.5f);
			//	*dst_data++ = (unsigned char)(((y / length + 1.0f) / 2.0f) * 255.0f + 0.5f);
			//	if (gOptions.outputZzeroToOne) {
			//		*dst_data++ = (unsigned char)((1.0f / length) * 255.0f + 0.5f);
			//	}
			//	else {
			//		*dst_data++ = (unsigned char)(((1.0f / length + 1.0f) / 2.0f) * 255.0f + 0.5f);
			//	}
			//}
		}
	}
}

// TODO: add conversion of normals to heightfield. Can we derive DirectX vs. OpenGL style from this info in some way? (correlation of bumps)

void cleanAndCopyNormalTexture(progimage_info* dst, progimage_info* src, int image_type, bool output_zzero, bool y_flip, bool heatmap, bool reason_xy)
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
			unsigned char heatmap_r, heatmap_g, heatmap_b;
			heatmap_r = 0;
			heatmap_g = 0;
			heatmap_b = 0;

			// are X and Y invalid, forming a vector > 1.0 in length?
			float xy_len2 = x * x + y * y;
			if (xy_len2 > 1.0f) {
				heatmap_r = 255;
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

			// y-flip, if needed
			if (y_flip) {
				y = -y;
			}

			// if we are told to preserve Z's sign, do so here
			if (!gOptions.outputZzeroToOne && gOptions.allowNegativeZ) {
				// Is the blue channel meaningful?
				if (image_type != IMAGE_TYPE_NORMAL_XY_ONLY) {
					// if blue channel value is negative (< 127) then negate Z
					if (src_data[2] < 128) {
						z = -z;
					}
				}
			}

			if (heatmap) {
				// red is XY's > 1, green is Z's not matching or out of bounds, blue is negative Z's
				float zdata, diff;
				unsigned char bcalc, bread;
				// is image a full or one we're guessing is XY-only?
				if (image_type == IMAGE_TYPE_NORMAL_FULL ||
					(reason_xy && image_type == IMAGE_TYPE_NORMAL_XY_ONLY)) {
					CONVERT_CHANNEL_TO_FULL(src_data[2], zdata);
					// convert both z's back to pixel space and use difference
					CONVERT_FULL_TO_CHANNEL(zdata, bread);
					CONVERT_FULL_TO_CHANNEL(z, bcalc);
					// 3 levels or more is bright green
					diff = abs(bread - bcalc) * 255.0f / 3.0f;
					if (diff > 255.0f) {
						diff = 255.0f;
					}
					heatmap_g = (unsigned char)diff;
					if (src_data[2] <= 127) {
						// just barely negative, by one level; precision
						heatmap_b = 255 - src_data[1];
					}
				}
				else if (image_type == IMAGE_TYPE_NORMAL_ZERO) {
					CONVERT_CHANNEL_TO_ZERO(src_data[2], zdata);
					// convert both z's back to pixel space and use difference
					CONVERT_ZERO_TO_CHANNEL(zdata, bread);
					CONVERT_ZERO_TO_CHANNEL(z, bcalc);
					diff = abs(bread - bcalc) * 255.0f / 3.0f;
					if (diff > 255.0f) {
						diff = 255.0f;
					}
					heatmap_g = (unsigned char)diff;
				}
				dst_data[0] = heatmap_r;
				dst_data[1] = heatmap_g;
				dst_data[2] = heatmap_b;
			}
			else {

				// and save; Z is (normally) guaranteed to not be negative because we computed it
				if (output_zzero) {
					// convert back to rgb
					CONVERT_Z_ZERO_TO_RGB(x, y, z, dst_data[0], dst_data[1], dst_data[2]);
				}
				else {
					// convert back to rgb
					CONVERT_Z_FULL_TO_RGB(x, y, z, dst_data[0], dst_data[1], dst_data[2]);
				}
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
		// Could add many more extensions to flag as being ignored...
		// JPG and BMP seem like the main ones.
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