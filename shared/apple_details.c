/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#ifdef __APPLE__

//
//
//

#include <CoreFoundation/CoreFoundation.h>
#include <sys/stat.h>
#include <libgen.h>

#include "common.h"

//
//
//

#define NUM_PARTS 2

//
//
//

typedef struct {
	char * result;
	const char * substr;
} SearchPart;

typedef struct {
	SearchPart parts[NUM_PARTS];
	int ndone;
} SearchState;

//
//
//

static int FindCharEarlier (const char * str, int chr)
{
	int offset = 0;
	
	while (str[offset] != chr) --offset; // n.b. assumes presence of character
	
	return offset;
}

//
//
//

static const bool IsSearchDone (const SearchState * ss)
{
	return NUM_PARTS == ss->ndone;
}

//
//
//

static void TryString (const void * str, SearchState * ss)
{
	if (IsSearchDone(ss)) return;
	
	CFIndex len = CFStringGetLength(str);
		   
	if (len >= PATH_MAX || 0 == len) return;
	
	/* ----- */
	
	for (int i = 0; i < NUM_PARTS; ++i)
	{
		if (*ss->parts[i].result) continue;
		
		/* ----- */
		
		// Optimistically load the string into the output.
		CFStringGetCString(str, ss->parts[i].result, len + 1, kCFStringEncodingUTF8);

		/* ----- */
		
		if (strstr(ss->parts[i].result, ss->parts[i].substr)) // success?
		{
			++ss->ndone;

			char * end = ss->parts[i].result + len;
			int offset = FindCharEarlier(end, '/');

			end[offset] = '\0';

			return;
		}
		
		*ss->parts[i].result = '\0'; // not found, so invalidate the load
	}
}

//
//
//

static void AuxSearch (const void * key, const void * value, void * context)
{
	SearchState * ss = context;

	/* ----- */
	
	if (CFStringGetTypeID() == CFGetTypeID(key)) TryString(key, ss);

	/* ----- */
	
	CFTypeID vtype = CFGetTypeID(value);
	
	if (CFStringGetTypeID() == vtype) TryString(value, ss);
	else if (CFDictionaryGetTypeID() == vtype && !IsSearchDone(ss)) CFDictionaryApplyFunction(value, AuxSearch, ss);
}

//
//
//

static void SearchPlist (const char * file_path, SearchState * ss)
{
	SInt32 error_code;
	CFDataRef plist_data;
	CFURLRef file_URL = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)file_path, strlen(file_path), false);

	if (CFURLCreateDataAndPropertiesFromResource(NULL, file_URL, &plist_data, NULL, NULL, &error_code))
	{
		CFPropertyListRef plist = CFPropertyListCreateWithData(kCFAllocatorDefault, plist_data, kCFPropertyListImmutable, NULL, NULL);
		
		if (CFGetTypeID(plist) == CFDictionaryGetTypeID()) CFDictionaryApplyFunction(plist, AuxSearch, ss);
		
		CFRelease(plist);
		CFRelease(plist_data);
	}
	
	CFRelease(file_URL);
}

//
//
//

static const char * ProbablyLaunchedFromXcode (const char * path)
{
	const char * build_part = strstr(path, "/Build/Products/");

	if (!strstr(path, "/Library/Developer/Xcode/DerivedData/")) build_part = NULL; // slight sanity check
	
	return build_part;
}

//
//
//

static SearchPart MakeSearchPart (char * buf, const char * substr)
{
	*buf = '\0';
	
	return (SearchPart) { .result = buf, .substr = substr };
}

//
//
//

static void CopyRange (char * dst, const char * begin, const char * end, bool skip)
{
	ptrdiff_t len = end - begin;
	
	strncpy(dst, begin + skip, len); // skip -> use range [begin + 1, end + 1)
	
	dst[len] = '\0';
}

//
//
//

static void SetUpPathsFromXcode (Paths * paths, const char * exe_path, const char * build_part)
{
	char plist_path[PATH_MAX];

	CopyRange(plist_path, exe_path, build_part, false);
	
	strcat(plist_path, "/SymbolCache/project.plist");
	
	/* ----- */
	
	SearchState ss = {
		MakeSearchPart(paths->Corona, "librtt/Corona/CoronaPublicTypes.h"),
		MakeSearchPart(paths->Lua, "external/lua-5.1.3/src/lua.h")
	};

	SearchPlist(plist_path, &ss);
}

//
//
//

static void SetUpPathsRelativeToExe (Paths * paths, const char * exe_path)
{
	char full[PATH_MAX];
	
	sprintf(full, "%s/Native/Corona/shared/include", exe_path);
	
	char real_path_name[PATH_MAX];
	
	realpath(full, real_path_name);
	
	sprintf(paths->Corona, "%s/Corona", real_path_name);
	sprintf(paths->Lua, "%s/lua", real_path_name);
}

//
//
//

static void GetExePath (lua_State * L, char * exe_path)
{
	lua_getglobal(L, "package"); // ..., package
	luaL_argcheck(L, lua_istable(L, -1), -1, "`package` table is missing");
	lua_getfield(L, -1, "cpath"); // ..., package, package.cpath
	luaL_argcheck(L, LUA_TSTRING == lua_type(L, -1), -1, "`package.cpath` not a string");
	
	/* ----- */
	
	const char * path = strstr(lua_tostring(L, -1), "/Corona Simulator.app");
	int offset = FindCharEarlier(path, ';');

	CopyRange(exe_path, path + offset, path, true); // omit the semi-colon, but include the slash

	lua_pop(L, 2); // ...
}

//
//
//

void FixLib (void)
{
#ifdef TARGET_CPU_ARM64
	const char * old_name = "libtcc_arm64.a";
#else
	// TODO? x86 version
#endif
	
	char old_buf[PATH_MAX];
	
	strcpy(old_buf, GetFileInTempDir(old_name));
	rename(old_buf, GetFileInTempDir("libtcc1.a"));
}

//
//
//

void MakeDirectory (const char * filename)
{
	mkdir(filename, 0777);
}

//
//
//

void SetUpPaths (lua_State * L, Paths * paths)
{
	char exe_path[PATH_MAX];
	
	GetExePath(L, exe_path);

	const char * build_part = ProbablyLaunchedFromXcode(exe_path);

	if (build_part) SetUpPathsFromXcode(paths, exe_path, build_part);
	else SetUpPathsRelativeToExe(paths, exe_path);
}

//
//
//

#endif
