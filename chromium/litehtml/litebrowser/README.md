## litebrowser (litehtml engine)

**Litebrowser** is simple web browser designed to test the [litehtml HTML rendering engine](https://github.com/tordex/litehtml).

### Building

You can build litebrowser with Visual Studio 2022 and newer. **Note**: this project contains some git submodules:

  * [freeimage](https://github.com/tordex/freeimage) - used to draw images
  * [cairo](https://github.com/tordex/cairo) - 2D graphics library
  * [txdib](https://github.com/tordex/txdib) - wrapper for freeimage
  * [simpledib](https://github.com/tordex/simpledib) - library for memory device context and DIBs
  * [litehtml](https://github.com/tordex/litehtml) - html rendering engine

Please be sure the submodules are fetched, or download them from github and copy into libs subfolder.

### Download binaries

You can download the binary files from the [Releases](https://github.com/litehtml/litebrowser/releases) page.

### Using litebrowser

Before running litebrowser copy the files cairo.dll (from libs\cairo) and freeimage.dll (libs\freeimage) into the same folder where litebrowser.exe is.

Type url in the address bar and press [ENTER]





V8 BUILD:

VS2022 x64 tool:

```powershell
cd v8
chcp 65001
set PYTHONIOENCODING=utf-8
gn args out/x64.release
```

args.gn:

```gn
v8_monolithic = true
is_debug = false
is_clang = false
v8_target_cpu = "x64"
target_cpu = "x64"
v8_enable_backtrace = true
v8_optimized_debug = false
is_component_build = false
v8_static_library = true
use_custom_libcxx = false
use_custom_libcxx_for_host = false
treat_warnings_as_errors = false
v8_enable_i18n_support = false
v8_use_external_startup_data = false
v8_enable_pointer_compression = true

defines = [
  "V8_COMPRESS_POINTERS",
  "V8_ENABLE_SANDBOX"
]
```

```powershell
ninja -C out/x64.release v8_monolith
```

