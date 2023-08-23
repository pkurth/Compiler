
-- Premake extension to include files at solution-scope. From https://github.com/premake/premake-core/issues/1061#issuecomment-441417853

require('vstudio')
premake.api.register {
	name = "workspacefiles",
	scope = "workspace",
	kind = "list:string",
}

premake.override(premake.vstudio.sln2005, "projects", function(base, wks)
	if wks.workspacefiles and #wks.workspacefiles > 0 then
		premake.push('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "Solution Items", "Solution Items", "{' .. os.uuid("Solution Items:"..wks.name) .. '}"')
		premake.push("ProjectSection(SolutionItems) = preProject")
		for _, file in ipairs(wks.workspacefiles) do
			file = path.rebase(file, ".", wks.location)
			premake.w(file.." = "..file)
		end
		premake.pop("EndProjectSection")
		premake.pop("EndProject")
	end
	base(wks)
end)


-----------------------------------------
-- GENERATE SOLUTION
-----------------------------------------

workspace "Compiler"
	architecture "x64"
	startproject "Compiler"

	configurations {
		"Debug",
		"Release"
	}

	flags {
		"MultiProcessorCompile"
	}
	
	workspacefiles {
        "premake5.lua",
    }


outputdir = "%{cfg.buildcfg}_%{cfg.architecture}"


project "Compiler"
	--location "bin/Compiler"
	kind "ConsoleApp"
	language "C"
	cdialect "C11"
	staticruntime "Off"

	targetdir ("./bin/" .. outputdir)
	objdir ("./bin_int/" .. outputdir ..  "/%{prj.name}")

	debugenvs {
		"PATH=ext/bin;%PATH%;"
	}
	debugdir "."

	files {
		"src/**.h",
		"src/**.c",
	}

	vpaths {
		["Headers/*"] = { "src/**.h" },
		["Sources/*"] = { "src/**.c" },
	}

	includedirs {
		"src",
	}

	vectorextensions "AVX2"
	floatingpoint "Fast"

	filter "configurations:Debug"
        runtime "Debug"
		symbols "On"
		
	filter "configurations:Release"
        runtime "Release"
		optimize "On"
		inlining "Auto"

	filter "system:windows"
		systemversion "latest"

		defines {
			"_UNICODE",
			"UNICODE",
			"_CRT_SECURE_NO_WARNINGS",
		}
