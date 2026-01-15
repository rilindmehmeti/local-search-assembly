# Daemon Execution Script

The `run_daemon.sh` script runs a single instance of the v5.cpp executable as a daemon (background process), perfect for long-running jobs on servers.

## Features

- ✅ Runs single execution in background (daemon mode)
- ✅ Works on both Linux and Mac
- ✅ Automatic log management (creates `log/` directory)
- ✅ PID tracking for process management
- ✅ Error logging and monitoring
- ✅ Detached from terminal (survives SSH disconnection)
- ✅ Process status checking

## Usage

### Basic Usage

```bash
# Run as daemon with basic parameters
./scripts/run_daemon.sh -m B --local-search --iterations 100

# With all parameters
./scripts/run_daemon.sh -m C --local-search --iterations 200 \
    --fix-task-eff --task-eff 1.5 --dist-penalty 1.2

# Simple execution (no local search)
./scripts/run_daemon.sh -m B
```

## How It Works

1. **Daemon Process**: Starts the executable in background using `nohup`
2. **Logging**: 
   - Standard output → `log/daemon_MAP_TIMESTAMP.log`
   - Standard error → `log/daemon_MAP_TIMESTAMP.err`
3. **PID Tracking**: Saves process ID to `log/daemon_MAP_TIMESTAMP.pid`
4. **Detached**: Process continues even if terminal is closed

## Output Structure

```
log/
├── daemon_B_20241230_143022.log    (stdout)
├── daemon_B_20241230_143022.err    (stderr)
└── daemon_B_20241230_143022.pid    (process ID)
```

## Monitoring Commands

### View Log Output

```bash
# View log in real-time
tail -f log/daemon_B_*.log

# View last 100 lines
tail -n 100 log/daemon_B_*.log

# View errors
tail -f log/daemon_B_*.err
```

### Check Process Status

```bash
# Get PID from PID file
PID=$(cat log/daemon_B_*.pid)
ps -p $PID

# Or check if process is running
ps aux | grep "bin/v5" | grep -v grep
```

### Stop Daemon

```bash
# Get PID and kill
PID=$(cat log/daemon_B_*.pid)
kill $PID

# Or kill by name (kills all instances)
pkill -f "bin/v5"
```

## Example Output

```
=== Daemon Execution Script ===
OS: Mac

Configuration:
  Map: B
  Log file: log/daemon_B_20241230_143022.log
  Error file: log/daemon_B_20241230_143022.err
  PID file: log/daemon_B_20241230_143022.pid
  Arguments: -m B --local-search --iterations 100

Starting daemon process...
✓ Daemon started successfully
  PID: 12345
  Log: log/daemon_B_20241230_143022.log
  Error: log/daemon_B_20241230_143022.err
  PID file: log/daemon_B_20241230_143022.pid

Commands to monitor:
  View log: tail -f log/daemon_B_20241230_143022.log
  View errors: tail -f log/daemon_B_20241230_143022.err
  Check status: ps -p 12345
  Stop daemon: kill 12345

Daemon is running in background.
```

## Server Deployment Tips

### 1. Start Daemon on Server

```bash
# SSH to server and start daemon
ssh user@server
cd /path/to/local-search-assembly
./scripts/run_daemon.sh -m B --local-search --iterations 1000

# Disconnect - daemon continues running
exit
```

### 2. Monitor Progress Remotely

```bash
# SSH back and check logs
ssh user@server
tail -f log/daemon_B_*.log
```

### 3. Check Multiple Daemons

```bash
# List all running daemons
ps aux | grep "bin/v5" | grep -v grep

# List all PID files
ls -lh log/*.pid
```

### 4. Stop All Daemons

```bash
# Kill all v5 processes
pkill -f "bin/v5"

# Or kill specific one
kill $(cat log/daemon_B_*.pid)
```

### 5. Check Completion

```bash
# Check if process is still running
PID=$(cat log/daemon_B_*.pid)
if ps -p $PID > /dev/null; then
    echo "Still running"
else
    echo "Completed"
fi
```

## Error Handling

- **Startup Failure**: Script checks if process started successfully
- **Error Logging**: All errors go to `.err` file
- **PID Cleanup**: PID file is removed when process exits
- **Exit Codes**: Script exits with code 1 if daemon fails to start

## Comparison with Parallel Script

| Feature | `run_daemon.sh` | `run_parallel.sh` |
|---------|----------------|-------------------|
| Instances | 1 | 5 |
| Use Case | Single long job | Multiple parallel runs |
| Log Files | 1 log + 1 err | 5 logs + 5 errs |
| Best For | Long-running single execution | Testing multiple seeds/configs |

## Notes

- The script automatically builds the executable if `bin/v5` doesn't exist
- Output files are saved to `output/` directory as usual
- JSON parameter files are also created
- Process survives SSH disconnection (uses `nohup`)
- PID file is automatically cleaned up on exit

## Troubleshooting

### Daemon Not Starting

```bash
# Check error log
cat log/daemon_*.err

# Check if executable exists
ls -lh bin/v5

# Try running directly (not as daemon)
./bin/v5 -m B --local-search
```

### Process Died Unexpectedly

```bash
# Check error log for crash details
cat log/daemon_*.err

# Check system resources
top
df -h
```

### Can't Find PID File

```bash
# PID file is removed when process exits
# Check if process is still running
ps aux | grep "bin/v5"
```

## Advanced Usage

### Run Multiple Daemons with Different Configs

```bash
# Terminal 1
./scripts/run_daemon.sh -m B --local-search --fix-task-eff

# Terminal 2
./scripts/run_daemon.sh -m B --local-search --fix-dist-penalty

# Terminal 3
./scripts/run_daemon.sh -m C --local-search --fix-ownership-factor
```

### Schedule with Cron

```bash
# Add to crontab (runs daily at 2 AM)
0 2 * * * cd /path/to/local-search-assembly && ./scripts/run_daemon.sh -m B --local-search --iterations 1000 >> /tmp/cron.log 2>&1
```

