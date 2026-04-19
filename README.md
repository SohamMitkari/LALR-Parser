# LALR(1) Parser — Compiler Design Project

An interactive LALR(1) parser for arithmetic expressions with a web-based GUI.

## Grammar
E → E + T | T  
T → T * F | F  
F → ( E ) | id

## How to Run

Requirements: GCC, Python 3

# Step 1 — Compile the parser
gcc lalr_parser.c -o lalr_parser

# Step 2 — Start the server
python3 server.py

# Step 3 — Open in browser
http://localhost:8765

## Features
- Step-by-step parse trace table
- Interactive parse tree visualization with pan and zoom
- Animated playback with play, pause, and scrub controls
- FIRST and FOLLOW sets reference panel
- Syntax error highlighting with token underline