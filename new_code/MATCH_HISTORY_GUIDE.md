# Match History System Guide

## Overview
The match history system allows players to view their past games with detailed statistics and replay information.

## Features

### 1. Match History Screen (List View)
- Accessible from lobby via purple "MATCH HISTORY" button
- Shows up to 20 recent matches
- Displays for each match:
  - **Date/Time**: When the match was played
  - **Opponent**: Username and ID
  - **Result**: WIN (green) or LOSS (red)
  - **Shots**: Hit/Miss counts with accuracy percentage
  - **ELO Change**: Gain/loss (green for positive, red for negative)
  - **Duration**: Match length in minutes:seconds
  - **View Details**: Button to see shot-by-shot replay

### 2. Match Detail Screen (Replay View)
- Shows detailed shot-by-shot breakdown
- Two-column layout:
  - **YOUR SHOTS**: All shots you fired
  - **OPPONENT SHOTS**: All shots opponent fired
- For each shot displays:
  - Shot number (#1, #2, etc.)
  - Coordinate (e.g., A5, B10)
  - Result: HIT (green) or MISS (gray)
  - Ship length if hit (e.g., "length 4")
  - "Sunk ship" indicator in yellow when a ship was destroyed

## Usage

### Viewing Match History
1. From lobby, click the purple "MATCH HISTORY" button (top center)
2. Browse your recent matches
3. Click "VIEW DETAILS" on any match to see shot-by-shot replay
4. Click "< BACK" to return to lobby

### Viewing Match Details
1. From match history, click "VIEW DETAILS" button on desired match
2. Scroll through shot history (your shots on left, opponent on right)
3. Click "< BACK TO HISTORY" to return to match list

## Server Commands

### GET_MATCH_HISTORY
- **Format**: `GET_MATCH_HISTORY#`
- **Response**: `MATCH_HISTORY:count|match1_data|match2_data|...#`
- **Match Data Format**: `match_id,timestamp,opponent_id,opponent_name,is_win,hits,misses,elo_change,duration`

### GET_MATCH_DETAIL
- **Format**: `GET_MATCH_DETAIL:match_id#`
- **Response**: `MATCH_DETAIL:match_id:match_data#`
- **Match Data Format**: Shot entries separated by `;`
  - Each shot: `x,y,hit,ship_len,sunk`
  - Alternating: my_shot;opponent_shot;my_shot;opponent_shot;...

## Database Schema

### match_history Table
```sql
- match_id (PRIMARY KEY)
- player1_id, player2_id
- winner_id
- player1_score, player2_score
- player1_elo_before, player2_elo_before
- player1_elo_gain, player2_elo_gain
- player1_elo_after, player2_elo_after
- player1_hit_diff, player2_hit_diff
- player1_accuracy, player2_accuracy
- player1_total_shots, player2_total_shots
- game_duration_seconds
- match_data (TEXT) - Stores shot-by-shot replay data
- played_at (DATETIME)
```

## Current Limitations

1. **Shot Data Storage**: Match data (detailed shot history) is currently stored as empty string. To enable full shot replay:
   - Update `handle_fire()` in server_lobby.c to track each shot
   - Store shots in match_data field as: `x,y,hit,ship_len,sunk;...`
   - Shots should alternate: player1, player2, player1, player2, etc.

2. **Match Details**: When match_data is empty, detail screen shows "No detailed shot data available"

## Future Enhancements

### To implement full shot tracking:
1. Add shot logging to `handle_fire()`:
   ```c
   // In Client structure, add:
   char shot_log[4096];
   
   // In handle_fire(), append each shot:
   char shot_entry[50];
   snprintf(shot_entry, sizeof(shot_entry), "%d,%d,%d,%d,%d;", 
            col, row, is_hit, ship_length, ship_sunk);
   strcat(client->shot_log, shot_entry);
   strcat(opponent->shot_log, shot_entry);
   ```

2. In game over handler, build complete match_data:
   ```c
   // Interleave shots from both players
   snprintf(match.match_data, sizeof(match.match_data), 
            "%s:%s", client->shot_log, opponent->shot_log);
   ```

3. Update protocol parsing to correctly separate player shots

## Files Modified

### Client:
- `src/core/game_data.h` - Added STATE_MATCH_HISTORY, STATE_MATCH_DETAIL, MatchHistoryEntry, MatchDetail structures
- `src/ui/screens/match_history_screen.c/.h` - Match list view
- `src/ui/screens/match_detail_screen.c/.h` - Shot replay view
- `src/ui/screens/lobby_screen.c` - Added Match History button
- `src/client/client_gui_complete.c` - Integrated new screens
- `src/network/protocol.c` - Parse MATCH_HISTORY and MATCH_DETAIL responses
- `Makefile` - Added new screen sources

### Server:
- `src/server_lobby.c` - Added handle_get_match_history() and handle_get_match_detail()
- `src/database.c` - Already has db_get_match_history() and db_get_match_for_rewatch()
- `src/database.h` - MatchHistory structure with match_data field

## Testing

1. Play a match to completion
2. Return to lobby
3. Click "MATCH HISTORY" button
4. Verify match appears in list with correct stats
5. Click "VIEW DETAILS" to check detail screen
6. Verify back buttons work correctly

## Notes

- Match history is per-user (only shows matches you participated in)
- History is limited to 20 most recent matches
- ELO changes are color-coded (green for gains, red for losses)
- Accuracy is calculated as: (hits / total_shots) * 100%
- Duration is time from game start to game end
