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
    time_t rawtime;
    struct tm *timeinfo;
    static char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return buffer;
}

// Function to get network interfaces
void get_network_interfaces() {
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;

    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, addr);
        }
    }
    freeifaddrs(ifap);
}

// Function to monitor signal strength
void monitor_signal_strength(int duration) {
    FILE *log = fopen("signal_strength.log", "a");
    if (!log) {
        perror("Failed to open log file");
        return;
    }

    time_t start_time = time(NULL);
    const char* airport_cmd = "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I";

    while (time(NULL) - start_time < duration) {
        char buffer[256];
        FILE *fp = popen(airport_cmd, "r");
        if (fp == NULL) {
            perror("Failed to run airport command");
            fclose(log);
            return;
        }

        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            fprintf(log, "[%s] %s", get_current_time(), buffer);
        }
        pclose(fp);

        fflush(log);
        sleep(1); // Log every 1 second
    }

    fclose(log);
}

// Function to check connectivity
void check_connectivity(int duration) {
    FILE *log = fopen("connectivity.log", "a");
    if (!log) {
        perror("Failed to open log file");
        return;
    }

    time_t start_time = time(NULL);

    while (time(NULL) - start_time < duration) {
        char buffer[128];
        FILE *fp = popen("ping -c 1 8.8.8.8", "r");
        if (fp == NULL) {
            perror("Failed to run ping command");
            fclose(log);
            return;
        }

        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            fprintf(log, "[%s] %s", get_current_time(), buffer);
        }
        pclose(fp);

        fflush(log);
        sleep(1); // Log every 1 second
    }

    fclose(log);
}

// Function to test network speed
void test_network_speed() {
    int ret = system("wget -O /dev/null http://speedtest.tele2.net/1MB.zip");
    if (ret == -1) {
        perror("Failed to run wget");
    }
}

// Function to display a status bar
void display_status_bar(int duration) {
    time_t start_time = time(NULL);
    int elapsed;
    while ((elapsed = time(NULL) - start_time) < duration) {
        int progress = (elapsed * 50) / duration; // 50 is the width of the progress bar
        printf("\rTesting: [%-*.*s] %d%%", 
            50, progress, "==================================================", 
            (elapsed * 100) / duration);
        fflush(stdout);
        sleep(1);
    }
    printf("\rTesting: [==================================================] 100%%\n");
}

void generate_summary(int duration) {
    FILE *signal_log = fopen("signal_strength.log", "r");
    FILE *connectivity_log = fopen("connectivity.log", "r");
    FILE *system_log = fopen("system_logs.log", "r");

    if (!signal_log || !connectivity_log || !system_log) {
        perror("Failed to open log files");
        return;
    }

    char buffer[256];
    int signal_count = 0, rssi_total = 0, noise_total = 0;
    int ping_count = 0, ping_success = 0;
    int packet_count_mismatch = 0, high_retries = 0;
    int fcs_failures = 0, plcp_bad = 0, good_plcps = 0, crs_glitches = 0;

    // Parsing signal strength log
    while (fgets(buffer, sizeof(buffer), signal_log)) {
        int rssi, noise;
        if (sscanf(buffer, "[%*[^]]] agrCtlRSSI: %d", &rssi) == 1) {
            rssi_total += rssi;
            signal_count++;
        }
        if (sscanf(buffer, "[%*[^]]] agrCtlNoise: %d", &noise) == 1) {
            noise_total += noise;
        }
    }

    // Parsing connectivity log
    while (fgets(buffer, sizeof(buffer), connectivity_log)) {
        if (strstr(buffer, "PING")) {
            ping_count++;
        }
        if (strstr(buffer, "64 bytes from")) {
            ping_success++;
        }
    }

    // Parsing system log
    while (fgets(buffer, sizeof(buffer), system_log)) {
        if (strstr(buffer, "Host and FW packet count mismatch")) {
            packet_count_mismatch++;
        }
        if (strstr(buffer, "retries:")) {
            char *token = strtok(buffer, " ");
            while (token != NULL) {
                if (strncmp(token, "retries:", 8) == 0) {
                    int retries = strtol(token + 8, NULL, 10);
                    if (retries > 50) {
                        high_retries++;
                    }
                }
                token = strtok(NULL, " ");
            }
        }
        if (strstr(buffer, "fcsFail:")) {
            char *token = strtok(buffer, " ");
            while (token != NULL) {
                if (strncmp(token, "fcsFail:", 8) == 0) {
                    fcs_failures += strtol(token + 8, NULL, 10);
                } else if (strncmp(token, "plcpBad:", 8) == 0) {
                    plcp_bad += strtol(token + 8, NULL, 10);
                } else if (strncmp(token, "goodPlcps:", 10) == 0) {
                    good_plcps += strtol(token + 10, NULL, 10);
                } else if (strncmp(token, "crsGlitches:", 12) == 0) {
                    crs_glitches += strtol(token + 12, NULL, 10);
                }
                token = strtok(NULL, " ");
            }
        }
    }

    fclose(signal_log);
    fclose(connectivity_log);
    fclose(system_log);

    float avg_rssi = (signal_count > 0) ? (float)rssi_total / signal_count : 0;
    float avg_noise = (signal_count > 0) ? (float)noise_total / signal_count : 0;
    float packet_loss = (ping_count > 0) ? (float)(ping_count - ping_success) / ping_count * 100 : 0;

    printf("\n=== WiFi Stability Test Summary ===\n");
    printf("Duration: %d seconds\n", duration);
    printf("Average Signal Strength (RSSI): %.2f dBm\n", avg_rssi);
    printf("Average Noise Level: %.2f dBm\n", avg_noise);
    printf("Packet Loss: %.2f%%\n", packet_loss);
    printf("Packet Count Mismatches: %d\n", packet_count_mismatch);
    printf("High Retries: %d\n", high_retries);
    printf("FCS Failures: %d\n", fcs_failures);
    printf("PLCP Errors: %d\n", plcp_bad);
    printf("Good PLCPS: %d\n", good_plcps);
    printf("CRS Glitches: %d\n", crs_glitches);
    printf("===================================\n");

    // Diagnosis
    if (packet_loss > 50) {
        printf("Diagnosis: High packet loss detected. Likely due to network congestion or external factors.\n");
    } else if (packet_count_mismatch > 0) {
        printf("Diagnosis: Packet count mismatches detected. This is likely due to hardware issues with the WiFi adapter.\n");
    } else if (high_retries > 0) {
        printf("Diagnosis: High retry rates detected. This may be due to interference or a malfunctioning WiFi adapter.\n");
    } else if (fcs_failures > 50 || plcp_bad > 50) {
        printf("Diagnosis: High number of FCS or PLCP errors detected. This indicates potential hardware issues or significant interference.\n");
    } else if (crs_glitches > 50) {
        printf("Diagnosis: High number of CRS glitches detected. This suggests possible hardware issues or interference.\n");
    } else if (avg_rssi < -70) {
        printf("Diagnosis: Poor signal strength detected. Consider moving closer to the router or checking the WiFi hardware.\n");
    } else if (avg_noise > -85) {
        printf("Diagnosis: High noise level detected. This could be due to interference from other devices or networks.\n");
    } else {
        printf("Diagnosis: WiFi performance is within normal parameters.\n");
    }
}

// Function to check system logs for errors
void check_system_logs() {
    printf("\nChecking system logs for WiFi-related errors...\n");
    int ret = system("sudo dmesg | grep -i wifi > system_logs.log");
    if (ret == -1) {
        perror("Failed to obtain system logs");
    } else {
        printf("System logs have been saved to system_logs.log\n");
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <duration_in_seconds>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int duration = atoi(argv[1]);

    printf("Network Interfaces:\n");
    get_network_interfaces();

    printf("\nMonitoring Signal Strength:\n");
    if (fork() == 0) {
        monitor_signal_strength(duration);
        exit(0); // Child process should exit after its task
    }

    printf("\nChecking Connectivity:\n");
    if (fork() == 0) {
        check_connectivity(duration);
        exit(0); // Child process should exit after its task
    }

    printf("\nTesting Network Speed:\n");
    test_network_speed();

    // Display status bar
    display_status_bar(duration);

    // Wait for all child processes to finish
    int status;
    while (wait(&status) > 0);

    printf("\nWiFi Stability Tests Completed.\n");

    // Check system logs for errors
    check_system_logs();

    // Generate summary report
    generate_summary(duration);

    return 0;
}
