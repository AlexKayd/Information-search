@echo off
setlocal

:: Проверка .env
if not exist .env (
    echo Файл .env не найден.
    echo Создайте .env на основе .env.example и укажите пароль от PostgreSQL.
    pause
    exit /b 1
)

:: Проверка PostgreSQL
where psql >nul 2>&1
if errorlevel 1 (
    echo Команда psql не найдена. Убедитесь, что PostgreSQL установлен и добавлен в PATH.
    pause
    exit /b 1
)

:: Проверка Python
where python >nul 2>&1
if errorlevel 1 (
    echo Python не найден. Установите Python 3.8+ и добавьте его в PATH.
    pause
    exit /b 1
)

:: Проверка g++
where g++ >nul 2>&1
if errorlevel 1 (
    echo g++ не найден. Установите MinGW-w64 или MSYS2 и добавьте в PATH.
    pause
    exit /b 1
)

:: Инициализация БД
echo [1/7] Создание таблицы в PostgreSQL...
psql -U postgres -h localhost -f setup_db.sql
if errorlevel 1 (
    echo Ошибка при создании таблицы. Убедитесь, что PostgreSQL запущен и пользователь "postgres" существует.
    pause
    exit /b 1
)

:: Установка Python-зависимостей
echo [2/7] Установка зависимостей Python...
pip install -r crawler/requirements.txt
if errorlevel 1 (
    echo Ошибка при установке Python-пакетов.
    pause
    exit /b 1
)

:: Сборка C++ программ
echo [3/7] Сборка C++ утилит...
if not exist preprocessor\tokenizer.exe call build_cpp.bat
if errorlevel 1 exit /b 1

:: Шаг 4: Сбор данных
echo [4/7] Запуск crawler.py...
cd crawler
python crawler.py ../config.yaml
if errorlevel 1 (
    echo Ошибка в crawler.py
    cd ..
    pause
    exit /b 1
)
cd ..

:: Экспорт текстов
echo [5/7] Экспорт clean_text из БД...
python preprocessor/export_clean_text.py config.yaml
if errorlevel 1 (
    echo Ошибка в export_clean_text.py
    pause
    exit /b 1
)

:: Токенизация и стемминг
echo [6/7] Токенизация и стемминг...
preprocessor\tokenizer.exe
preprocessor\stemmer.exe

:: Индексация
echo [7/7] Построение булева индекса...
searcher\indexer.exe

echo.
echo Проект готов.
echo Можете запустить поиск:
echo   searcher\searcher.exe
echo.
pause