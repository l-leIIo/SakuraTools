# 这个入口统一转发到 proxy/launcher.ps1；目标选择、版本/加载器识别均由那里完成。
& "$PSScriptRoot/launcher.ps1"
exit $LASTEXITCODE
