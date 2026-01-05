#!/usr/bin/env python3
"""Script to populate database with test users for leaderboard testing"""

import sqlite3
import os

DB_FILE = "battleship.db"

# Test users with different ELO ratings
test_users = [
    ("Alice", "123", 1500, 50, 30),
    ("Bob", "123", 1450, 45, 25),
    ("Charlie", "123", 1400, 40, 20),
    ("David", "123", 1350, 35, 18),
    ("Emma", "123", 1300, 30, 15),
    ("Frank", "123", 1250, 25, 12),
    ("Grace", "123", 1200, 20, 10),
    ("Henry", "123", 1150, 15, 7),
    ("Ivy", "123", 1100, 10, 4),
    ("Jack", "123", 1050, 5, 2),
]

def main():
    if not os.path.exists(DB_FILE):
        print(f"Error: {DB_FILE} not found!")
        print("Please run the server first to create the database.")
        return

    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()

    print("Populating database with test users...\n")

    for username, password, elo, total_games, wins in test_users:
        losses = total_games - wins

        try:
            # Check if user exists
            cursor.execute("SELECT user_id FROM users WHERE username = ?", (username,))
            existing = cursor.fetchone()

            if existing:
                # Update existing user
                cursor.execute("""
                    UPDATE users
                    SET elo_rating = ?, total_games = ?, wins = ?, losses = ?
                    WHERE username = ?
                """, (elo, total_games, wins, losses, username))
                print(f"✓ Updated {username}: ELO={elo}, Games={total_games}, Wins={wins}")
            else:
                # Insert new user
                cursor.execute("""
                    INSERT INTO users (username, password, elo_rating, total_games, wins, losses)
                    VALUES (?, ?, ?, ?, ?, ?)
                """, (username, password, elo, total_games, wins, losses))
                print(f"✓ Created {username}: ELO={elo}, Games={total_games}, Wins={wins}")

        except sqlite3.Error as e:
            print(f"✗ Error with {username}: {e}")

    conn.commit()
    conn.close()

    print("\n✅ Done! Test users created successfully!")
    print("You can login with any username above, password: 123")
    print("\nLeaderboard preview:")

    # Show leaderboard
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    cursor.execute("""
        SELECT username, elo_rating, total_games, wins
        FROM users
        ORDER BY elo_rating DESC
        LIMIT 10
    """)

    print("\nRank | Username  | ELO  | Games | Wins")
    print("-" * 45)
    for i, row in enumerate(cursor.fetchall(), 1):
        print(f"{i:4} | {row[0]:9} | {row[1]:4} | {row[2]:5} | {row[3]:4}")

    conn.close()

if __name__ == "__main__":
    main()
