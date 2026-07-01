@echo off
REM Phase 4 — C Agents Build (Windows)
REM Requires: MinGW gcc (or MSVC cl)
REM
REM Usage:
REM   build.bat          — build all agents
REM   build.bat clean    — remove compiled binaries
REM   build.bat test     — run pipeline on sample data

SET CC=gcc
SET CFLAGS=-Wall -Wextra -std=c11 -O2
SET SRC=agents_c

IF "%1"=="clean" GOTO clean
IF "%1"=="test" GOTO test

REM === Build all agents ===
echo Building data_agent...
%CC% %CFLAGS% -o %SRC%\data_agent.exe %SRC%\data_agent.c %SRC%\json_utils.c -lm
IF ERRORLEVEL 1 (echo FAILED: data_agent & exit /b 1)

echo Building analysis_agent...
%CC% %CFLAGS% -o %SRC%\analysis_agent.exe %SRC%\analysis_agent.c %SRC%\json_utils.c -lm
IF ERRORLEVEL 1 (echo FAILED: analysis_agent & exit /b 1)

echo Building classification_agent...
%CC% %CFLAGS% -o %SRC%\classification_agent.exe %SRC%\classification_agent.c %SRC%\json_utils.c -lm
IF ERRORLEVEL 1 (echo FAILED: classification_agent & exit /b 1)

echo All C agents built successfully.
GOTO end

:clean
echo Cleaning build artifacts...
del /Q %SRC%\*.exe 2>nul
del /Q %SRC%\*.o 2>nul
echo Cleaned.
GOTO end

:test
echo === Running C pipeline on sample data ===
%SRC%\data_agent.exe data\sample_metrics.csv > outputs\validated_data.json
echo --- data_agent output written to outputs\validated_data.json ---

%SRC%\analysis_agent.exe outputs\validated_data.json > outputs\analysis_results.json
echo --- analysis_agent output written to outputs\analysis_results.json ---

%SRC%\classification_agent.exe outputs\analysis_results.json > outputs\fleet_status.json
echo --- classification_agent output written to outputs\fleet_status.json ---

echo === C pipeline complete ===
GOTO end

:end
