# PowerShell script to clean up build artifacts
Write-Host "Starting cleanup of build artifacts and temporary files..." -ForegroundColor Green

$rootPath = $PSScriptRoot
Write-Host "Working directory: $rootPath"

# Define file patterns to clean
$patterns = @(
    "*.obj",
    "*.pdb", 
    "*.ilk",
    "*.exp",
    "*.tmp",
    "*.temp",
    "*.log"
)

$totalFilesRemoved = 0

# Clean each pattern
foreach ($pattern in $patterns) {
    Write-Host "`nCleaning $pattern files..." -ForegroundColor Yellow
    
    $files = Get-ChildItem -Path $rootPath -Recurse -Name $pattern -ErrorAction SilentlyContinue
    
    if ($files) {
        foreach ($file in $files) {
            $fullPath = Join-Path $rootPath $file
            # Skip files in the main build directory (they should stay there)
            # But remove files in CMake build directories and root
            if (($fullPath -notmatch "\\build\\[^\\]*\.(exe|dll)$") -and 
                ($fullPath -match "\\build\\.+\\Debug\\" -or 
                 $fullPath -match "\\build\\.+\\Release\\" -or
                 $fullPath -notmatch "\\build\\")) {
                try {
                    Remove-Item $fullPath -Force -Verbose
                    $totalFilesRemoved++
                }
                catch {
                    Write-Warning "Could not remove: $fullPath"
                }
            }
        }
    }
    else {
        Write-Host "No $pattern files found"
    }
}

# Clean executables outside main build directory
Write-Host "`nCleaning .exe files outside main build directory..." -ForegroundColor Yellow
$exeFiles = Get-ChildItem -Path $rootPath -Recurse -Name "*.exe" -ErrorAction SilentlyContinue

if ($exeFiles) {
    foreach ($exe in $exeFiles) {
        $fullPath = Join-Path $rootPath $exe
        # Only keep executables in the main build directory
        if ($fullPath -notmatch "\\build\\[^\\]*\.exe$") {
            try {
                Remove-Item $fullPath -Force -Verbose
                $totalFilesRemoved++
            }
            catch {
                Write-Warning "Could not remove: $fullPath"
            }
        }
    }
}
else {
    Write-Host "No .exe files found outside main build directory"
}

# Clean CMake intermediate directories
Write-Host "`nCleaning CMake intermediate directories..." -ForegroundColor Yellow
$cmakeIntermediateDirs = @(
    "Project\build\client.dir",
    "Project\build\server.dir", 
    "Project\build\CMakeFiles"
)

foreach ($dir in $cmakeIntermediateDirs) {
    $fullDirPath = Join-Path $rootPath $dir
    if (Test-Path $fullDirPath) {
        try {
            Remove-Item $fullDirPath -Recurse -Force -Verbose
            Write-Host "Removed directory: $fullDirPath" -ForegroundColor Green
        }
        catch {
            Write-Warning "Could not remove directory: $fullDirPath"
        }
    }
}

Write-Host "`nCleanup completed!" -ForegroundColor Green
Write-Host "Total files removed: $totalFilesRemoved" -ForegroundColor Cyan

# Verify main build directory structure
if (Test-Path (Join-Path $rootPath "build")) {
    $buildFiles = Get-ChildItem -Path (Join-Path $rootPath "build") | Measure-Object
    Write-Host "Main build directory contains $($buildFiles.Count) files" -ForegroundColor Blue
}
else {
    Write-Host "Main build directory not found" -ForegroundColor Yellow
}

Write-Host "`nPress any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
