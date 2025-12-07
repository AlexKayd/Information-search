import os
import sys
import re
import time
import yaml
import psycopg2
from dotenv import load_dotenv

load_dotenv()

MAX_TEXT_LENGTH = 100_000
SHORT_TOKENS = {"я", "ты", "вы", "он", "она", "оно", "мы", "они"}
STOP_WORDS = {"в", "с", "у", "о", "к", "а", "и", "но", "да", "же", "ли", "бы", "то","от", "на", "по", "за", "из", "до", "со"}

def load_known_abbrevs(path="known_abbrevs.txt"):
    if not os.path.exists(path):
        print(f"Файл known_abbrevs.txt не найден")
        return set()
    with open(path, 'r', encoding='utf-8') as f:
        return {line.strip() for line in f if line.strip()}

KNOWN_ABBREVS = load_known_abbrevs()

def tokenize(text):
    if not text or not isinstance(text, str):
        return []
    if len(text) > MAX_TEXT_LENGTH:
        text = text[:MAX_TEXT_LENGTH]
    tokens = re.findall(r"[а-яёa-z0-9]+(?:-[а-яёa-z0-9]+)*", text.lower())
    result = []
    for t in tokens:
        if t.upper() in KNOWN_ABBREVS:
            result.append(t.upper())
        elif t in SHORT_TOKENS:
            result.append(t)
        elif len(t) >= 2 and t not in STOP_WORDS:
            result.append(t)
    return result

def save_tokens(doc_id, tokens, out_dir="tokens"):
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, f"{doc_id}.tokens"), "w", encoding="utf-8") as f:
        f.write("\n".join(tokens))

def main(config_path):
    with open(config_path, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)

    try:
        conn = psycopg2.connect(
            host=config['db']['host'],
            port=config['db']['port'],
            database=config['db']['database'],
            user=config['db']['user'],
            password=os.getenv('DB_PASSWORD')
        )
        print("Подключено к бд")
    except Exception as e:
        print(f"Ошибка подключения к бд: {e}")
        sys.exit(1)

    cur = conn.cursor()
    cur.execute("SELECT id, clean_text FROM documents ORDER BY id")

    total_tokens = 0
    total_input_bytes = 0
    total_token_chars = 0 
    processed_docs = 0
    start_time = time.time()

    for doc_id, clean_text in cur:
        if not clean_text or not clean_text.strip():
            continue

        text_bytes = clean_text.encode('utf-8')
        total_input_bytes += len(text_bytes)
        tokens = tokenize(clean_text)
        save_tokens(doc_id, tokens)
        total_tokens += len(tokens)
        total_token_chars += sum(len(token) for token in tokens)
        processed_docs += 1

        if processed_docs % 1000 == 0:
            elapsed = time.time() - start_time
            total_kb = total_input_bytes / 1024
            speed_kb_sec = total_kb / elapsed if elapsed > 0 else 0
            print(f"Обработано {processed_docs} документов, токенов: {total_tokens}, скорость: {speed_kb_sec:.2f} КБ/сек")

    duration = time.time() - start_time
    total_kb = total_input_bytes / 1024
    speed_kb_sec = total_kb / duration
    avg_token_len_chars = total_token_chars / total_tokens
    avg_token_len_bytes = total_input_bytes / total_tokens

    print(f"\nДокументов обработано: {processed_docs}")
    print(f"Всего токенов: {total_tokens:,}")
    print(f"Средняя длина токена: {avg_token_len_chars:.2f} символов ({avg_token_len_bytes:.2f} байт)")
    print(f"Время выполнения: {duration:.2f} сек")
    print(f"Скорость токенизации: {speed_kb_sec:.2f} КБ/сек")

    conn.close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(1)
    main(sys.argv[1])