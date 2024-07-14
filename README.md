# WiFi Stability Analyzer (WSA)

WiFi Stability Analyzer (WSA) is a command-line tool designed to monitor and log the stability of your WiFi connection. It measures signal strength, connectivity, and network speed over a specified duration, and logs the results for analysis.

## Features

- **Signal Strength Monitoring:** Logs WiFi signal strength at regular intervals.
- **Connectivity Check:** Logs connectivity status by pinging a known server.
- **Network Speed Test:** Measures download speed by downloading a file from a known server.
- **Progress Bar:** Displays a progress bar indicating the progress of the tests.

## Installation

### Prerequisites

- **macOS**: The tool is designed to work on macOS.
- **Homebrew**: Ensure Homebrew is installed on your system.
- **GCC**: Install GCC via Homebrew if not already installed.

```sh
brew install gcc
```

### Clone the Repository

Clone the repository to your local machine:

```sh
git clone https://github.com/yourusername/wifi-stability-analyzer.git
cd wifi-stability-analyzer
```

## Usage

### Build the Program

To build the program, use the following command:

```sh
gcc -g main.c -o wsa
```

### Run the Program

To run the program, use the following command format:

```sh
./wsa <duration_in_seconds>
```

For example, to run the analyzer for 30 seconds:

```sh
./wsa 30
```

### Output

The program generates two log files:

- `signal_strength.log`: Contains logs of WiFi signal strength.
- `connectivity.log`: Contains logs of connectivity status.

## Components

### Main Functions

- **get_current_time:** Returns the current time as a formatted string.
- **get_network_interfaces:** Displays network interfaces and their addresses.
- **monitor_signal_strength:** Logs WiFi signal strength at regular intervals.
- **check_connectivity:** Logs connectivity status by pinging a known server.
- **test_network_speed:** Measures download speed by downloading a file from a known server.
- **display_status_bar:** Displays a progress bar indicating the progress of the tests.

### Code Structure

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <time.h>
#include <sys/wait.h>

// Function to get the current time as a string
char* get_current_time() {
    // Implementation here
}

// Function to get network interfaces
void get_network_interfaces() {
    // Implementation here
}

// Function to monitor signal strength
void monitor_signal_strength(int duration) {
    // Implementation here
}

// Function to check connectivity
void check_connectivity(int duration) {
    // Implementation here
}

// Function to test network speed
void test_network_speed() {
    // Implementation here
}

// Function to display a status bar
void display_status_bar(int duration) {
    // Implementation here
}

// Main function
int main(int argc, char *argv[]) {
    // Implementation here
}
```

## Troubleshooting

### `signal_strength.log` is Empty

If `signal_strength.log` is empty, ensure the `airport` command is available and working:

```sh
/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I
```

This command should output information about your WiFi connection.

### `gcc` is Not Installed

If `gcc` is not installed, install it using Homebrew:

```sh
brew install gcc
```

## Contributing

Contributions are welcome! Please fork the repository and submit pull requests.

## License

This project is licensed under the MIT License.
