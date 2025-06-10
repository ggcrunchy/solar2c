// Original implementation:

/*
Copyright (c) 2006-2010 Jérôme Vuarand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// Modifications also under the same license

#include <CoronaLua.h>
#include <string.h>
#include <stdbool.h>

#include "common.h"

//
//
//

#define TCC_METATABLE_NAME "solar2c.box"

//
//
//

static TCCState ** GetBox (lua_State * L)
{
	return luaL_checkudata(L, 1, TCC_METATABLE_NAME);
}

//
//
//

static TCCState * GetState (lua_State * L)
{
	return *GetBox(L);
}

//
//
//

static int AddSymbol (lua_State * L)
{
	if (tcc_add_symbol(GetState(L), luaL_checkstring(L, 2), lua_touserdata(L, 3)))
	{
		return luaL_error(L, "error adding symbol");
	}

	return 0;
}

static int DefineSymbol (lua_State * L)
{
	tcc_define_symbol(GetState(L), luaL_checkstring(L, 2), luaL_optstring(L, 3, ""));

	return 0;
}

/* function context:compile(source [, chunkname]) end */
static int lua__tcc__compile(lua_State* L)
{
	/* compile */
	if (tcc_compile_string(GetState(L), luaL_checkstring(L, 2)))
	{
		return luaL_error(L, "unknown compilation error");
	}
	
	return 0;
}

/* function context:add_file(filename) end */
static int lua__tcc__add_file(lua_State* L)
{
	const char* filename = GetResolvedFilename(L, 2, 3);

	/* add file */
	if (tcc_add_file(GetState(L), filename))
		return luaL_error(L, "can't load file %s", filename);
	
	return 0;
}

static int AddMultipleFiles(lua_State* L)
{
	return ForEachFile(L, GetState(L), tcc_add_file, "add file");
}

/* function context:add_library(libraryname) end */
static int lua__tcc__add_library(lua_State* L)
{
	const char* libname = luaL_checkstring(L, 2);
	
	/* add libs */
	if (tcc_add_library(GetState(L), libname))
		return luaL_error(L, "can't load library %s", libname);
	
	return 0;
}

/* function context:relocate() end */
static int lua__tcc__relocate(lua_State* L)
{
	/* link */
	if (tcc_relocate(GetState(L)))
		return luaL_error(L, "unknown relocation (link) error");
	
	return 0;
}

/* function context:get_symbol(symbolname) return symbol end */
static int lua__tcc__get_symbol(lua_State* L)
{
	const char* funcname = luaL_checkstring(L, 2);
	void* sym = tcc_get_symbol(GetState(L), funcname);
	if (!sym)
		return luaL_error(L, "can't get symbol %s", funcname);
	lua_CFunction f = sym;
	
	// The symbol might be coming from a state that is, or will
	// be, unanchored. Since collecting the state would make the
	// symbol invalid, the latter holds a reference of its own.

	lua_pushvalue(L, 1); // state, name, state
	lua_pushcclosure(L, f, 1); // state, name, symbol
	
	return 1;
}

/* function context:add_library_path(path) end */
static int lua__tcc__add_library_path(lua_State *L)
{
	tcc_add_library_path(GetState(L), GetResolvedFilename(L, 2, 3));
	
	return 0;
}

static int AddMultipleLibraryPaths (lua_State * L)
{
	return ForEachFile(L, GetState(L), tcc_add_library_path, "add library path");
}

/* function context:add_include_path(path) end */
static int lua__tcc__add_include_path(lua_State *L)
{
	tcc_add_include_path(GetState(L), GetResolvedFilename(L, 2, 3));
	
	return 0;
}

static int AddMultipleIncludePaths (lua_State * L)
{
	return ForEachFile(L, GetState(L), tcc_add_include_path, "add include path");
}


/* function context:add_sysinclude_path(path) end */
static int lua__tcc__add_sysinclude_path(lua_State *L)
{
	tcc_add_sysinclude_path(GetState(L), GetResolvedFilename(L, 2, 3));
	
	return 0;
}

static int AddMultipleSysincludePaths (lua_State * L)
{
	return ForEachFile(L, GetState(L), tcc_add_sysinclude_path, "add sysinclude path");
}

static int lua__tcc__detach(lua_State *L)
{
	luaL_checkudata(L, 1, TCC_METATABLE_NAME);
	lua_settop(L, 1); // state
	lua_pushnil(L); // state, nil
	lua_rawset(L, lua_upvalueindex(1)); // (empty); anchor[state] = nil
	
	return 0;
}

static int lua__tcc___gc(lua_State* L)
{
	TCCState** ptcc = GetBox(L);
	
	if (ptcc && *ptcc)
	{
		tcc_delete(*ptcc);
	}
	
	return 0;
}

static const struct luaL_reg tcc_methods[] = {
	{"add_symbol", AddSymbol},
	{"define_symbol", DefineSymbol},
	{"compile", lua__tcc__compile},
	{"add_file", lua__tcc__add_file},
	{"add_multiple_files", AddMultipleFiles},
	{"add_library", lua__tcc__add_library},
	{"relocate", lua__tcc__relocate},
	{"get_symbol", lua__tcc__get_symbol},
	{"add_library_path", lua__tcc__add_library_path},
	{"add_include_path", lua__tcc__add_include_path},
	{"add_sysinclude_path", lua__tcc__add_sysinclude_path},
	{"add_multiple_library_paths", AddMultipleLibraryPaths},
	{"add_multiple_include_paths", AddMultipleIncludePaths},
	{"add_multiple_sysinclude_paths", AddMultipleSysincludePaths},
	{NULL, NULL}
};

void luatcc__error_func(void* opaque, const char* msg)
{
	if (strstr(msg, "warning: "))
	{
		CoronaLog("WARNING: %s", msg);
	}
	
	else
	{
		luaL_error((lua_State*)opaque, "%s", msg);
	}
}

//
//
//

static int lua__new(lua_State* L)
{
	TCCState* tcc = tcc_new();
	if (!tcc)
		return luaL_error(L, "can't create tcc state");
	
	tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY);
	tcc_set_error_func(tcc, L, luatcc__error_func);
	
	/* ----- */

	Paths * paths = lua_touserdata(L, lua_upvalueindex(2));

	tcc_add_sysinclude_path(tcc, paths->Corona);
	tcc_add_sysinclude_path(tcc, paths->Lua);
	
	lua_getfield(L, lua_upvalueindex(1), "_HEADERS"); // headers?
	
	if (!lua_isnil(L, -1)) tcc_add_sysinclude_path(tcc, lua_tostring(L, -1));
	
	tcc_add_include_path(tcc, GetFileInTempDir("include"));
	tcc_add_library_path(tcc, GetFileInTempDir(NULL));
	
	/* ----- */

	TCCState** ptcc = lua_newuserdata(L, sizeof(TCCState*)); // headers?, state
	*ptcc = tcc;
	
	if (luaL_newmetatable(L, TCC_METATABLE_NAME)) // headers?, state, mt
	{
		lua_pushvalue(L, -1); // headers?, state, mt, mt
		lua_setfield(L, -2, "__index"); // headers?, state, mt = { __index = mt }
		luaL_register(L, NULL, tcc_methods);
		lua_pushvalue(L, lua_upvalueindex(1)); // headers?, state, mt, anchor
		lua_pushcclosure(L, lua__tcc__detach, 1); // headers?, state, mt, Detach
		lua_setfield(L, -2, "detach"); // headers?, state, mt = { __index, detach = Detach }
		lua_pushcfunction(L, lua__tcc___gc); // headers?, state, mt, GC
		lua_setfield(L, -2, "__gc"); // headers?, state, mt = { __index, detach, __gc = GC }
	}
	
	lua_setmetatable(L, -2); // headers?, state; state.metatable = mt
	lua_pushvalue(L, -1); // headers?, state, state
	lua_pushboolean(L, 1); // headers?, state, state, true
	lua_rawset(L, lua_upvalueindex(1)); // headers?, state; anchor[state] = true
	
	return 1;
}

//
//
//

static int lua__set_system_headers (lua_State * L)
{
	lua_settop(L, 1); // headers
	luaL_argcheck(L, LUA_TSTRING == lua_type(L, 1) && lua_objlen(L, 1) < PATH_MAX, 1, "System headers path too long");
	lua_setfield(L, lua_upvalueindex(1), "_HEADERS"); // (empty); anchor._HEADERS = headers
	
	return 0;
}

//
//
//

static void ExtractToTempDir (lua_State * L)
{
	PrepareToUnzip(L);
	ExtractZip(tccData, tccSize);
	FixLib(); // rename the architecture-appropriate binary so TinyCC can find it
}

//
//
//

static void CheckCoronaHeaders (lua_State * L, const Paths * paths)
{
	// Starting a while back, and up until build 3719, a few
	// Solar headers were not quite C-friendly. Any offender
	// basically needs some typedef fixes.

	// The fix was submitted in https://github.com/coronalabs/corona/pull/804
	// and made available in https://github.com/coronalabs/corona/releases/tag/3719.
	
	// The "public types" header is the simplest of the bunch:
	// the original has no typedefs, but needs one added, so a
	// simple search can detect if the fix was applied.
	
	lua_getglobal(L, "io"); // ..., io
	luaL_argcheck(L, lua_istable(L, -1), -1, "`io` not a table");
	lua_getfield(L, -1, "open"); // ..., io, io.open
	luaL_argcheck(L, lua_isfunction(L, -1), -1, "`io.open` not a function");
	lua_pushfstring(L, "%s/CoronaPublicTypes.h", paths->Corona); // ..., io, io.open, filename
	lua_pushliteral(L, "rb"); // ..., io, io.open, filename, "rb"
	lua_call(L, 2, 1); // ..., io, file?
	luaL_argcheck(L, !lua_isnil(L, -1), -1, "Unable to open test file");
	lua_getfield(L, -1, "read"); // ..., io, file, file:read
	lua_pushvalue(L, -2 ); // ..., io, file, file:read, file
	lua_pushliteral(L, "*a"); // ..., io, file, file:read, file, "*a"
	lua_call(L, 2, 1); // ..., io, file, contents

	bool has_typedef = strstr(lua_tostring(L, -1), "typedef");

	lua_getfield(L, -2, "close"); // ..., io, file, contents, file:close
	lua_pushvalue(L, -3); // ..., io, file, contents, file:close, file
	lua_call(L, 1, 0); // ..., io, file, contents
	lua_pop(L, 3); // ...
	
	if (!has_typedef) CoronaLog("WARNING: pre-3719 build; some Solar API headers are incompatible with C");
}

//
//
//

static void PopulatePaths (lua_State * L)
{
	ExtractToTempDir(L);
	
	/* ----- */
	
	Paths * paths = lua_newuserdata(L, sizeof(Paths)); // ..., paths
	
	SetUpPaths(L, paths);
	
	/* ----- */
	
	CheckCoronaHeaders(L, paths);
}

//
//
//

CORONA_EXPORT int luaopen_plugin_solar2c (lua_State * L)
{
    lua_newtable(L); // plugin
	
	// TinyCC states are deleted in __gc metamethods.

	// Doing so leads to proper cleanup on relaunches. However,
	// the rug gets pulled on any executable code still in use,
	// e.g callbacks might have segfaults or bus errors.

	// States are thus anchored by default until Solar closes
	// or relaunches, and may opt out with detach().
	
	// The anchor table consists of (state, true) pairs. These
	// keys are all userdata, so a (string, string) pair can be
	// smuggled in without conflict: this is how user-defined
	// system headers, if any, are stored.
	
	lua_newtable(L); // plugin, anchor
	
	PopulatePaths(L); // plugin, anchor, paths

	lua_pushvalue(L, -2); // plugin, anchor, paths, anchor
	lua_pushcclosure(L, lua__set_system_headers, 1); // plugin, anchor, SetSystemHeaders
	lua_setfield(L, -3, "set_system_headers"); // plugin = { set_system_headers = SetSystemHeaders }, anchor
	lua_pushcclosure(L, lua__new, 2); // plugin, new
	lua_setfield(L, -2, "new"); // plugin = { set_system_headers, new = new }
	
    return 1;
}
