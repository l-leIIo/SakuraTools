param(
    [int]$ProcId,
    [string]$Dll,
    [string]$Version,
    [ValidateSet('auto','vanilla','forge','neoforge','fabric','quilt')]
    [string]$Loader = 'auto'
)
if (-not $ProcId -or -not $Dll) { Write-Error "usage: -ProcId <pid> -Dll <path> [-Version <version>] [-Loader <loader>]"; exit 2 }
if (-not (Test-Path -LiteralPath $Dll)) { Write-Error "dll not found: $Dll"; exit 2 }
$Dll = (Resolve-Path -LiteralPath $Dll).Path

# The injected DLL is selected by the target JVM profile.  Keep this file as the
# single entry point so the profile is written before LoadLibrary executes.
$profileDir = Join-Path $env:APPDATA 'SakuraTools'
$profilePath = Join-Path $profileDir 'runtime_profile.json'
if (-not (Test-Path -LiteralPath $profileDir)) { New-Item -ItemType Directory -Path $profileDir -Force | Out-Null }
if ([string]::IsNullOrWhiteSpace($Version)) { $Version = 'auto' }
@{
    version = $Version
    loader = $Loader
    user_override = ($Version -ne 'auto' -or $Loader -ne 'auto')
    target_pid = $ProcId
    updated_utc = [DateTime]::UtcNow.ToString('o')
} | ConvertTo-Json | Set-Content -LiteralPath $profilePath -Encoding UTF8

try {
    $resume = [System.Threading.EventWaitHandle]::OpenExisting("Local\SakuraTools.Resume.v1.$ProcId")
    try {
        [void]$resume.Set()
        Write-Host "Reactivation requested for resident proxy in PID $ProcId."
        Write-Host "Disk DLL was NOT loaded. Restart the target JVM to apply a new build."
    } finally { $resume.Dispose() }
    exit 0
} catch [System.Threading.WaitHandleCannotBeOpenedException] {
    # First load: no resident controller exists yet.
}

$asm = [System.Reflection.Emit.AssemblyBuilder]::DefineDynamicAssembly(
    [System.Reflection.AssemblyName]::new('DynInj'),
    [System.Reflection.Emit.AssemblyBuilderAccess]::Run)
$mod = $asm.DefineDynamicModule('m')
$tb  = $mod.DefineType('N', 'Public, Class')
function Add-PInvoke($tb, $name, $ret, $params) {
    $mb = $tb.DefinePInvokeMethod($name, 'kernel32.dll', $name,
        [System.Reflection.MethodAttributes]'Public, Static, PinvokeImpl',
        [System.Runtime.InteropServices.CallingConventions]::Standard, $ret, $params,
        [System.Runtime.InteropServices.CallingConvention]::Winapi,
        [System.Runtime.InteropServices.CharSet]::Ansi)
    $mb.SetImplementationFlags($mb.GetMethodImplementationFlags() -bor 'PreserveSig')
}
Add-PInvoke $tb 'OpenProcess' ([IntPtr]) ([uint32],[bool],[int])
Add-PInvoke $tb 'VirtualAllocEx' ([IntPtr]) ([IntPtr],[IntPtr],[uint32],[uint32],[uint32])
Add-PInvoke $tb 'WriteProcessMemory' ([bool]) ([IntPtr],[IntPtr],[byte[]],[uint32],[UIntPtr].MakeByRefType())
Add-PInvoke $tb 'GetModuleHandleA' ([IntPtr]) ([string])
Add-PInvoke $tb 'GetProcAddress' ([IntPtr]) ([IntPtr],[string])
Add-PInvoke $tb 'CreateRemoteThread' ([IntPtr]) ([IntPtr],[IntPtr],[uint32],[IntPtr],[IntPtr],[uint32],[uint32].MakeByRefType())
Add-PInvoke $tb 'WaitForSingleObject' ([uint32]) ([IntPtr],[uint32])
Add-PInvoke $tb 'GetExitCodeThread' ([bool]) ([IntPtr],[uint32].MakeByRefType())
Add-PInvoke $tb 'CloseHandle' ([bool]) ([IntPtr])
$N = $tb.CreateType()
function Err() { [Runtime.InteropServices.Marshal]::GetLastWin32Error() }
$hProc = $N::OpenProcess(0x1F0FFF, $false, $ProcId)
if ($hProc -eq [IntPtr]::Zero) { Write-Host "OpenProcess: err=$(Err)"; exit 1 }
try {
    $path = [Text.Encoding]::ASCII.GetBytes($Dll + "`0")
    $remote = $N::VirtualAllocEx($hProc, [IntPtr]::Zero, [uint32]$path.Length, 0x3000, 0x04)
    if ($remote -eq [IntPtr]::Zero) { Write-Host "VirtualAllocEx: err=$(Err)"; exit 1 }
    $wr = [UIntPtr]::Zero
    if (-not $N::WriteProcessMemory($hProc, $remote, $path, [uint32]$path.Length, [ref]$wr)) { Write-Host "WriteProcessMemory: err=$(Err)"; exit 1 }
    $k32 = $N::GetModuleHandleA('kernel32.dll')
    $llA = $N::GetProcAddress($k32, 'LoadLibraryA')
    $tid = [uint32]0
    $hT = $N::CreateRemoteThread($hProc, [IntPtr]::Zero, 0, $llA, $remote, 0, [ref]$tid)
    if ($hT -eq [IntPtr]::Zero) { Write-Host "CreateRemoteThread: err=$(Err)"; exit 1 }
    try {
        [void]$N::WaitForSingleObject($hT, 15000)
        $exit = [uint32]0
        [void]$N::GetExitCodeThread($hT, [ref]$exit)
    } finally { [void]$N::CloseHandle($hT) }
} finally { [void]$N::CloseHandle($hProc) }
if ($exit -ne 0) { Write-Host "LoadLibraryA returned low32=0x$($exit.ToString('x'))"; exit 0 }
Write-Host 'LoadLibraryA returned NULL'; exit 1
