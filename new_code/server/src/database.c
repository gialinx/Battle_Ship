#include "database.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static sqlite3* db = NULL;

int db_init() {
    int rc = sqlite3_open(DB_NAME, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    const char* sql_users = 
        "CREATE TABLE IF NOT EXISTS users ("
        "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL,"
        "total_games INTEGER DEFAULT 0,"
        "wins INTEGER DEFAULT 0,"
        "losses INTEGER DEFAULT 0,"
        "total_score INTEGER DEFAULT 0,"
        "elo_rating INTEGER DEFAULT 1000,"
        "status TEXT DEFAULT 'offline',"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    const char* sql_matches = 
        "CREATE TABLE IF NOT EXISTS match_history ("
        "match_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "player1_id INTEGER NOT NULL,"
        "player2_id INTEGER NOT NULL,"
        "winner_id INTEGER,"
        "player1_score INTEGER DEFAULT 0,"
        "player2_score INTEGER DEFAULT 0,"
        "player1_elo_before INTEGER DEFAULT 1000,"
        "player2_elo_before INTEGER DEFAULT 1000,"
        "player1_elo_gain INTEGER DEFAULT 0,"
        "player2_elo_gain INTEGER DEFAULT 0,"
        "player1_elo_after INTEGER DEFAULT 1000,"
        "player2_elo_after INTEGER DEFAULT 1000,"
        "player1_hit_diff INTEGER DEFAULT 0,"
        "player2_hit_diff INTEGER DEFAULT 0,"
        "player1_accuracy REAL DEFAULT 0.0,"
        "player2_accuracy REAL DEFAULT 0.0,"
        "player1_total_shots INTEGER DEFAULT 0,"
        "player2_total_shots INTEGER DEFAULT 0,"
        "game_duration_seconds INTEGER DEFAULT 0,"
        "match_data TEXT,"
        "played_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(player1_id) REFERENCES users(user_id),"
        "FOREIGN KEY(player2_id) REFERENCES users(user_id),"
        "FOREIGN KEY(winner_id) REFERENCES users(user_id)"
        ");";

    char* err_msg = NULL;
    rc = sqlite3_exec(db, sql_users, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (users): %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    rc = sqlite3_exec(db, sql_matches, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (matches): %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    printf("Database initialized successfully\n");
    return 0;
}

double calculate_k_factor(int total_games, int ra, int rb) {
    double k_base;
    
    if (total_games < 30) {
        k_base = 40.0;
    } else if (total_games >= 30 && total_games <= 200) {
        k_base = 20.0;
    } else {
        k_base = 10.0;
    }
    
    double rank_diff = fabs((double)(ra - rb)) / 1000.0;
    double k = k_base * fmax(0.25, 1.0 - rank_diff);
    
    return k;
}

double calculate_expected_score(int ra, int rb) {
    return 1.0 / (1.0 + pow(10.0, (double)(rb - ra) / 400.0));
}

// Calculate performance bonus based on time and efficiency
// Only for winners, max +10 ELO (20% of typical K=50)
double calculate_performance_bonus(int total_shots, int game_duration_seconds, int is_winner) {
    if (!is_winner) return 0.0;
    
    // Time bonus: < 5 min = +5 ELO, > 15 min = 0 ELO
    double time_minutes = game_duration_seconds / 60.0;
    double time_bonus = 5.0 * fmax(0.0, 1.0 - time_minutes / 15.0);
    time_bonus = fmin(time_bonus, 5.0);
    
    // Efficiency bonus: < 30 shots = +5 ELO, > 60 shots = 0 ELO
    double efficiency_bonus = 5.0 * fmax(0.0, 1.0 - (double)total_shots / 60.0);
    efficiency_bonus = fmin(efficiency_bonus, 5.0);
    
    double total_bonus = time_bonus + efficiency_bonus;
    
    printf("  Performance Bonus: Time=%.1f min (bonus=+%.1f), Shots=%d (bonus=+%.1f), Total=+%.1f\n",
           time_minutes, time_bonus, total_shots, efficiency_bonus, total_bonus);
    
    return total_bonus;
}

int calculate_elo_change(int player_elo, int opponent_elo, int total_games,
                         float hit_diff, float accuracy, int total_shots,
                         int game_duration_seconds, int is_winner) {
    // Standard ELO calculation
    double k = calculate_k_factor(total_games, player_elo, opponent_elo);
    double ea = calculate_expected_score(player_elo, opponent_elo);
    
    // Result: 1 for win, 0 for loss
    double actual_result = is_winner ? 1.0 : 0.0;
    
    // Base ELO change
    double base_change = k * (actual_result - ea);
    
    // Performance bonus (only for winner)
    double performance_bonus = calculate_performance_bonus(total_shots, game_duration_seconds, is_winner);
    
    // Final ELO change
    double total_change = base_change + performance_bonus;
    int elo_change = (int)round(total_change);
    
    printf("ELO Calculation: K=%.2f, EA=%.4f, Result=%.0f, Base=%.1f, Bonus=+%.1f, Final=%+d\n", 
           k, ea, actual_result, base_change, performance_bonus, elo_change);
    
    return elo_change;
}

void db_update_elo_after_match(MatchHistory* match) {
    UserProfile p1, p2;
    db_get_user_profile(match->player1_id, &p1);
    db_get_user_profile(match->player2_id, &p2);
    
    match->player1_elo_before = p1.elo_rating;
    match->player2_elo_before = p2.elo_rating;
    
    int p1_is_winner = (match->winner_id == match->player1_id) ? 1 : 0;
    match->player1_elo_gain = calculate_elo_change(
        p1.elo_rating, p2.elo_rating, p1.total_games,
        match->player1_hit_diff, match->player1_accuracy,
        match->player1_total_shots, match->game_duration_seconds, p1_is_winner
    );
    
    int p2_is_winner = (match->winner_id == match->player2_id) ? 1 : 0;
    match->player2_elo_gain = calculate_elo_change(
        p2.elo_rating, p1.elo_rating, p2.total_games,
        match->player2_hit_diff, match->player2_accuracy,
        match->player2_total_shots, match->game_duration_seconds, p2_is_winner
    );
    
    match->player1_elo_after = p1.elo_rating + match->player1_elo_gain;
    match->player2_elo_after = p2.elo_rating + match->player2_elo_gain;
    
    const char* sql = "UPDATE users SET elo_rating = ? WHERE user_id = ?;";
    sqlite3_stmt* stmt;
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, match->player1_elo_after);
    sqlite3_bind_int(stmt, 2, match->player1_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, match->player2_elo_after);
    sqlite3_bind_int(stmt, 2, match->player2_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    printf("ELO Updated: P1: %d->%d (%+d), P2: %d->%d (%+d)\n", 
           match->player1_elo_before, match->player1_elo_after, match->player1_elo_gain,
           match->player2_elo_before, match->player2_elo_after, match->player2_elo_gain);
}

int db_register_user(const char* username, const char* password) {
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Registration failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    return sqlite3_last_insert_rowid(db);
}

int db_login_user(const char* username, const char* password, UserProfile* profile) {
    const char* sql = "SELECT user_id, username, total_games, wins, losses, total_score, elo_rating, status "
                      "FROM users WHERE username = ? AND password = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        profile->user_id = sqlite3_column_int(stmt, 0);
        strcpy(profile->username, (const char*)sqlite3_column_text(stmt, 1));
        profile->total_games = sqlite3_column_int(stmt, 2);
        profile->wins = sqlite3_column_int(stmt, 3);
        profile->losses = sqlite3_column_int(stmt, 4);
        profile->total_score = sqlite3_column_int(stmt, 5);
        profile->elo_rating = sqlite3_column_int(stmt, 6);
        strcpy(profile->status, (const char*)sqlite3_column_text(stmt, 7));

        sqlite3_finalize(stmt);
        db_update_user_status(profile->user_id, "online");
        return profile->user_id;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_logout_user(int user_id) {
    return db_update_user_status(user_id, "offline");
}

int db_update_user_status(int user_id, const char* status) {
    const char* sql = "UPDATE users SET status = ? WHERE user_id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_set_elo(int user_id, int new_elo) {
    const char* sql = "UPDATE users SET elo_rating = ? WHERE user_id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, new_elo);
    sqlite3_bind_int(stmt, 2, user_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_user_profile(int user_id, UserProfile* profile) {
    const char* sql = "SELECT user_id, username, total_games, wins, losses, total_score, elo_rating, status "
                      "FROM users WHERE user_id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, user_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        profile->user_id = sqlite3_column_int(stmt, 0);
        strcpy(profile->username, (const char*)sqlite3_column_text(stmt, 1));
        profile->total_games = sqlite3_column_int(stmt, 2);
        profile->wins = sqlite3_column_int(stmt, 3);
        profile->losses = sqlite3_column_int(stmt, 4);
        profile->total_score = sqlite3_column_int(stmt, 5);
        profile->elo_rating = sqlite3_column_int(stmt, 6);
        strcpy(profile->status, (const char*)sqlite3_column_text(stmt, 7));

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_save_match(MatchHistory* match) {
    printf("DEBUG db_save_match: Starting save for P1:%d vs P2:%d\n", 
           match->player1_id, match->player2_id);
    
    db_update_elo_after_match(match);
    
    printf("DEBUG db_save_match: After ELO update - P1 gain:%d, P2 gain:%d\n",
           match->player1_elo_gain, match->player2_elo_gain);
    
    const char* sql = "INSERT INTO match_history (player1_id, player2_id, winner_id, "
                      "player1_score, player2_score, player1_elo_before, player2_elo_before, "
                      "player1_elo_gain, player2_elo_gain, player1_elo_after, player2_elo_after, "
                      "player1_hit_diff, player2_hit_diff, player1_accuracy, player2_accuracy, "
                      "player1_total_shots, player2_total_shots, game_duration_seconds, "
                      "match_data) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "ERROR db_save_match: prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, match->player1_id);
    sqlite3_bind_int(stmt, 2, match->player2_id);
    sqlite3_bind_int(stmt, 3, match->winner_id);
    sqlite3_bind_int(stmt, 4, match->player1_score);
    sqlite3_bind_int(stmt, 5, match->player2_score);
    sqlite3_bind_int(stmt, 6, match->player1_elo_before);
    sqlite3_bind_int(stmt, 7, match->player2_elo_before);
    sqlite3_bind_int(stmt, 8, match->player1_elo_gain);
    sqlite3_bind_int(stmt, 9, match->player2_elo_gain);
    sqlite3_bind_int(stmt, 10, match->player1_elo_after);
    sqlite3_bind_int(stmt, 11, match->player2_elo_after);
    sqlite3_bind_int(stmt, 12, match->player1_hit_diff);
    sqlite3_bind_int(stmt, 13, match->player2_hit_diff);
    sqlite3_bind_double(stmt, 14, match->player1_accuracy);
    sqlite3_bind_double(stmt, 15, match->player2_accuracy);
    sqlite3_bind_int(stmt, 16, match->player1_total_shots);
    sqlite3_bind_int(stmt, 17, match->player2_total_shots);
    sqlite3_bind_int(stmt, 18, match->game_duration_seconds);
    sqlite3_bind_text(stmt, 19, match->match_data, -1, SQLITE_STATIC);

    printf("DEBUG db_save_match: Executing INSERT...\n");
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "ERROR db_save_match: step failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
        return -1;
    }

    int match_id = sqlite3_last_insert_rowid(db);
    printf("DEBUG db_save_match: Success! Match ID = %d\n", match_id);
    return match_id;
}

int db_get_match_history(int user_id, MatchHistory** matches, int* count) {
    const char* sql = "SELECT match_id, player1_id, player2_id, winner_id, "
                      "player1_score, player2_score, player1_elo_before, player2_elo_before, "
                      "player1_elo_gain, player2_elo_gain, player1_elo_after, player2_elo_after, "
                      "player1_hit_diff, player2_hit_diff, player1_accuracy, player2_accuracy, "
                      "player1_total_shots, player2_total_shots, game_duration_seconds, "
                      "match_data, strftime('%s', played_at) as played_timestamp "
                      "FROM match_history WHERE player1_id = ? OR player2_id = ? "
                      "ORDER BY played_at DESC LIMIT 20;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);

    int capacity = 20;
    *matches = malloc(sizeof(MatchHistory) * capacity);
    *count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        MatchHistory* m = &(*matches)[*count];
        m->match_id = sqlite3_column_int(stmt, 0);
        m->player1_id = sqlite3_column_int(stmt, 1);
        m->player2_id = sqlite3_column_int(stmt, 2);
        m->winner_id = sqlite3_column_int(stmt, 3);
        m->player1_score = sqlite3_column_int(stmt, 4);
        m->player2_score = sqlite3_column_int(stmt, 5);
        m->player1_elo_before = sqlite3_column_int(stmt, 6);
        m->player2_elo_before = sqlite3_column_int(stmt, 7);
        m->player1_elo_gain = sqlite3_column_int(stmt, 8);
        m->player2_elo_gain = sqlite3_column_int(stmt, 9);
        m->player1_elo_after = sqlite3_column_int(stmt, 10);
        m->player2_elo_after = sqlite3_column_int(stmt, 11);
        m->player1_hit_diff = sqlite3_column_int(stmt, 12);
        m->player2_hit_diff = sqlite3_column_int(stmt, 13);
        m->player1_accuracy = sqlite3_column_double(stmt, 14);
        m->player2_accuracy = sqlite3_column_double(stmt, 15);
        m->player1_total_shots = sqlite3_column_int(stmt, 16);
        m->player2_total_shots = sqlite3_column_int(stmt, 17);
        m->game_duration_seconds = sqlite3_column_int(stmt, 18);
        strcpy(m->match_data, (const char*)sqlite3_column_text(stmt, 19));
        m->played_at = sqlite3_column_int64(stmt, 20);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_get_match_for_rewatch(int match_id, MatchHistory* match) {
    const char* sql = "SELECT match_id, player1_id, player2_id, winner_id, "
                      "player1_score, player2_score, player1_elo_before, player2_elo_before, "
                      "player1_elo_gain, player2_elo_gain, player1_elo_after, player2_elo_after, "
                      "player1_hit_diff, player2_hit_diff, player1_accuracy, player2_accuracy, "
                      "match_data, played_at "
                      "FROM match_history WHERE match_id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, match_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        match->match_id = sqlite3_column_int(stmt, 0);
        match->player1_id = sqlite3_column_int(stmt, 1);
        match->player2_id = sqlite3_column_int(stmt, 2);
        match->winner_id = sqlite3_column_int(stmt, 3);
        match->player1_score = sqlite3_column_int(stmt, 4);
        match->player2_score = sqlite3_column_int(stmt, 5);
        match->player1_elo_before = sqlite3_column_int(stmt, 6);
        match->player2_elo_before = sqlite3_column_int(stmt, 7);
        match->player1_elo_gain = sqlite3_column_int(stmt, 8);
        match->player2_elo_gain = sqlite3_column_int(stmt, 9);
        match->player1_elo_after = sqlite3_column_int(stmt, 10);
        match->player2_elo_after = sqlite3_column_int(stmt, 11);
        match->player1_hit_diff = sqlite3_column_int(stmt, 12);
        match->player2_hit_diff = sqlite3_column_int(stmt, 13);
        match->player1_accuracy = sqlite3_column_double(stmt, 14);
        match->player2_accuracy = sqlite3_column_double(stmt, 15);
        strcpy(match->match_data, (const char*)sqlite3_column_text(stmt, 16));
        match->played_at = sqlite3_column_int(stmt, 17);

        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_update_score(int user_id, int score_change, int is_win) {
    const char* sql = "UPDATE users SET "
                      "total_games = total_games + 1, "
                      "wins = wins + ?, "
                      "losses = losses + ?, "
                      "total_score = total_score + ? "
                      "WHERE user_id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, is_win ? 1 : 0);
    sqlite3_bind_int(stmt, 2, is_win ? 0 : 1);
    sqlite3_bind_int(stmt, 3, score_change);
    sqlite3_bind_int(stmt, 4, user_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ==================== NEW: LEADERBOARD ====================
int db_get_leaderboard(char* output, int max_size) {
    const char* sql = "SELECT username, elo_rating, total_games, wins "
                      "FROM users ORDER BY elo_rating DESC LIMIT 10";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        strcpy(output, "ERROR#");
        return -1;
    }

    output[0] = '\0';
    int rank = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* username = (const char*)sqlite3_column_text(stmt, 0);
        int elo = sqlite3_column_int(stmt, 1);
        int games = sqlite3_column_int(stmt, 2);
        int wins = sqlite3_column_int(stmt, 3);

        char entry[256];
        snprintf(entry, sizeof(entry), "%d:%s:%d:%d:%d:", rank, username, elo, games, wins);
        strncat(output, entry, max_size - strlen(output) - 1);

        rank++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

// ==================== NEW: MATCH HISTORY ====================
int db_get_user_match_history(int user_id, char* output, int max_size) {
    const char* sql =
        "SELECT m.match_id, "
        "CASE WHEN m.player1_id = ? THEN u2.username ELSE u1.username END as opponent, "
        "CASE WHEN m.player1_id = ? THEN m.player1_elo_before ELSE m.player2_elo_before END as my_elo_before, "
        "CASE WHEN m.player1_id = ? THEN m.player1_elo_after ELSE m.player2_elo_after END as my_elo_after, "
        "CASE WHEN m.winner_id = ? THEN 1 ELSE 0 END as result, "
        "datetime(m.played_at, 'localtime') as date "
        "FROM match_history m "
        "JOIN users u1 ON m.player1_id = u1.user_id "
        "JOIN users u2 ON m.player2_id = u2.user_id "
        "WHERE m.player1_id = ? OR m.player2_id = ? "
        "ORDER BY m.played_at DESC LIMIT 10";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        strcpy(output, "ERROR#");
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, user_id);
    sqlite3_bind_int(stmt, 4, user_id);
    sqlite3_bind_int(stmt, 5, user_id);
    sqlite3_bind_int(stmt, 6, user_id);

    output[0] = '\0';

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int match_id = sqlite3_column_int(stmt, 0);
        const char* opponent = (const char*)sqlite3_column_text(stmt, 1);
        int elo_before = sqlite3_column_int(stmt, 2);
        int elo_after = sqlite3_column_int(stmt, 3);
        int result = sqlite3_column_int(stmt, 4);
        const char* date = (const char*)sqlite3_column_text(stmt, 5);

        int elo_change = elo_after - elo_before;

        char entry[512];
        snprintf(entry, sizeof(entry), "%d:%s:%d:%d:%d:%d:%s:",
                match_id, opponent, elo_before, elo_after, elo_change, result, date);
        strncat(output, entry, max_size - strlen(output) - 1);
    }

    sqlite3_finalize(stmt);
    return 0;
}

void db_close() {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}