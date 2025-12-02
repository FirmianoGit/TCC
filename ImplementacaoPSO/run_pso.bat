@echo off
REM Script para executar PSO com o caminho CORRETO do executavel

setlocal enabledelayedexpansion

echo ======================================
echo PSO - Batch Processing
echo ======================================
echo.

REM Caminho completo (ja sabemos que existe)
set EXE_PATH=C:\Users\Firmiano\Desktop\TCC\ImplementacaoPSO\cmake-build-debug\scheduling_pso.exe
set PROJECT_DIR=C:\Users\Firmiano\Desktop\TCC\ImplementacaoPSO

echo Executavel: %EXE_PATH%
echo.

REM Verificar se o executavel existe
if not exist "%EXE_PATH%" (
    echo ERRO: Arquivo nao encontrado!
    echo %EXE_PATH%
    pause
    exit /b 1
)

echo OK: Executavel encontrado
echo.

REM Criar diretorios se necessario
if not exist "%PROJECT_DIR%\Instancias" mkdir "%PROJECT_DIR%\Instancias"
if not exist "%PROJECT_DIR%\Resultados" mkdir "%PROJECT_DIR%\Resultados"

REM Verificar se tem arquivos
dir /B "%PROJECT_DIR%\Instancias\I*.txt" >nul 2>&1
if !ERRORLEVEL! NEQ 0 (
    echo ERRO: Nenhum arquivo I*.txt em Instancias\
    mkdir "%PROJECT_DIR%\Instancias"
    pause
    exit /b 0
)

echo ======================================
echo EXECUTANDO PSO
echo ======================================
echo.

cd /D "%PROJECT_DIR%"

"%EXE_PATH%" --instances Instancias --output Resultados --popsize 2000 --generations 2000

echo.
if !ERRORLEVEL! EQU 0 (
    echo ======================================
    echo SUCESSO! Processamento concluido
    echo ======================================
    echo.
    echo Resultados em: %PROJECT_DIR%\Resultados\
    dir /B "%PROJECT_DIR%\Resultados\*.csv"
) else (
    echo ERRO na execucao
)

echo.
pause
