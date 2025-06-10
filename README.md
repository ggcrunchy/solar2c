# solar2c
Solar2D plugin for Tiny C Compiler.

# Summary

This started with Jérôme Vuarand's TinyCC plugin, which was featured in [Gems](https://www.lua.org/gems/) ([book](https://www.lua.org/gems/lpg.pdf)). (There are bits of it there yet, although it's got a different focus.)

This ties it into [Solar2D](https://solar2d.com)'s C APIs, letting you use them and also play with a lot of C libraries out in the wild. This is useful, for instance, to prototype plugins on desktop (currently you wouldn't be able to use it on mobile).

# How to use

In your **build.settings**, add to your **plugins**: `{ ["plugin.solar2c"] = { publisherId = "com.solartools" }`.

List of functions and methods, to be documented:

* `state = plugin.new()`
* `plugin.set_system_headers(path)`

* `state:add_symbol(name, symbol)`
* `state:define_symbol(name, def="")`
* `state:compile(code str)`
* `state:add_file(name)`
* `state:add_multiple_files{ name1, ... }`
* `state:add_library(name)`
* `symbol = state:get_symbol(name)`
* `state:add_library_path(path)`
* `state:add_include_path(path)`
* `state:add_sysinclude_path(path)`
* `state:add_multiple_library_paths{ path1, ... }`
* `state:add_multiple_include_paths{ path1, ... }`
* `state:add_multiple_sysinclude_paths{ path1, ... }`

(TODO: `baseDir` in various... defaults to `system.ResourceDirectory`)

`state:relocate()`
`state:detach()`

TODO! (there are a few examples mostly ready to go, but might need some cleanup, verifying licenses, etc.)

# Building

On Mac, make sure you have Solar >= 3719 installed. 

Since you might have more than one version installed, the correct one needs to be specified. To do so, add a `Native` symbolic link in `Applications` that refers to the directory of the same name.

Then run `./build.sh` in the `mac` folder. This assumes Xcode (maybe merely Developer Tools?) is installed.

TODO: Windows (being worked on by @kan6868), Linux?

# Ideas / Work in Progress

* There's an API to set a systems headers path, but untested: in theory the plugin doesn't require Xcode / Developer Tools installed, but could maybe use a more lightweight local copy of Zig
* It's easier to segfault with this than the simulator and plain Lua code; some signal-handling / SEH might be handy, although only a limited set of use cases could really be addressed at the plugin level
* The `get_symbol()` approach can lead to such segfaults; maybe a `require()`-style policy could be implemented, through some inspection of TinyCC symbols and / or header declarations
* Investigate the RISC-V backend, e.g. as a portable bytecode when speed isn't the main concern
* Currently uses [incbin](https://github.com/graphitemaster/incbin) and [miniz](https://github.com/richgel999/miniz), but not sure if that's the final form; am meaning to explore a `ONE_SOURCE` build of TinyCC, which might change some of this
