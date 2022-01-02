PROJECT_GENERATOR_VERSION = 3

newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "path to garrysmod_common directory"
})

include(assert(_OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON"),
	"you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory"))

CreateWorkspace({name = "enginespew"})
	CreateProject({serverside = true})
		includedirs({"source", "include"})
		IncludeSDKCommon()
		IncludeSDKTier0()

	CreateProject({serverside = false})
		includedirs({"source", "include"})
		IncludeSDKCommon()
		IncludeSDKTier0()