add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")
add_repositories("iceblcokmc https://github.com/IceBlcokMC/xmake-repo.git")
add_repositories("miracleforest-repo https://github.com/MiracleForest/xmake-repo.git")


-- LeviMc(LiteLDev)
add_requires("levilamina 26.10.14", {configs = {target_type = "server"}})
add_requires("levibuildscript")

-- MiracleForest
add_requires("ilistenattentively 0.12.0")

-- IceBlockMC
add_requires("ll-bstats 0.3.0")
add_requires("economy_bridge 0.4.0")

-- xmake
add_requires("exprtk 0.0.3")
add_requires("abseil 20250127.0")

if has_config("devtool") then
    add_requires("imgui v1.91.6-docking", {configs = { opengl3 = true, glfw = true }})
    add_requires("glew 2.2.0")
end


if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("devtool") -- 开发工具
    set_default(true)
    set_showmenu(true)
option_end()

target("PLand")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_rules("plugin.compile_commands.autoupdate")
    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
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
    add_defines(
        "NOMINMAX",
        "UNICODE",
        "LDAPI_EXPORT",
        "LL_PLAT_S"
    )
    add_includedirs("src")
    add_files("src/**.cpp", "src/**.cc")
    add_headerfiles("src/(pland/**.h)")

    add_packages(
        "levilamina",
        "exprtk",
        "ilistenattentively",
        "ll-bstats",
        "economy_bridge",
        "abseil"
    )

    add_configfiles("src/BuildInfo.h.in")
    set_configdir("src/pland")

    if is_mode("debug") then
        add_defines("DEBUG")
        -- add_defines("PLAND_I18N_COLLECT_STRINGS", "LL_I18N_COLLECT_STRINGS", "LL_I18N_COLLECT_STRINGS_CUSTOM")
    end

    if has_config("devtool") then
        add_packages(
            "imgui",
            "glew"
        )
        add_includedirs("src-devtool", "src-devtool/deps")
        add_files("src-devtool/**.cc", "src-devtool/**.cpp")
        add_defines("LD_DEVTOOL")
    end

    after_build(function (target)
        local bindir = path.join(os.projectdir(), "bin")
        local outputdir = path.join(bindir, target:name())

        local assetsdir = path.join(os.projectdir(), "assets")
        local langDir = path.join(assetsdir, "lang")
        os.mkdir(path.join(outputdir, "lang"))
        os.cp(langDir, outputdir)
    end)
