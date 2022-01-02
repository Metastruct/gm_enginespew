# gm_enginespew
Catch engine and lua logs and pushes them back on the Lua stack with a hook.

## Usage
```lua
hook.Add("EngineSpew", "logs", function(logType, logMsg, logGroup, logLevel, red, green, blue) 
  local color = Color(red, green, blue)
  local out = ("[%s|%d] >> %s \n"):format(logGroup, logLevel, logMsg)
  MsgC(color, out)
end)
```

## For the x86_64 branch:
### Building the project for linux/macos
1) Get [premake](https://premake.github.io/download/) add it to your `PATH`
2) Get [garrysmod_common](https://github.com/danielga/garrysmod_common) (with `git clone https://github.com/danielga/garrysmod_common --recursive --branch=x86-64-support-sourcesdk`) and set an env var called `GARRYSMOD_COMMON` to the path of the local repo
3) Run `premake5 gmake` in your local copy of **this** repo
4) Navigate to the makefile directory (`cd /projects/linux/gmake` or `cd /projects/macosx/gmake`)
5) Run `make config=releasewithsymbols_x86_64`

### Building the project on windows
1) Get [premake](https://premake.github.io/download/) add it to your `PATH`
2) Get [garrysmod_common](https://github.com/danielga/garrysmod_common) (with `git clone https://github.com/danielga/garrysmod_common --recursive --branch=x86-64-support-sourcesdk`) and set an env var called `GARRYSMOD_COMMON` to the path of the local repo
3) Run `premake5 vs2019` in your local copy of **this** repo
4) Navigate to the project directory `cd /projects/windows/vs2019`
5) Open the .sln in Visual Studio 2019+
6) Select Release, and either x64 or x86
7) Build


## For the *current (x86)* main branch:
### Building the project for linux/macos
1) Get [premake](https://premake.github.io/download/) add it to your `PATH`
2) Get [garrysmod_common](https://github.com/danielga/garrysmod_common) (with `git clone https://github.com/danielga/garrysmod_common --recursive`) and set an env var called `GARRYSMOD_COMMON` to the path of the local repo
3) Edit premake5.lua and change `PROJECT_GENERATOR_VERSION` to `2`
4) Run `premake5 gmake` in your local copy of **this** repo
5) Navigate to the makefile directory (`cd /projects/linux/gmake` or `cd /projects/macosx/gmake`)
6) Run `make`

### Building the project on windows
1) Get [premake](https://premake.github.io/download/) add it to your `PATH`
2) Get [garrysmod_common](https://github.com/danielga/garrysmod_common) (with `git clone https://github.com/danielga/garrysmod_common --recursive`) and set an env var called `GARRYSMOD_COMMON` to the path of the local repo
3) Run `premake5 vs2019` in your local copy of **this** repo
4) Edit premake5.lua and change `PROJECT_GENERATOR_VERSION` to `2`
5) Navigate to the project directory `cd /projects/windows/vs2019`
6) Open the .sln in Visual Studio 2019+
7) Select Release, and either x64 or x86
8) Build
