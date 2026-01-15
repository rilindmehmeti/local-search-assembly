#!/bin/bash

# Parallel execution script for v5.cpp
# Runs 5 parallel instances with the same parameters
# Usage: ./scripts/run_parallel.sh [all v5.cpp arguments]
#        ./scripts/run_parallel.sh --daemon [all v5.cpp arguments]  # Run as daemon

# Don't exit on error - we want to continue even if one run fails
set +e

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

echo -e "${GREEN}=== Parallel Execution Script ===" 
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

# Number of parallel runs
NUM_RUNS=10

# Check for daemon mode
DAEMON_MODE=false
ARGS=()
for arg in "$@"; do
    if [ "$arg" == "--daemon" ]; then
        DAEMON_MODE=true
    else
        ARGS+=("$arg")
    fi
done

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
    echo "Usage: $0 [-m <map> | --daemon -m <map>] [other v5.cpp arguments]"
    exit 1
fi

# If daemon mode, redirect all output and run in background
if [ "$DAEMON_MODE" == true ]; then
    DAEMON_LOG="${LOG_DIR}/daemon_$(date +%Y%m%d_%H%M%S).log"
    nohup "$0" "${ARGS[@]}" > "$DAEMON_LOG" 2>&1 &
    DAEMON_PID=$!
    echo -e "${GREEN}Daemon started with PID: ${DAEMON_PID}${NC}"
    echo -e "Log file: ${DAEMON_LOG}"
    echo -e "To check status: tail -f ${DAEMON_LOG}"
    exit 0
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

echo -e "${BLUE}Configuration:${NC}"
echo -e "  Parallel runs: ${NUM_RUNS}"
echo -e "  Map: ${MAP_NAME}"
echo -e "  Log directory: ${LOG_DIR}/"
echo -e "  Arguments: ${ARGS[*]}"
echo ""

# Function to run a single instance
run_instance() {
    local run_num=$1
    local timestamp=$(date +"%Y%m%d_%H%M%S")
    local log_file="${LOG_DIR}/run${run_num}_${MAP_NAME}_${timestamp}.log"
    local err_file="${LOG_DIR}/run${run_num}_${MAP_NAME}_${timestamp}.err"
    
    # Create run-specific arguments (ensure different seeds for each run)
    local run_args=("${ARGS[@]}")
    
    # Check if seed is already provided - if not, each run will get a different random seed
    # If seed is provided, we'll add a small offset to make them different
    local has_seed=false
    for i in "${!run_args[@]}"; do
        if [ "${run_args[$i]}" == "--seed" ] && [ $((i+1)) -lt ${#run_args[@]} ]; then
            # Seed provided - add run number offset to make them different
            local original_seed=${run_args[$((i+1))]}
            run_args[$((i+1))]=$((original_seed + run_num))
            has_seed=true
            break
        fi
    done
    
    # If no seed provided, each run will use random seed (default behavior)
    # This ensures each run is different
    
    echo "[Run ${run_num}] Starting at $(date)" >> "$log_file"
    if [ "$has_seed" == true ]; then
        echo "  Using seed offset: +${run_num}" >> "$log_file"
    fi
    echo -e "${YELLOW}[Run ${run_num}] Starting...${NC}"
    
    # Run the executable with all arguments, redirecting output
    # Use timeout to prevent hanging (1 week max per run = 604800 seconds)
    local timeout_cmd=""
    if command -v timeout &> /dev/null; then
        timeout_cmd="timeout 604800"  # 1 week timeout (Linux)
    elif command -v gtimeout &> /dev/null; then
        timeout_cmd="gtimeout 604800"  # 1 week timeout (Mac with coreutils)
    fi
    
    local start_time=$(date +%s)
    
    if [ -n "$timeout_cmd" ]; then
        if $timeout_cmd ./bin/v5 "${run_args[@]}" >> "$log_file" 2>> "$err_file"; then
            local end_time=$(date +%s)
            local duration=$((end_time - start_time))
            echo "[Run ${run_num}] Completed successfully at $(date) (duration: ${duration}s)" >> "$log_file"
            echo -e "${GREEN}[Run ${run_num}] Completed successfully (${duration}s)${NC}"
            echo -e "  Log: ${log_file}"
            return 0
        else
            local exit_code=$?
            local end_time=$(date +%s)
            local duration=$((end_time - start_time))
            echo "[Run ${run_num}] Failed with exit code ${exit_code} at $(date) (duration: ${duration}s)" >> "$log_file"
            echo "Error details:" >> "$err_file"
            echo "Exit code: ${exit_code}" >> "$err_file"
            echo -e "${RED}[Run ${run_num}] Failed with exit code ${exit_code} (${duration}s)${NC}"
            echo -e "  Log: ${log_file}"
            echo -e "  Error: ${err_file}"
            return $exit_code
        fi
    else
        # No timeout command available, run without timeout
        if ./bin/v5 "${run_args[@]}" >> "$log_file" 2>> "$err_file"; then
            local end_time=$(date +%s)
            local duration=$((end_time - start_time))
            echo "[Run ${run_num}] Completed successfully at $(date) (duration: ${duration}s)" >> "$log_file"
            echo -e "${GREEN}[Run ${run_num}] Completed successfully (${duration}s)${NC}"
            echo -e "  Log: ${log_file}"
            return 0
        else
            local exit_code=$?
            local end_time=$(date +%s)
            local duration=$((end_time - start_time))
            echo "[Run ${run_num}] Failed with exit code ${exit_code} at $(date) (duration: ${duration}s)" >> "$log_file"
            echo "Error details:" >> "$err_file"
            echo "Exit code: ${exit_code}" >> "$err_file"
            echo -e "${RED}[Run ${run_num}] Failed with exit code ${exit_code} (${duration}s)${NC}"
            echo -e "  Log: ${log_file}"
            echo -e "  Error: ${err_file}"
            return $exit_code
        fi
    fi
}

# Run all instances in parallel
echo -e "${BLUE}Starting ${NUM_RUNS} parallel runs...${NC}"
echo ""

# Array to store PIDs
declare -a PIDS=()

# Start all runs in background
for i in $(seq 1 $NUM_RUNS); do
    run_instance $i &
    PIDS+=($!)
    # Small delay to avoid race conditions and file system contention
    sleep 0.2
done

# Wait for all processes and collect results
FAILED=0
SUCCESS=0

echo ""
echo -e "${BLUE}Waiting for all runs to complete...${NC}"
echo ""

for i in "${!PIDS[@]}"; do
    local pid=${PIDS[$i]}
    local run_num=$((i+1))
    
    if wait $pid; then
        SUCCESS=$((SUCCESS + 1))
        echo -e "${GREEN}✓ Run ${run_num} finished${NC}"
    else
        FAILED=$((FAILED + 1))
        echo -e "${RED}✗ Run ${run_num} failed${NC}"
    fi
done

# Summary
echo ""
echo -e "${GREEN}=== Execution Summary ===" 
echo -e "Total runs: ${NUM_RUNS}"
echo -e "Successful: ${SUCCESS}"
echo -e "Failed: ${FAILED}"
echo -e "Log directory: ${LOG_DIR}/${NC}"

# List failed runs
if [ $FAILED -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed runs:${NC}"
    for err_file in "${LOG_DIR}"/run*_${MAP_NAME}_*.err; do
        if [ -f "$err_file" ] && [ -s "$err_file" ]; then
            echo -e "  ${RED}✗${NC} $(basename "$err_file")"
        fi
    done
    echo ""
    echo -e "${RED}Warning: ${FAILED} run(s) failed. Check ${LOG_DIR}/ for error logs.${NC}"
    exit 1
else
    echo ""
    echo -e "${GREEN}All runs completed successfully!${NC}"
    exit 0
fi

