#!/usr/bin/env python3
"""
Migration script to add missing columns to match_history table
"""
import sqlite3
import sys

def migrate_database(db_path='battleship.db'):
    print(f"Migrating database: {db_path}")
    
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Check if columns already exist
        cursor.execute("PRAGMA table_info(match_history)")
        columns = [row[1] for row in cursor.fetchall()]
        
        migrations_needed = []
        
        if 'player1_total_shots' not in columns:
            migrations_needed.append("ALTER TABLE match_history ADD COLUMN player1_total_shots INTEGER DEFAULT 0")
        
        if 'player2_total_shots' not in columns:
            migrations_needed.append("ALTER TABLE match_history ADD COLUMN player2_total_shots INTEGER DEFAULT 0")
        
        if 'game_duration_seconds' not in columns:
            migrations_needed.append("ALTER TABLE match_history ADD COLUMN game_duration_seconds INTEGER DEFAULT 0")
        
        if 'match_data' not in columns:
            migrations_needed.append("ALTER TABLE match_history ADD COLUMN match_data TEXT")
        
        if not migrations_needed:
            print("✅ Database is already up to date!")
            return True
        
        print(f"📝 Running {len(migrations_needed)} migrations...")
        for sql in migrations_needed:
            print(f"  - {sql}")
            cursor.execute(sql)
        
        conn.commit()
        conn.close()
        
        print("✅ Migration completed successfully!")
        print("\n💡 You can now restart the server and matches will be saved correctly.")
        return True
        
    except Exception as e:
        print(f"❌ Migration failed: {e}")
        return False

if __name__ == '__main__':
    db_path = sys.argv[1] if len(sys.argv) > 1 else 'battleship.db.backup'
    success = migrate_database(db_path)
    sys.exit(0 if success else 1)
