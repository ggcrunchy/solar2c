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

#ifdef WIN32
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "miniz.h"

#include "common.h"

void FixLib(void)
{
	const char* old_name = "libtcc_win32.a";
	char old_buf[PATH_MAX];

	strcpy(old_buf, GetFileInTempDir(old_name));
	rename(old_buf, GetFileInTempDir("libtcc1.a"));
}

void SetUpPaths(lua_State* L, Paths* paths)
{
	lua_pushfstring(L, "%s\\Corona\\shared\\include\\Corona", getenv("CORONA_ROOT"));

	sprintf(paths->Corona, "%.*s", PATH_MAX - 1, lua_tostring(L, -1));

	lua_pop(L, 1);
	
	lua_pushfstring(L, "%s\\Corona\\shared\\include\\lua", getenv("CORONA_ROOT"));

	sprintf(paths->Lua, "%.*s", PATH_MAX - 1, lua_tostring(L, -1));

	lua_pop(L, 1);
}

#endif
