# WiFi Stability Analyzer (WSA)

WiFi Stability Analyzer (WSA) is a command-line tool designed to monitor and log the stability of your WiFi connection. It measures signal strength, connectivity, system-level WiFi logs, and network speed over a specified duration and interval, then outputs both raw logs and a combined CSV report for analysis.

## Features

- **Signal Strength Monitoring:** Logs WiFi RSSI and noise levels at regular intervals.
- **Connectivity Check:** Logs reachability and response times by pinging a known server (8.8.8.8).
- **System Log Parsing:** Captures WiFi-related kernel messages (e.g., retransmissions, PHY errors).
- **Combined CSV Report:** Aggregates all metrics (RSSI, noise, ping results, packet mismatches, retries, FCS/PLCP errors, etc.) into a single `report.csv`.
- **Network Speed Test:** Attempts a download of a 1 MB file via `wget` or falls back to `curl`, reporting success or error.
- **Progress Bar:** Displays a live status bar during data collection.

## Installation

### Prerequisites

- **macOS**: The tool is designed to work on macOS.
- **Homebrew**: Ensure Homebrew is installed on your system.
- **GCC**: Install GCC via Homebrew if not already installed:

  ```sh
  brew install gcc
  ```
- **wget** _or_ **curl**: One of these must be available for the network speed test:

  ```sh
  brew install wget
  # or
  brew install curl
  ```

### Clone the Repository

```sh
git clone https://github.com/yourusername/wifi-stability-analyzer.git
cd wifi-stability-analyzer
```

## Build

Compile the C source into the `wsa` executable:

```sh
gcc -g main.c -o wsa
```

## Usage

```sh
./wsa <duration_in_seconds> <interval_in_seconds>
```

- `<duration_in_seconds>`: Total logging time (e.g., 30).
- `<interval_in_seconds>`: Sampling interval (e.g., 0.5).

Example, log for 60 s at 0.5 s intervals:

```sh
./wsa 60 0.5
```

## Output

All output files are placed in the `output/` subfolder:

- `signal_strength.log`: Timestamped RSSI and noise readings.
- `connectivity.log`: Raw ping output per interval.
- `system_logs.log`: Captured `dmesg | grep -i wifi` lines and parsed counts.
- `report.csv`: Combined CSV report with columns:

  ```csv
  Timestamp,RSSI,Noise,PingSuccess,PingTimeMs,PacketMismatch,HighRetries,FcsFail,PlcpBad,GoodPlcps,CrsGlitches
  ```

After sampling completes, the tool runs a network speed test. If neither `wget` nor `curl` is installed, an error is printed.

## Troubleshooting

- **Empty `signal_strength.log`**: Ensure the command-line WiFi utility is available:

  ```sh
  /System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I
  ```

- **Missing `wget`/`curl`**: Install one of them via Homebrew.

- **Insufficient Permissions**: Some operations require `sudo` (e.g., reading `dmesg`).

## Contributing

Contributions are welcome! Please fork the repository and submit pull requests.

## License

This project is licensed under the MIT License.