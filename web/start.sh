#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Mini Pascal Compiler — Web Interface"
echo "======================================"

# Start FastAPI backend
echo "Starting backend on http://localhost:8000 ..."
cd "$SCRIPT_DIR"
uvicorn api:app --port 8000 --host 0.0.0.0 &
BACKEND_PID=$!

# Give backend a moment to start
sleep 1

# Start Vite frontend
echo "Starting frontend on http://localhost:5173 ..."
cd "$SCRIPT_DIR/frontend"
npm run dev &
FRONTEND_PID=$!

echo ""
echo "Open: http://localhost:5173"
echo "(Ctrl+C to stop both servers)"

# Wait and clean up on exit
trap "kill $BACKEND_PID $FRONTEND_PID 2>/dev/null; echo 'Stopped.'" EXIT INT TERM
wait
