#!/usr/bin/env python3
"""
Auto-merge client_gui_login.c and client_gui_v2.c into a complete client
"""

import re
import sys

def extract_functions(filepath, function_names):
    """Extract specific functions from a C file"""
    with open(filepath, 'r') as f:
        content = f.read()
    
    functions = {}
    for func_name in function_names:
        # Find function definition
        pattern = rf'((?:void|int|char\*)\s+{func_name}\s*\([^)]*\)\s*\{{[^}}]*(?:\{{[^}}]*\}}[^}}]*)*\}})'
        matches = re.findall(pattern, content, re.MULTILINE | re.DOTALL)
        if matches:
            functions[func_name] = matches[0]
    
    return functions

def main():
    print("Battleship Client Merger")
    print("=" * 50)
    
    # Functions to extract from client_gui_v2.c
    game_functions = [
        'draw_map',
        'draw_ship_preview',
        'render_placing_ships',
        'render_playing',
        'place_ship',
        'draw_button',
        'draw_text'
    ]
    
    print("\n1. Reading client_gui_v2.c...")
    try:
        functions = extract_functions('src/client_gui_v2.c', game_functions)
        print(f"   ✓ Extracted {len(functions)} functions")
    except Exception as e:
        print(f"   ✗ Error: {e}")
        return 1
    
    print("\n2. Reading client_gui_login.c...")
    try:
        with open('src/client_gui_login.c', 'r') as f:
            login_content = f.read()
        print(f"   ✓ Read {len(login_content)} characters")
    except Exception as e:
        print(f"   ✗ Error: {e}")
        return 1
    
    print("\n3. Merging files...")
    
    # Find where to insert game functions (before main())
    insert_pos = login_content.rfind('int main()')
    
    if insert_pos == -1:
        print("   ✗ Could not find main() function")
        return 1
    
    # Build merged content
    merged = login_content[:insert_pos]
    
    # Add extracted functions
    merged += "\n// ==================== GAME FUNCTIONS (from client_gui_v2.c) ====================\n\n"
    for func_name, func_code in functions.items():
        merged += f"// {func_name}\n{func_code}\n\n"
    
    # Add rest of login content
    merged += login_content[insert_pos:]
    
    print(f"   ✓ Created merged file ({len(merged)} characters)")
    
    print("\n4. Writing output...")
    try:
        with open('src/client_gui_complete.c', 'w') as f:
            f.write(merged)
        print("   ✓ Wrote to src/client_gui_complete.c")
    except Exception as e:
        print(f"   ✗ Error: {e}")
        return 1
    
    print("\n" + "=" * 50)
    print("SUCCESS! Created src/client_gui_complete.c")
    print("\nNext steps:")
    print("1. Review the merged file")
    print("2. Update Game struct with ship fields")
    print("3. Replace render_game_screen() calls")
    print("4. Add ship placement event handlers")
    print("5. Compile: gcc src/client_gui_complete.c -o client_gui_complete `sdl2-config --cflags --libs` -lSDL2_ttf -lm -pthread")
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
