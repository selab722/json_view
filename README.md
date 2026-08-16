## Prequest

#### 1. Download requirement

This project relies on a few 3rd libraries:
1. glfw (from https://www.glfw.org);
2. glad2 (from https://gen.glad.sh, choose core GL 3.3, toggle "loader" and "mx" in Optional);
3. imgui (from https://github.com/ocornut/imgui).


So first download them and uncompress to lib/. The project directory looks like this:

```
project_root
├── CMakeLists.txt
├── README.md
├── lib
│   ├── glfw
│   │   ├── CMakeLists.txt
│   │   ├── src
│   │   └── ...
│   ├── glad2
│   │   ├── include
│   │   └── src
|   └── imgui
│       ├── imgui.h
│       ├── backends
│       └── ...
└── src
    ├── main.cpp
    └── ...
```


#### 2. Install dependencies

Install dependencies if you are on Ubuntu. For other platform you usually don't need to. Check compile guide.
```
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
```

#### 3. Enter project root directory

Use `cd` to enter project root directory.


## Compile

Follow the [compile guide](https://www.glfw.org/docs/latest/compile_guide.html) of glfw. Basically, in this project, you need to do following:
```
cmake -S . -B build
cmake --build build --parallel --config Release
cmake --install build
```

After that, if you want to compile main.cpp individually, run (one for msys2 and one for linux):
```
g++ -static -static-libgcc -static-libstdc++ -Ibuild/install/include src/main.cpp build/install/lib/*.a -lws2_32 -lgdi32 -mwindows -o main.exe
g++ -Ibuild/install/include src/main.cpp build/install/lib/*.a -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lXinerama -lXcursor
```

## Run

Find executable in `build/Release` directory if you use Windows or `build` if you use Linux. On wsl there is something wrong, I tried
```
cmake -S . -B build -DGLFW_BUILD_WAYLAND=OFF && cmake --build build --parallel --config Release
cmake --install build
```
