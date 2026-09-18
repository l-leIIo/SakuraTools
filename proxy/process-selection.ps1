function Get-JavaMinecraftProcesses {
    $items = @()
    Get-CimInstance Win32_Process -Filter "Name='java.exe' OR Name='javaw.exe'" -ErrorAction SilentlyContinue | ForEach-Object {
        $cmd = [string]$_.CommandLine
        if ($cmd -notmatch 'minecraft|net\.minecraft|BootstrapLauncher|forgeclient|fabric|quilt') { return }
        $version = 'auto'
        foreach ($pattern in @('(?<!\d)1\.\d+(?:\.\d+)?(?!\d)', 'minecraftVersion[=:]([0-9.]+)', 'version[=:]([0-9.]+)')) {
            $m = [regex]::Match($cmd, $pattern)
            if ($m.Success) { $version = if ($m.Groups.Count -gt 1 -and $m.Groups[1].Value -match '1\.') { $m.Groups[1].Value } else { $m.Value }; break }
        }
        $loader = 'auto'
        if ($cmd -match '(?i)neoforge') { $loader = 'neoforge' }
        elseif ($cmd -match '(?i)forge|ModLauncher|LaunchWrapper') { $loader = 'forge' }
        elseif ($cmd -match '(?i)quilt') { $loader = 'quilt' }
        elseif ($cmd -match '(?i)fabric') { $loader = 'fabric' }
        else { $loader = 'vanilla' }
        try { $p = Get-Process -Id $_.ProcessId -ErrorAction Stop } catch { return }
        $items += [pscustomobject]@{ PID=$_.ProcessId; Name=$p.ProcessName; Version=$version; Loader=$loader; CommandLine=$cmd }
    }
    return @($items | Sort-Object PID)
}

function Select-JavaMinecraftProcess {
    $items = @(Get-JavaMinecraftProcesses)
    if ($items.Count -eq 0) { Write-Host '未检测到 Minecraft Java 进程。'; return $null }
    if ($items.Count -eq 1) { return $items[0] }
    Write-Host '检测到多个 Minecraft Java 进程，请选择目标：' -ForegroundColor Yellow
    for ($i=0; $i -lt $items.Count; $i++) {
        Write-Host ("[{0}] PID={1}  {2}.exe  版本={3}  加载器={4}" -f ($i+1),$items[$i].PID,$items[$i].Name,$items[$i].Version,$items[$i].Loader)
    }
    do { $choice = Read-Host "输入编号 (1-$($items.Count))"; $index = 0; $ok = [int]::TryParse($choice, [ref]$index) } while (-not $ok -or $index -lt 1 -or $index -gt $items.Count)
    return $items[$index-1]
}

function Do-InjectPid {
    if (-not (Test-Path -LiteralPath $InjectPs1)) { Write-Host "ERROR: inject.ps1 not found" -ForegroundColor Red; return }
    $target = Select-JavaMinecraftProcess
    if (-not $target) { return }
    Write-Host ("目标 PID {0}: {1} {2}" -f $target.PID,$target.Version,$target.Loader) -ForegroundColor Cyan
    $v = Read-Host "版本（直接回车使用自动识别: $($target.Version)）"
    if ([string]::IsNullOrWhiteSpace($v)) { $v = $target.Version }
    $l = Read-Host "加载器（直接回车使用自动识别: $($target.Loader)）"
    if ([string]::IsNullOrWhiteSpace($l)) { $l = $target.Loader }
    & powershell -NoProfile -ExecutionPolicy Bypass -File $InjectPs1 -ProcId $target.PID -Dll $Dll -Version $v -Loader $l
}
