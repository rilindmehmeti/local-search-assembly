# Parallel Execution Script

The `run_parallel.sh` script runs 5 parallel instances of the v5.cpp executable with the same parameters, perfect for server deployments.

## Features

- ✅ Runs 5 parallel executions simultaneously
- ✅ Works on both Linux and Mac
- ✅ Daemon mode for background execution
- ✅ Automatic log management (creates `log/` directory)
- ✅ Error tracking and reporting
- ✅ Automatic seed differentiation (each run gets different seed)
- ✅ Timeout protection (24 hours max per run)
- ✅ Progress tracking and summary

## Usage

### Basic Usage

```bash
# Run 5 parallel instances with same parameters
./scripts/run_parallel.sh -m B --local-search --iterations 100

# With all parameters
./scripts/run_parallel.sh -m C --local-search --iterations 200 \
    --fix-task-eff --task-eff 1.5 --dist-penalty 1.2
```

### Daemon Mode (Background Execution)

```bash
# Run as daemon (detached from terminal)
./scripts/run_parallel.sh --daemon -m B --local-search --iterations 100

# The script will:
# - Start in background
# - Return immediately with PID
# - Log all output to log/daemon_TIMESTAMP.log
```

### Check Daemon Status

```bash
# View daemon log
tail -f log/daemon_*.log

# Check if process is running
ps aux | grep run_parallel
```

## How It Works

1. **Parallel Execution**: All 5 runs start simultaneously in the background
2. **Seed Management**: 
   - If no `--seed` is provided: Each run uses a random seed (different each time)
   - If `--seed N` is provided: Each run gets seed N+1, N+2, N+3, N+4, N+5
3. **Logging**: 
   - Each run logs to: `log/run{N}_MAP_TIMESTAMP.log`
   - Errors go to: `log/run{N}_MAP_TIMESTAMP.err`
4. **Non-blocking**: Runs execute in parallel, so they finish around the same time

## Output Structure

```
log/
├── run1_B_20241230_143022.log
├── run1_B_20241230_143022.err
├── run2_B_20241230_143022.log
├── run2_B_20241230_143022.err
├── ...
└── daemon_20241230_143000.log  (if daemon mode)
```

## Example Output

```
=== Parallel Execution Script ===
OS: Mac

Configuration:
  Parallel runs: 5
  Map: B
  Log directory: log/
  Arguments: -m B --local-search --iterations 100

Starting 5 parallel runs...

[Run 1] Starting...
[Run 2] Starting...
[Run 3] Starting...
[Run 4] Starting...
[Run 5] Starting...

Waiting for all runs to complete...

✓ Run 1 finished
✓ Run 2 finished
✓ Run 3 finished
✓ Run 4 finished
✓ Run 5 finished

=== Execution Summary ===
Total runs: 5
Successful: 5
Failed: 0
Log directory: log/

All runs completed successfully!
```

## Error Handling

- Failed runs are logged to `.err` files
- Summary shows which runs failed
- Script exits with code 1 if any run fails
- All runs continue even if one fails (non-blocking)

## Server Deployment Tips

1. **Use daemon mode** for long-running jobs:
   ```bash
   nohup ./scripts/run_parallel.sh --daemon -m B --local-search &
   ```

2. **Monitor progress**:
   ```bash
   watch -n 5 'ls -lh log/ | tail -10'
   ```

3. **Check for completion**:
   ```bash
   # Count completed runs
   ls log/run*_B_*.log | wc -l
   ```

4. **Kill all runs if needed**:
   ```bash
   pkill -f "bin/v5"
   ```

## Notes

- The script automatically builds the executable if `bin/v5` doesn't exist
- Each run is independent and won't interfere with others
- Output files are saved to `output/` directory as usual
- JSON parameter files are also created for each run

