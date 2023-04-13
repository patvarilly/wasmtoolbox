# wasmtoolbox

Personal tools for poking at WASM artifacts.

Existing tools already do everything here, and much more, better.  The main goal here is learning through doing.  Happy
if someone else finds these useful.

## Compiling

```
mkdir -p build/debug
cd build/debug
conan install ../..
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
make -j 8
```

## Sample Usage

```
./wasmtoolbox wasm2wat my_module.wasm
```