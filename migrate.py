import json
import sqlite3
import os

json_path = 'data/app_list.json'
db_path = 'data/app_data.db'

def migrate():
    if not os.path.exists('data'): os.makedirs('data')
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("DROP TABLE IF EXISTS games")
    cursor.execute("CREATE TABLE games (appid INTEGER PRIMARY KEY, name TEXT)")
    cursor.execute("CREATE INDEX idx_name ON games(name)")

    print("Loading JSON...")
    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    print(f"Migrating {len(data)} games...")
    games_list = [(item['appid'], item['name']) for item in data]
    cursor.executemany("INSERT OR IGNORE INTO games VALUES (?, ?)", games_list)
    
    conn.commit()
    conn.close()
    print("Success! Data is ready for C++.")

if __name__ == "__main__":
    migrate()