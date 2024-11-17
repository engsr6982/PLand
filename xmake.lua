add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")
add_repositories("engsr6982-repo https://github.com/engsr6982/xmake-repo.git")
add_repositories("oeotyan-repo https://github.com/oeotyan/xmake-repo.git")

-- add_requires("levilamina x.x.x") for a specific version
-- add_requires("levilamina develop") to use develop version
-- please note that you should add bdslibrary yourself if using dev version
add_requires("levilamina 0.13.5")
add_requires("exprtk 2022.01.01")
add_requires("legacymoney 0.8.3")
add_requires("more_events 0.2.0")
add_requires("bsci 0.1.4")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("PLand") -- Change this to your mod name.
    add_cxflags(
        "/EHa",
        "/utf-8",
        "/W4",
        "/w44265",
        "/w44289",
        "/w44296",
        "/w45263",
        "/w44738",
        "/w45204"
    )
    add_defines("NOMINMAX", "UNICODE", "LDAPI_EXPORT")
    add_files("src/**.cpp", "src/**.cc")
    add_includedirs("src", "include")
    add_packages(
        "levilamina",
        "exprtk",
        "more_events",
        "legacymoney",
        "bsci"
    )

    -- MoreEvents test
    -- add_includedirs("D:/Projects/MoreEvents/bin/include")
    -- add_links("D:/Projects/MoreEvents/bin/lib/*.lib")

    add_shflags("/DELAYLOAD:bedrock_server.dll") -- To use symbols provided by SymbolProvider.
    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("c++20")

    add_defines("PLUGIN_NAME=\"[PLand] \"")
    add_defines("BUILD_TIME=\"" .. os.date('%Y-%m-%d %H:%M:%S') .. "\"")

    if is_mode("debug") then
        add_defines("DEBUG", "LL_I18N_COLLECT_STRINGS")
        set_symbols("debug", "edit")
    else 
        set_symbols("debug")
    end 

    after_build(function (target)
        local mod_packer = import("scripts.after_build")

        local tag = os.iorun("git describe --tags --abbrev=0 --always")
        local major, minor, patch, suffix = tag:match("v(%d+)%.(%d+)%.(%d+)(.*)")
        if not major then
            print("Failed to parse version tag, using 0.0.0")
            major, minor, patch = 0, 0, 0
        end
        local mod_define = {
            modName = target:name(),
            modFile = path.filename(target:targetfile()),
            modVersion = major .. "." .. minor .. "." .. patch,
        }
        
        mod_packer.pack_mod(target,mod_define)
    end)
