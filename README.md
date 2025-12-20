# Поисковая система по статьям о материнстве

Проект собирает статьи с сайтов `7ya.ru`, `mama.ru`, `letidor.ru`, индексирует их и позволяет выполнять булев поиск.

## Как работает система

1. **Сбор** (`crawler.py`) → загружает статьи → сохраняет в БД.
2. **Экспорт** (`export_clean_text.py`) → извлекает `clean_text` → пишет в `docs/`.
3. **Токенизация** (`tokenizer.exe`) → разбивает тексты на токены → пишет в `tokens/`.
4. **Стемминг** (`stemmer.exe`) → нормализует токены → пишет в `stems/`.
5. **Индексация** (`indexer.exe`) → строит `boolean_index.txt` из `stems/`.
6. **Поиск** (`searcher.exe`) → принимает запрос → использует индекс → выводит результаты.
**Замечание**: searcher.exe может выполнять как полный поиск с выводом ID документов, названий статей и ссылок на статьи при наличии данных в бд, так и поиск с выводом ID документов.

## Что нужно

- **Windows** (PowerShell или CMD)
- **Python 3.8+**
- **g++**
- **PostgreSQL 16**
- Библиотеки Python (см. `crawler/requirements.txt`)
(Убедитесь, что `python`, `g++` и `psql` доступны из командной строки (добавлены в PATH).)

## Быстрый запуск
1. [Скачайте full_corpus_dump.zip (5,38 ГБ)](https://drive.google.com/file/d/1W2yfCEE_nayc9hkZmfkBZQ5o8fEL55kB/view?usp=drive_link).
2. Распакуйте его и расположите в корне проекта.
3. Убедитесь, что PostgreSQL запущен. Создайте структуру таблицы: 
`psql -U postgres -h localhost -f setup_db.sql`.
4. Загрузите данные: 
`psql -U postgres -h localhost -d search_corpus -f full_corpus_dump.sql`.
5. Настройте файл .env: DB_PASSWORD=ваш_пароль_от_postgres. (Убедитесь в правильности написания config.yaml db).
6. Запустите поиск: `cd searcher`
    - `searcher.exe` - выводит ID документов, название статьи и ссылку на статью;
    - `searcher.exe --ids-only` - выводит только ID документов.

## Полноценный запуск

1. Создайте базу данных:
`psql -U postgres -h localhost -f setup_db.sql`.
2. Настройте файл .env: DB_PASSWORD=ваш_пароль_от_postgres. (Убедитесь в правильности написания config.yaml db).
3. Установите зависимости Python: 
`pip install -r crawler/requirements.txt`.
4. Соберите C++ программы: 
`build_cpp.bat`.
5. Запустите полный пайплайн (если PostgreSQL установлен в другом месте, отредактируйте путь вручную): 
`run_full_pipeline.bat`.
6. Запустите поиск: `cd searcher`
    - `searcher.exe` - выводит ID документов, название статьи и ссылку на статью;
    - `searcher.exe --ids-only` - выводит только ID документов.

### Автор: Кайдалова Александра