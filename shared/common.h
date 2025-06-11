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

#ifndef common_h
#define common_h

#include <CoronaLua.h>
#include <stdbool.h>
#include <string.h>

#define INCBIN_PREFIX

#include "libtcc.h"
#include "incbin.h"

#ifdef WIN32
	#ifndef PATH_MAX
	#  define PATH_MAX 260
	#endif

	INCBIN_EXTERN(libs);
#endif // WIN32
//
//
//

INCBIN_EXTERN(tcc);

//
//
//

int ForEachFile (lua_State * L, TCCState * tcc, int (*action) (TCCState * tcc, const char * filename), const char * what);

const char * GetResolvedFilename (lua_State * L, int file_index, int dir_index);
const char * GetFileInTempDir (const char * file);

void PrepareToUnzip (lua_State * L);
void ExtractZip (const unsigned char buf[], const size_t size);
void FixLib (void);
void MakeDirectory (const char * filename);

//
//
//

typedef struct {
	char Corona[PATH_MAX];
	char Lua[PATH_MAX];
} Paths;

//
//
//

void SetUpPaths (lua_State * L, Paths * paths); 

//
//
//

#endif
