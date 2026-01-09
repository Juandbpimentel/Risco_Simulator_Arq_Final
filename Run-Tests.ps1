param(
    [string[]]$Tests = @('ex1','ex2','ex3','ex4','ex5'),
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

function Write-TestInputFile {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string[]]$Lines
    )

    $path = Join-Path $root ($Name + '.in')
    Set-Content -Path $path -Value $Lines -Encoding ascii
    return $path
}

function Invoke-Simulator {
    param(
        [Parameter(Mandatory=$true)][string]$InputPath
    )

    if (-not (Test-Path -Path $InputPath)) {
        throw "Arquivo de entrada nao encontrado: $InputPath"
    }

    $exe = Join-Path $root 'projetoFinal.exe'
    if (-not (Test-Path -Path $exe)) {
        throw "Executavel nao encontrado: $exe (compile primeiro)"
    }

    Get-Content -Path $InputPath | & $exe
}

if (-not $NoBuild) {
    $gcc = (Get-Command gcc -ErrorAction SilentlyContinue)
    if (-not $gcc) {
        throw 'gcc nao encontrado no PATH. Instale MinGW-w64/clang ou use -NoBuild se ja tiver o .exe.'
    }

    Write-Host 'Compilando projetoFinal.c ...'
    & gcc -std=c11 -Wall -Wextra -O2 -o projetoFinal.exe projetoFinal.c
}

$testsToRun = $Tests | ForEach-Object { $_.ToLowerInvariant() }

if ($testsToRun -contains 'ex1') {
    Write-Host "`n=== Exemplo 1 ==="
    $in1 = Write-TestInputFile -Name 'ex1' -Lines @(
        '0',
        '0000 0054',
        '0001 1056',
        '0002 2158',
        '0003 3125',
        '0004 4327',
        '0005 540A',
        '0006 6509',
        '0007 752C',
        '0008 872B',
        '0009 FFFF',
        '0000 0000'
    )
    Invoke-Simulator -InputPath $in1
}

if ($testsToRun -contains 'ex2') {
    Write-Host "`n=== Exemplo 2 ==="
    $in2 = Write-TestInputFile -Name 'ex2' -Lines @(
        '0',
        '0000 00C4',
        '0001 1002',
        '0002 2012',
        '0003 3022',
        '0004 001E',
        '0005 002E',
        '0006 003E',
        '0007 4125',
        '0008 4435',
        '0009 3043',
        '000A 500F',
        '000B FFFF',
        '000C 000A',
        '000D 0014',
        '000E 001E',
        '000F 0000',
        '0000 0000'
    )
    Invoke-Simulator -InputPath $in2
}

if ($testsToRun -contains 'ex3') {
    Write-Host "`n=== Exemplo 3 ==="
    $in3 = Write-TestInputFile -Name 'ex3' -Lines @(
        '0',
        '0000 00A4',
        '0001 3004',
        '0002 1002',
        '0003 0016',
        '0004 2002',
        '0005 3325',
        '0006 1118',
        '0007 7FB1',
        '0008 1033',
        '0009 FFFF',
        '000A 0005',
        '000B 0001',
        '000C 0002',
        '000D 0003',
        '000E 0004',
        '000F 0005',
        '0010 0000',
        '0000 0000'
    )
    Invoke-Simulator -InputPath $in3
}

if ($testsToRun -contains 'ex4') {
    Write-Host "`n=== Exemplo 4 ==="
    $in4 = Write-TestInputFile -Name 'ex4' -Lines @(
        '0',
        '0000 00B4',
        '0001 A002',
        '0002 1A22',
        '0003 2A22',
        '0004 3125',
        '0005 3A33',
        '0006 030D',
        '0007 BFA1',
        '0008 9414',
        '0009 1A93',
        '000A FFFF',
        '000B F000',
        '0000 0000',
        '1',
        '1',
        '6',
        '6'
    )
    Invoke-Simulator -InputPath $in4
}

if ($testsToRun -contains 'ex5') {
    Write-Host "`n=== teste_normal/example5.asm ==="

    $python = (Get-Command python -ErrorAction SilentlyContinue)
    if (-not $python) {
        throw 'python nao encontrado no PATH (necessario para rodar CompiladorDeAssembly.py).'
    }

    $asm = Join-Path $root 'teste_normal\example5.asm'
    if (-not (Test-Path -Path $asm)) {
        throw "Arquivo asm nao encontrado: $asm"
    }

    $programPath = Join-Path $root 'ex5prog.txt'
    $in5Path = Join-Path $root 'ex5.in'

    & python .\CompiladorDeAssembly.py $asm | Set-Content -Path $programPath -Encoding ascii

    $lines = @('0') + (Get-Content -Path $programPath)
    Set-Content -Path $in5Path -Value $lines -Encoding ascii

    Invoke-Simulator -InputPath $in5Path
}

Write-Host "`nPronto." -ForegroundColor Green
