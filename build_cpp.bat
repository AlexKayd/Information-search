@echo off
setlocal

echo [1/4] Сборка tokenizer.exe...
g++ -std=c++17 -O2 preprocessor/tokenizer.cpp -o preprocessor/tokenizer.exe
if errorlevel 1 (
    echo Ошибка при сборке tokenizer.exe
    exit /b 1
)

echo [2/4] Сборка stemmer.exe...
g++ -std=c++17 -O2 preprocessor/stemmer.cpp -o preprocessor/stemmer.exe
if errorlevel 1 (
    echo Ошибка при сборке stemmer.exe
    exit /b 1
)

echo [3/4] Сборка indexer.exe...
g++ -std=c++17 -O2 searcher/indexer.cpp -o searcher/indexer.exe
if errorlevel 1 (
    echo Ошибка при сборке indexer.exe
    exit /b 1
)

echo [4/4] Сборка searcher.exe (требуется PostgreSQL 16)...
g++ -std=c++17 -O2 searcher/searcher.cpp ^
    -I"C:\Program Files\PostgreSQL\16\include" ^
    -L"C:\Program Files\PostgreSQL\16\lib" ^
    -lpq -o searcher/searcher.exe
if errorlevel 1 (
    echo Ошибка при сборке searcher.exe.
    echo Убедитесь, что PostgreSQL 16 установлен в "C:\Program Files\PostgreSQL\16".
    exit /b 1
)

echo.
echo Все C++ программы успешно скомпилированы.