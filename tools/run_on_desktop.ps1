param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath,
    [string]$ArgumentList = "",
    [string]$WorkingDirectory = "",
    [string]$DesktopName = "fear",
    [switch]$Switch,
    [int]$SwitchBackSeconds = 0,
    [switch]$Wait,
    [switch]$KeepDesktop,
    [string]$LogPath = ""
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $FilePath)) {
    throw "Executable not found: $FilePath"
}
if (-not $WorkingDirectory) {
    $WorkingDirectory = Split-Path -Parent (Resolve-Path -LiteralPath $FilePath).ProviderPath
}

Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class FearDesktop
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct STARTUPINFO
    {
        public int cb;
        public string lpReserved;
        public string lpDesktop;
        public string lpTitle;
        public int dwX;
        public int dwY;
        public int dwXSize;
        public int dwYSize;
        public int dwXCountChars;
        public int dwYCountChars;
        public int dwFillAttribute;
        public int dwFlags;
        public short wShowWindow;
        public short cbReserved2;
        public IntPtr lpReserved2;
        public IntPtr hStdInput;
        public IntPtr hStdOutput;
        public IntPtr hStdError;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PROCESS_INFORMATION
    {
        public IntPtr hProcess;
        public IntPtr hThread;
        public int dwProcessId;
        public int dwThreadId;
    }

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr CreateDesktopW(string lpszDesktop, IntPtr lpszDevice, IntPtr pDevmode, int dwFlags, uint dwDesiredAccess, IntPtr lpsa);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr OpenDesktopW(string lpszDesktop, int dwFlags, bool fInherit, uint dwDesiredAccess);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr hDesktop);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SwitchDesktop(IntPtr hDesktop);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool CreateProcessW(string lpApplicationName, string lpCommandLine, IntPtr lpProcessAttributes, IntPtr lpThreadAttributes, bool bInheritHandles, uint dwCreationFlags, IntPtr lpEnvironment, string lpCurrentDirectory, ref STARTUPINFO lpStartupInfo, out PROCESS_INFORMATION lpProcessInformation);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr CreateFileW(string lpFileName, uint dwDesiredAccess, uint dwShareMode, IntPtr lpSecurityAttributes, uint dwCreationDisposition, uint dwFlagsAndAttributes, IntPtr hTemplateFile);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool SetHandleInformation(IntPtr hObject, uint dwMask, uint dwFlags);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool CloseHandle(IntPtr hObject);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);
}
'@

$desktopAccess = 0x10000000
$logHandle = [IntPtr]::Zero
$switched = $false
$restored = $false
$defaultDesktop = [IntPtr]::Zero
$desktop = [FearDesktop]::OpenDesktopW($DesktopName, 0, $false, $desktopAccess)
if ($desktop -eq [IntPtr]::Zero) {
    $desktop = [FearDesktop]::CreateDesktopW($DesktopName, [IntPtr]::Zero, [IntPtr]::Zero, 0, $desktopAccess, [IntPtr]::Zero)
}
if ($desktop -eq [IntPtr]::Zero) {
    $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
    throw "Unable to open or create desktop '$DesktopName' (Win32 error $code)"
}

try {
    if ($Switch) {
        $defaultDesktop = [FearDesktop]::OpenDesktopW("Default", 0, $false, $desktopAccess)
        if (-not [FearDesktop]::SwitchDesktop($desktop)) {
            $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
            throw "SwitchDesktop failed (Win32 error $code)"
        }
        $switched = $true
    }

    $quotedArgs = $ArgumentList
    $commandLine = '"' + (Resolve-Path -LiteralPath $FilePath).ProviderPath + '"'
    if ($quotedArgs) {
        $commandLine += " " + $quotedArgs
    }

    $startup = New-Object FearDesktop+STARTUPINFO
    $startup.cb = [Runtime.InteropServices.Marshal]::SizeOf([type][FearDesktop+STARTUPINFO])
    $startup.lpDesktop = "winsta0\" + $DesktopName
    $inheritHandles = $false
    if ($LogPath) {
        if ([IO.Path]::IsPathRooted($LogPath)) {
            $logFullPath = [IO.Path]::GetFullPath($LogPath)
        } else {
            $logFullPath = [IO.Path]::GetFullPath((Join-Path $WorkingDirectory $LogPath))
        }
        $logDirectory = Split-Path -Parent $logFullPath
        if ($logDirectory) {
            New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
        }
        $logHandle = [FearDesktop]::CreateFileW(
            $logFullPath,
            0x40000000,
            0x00000001,
            [IntPtr]::Zero,
            2,
            0x80,
            [IntPtr]::Zero)
        if ($logHandle -eq [IntPtr]::Zero -or $logHandle -eq [IntPtr](-1)) {
            throw "Unable to create log file '$logFullPath' (Win32 error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))"
        }
        [FearDesktop]::SetHandleInformation($logHandle, 1, 1) | Out-Null
        $startup.dwFlags = 0x100
        $startup.hStdOutput = $logHandle
        $startup.hStdError = $logHandle
        $inheritHandles = $true
    }
    $processInfo = New-Object FearDesktop+PROCESS_INFORMATION
    $created = [FearDesktop]::CreateProcessW(
        (Resolve-Path -LiteralPath $FilePath).ProviderPath,
        $commandLine,
        [IntPtr]::Zero,
        [IntPtr]::Zero,
        $inheritHandles,
        0,
        [IntPtr]::Zero,
        $WorkingDirectory,
        [ref]$startup,
        [ref]$processInfo)
    if (-not $created) {
        $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "CreateProcess failed (Win32 error $code)"
    }

    Write-Host "Started PID $($processInfo.dwProcessId) on desktop '$DesktopName'"
    if ($switched -and -not $KeepDesktop -and $SwitchBackSeconds -gt 0 -and $defaultDesktop -ne [IntPtr]::Zero) {
        Start-Sleep -Seconds $SwitchBackSeconds
        if ([FearDesktop]::SwitchDesktop($defaultDesktop)) {
            $restored = $true
        } else {
            Write-Warning "Unable to restore the default desktop (Win32 error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))"
        }
    }
    if ($Wait) {
        [FearDesktop]::WaitForSingleObject($processInfo.hProcess, [uint32]::MaxValue) | Out-Null
        Write-Host "Process $($processInfo.dwProcessId) exited"
    }
    if ($switched -and -not $restored -and -not $KeepDesktop -and $defaultDesktop -ne [IntPtr]::Zero) {
        if ([FearDesktop]::SwitchDesktop($defaultDesktop)) {
            $restored = $true
        } else {
            Write-Warning "Unable to restore the default desktop (Win32 error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))"
        }
    }
    [FearDesktop]::CloseHandle($processInfo.hThread) | Out-Null
    [FearDesktop]::CloseHandle($processInfo.hProcess) | Out-Null
}
finally {
    if ($logHandle -ne [IntPtr]::Zero) {
        [FearDesktop]::CloseHandle($logHandle) | Out-Null
    }
    if ($switched -and -not $restored -and -not $KeepDesktop -and $defaultDesktop -ne [IntPtr]::Zero) {
        if (-not [FearDesktop]::SwitchDesktop($defaultDesktop)) {
            Write-Warning "Unable to restore the default desktop during cleanup (Win32 error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))"
        }
    }
    if ($defaultDesktop -ne [IntPtr]::Zero) {
        [FearDesktop]::CloseDesktop($defaultDesktop) | Out-Null
    }
    [FearDesktop]::CloseDesktop($desktop) | Out-Null
}
