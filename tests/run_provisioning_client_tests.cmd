@echo off
setlocal
cd /d "%~dp0\.."
python -m unittest discover -s tests\python -v
if errorlevel 1 exit /b 1
python -m py_compile tools\credential_provisioning_client.py tests\python\test_credential_provisioning_client.py
if errorlevel 1 exit /b 1
echo PASS: Python provisioning client tests and syntax check
exit /b 0
