@echo off
setlocal

:: Check if the first argument is "test" to enable testing
if "%1"=="test" (
    set TEST_PARAM=-DTEST=ON
) else (
    set TEST_PARAM=-DTEST=OFF
)

:: Create build directory if it doesn't exist
if not exist build mkdir build
cd build

:: Run cmake to configure the project with the test option
cmake .. %TEST_PARAM%

:: Build the project
cmake --build .

:: If testing is enabled, run the test executable
if "%TEST_PARAM%"=="-DTEST=ON" (
    :: Check for Windows (runTests.exe) and copy cmocka.dll if it exists
    if exist E:\ClawsoftSolutions\purrsock\build\test\Debug\runTests.exe (
        echo Running tests on Windows...

        :: Assuming CMocka was built in a directory like CMocka/build/bin/
        :: Adjust the source path to your cmocka.dll location (change if needed)
        copy E:\ClawsoftSolutions\purrsock\build\_deps\cmocka-build\src\Debug\cmocka.dll E:\ClawsoftSolutions\purrsock\build\test\Debug

        E:\ClawsoftSolutions\purrsock\build\test\Debug\runTests.exe
    ) else (
        echo Error: runTests.exe not found.
    )
    
    :: Check for Linux (runTests) and run the executable if it exists
    if exist runTests (
        echo Running tests on Linux...
        ./runTests
    ) else (
        echo Error: runTests not found.
    )
)

endlocal
