#!/bin/bash

# Daemon execution script for v5.cpp
# Runs a single instance as a daemon (background process)
# Usage: ./scripts/run_daemon.sh [all v5.cpp arguments]

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Detect OS
OS="$(uname -s)"
case "${OS}" in
    Linux*)     OS_TYPE="Linux";;
    Darwin*)    OS_TYPE="Mac";;
    *)          echo -e "${RED}Error: Unsupported OS: ${OS}${NC}" >&2; exit 1;;
esac

echo -e "${GREEN}=== Daemon Execution Script ===" 
echo -e "OS: ${OS_TYPE}${NC}"
echo ""

# Check if executable exists
if [ ! -f "bin/v5" ]; then
    echo -e "${YELLOW}Executable not found. Building...${NC}"
    ./scripts/build.sh
    echo ""
fi

# Create log directory if it doesn't exist
LOG_DIR="log"
mkdir -p "$LOG_DIR"

# Get all arguments passed to this script
ARGS=("$@")

# Check if map is provided
HAS_MAP=false
for arg in "${ARGS[@]}"; do
    if [ "$arg" == "-m" ] || [[ "$arg" == "-m"* ]]; then
        HAS_MAP=true
        break
    fi
done

if [ "$HAS_MAP" == false ]; then
    echo -e "${RED}Error: Map (-m) is required${NC}" >&2
    echo "Usage: $0 -m <map> [other v5.cpp arguments]"
    exit 1
fi

# Extract map name for log files
MAP_NAME=""
for i in "${!ARGS[@]}"; do
    if [ "${ARGS[$i]}" == "-m" ] && [ $((i+1)) -lt ${#ARGS[@]} ]; then
        MAP_NAME="${ARGS[$((i+1))]}"
        break
    fi
done

if [ -z "$MAP_NAME" ]; then
    MAP_NAME="unknown"
fi

# Generate timestamp for log files
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
DAEMON_LOG="${LOG_DIR}/daemon_${MAP_NAME}_${TIMESTAMP}.log"
DAEMON_ERR="${LOG_DIR}/daemon_${MAP_NAME}_${TIMESTAMP}.err"
PID_FILE="${LOG_DIR}/daemon_${MAP_NAME}_${TIMESTAMP}.pid"

echo -e "${BLUE}Configuration:${NC}"
echo -e "  Map: ${MAP_NAME}"
echo -e "  Log file: ${DAEMON_LOG}"
echo -e "  Error file: ${DAEMON_ERR}"
echo -e "  PID file: ${PID_FILE}"
echo -e "  Arguments: ${ARGS[*]}"
echo ""

# Function to cleanup on exit
cleanup() {
    if [ -f "$PID_FILE" ]; then
        rm -f "$PID_FILE"
    fi
}

# Set trap for cleanup
trap cleanup EXIT

# Start daemon process
echo -e "${YELLOW}Starting daemon process...${NC}"

# Use nohup to detach from terminal and redirect output
nohup ./bin/v5 "${ARGS[@]}" > "$DAEMON_LOG" 2> "$DAEMON_ERR" &
DAEMON_PID=$!

# Save PID to file
echo $DAEMON_PID > "$PID_FILE"

# Wait a moment to check if process started successfully
sleep 1

# Check if process is still running
if ps -p $DAEMON_PID > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Daemon started successfully${NC}"
    echo -e "  PID: ${DAEMON_PID}"
    echo -e "  Log: ${DAEMON_LOG}"
    echo -e "  Error: ${DAEMON_ERR}"
    echo -e "  PID file: ${PID_FILE}"
    echo ""
    echo -e "${BLUE}Commands to monitor:${NC}"
    echo -e "  View log: tail -f ${DAEMON_LOG}"
    echo -e "  View errors: tail -f ${DAEMON_ERR}"
    echo -e "  Check status: ps -p ${DAEMON_PID}"
    echo -e "  Stop daemon: kill ${DAEMON_PID}"
    echo ""
    echo -e "${GREEN}Daemon is running in background.${NC}"
    exit 0
else
    # Process failed to start
    echo -e "${RED}✗ Daemon failed to start${NC}"
    echo -e "  Check error log: ${DAEMON_ERR}"
    if [ -f "$DAEMON_ERR" ]; then
        echo ""
        echo -e "${RED}Error output:${NC}"
        cat "$DAEMON_ERR"
    fi
    rm -f "$PID_FILE"
    exit 1
fi

