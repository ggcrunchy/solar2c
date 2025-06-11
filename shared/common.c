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

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "miniz.h"

//
//
//

static int sPathForFileRef;

//
//
//

int ForEachFile (lua_State * L, TCCState * tcc, int (*action) (TCCState * tcc, const char * filename), const char * what)
{
	luaL_argcheck(L, lua_istable(L, 2), 2, "Expected array of files");
	lua_getfield(L, 1, "baseDir"); // tcc, list, baseDir?

	int dir_index = lua_gettop(L); // ensure positive
	
	for (size_t i = 1, n = lua_objlen(L, 2); i <= n; ++i)
	{
		lua_rawgeti(L, 2, (int)i); // tcc, list, baseDir?, name

		const char * resolved = GetResolvedFilename(L, dir_index + 1, dir_index); // list, baseDir?, resolved_name

		if (action(tcc, resolved)) return luaL_error(L, "failed to %s `%s`", what, resolved);
		
		lua_pop(L, 1); // tcc, list, baseDir?
	}
	
	return 0;
}

//
//
//

const char * GetResolvedFilename (lua_State * L, int file_index, int dir_index)
{
	luaL_checkstring(L, file_index);
	
	if (lua_type(L, dir_index) != LUA_TSTRING)
	{
		bool dir_absent = lua_isnoneornil(L, dir_index);
	
		// n.b. indices assumed to be > 0
		
		if (!dir_absent) lua_pushvalue(L, dir_index); // ...[, dir]
		
		lua_getref(L, sPathForFileRef); // ..., pathForFile
		lua_pushvalue(L, file_index); // ..., pathForFile, filename
		
		if (!dir_absent) lua_pushvalue(L, dir_index); // ..., pathForFile, filename[, dir]
		
		lua_call(L, dir_absent ? 1 : 2, 1); // ..., filename?
		
		if (lua_isnil(L, -1))
		{
			luaL_error(L, "Unable to load file: `%s`", lua_tostring(L, file_index));
		}
		
		lua_replace(L, file_index); // ..., filename, ...
	}
	
	else if (strcmp(lua_tostring(L, dir_index), "absolute") != 0)
	{
		luaL_error(L, "Invalid directory");
	}
	
	return lua_tostring(L, file_index);
}

//
//
//

static char tempfile_buf[PATH_MAX];

static size_t tempfile_offset;

//
//
//

const char * GetFileInTempDir (const char * file)
{
	if (file)
	{
		tempfile_buf[tempfile_offset] = '/';
		
		strcpy(tempfile_buf + tempfile_offset + 1, file);
	}
	
	else
	{
		tempfile_buf[tempfile_offset] = '\0';
	}

	return tempfile_buf;
}

//
//
//

void PrepareToUnzip (lua_State * L)
{
	lua_getglobal(L, "system"); // ..., system
	lua_getfield(L, -1, "pathForFile"); // ..., system, system.pathForFile
	luaL_argcheck(L, lua_isfunction(L, -1), -1, "`system.pathForFile` missing or not a function");
	lua_pushvalue(L, -1); // ..., system, system.pathForFile, system.pathForfile
	
	sPathForFileRef = lua_ref(L, 1); // ..., system, system.pathForFile; ref = system.pathForFile
	
	/* ----- */
	
	lua_pushliteral(L, ""); // ..., system, system.pathForFile, ""
	lua_getfield(L, -3, "TemporaryDirectory"); // ..., system, system.pathForFile, "" system.TemporaryDirectory
	luaL_argcheck(L, !lua_isnil(L, -1), -1, "`system.TemporaryDirectory` missing");
	lua_call(L, 2, 1); // ..., system, temp_dir_path
	
	strcpy(tempfile_buf, lua_tostring(L, -1));
	
	tempfile_offset = strlen(tempfile_buf);
	
	lua_pop(L, 2); // ...
}

//
//
//

static bool ShouldIgnore (const char * filename)
{
#ifdef WIN32
	return true;
#elif _MAC
	// Check for unrelated material introduced by the compressor on Mac.
	const char prefix[] = "__MACOSX/";
	
	return strncmp(filename, prefix, strlen(prefix)) == 0;
#endif 
}

//
//
//

void ExtractZip (const unsigned char buf[], const size_t buf_size)
{
	mz_zip_archive zip = { 0 };
	
	if (mz_zip_reader_init_mem(&zip, buf, buf_size, 0))
	{
		char filename[PATH_MAX];

		/* ----- */
		
		mz_uint n = mz_zip_reader_get_num_files(&zip);
		
		for (mz_uint i = 0; i < n; ++i)
		{
			if (!mz_zip_reader_is_file_a_directory(&zip, i)) continue;
			
			mz_zip_reader_get_filename(&zip, i, filename, PATH_MAX);

			MakeDirectory(GetFileInTempDir(filename));
		}
		
		/* ----- */
		
		for (mz_uint i = 0; i < n; ++i)
		{
			mz_zip_reader_get_filename(&zip, i, filename, PATH_MAX);
			
			if (!mz_zip_reader_is_file_a_directory(&zip, i) && !ShouldIgnore(filename))
			{
				mz_zip_reader_extract_to_file(&zip, i, GetFileInTempDir(filename), 0);
			}
		}

		/* ----- */
		
		mz_zip_reader_end(&zip);
	}
}
