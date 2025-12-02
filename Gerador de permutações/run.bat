@echo off
echo ============================================================
echo EXECUTANDO TESTE DO ALGORITMO HFS
echo ============================================================

REM Verificar se os arquivos existem
if not exist "instance_small_01.txt" (
    echo [ERRO] Arquivo instance_small_01.txt nao encontrado!
    echo.
    echo Gerando instancias com Python...
    python ..\gerador_hfs_para_cpp.py
    echo.
)

REM Executar o programa
echo.
echo [INFO] Executando algoritmo...
echo.
Implementacao_com_codificacao.exe instance_small_01.txt permutation_small_01.txt 100

echo.
echo ============================================================
echo TESTE CONCLUIDO
echo ============================================================
pause
