import os
import sys
import yaml
import psycopg2
from dotenv import load_dotenv

load_dotenv()

def connect_to_database(config):
    db_conf = config['db']
    return psycopg2.connect(
        host=db_conf['host'],
        port=db_conf['port'],
        database=db_conf['database'],
        user=db_conf['user'],
        password=os.getenv('DB_PASSWORD')
    )

def main(config_path):
    with open(config_path, 'r', encoding='utf-8') as f:
        config = yaml.safe_load(f)

    conn = connect_to_database(config)
    cur = conn.cursor()
    cur.execute("SELECT id, clean_text FROM documents")
    os.makedirs("docs", exist_ok=True)

    count = 0
    for doc_id, text in cur:
        if text and text.strip():
            with open(f"docs/{doc_id}.txt", "w", encoding="utf-8") as f:
                f.write(text)
            count += 1

    print(f"Экспорт завершён. Сохранено {count} документов с текстом.")
    conn.close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(1)
    main(sys.argv[1])