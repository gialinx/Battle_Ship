#!/usr/bin/env python3
import sqlite3

db = sqlite3.connect('battleship.db')
cursor = db.cursor()

print("=== USERS ===")
cursor.execute("SELECT user_id, username, total_games, wins, losses, elo_rating FROM users")
for row in cursor.fetchall():
    print(f"ID:{row[0]}, User:{row[1]}, Games:{row[2]}, W:{row[3]}, L:{row[4]}, ELO:{row[5]}")

print("\n=== MATCHES ===")
cursor.execute("SELECT match_id, player1_id, player2_id, winner_id, played_at FROM match_history ORDER BY played_at DESC LIMIT 10")
matches = cursor.fetchall()
print(f"Total matches in database: {len(matches)}")
for row in matches:
    print(f"Match ID:{row[0]}, P1:{row[1]} vs P2:{row[2]}, Winner:{row[3]}, Time:{row[4]}")

db.close()
