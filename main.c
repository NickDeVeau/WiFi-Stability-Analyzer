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
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>

// Get current time with milliseconds
typedef long long ll;
char* get_current_time() {
    static char buffer[100];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm_info = localtime(&tv.tv_sec);
    char time_part[64];
    strftime(time_part, sizeof(time_part), "%Y-%m-%d %H:%M:%S", tm_info);
    int ms = tv.tv_usec / 1000;
    snprintf(buffer, sizeof(buffer), "%s.%03d", time_part, ms);
    return buffer;
}

void get_network_interfaces() {
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    if (getifaddrs(&ifap) == -1) { perror("getifaddrs"); return; }
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in*)ifa->ifa_addr;
            printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, inet_ntoa(sa->sin_addr));
        }
    }
    freeifaddrs(ifap);
}

void display_status_bar(int duration) {
    time_t start_time = time(NULL);
    int elapsed;
    while ((elapsed = time(NULL) - start_time) < duration) {
        int width = 50;
        int pos = (elapsed * width) / duration;
        printf("\rLogging: [");
        for (int i = 0; i < width; ++i) printf(i < pos ? "#" : "-");
        printf("] %3d%%", (elapsed * 100) / duration);
        fflush(stdout);
        sleep(1);
    }
    printf("\rLogging: [##################################################] 100%%\n");
}

void sample_all(int duration, double interval) {
    // Ensure output directory exists
    mkdir("output", 0755);

    FILE *sig_log = fopen("output/signal_strength.log", "w");
    FILE *con_log = fopen("output/connectivity.log", "w");
    FILE *sys_log = fopen("output/system_logs.log", "w");
    FILE *csv_log = fopen("output/report.csv", "w");
    if (!sig_log || !con_log || !sys_log || !csv_log) {
        perror("Failed to open log files");
        exit(EXIT_FAILURE);
    }

    // CSV header
    fprintf(csv_log, "Timestamp,RSSI,Noise,PingSuccess,PingTimeMs,PacketMismatch,HighRetries,FcsFail,PlcpBad,GoodPlcps,CrsGlitches\n");
    fflush(csv_log);

    int samples = (int)(duration / interval);
    ll interval_ns = (ll)(interval * 1e9 + 0.5);
    struct timespec start_ts; clock_gettime(CLOCK_MONOTONIC, &start_ts);
    ll start_ns = start_ts.tv_sec * 1000000000LL + start_ts.tv_nsec;

    for (int i = 0; i < samples; i++) {
        ll target_ns = start_ns + i * interval_ns;
        struct timespec now_ts;
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        ll now_ns = now_ts.tv_sec * 1000000000LL + now_ts.tv_nsec;
        ll sleep_ns = target_ns - now_ns;
        if (sleep_ns > 0) {
            struct timespec sleep_ts = { sleep_ns / 1000000000LL, sleep_ns % 1000000000LL };
            nanosleep(&sleep_ts, NULL);
        }

        char *ts = get_current_time();
        int rssi = 0, noise = 0;
        int ping_success = 0;
        double ping_time = 0.0;
        int packet_mismatch = 0;
        int high_retries = 0;
        int fcs_fail = 0, plcp_bad = 0, good_plcps = 0, crs_glitches = 0;

        // Signal strength
        FILE *fp_sig = popen("sudo wdutil info 2>/dev/null", "r");
        if (fp_sig) {
            char buf[256];
            while (fgets(buf, sizeof(buf), fp_sig)) {
                if (strstr(buf, "RSSI")) sscanf(buf, " RSSI : %d dBm", &rssi);
                else if (strstr(buf, "Noise")) sscanf(buf, " Noise : %d dBm", &noise);
            }
            pclose(fp_sig);
            fprintf(sig_log, "[%s] RSSI: %d Noise: %d\n", ts, rssi, noise);
            fflush(sig_log);
        }

        // Connectivity (ping)
        FILE *fp_con = popen("ping -c 1 8.8.8.8", "r");
        if (fp_con) {
            char buf[256];
            while (fgets(buf, sizeof(buf), fp_con)) {
                fprintf(con_log, "[%s] %s", ts, buf);
                if (strstr(buf, "time=")) {
                    ping_success = 1;
                    sscanf(strstr(buf, "time="), "time=%lf ms", &ping_time);
                }
            }
            pclose(fp_con);
            fflush(con_log);
        }

        // System logs (WiFi-related)
        FILE *fp_sys = popen("sudo dmesg | grep -i wifi", "r");
        if (fp_sys) {
            char buf[512];
            while (fgets(buf, sizeof(buf), fp_sys)) {
                fprintf(sys_log, "[%s] %s", ts, buf);
                // Parse packet mismatch
                if (strstr(buf, "Host and FW packet count mismatch")) packet_mismatch++;
                // Parse retries
                if (strstr(buf, "retries:")) {
                    char *tok = strtok(buf, " ");
                    while (tok) {
                        if (strncmp(tok, "retries:", 8) == 0) {
                            high_retries += atoi(tok + 8);
                        }
                        tok = strtok(NULL, " ");
                    }
                }
                // Parse PHY errors
                if (strstr(buf, "fcsFail:") || strstr(buf, "plcpBad:") || strstr(buf, "goodPlcps:") || strstr(buf, "crsGlitches:")) {
                    char *tok = strtok(buf, " ");
                    while (tok) {
                        if (strncmp(tok, "fcsFail:", 8) == 0) fcs_fail += atoi(tok + 8);
                        else if (strncmp(tok, "plcpBad:", 8) == 0) plcp_bad += atoi(tok + 8);
                        else if (strncmp(tok, "goodPlcps:", 10) == 0) good_plcps += atoi(tok + 10);
                        else if (strncmp(tok, "crsGlitches:", 12) == 0) crs_glitches += atoi(tok + 12);
                        tok = strtok(NULL, " ");
                    }
                }
            }
            pclose(fp_sys);
            fflush(sys_log);
        }

        // Write combined CSV row
        fprintf(csv_log,
            "\"%s\",%d,%d,%d,%.2f,%d,%d,%d,%d,%d,%d\n",
            ts, rssi, noise, ping_success, ping_time,
            packet_mismatch, high_retries, fcs_fail, plcp_bad, good_plcps, crs_glitches);
        fflush(csv_log);
    }

    fclose(sig_log);
    fclose(con_log);
    fclose(sys_log);
    fclose(csv_log);
}

void test_network_speed() {
    int ret = -1;
    // Try wget
    if (system("command -v wget >/dev/null 2>&1") == 0) {
        ret = system("wget -O /dev/null http://speedtest.tele2.net/1MB.zip");
    }
    // Fallback to curl
    else if (system("command -v curl >/dev/null 2>&1") == 0) {
        ret = system("curl -s -o /dev/null http://speedtest.tele2.net/1MB.zip");
    } else {
        fprintf(stderr, "Error: neither wget nor curl is installed.\n");
        return;
    }
    if (ret != 0) {
        fprintf(stderr, "Network speed test failed.\n");
    }
}

void generate_summary(int duration) {
    // existing summary logic unchanged...
    // (omitted for brevity)
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <duration> <interval>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int duration = atoi(argv[1]);
    double interval = atof(argv[2]);

    printf("Network Interfaces:\n");
    get_network_interfaces();

    printf("\nStarting periodic logging for %d seconds at %.3f-second intervals...\n", duration, interval);
    pid_t pid = fork();
    if (pid == 0) {
        display_status_bar(duration);
        exit(0);
    }
    sample_all(duration, interval);
    wait(NULL);

    printf("\nRunning network speed test...\n");
    test_network_speed();

    generate_summary(duration);
    return 0;
}