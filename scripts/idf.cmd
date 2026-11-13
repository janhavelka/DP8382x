@echo off
rem Run idf.py with the pinned ESP-IDF v6.1 (see scripts\idf.ps1).
rem Bypass applies to this one PowerShell process only; no system policy is changed.
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0idf.ps1" %*
exit /b %ERRORLEVEL%
