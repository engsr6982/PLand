function main()
    -- 创建include/pland目录（如果不存在）
    os.mkdir("src/pland")
    
    -- 获取最新的tag
    local latest_tag = os.iorunv("git", {"describe", "--tags", "--abbrev=0"}) or "v0.0.0"
    latest_tag = latest_tag:gsub("\n", "")
    
    -- 解析版本号
    local major, minor, patch = latest_tag:match("v(%d+)%.(%d+)%.(%d+)")
    major = tonumber(major or "0")
    minor = tonumber(minor or "0")
    patch = tonumber(patch or "0")
    
    -- 获取从最近tag到当前的提交数量
    local build_number = os.iorunv("git", {"rev-list", latest_tag .. "..HEAD", "--count"}) or "0"
    build_number = build_number:gsub("\n", "")
    build_number = tonumber(build_number or "0")
    
    -- 获取当前提交的哈希值
    local commit_hash = os.iorunv("git", {"rev-parse", "--short=7", "HEAD"}) or "unknown"
    commit_hash = commit_hash:gsub("\n", "")
    
    -- 获取tag的提交哈希值
    local tag_hash = os.iorunv("git", {"rev-list", "-n", "1", latest_tag}) or "unknown"
    tag_hash = tag_hash:gsub("\n", "")
    
    -- 判断是否是快照版本（当前提交不是tag的提交）
    local is_snapshot = commit_hash ~= tag_hash:sub(1, 7)
    
    -- 生成版本字符串
    local version_type = is_snapshot and "Snapshot" or "Release"
    local version_string = string.format("[%s] %s-%s+%d", version_type, latest_tag, commit_hash, build_number)
    
    -- 生成Version.h文件内容
    local content = string.format([[#pragma once

// 自动生成的版本信息，请勿手动修改
// Auto-generated version information, do not modify manually

// 主版本号
#define PLAND_VERSION_MAJOR %d

// 次版本号
#define PLAND_VERSION_MINOR %d

// 修订号
#define PLAND_VERSION_PATCH %d

// 编译号（从tag起算）
#define PLAND_VERSION_BUILD %d

// 最近一次的提交哈希
#define PLAND_COMMIT_HASH "%s"

// 是否是快照版本
#define PLAND_VERSION_SNAPSHOT %s

// 是否是正式发布版本
#define PLAND_VERSION_RELEASE !PLAND_VERSION_SNAPSHOT

// 完整版本字符串
#define PLAND_VERSION_STRING "%s"
]], 
        major, minor, patch, build_number, commit_hash, 
        is_snapshot and "true" or "false", version_string)

    local filePath = "src/pland/Version.h"
    if os.exists(filePath) then
        import("core.base.bytes")

        -- 读取并计算Version.h文件的sha256
        local fileContent = io.readfile(filePath)
        fileContent = fileContent:gsub("\r\n", "\n") -- 将文件内容中的\r\n替换为\n
        local rawFileSha256 = hash.sha256(bytes.new(fileContent)) -- 计算文件的sha256
        -- print("Raw file sha256: " .. rawFileSha256)

        -- 计算新的文件内容的sha256
        local contentSha256 = hash.sha256(bytes.new(content))
        -- print("Content sha256: " .. contentSha256)
        if rawFileSha256 == contentSha256 then
            print("Version.h is up to date.")
            return
        end
    end

    -- 写入文件
    local file = io.open(filePath, "w")
    if file then
        file:write(content)
        file:close()
        print("Generated Version.h: " .. version_string)
    else
        print("Failed to write Version.h")
    end
end

return main