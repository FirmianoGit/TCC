@echo off
REM Script para executar GA em batch usando o executavel do CMake

setlocal enabledelayedexpansion

echo ======================================
echo GA - Batch Processing (CMake)
echo ======================================
echo.

REM Caminho do executavel
set EXE_PATH=C:\Users\Firmiano\Desktop\TCC\ImplementacaoGA\cmake-build-debug\scheduling_genetic_algorithm.exe
set PROJECT_DIR=C:\Users\Firmiano\Desktop\TCC\ImplementacaoGA

echo Executavel: %EXE_PATH%
echo.
echo.

REM Criar diretorios se necessario
if not exist "%PROJECT_DIR%\Instancias" mkdir "%PROJECT_DIR%\Instancias"
if not exist "%PROJECT_DIR%\Permutacoes" mkdir "%PROJECT_DIR%\Permutacoes"
if not exist "%PROJECT_DIR%\Resultados" mkdir "%PROJECT_DIR%\Resultados"

REM Verificar se tem arquivos
dir /B "%PROJECT_DIR%\Instancias\I*.txt" >nul 2>&1
if !ERRORLEVEL! NEQ 0 (
    echo ERRO: Nenhum arquivo I*.txt em Instancias\
    mkdir "%PROJECT_DIR%\Instancias"
    pause
    exit /b 0
)

dir /B "%PROJECT_DIR%\Permutacoes\P*.txt" >nul 2>&1
if !ERRORLEVEL! NEQ 0 (
    echo ERRO: Nenhum arquivo P*.txt em Permutacoes\
    mkdir "%PROJECT_DIR%\Permutacoes"
    pause
    exit /b 0
)

echo ======================================
echo EXECUTANDO GA EM BATCH
echo ======================================
echo.

cd /D "%PROJECT_DIR%"

"%EXE_PATH%" --instances Instancias --permutations Permutacoes --output Resultados --selection tournament --crossover obx --mutation insert --popsize 250 --pc 0.95 --pm 0.03 --time 200 --popsize 2000

if !ERRORLEVEL! EQU 0 (
    echo.
    echo ======================================
    echo SUCESSO! Processamento concluido
    echo ======================================
    echo.
    echo Resultados em: %PROJECT_DIR%\Resultados\
    dir /B "%PROJECT_DIR%\Resultados\*.csv"
) else (
    echo.
    echo ERRO na execucao do GA
)

echo.
pause
