# Sequential Execution Script

The `run_sequential.sh` script runs 10 instances of the v5.cpp executable sequentially (one after another) as a daemon, perfect for long-running jobs where you want to process multiple runs automatically without manual intervention.

## Features

- ✅ Runs 10 executions sequentially (one completes, next starts automatically)
- ✅ Works on both Linux and Mac
- ✅ Daemon mode for background execution
- ✅ Automatic log management (creates `log/` directory)
- ✅ Error tracking and reporting
- ✅ Automatic seed differentiation (each run gets different seed)
- ✅ 1 week timeout protection (604800 seconds) per run
- ✅ Progress tracking and summary
- ✅ No manual intervention needed - fully automated

## Usage

### Basic Usage

```bash
# Run 10 sequential instances with same parameters
./scripts/run_sequential.sh -m B --local-search --iterations 100

# With all parameters
./scripts/run_sequential.sh -m C --local-search --iterations 200 \
    --fix-task-eff --task-eff 1.5 --dist-penalty 1.2
```

### Daemon Mode (Background Execution)

```bash
# Run as daemon (detached from terminal)
./scripts/run_sequential.sh --daemon -m B --local-search --iterations 100

# The script will:
# - Start in background
# - Return immediately with PID
# - Automatically run all 10 instances sequentially
# - Log all output to log/sequential_daemon_TIMESTAMP.log
```

### Check Daemon Status

```bash
# View daemon log
tail -f log/sequential_daemon_*.log

# Check if process is running
ps aux | grep run_sequential
```

## How It Works

1. **Sequential Execution**: Runs execute one after another (not in parallel)
2. **Automatic Continuation**: When one run finishes, the next starts automatically
3. **Seed Management**: 
   - If no `--seed` is provided: Each run uses a random seed (different each time)
   - If `--seed N` is provided: Each run gets seed N+1, N+2, N+3, ..., N+10
4. **Logging**: 
   - Each run logs to: `log/seq{N}_MAP_TIMESTAMP.log`
   - Errors go to: `log/seq{N}_MAP_TIMESTAMP.err`
   - Daemon log: `log/sequential_daemon_TIMESTAMP.log`
5. **Timeout**: Each run has a 1 week (604800 seconds) timeout
6. **Non-blocking**: Runs as daemon, so you can disconnect and it continues

## Output Structure

```
log/
├── sequential_daemon_20241230_143000.log  (if daemon mode)
├── seq1_B_20241230_143022.log
├── seq1_B_20241230_143022.err
├── seq2_B_20241230_150145.log
├── seq2_B_20241230_150145.err
├── ...
└── seq10_B_20241230_180230.log
```

## Example Output

```
=== Sequential Execution Script ===
OS: Mac

Configuration:
  Sequential runs: 10
  Map: B
  Log directory: log/
  Arguments: -m B --local-search --iterations 100
  Timeout: 1 week (604800 seconds)

Starting 10 sequential runs...

=== Run 1/10 ===
[Run 1/10] Starting...
[Run 1/10] Completed successfully (2h 15m 30s)
  Log: log/seq1_B_20241230_143022.log
✓ Run 1 finished successfully
Waiting 2 seconds before starting next run...

=== Run 2/10 ===
[Run 2/10] Starting...
[Run 2/10] Completed successfully (2h 18m 45s)
  Log: log/seq2_B_20241230_150145.log
✓ Run 2 finished successfully
Waiting 2 seconds before starting next run...

...

=== Execution Summary ===
Total runs: 10
Successful: 10
Failed: 0
Total duration: 22h 35m 12s
Log directory: log/

All runs completed successfully!
```

## Comparison with Other Scripts

| Feature | `run_sequential.sh` | `run_parallel.sh` | `run_daemon.sh` |
|---------|---------------------|------------------|-----------------|
| Instances | 10 | 5 (or 10) | 1 |
| Execution | Sequential | Parallel | Single |
| Use Case | Long jobs, one at a time | Fast parallel testing | Single long job |
| Timeout | 1 week | 24 hours | None |
| Best For | Long-running sequential runs | Quick parallel testing | Single execution |

## Server Deployment Tips

### 1. Start Sequential Daemon on Server

```bash
# SSH to server and start daemon
ssh user@server
cd /path/to/local-search-assembly
./scripts/run_sequential.sh --daemon -m B --local-search --iterations 1000

# Disconnect - daemon continues running all 10 sequentially
exit
```

### 2. Monitor Progress Remotely

```bash
# SSH back and check logs
ssh user@server

# View daemon log (shows overall progress)
tail -f log/sequential_daemon_*.log

# View current run log
tail -f log/seq*_B_*.log | tail -20
```

### 3. Check Current Run Number

```bash
# Count completed runs
ls log/seq*_B_*.log | wc -l

# Check latest run
ls -lt log/seq*_B_*.log | head -1
```

### 4. Stop Sequential Daemon

```bash
# Kill the sequential script (stops after current run)
pkill -f "run_sequential.sh"

# Or kill current run and stop
pkill -f "bin/v5"
```

### 5. Resume After Interruption

If the daemon is stopped, you can check which runs completed and manually run the remaining ones:

```bash
# Check completed runs
ls log/seq*_B_*.log | wc -l

# If 5 completed, manually run remaining 5 with adjusted seed
# (if you used --seed, add 5 to the original seed)
```

## Error Handling

- **Failed Runs**: Script continues to next run even if one fails
- **Error Logging**: Failed runs are logged to `.err` files
- **Summary**: Final summary shows success/failure counts
- **Exit Code**: Script exits with code 1 if any run fails

## Timing Information

- **Per Run Duration**: Shows hours, minutes, and seconds for each run
- **Total Duration**: Shows total time for all 10 runs
- **Timeout**: Each run has 1 week (604800 seconds) maximum

## Notes

- The script automatically builds the executable if `bin/v5` doesn't exist
- Each run is independent and won't interfere with others
- Output files are saved to `output/` directory as usual
- JSON parameter files are also created for each run
- 2 second delay between runs to avoid file system contention
- Process survives SSH disconnection (uses `nohup` in daemon mode)

## Troubleshooting

### Daemon Not Starting

```bash
# Check error log
cat log/sequential_daemon_*.log

# Check if executable exists
ls -lh bin/v5

# Try running directly (not as daemon)
./scripts/run_sequential.sh -m B --local-search
```

### Run Taking Too Long

```bash
# Check current run log
tail -f log/seq*_B_*.log

# Check if process is still running
ps aux | grep "bin/v5"
```

### All Runs Failed

```bash
# Check error logs
cat log/seq*_B_*.err

# Check system resources
top
df -h
```

## Advanced Usage

### Run Multiple Sequential Daemons with Different Configs

```bash
# Terminal 1
./scripts/run_sequential.sh --daemon -m B --local-search --fix-task-eff

# Terminal 2
./scripts/run_sequential.sh --daemon -m C --local-search --fix-dist-penalty

# Terminal 3
./scripts/run_sequential.sh --daemon -m D --local-search --fix-ownership-factor
```

### Schedule with Cron

```bash
# Add to crontab (runs daily at 2 AM)
0 2 * * * cd /path/to/local-search-assembly && ./scripts/run_sequential.sh --daemon -m B --local-search --iterations 1000 >> /tmp/cron.log 2>&1
```

### Monitor Progress Script

Create a simple monitoring script:

```bash
#!/bin/bash
# monitor_sequential.sh
while true; do
    clear
    echo "=== Sequential Run Status ==="
    COMPLETED=$(ls log/seq*_B_*.log 2>/dev/null | wc -l)
    echo "Completed: $COMPLETED / 10"
    if [ $COMPLETED -gt 0 ]; then
        echo ""
        echo "Latest run:"
        ls -lt log/seq*_B_*.log | head -1 | awk '{print $NF}'
        echo ""
        tail -5 $(ls -t log/seq*_B_*.log | head -1)
    fi
    sleep 30
done
```

