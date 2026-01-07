#!/usr/bin/env python3
# Migration script to add ship placement columns to match_history table

import sqlite3

DB_NAME = "battleship.db"

def migrate():
    conn = sqlite3.connect(DB_NAME)
    cur = conn.cursor()
    
    # Check if columns already exist
    cur.execute("PRAGMA table_info(match_history)")
    columns = [row[1] for row in cur.fetchall()]
    
    if 'player1_ships' in columns and 'player2_ships' in columns:
        print("✓ Columns player1_ships and player2_ships already exist")
        conn.close()
        return
    
    print("Adding ship placement columns to match_history table...")
    
    try:
        # Add player1_ships column
        if 'player1_ships' not in columns:
            cur.execute("ALTER TABLE match_history ADD COLUMN player1_ships TEXT")
            print("✓ Added column: player1_ships")
        
        # Add player2_ships column
        if 'player2_ships' not in columns:
            cur.execute("ALTER TABLE match_history ADD COLUMN player2_ships TEXT")
            print("✓ Added column: player2_ships")
        
        conn.commit()
        print("\n✓ Migration completed successfully!")
        
    except Exception as e:
        print(f"\n✗ Migration failed: {e}")
        conn.rollback()
    
    finally:
        conn.close()

if __name__ == "__main__":
    migrate()
