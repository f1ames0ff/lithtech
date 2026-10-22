param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath,
    [string]$ArgumentList = "",
    [string]$WorkingDirectory = "",
    [string]$DesktopName = "fear",
    [switch]$Switch,
    [int]$SwitchBackSeconds = 0,
    [switch]$Wait
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

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool CloseHandle(IntPtr hObject);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);
}
'@

$desktopAccess = 0x10000000
$desktop = [FearDesktop]::OpenDesktopW($DesktopName, 0, $false, $desktopAccess)
if ($desktop -eq [IntPtr]::Zero) {
    $desktop = [FearDesktop]::CreateDesktopW($DesktopName, [IntPtr]::Zero, [IntPtr]::Zero, 0, $desktopAccess, [IntPtr]::Zero)
}
if ($desktop -eq [IntPtr]::Zero) {
    $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
    throw "Unable to open or create desktop '$DesktopName' (Win32 error $code)"
}

try {
    $defaultDesktop = [IntPtr]::Zero
    if ($Switch) {
        $defaultDesktop = [FearDesktop]::OpenDesktopW("Default", 0, $false, $desktopAccess)
        if (-not [FearDesktop]::SwitchDesktop($desktop)) {
            $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
            throw "SwitchDesktop failed (Win32 error $code)"
        }
    }

    $quotedArgs = $ArgumentList
    $commandLine = '"' + (Resolve-Path -LiteralPath $FilePath).ProviderPath + '"'
    if ($quotedArgs) {
        $commandLine += " " + $quotedArgs
    }

    $startup = New-Object FearDesktop+STARTUPINFO
    $startup.cb = [Runtime.InteropServices.Marshal]::SizeOf([type][FearDesktop+STARTUPINFO])
    $startup.lpDesktop = "winsta0\" + $DesktopName
    $processInfo = New-Object FearDesktop+PROCESS_INFORMATION
    $created = [FearDesktop]::CreateProcessW(
        (Resolve-Path -LiteralPath $FilePath).ProviderPath,
        $commandLine,
        [IntPtr]::Zero,
        [IntPtr]::Zero,
        $false,
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
    if ($Switch -and $SwitchBackSeconds -gt 0 -and $defaultDesktop -ne [IntPtr]::Zero) {
        Start-Sleep -Seconds $SwitchBackSeconds
        [FearDesktop]::SwitchDesktop($defaultDesktop) | Out-Null
    }
    if ($Wait) {
        [FearDesktop]::WaitForSingleObject($processInfo.hProcess, [uint32]::MaxValue) | Out-Null
        Write-Host "Process $($processInfo.dwProcessId) exited"
    }
    [FearDesktop]::CloseHandle($processInfo.hThread) | Out-Null
    [FearDesktop]::CloseHandle($processInfo.hProcess) | Out-Null
}
finally {
    if ($defaultDesktop -ne [IntPtr]::Zero) {
        [FearDesktop]::CloseDesktop($defaultDesktop) | Out-Null
    }
    [FearDesktop]::CloseDesktop($desktop) | Out-Null
}
