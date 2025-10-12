@echo off
setlocal enabledelayedexpansion
set "run_result=0"
set "xml_output=test.xml"
set "output_file=test.txt"

REM Check test result
if "!run_result!"=="124" (
    echo Timeout
    echo test>> timeout.txt
    echo test>> failed.txt
) else (
    if "!run_result!"=="125" (
        echo Crashed
        echo test>> crashed.txt
        echo test>> failed.txt
    ) else (
        REM Check exit code
        if exist "%xml_output%" (
            echo XML exists
        ) else (
            if exist "%output_file%" (
                echo Output exists
            ) else (
                echo No output
            )
        )
    )
)

echo Done

