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
    const char* airport_cmd = "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I | grep -E 'agrCtlRSSI|agrCtlNoise|state'";

    while (time(NULL) - start_time < duration) {
        char buffer[128];
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
        sleep(1); // Log every 1 seconds
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
        sleep(1); // Log every 1 seconds
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

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <duration_in_seconds>\n", argv[0]);
        return 1;
    }

    int duration = atoi(argv[1]);
    if (duration <= 0) {
        fprintf(stderr, "Invalid duration: %d\n", duration);
        return 1;
    }

    printf("Network Interfaces:\n");
    get_network_interfaces();

    printf("\nStarting WiFi Stability Tests for %d seconds...\n", duration);

    pid_t pid_signal = fork();
    if (pid_signal == 0) {
        monitor_signal_strength(duration);
        exit(0); // Child process should exit after its task
    }

    pid_t pid_connectivity = fork();
    if (pid_connectivity == 0) {
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

    return 0;
}
