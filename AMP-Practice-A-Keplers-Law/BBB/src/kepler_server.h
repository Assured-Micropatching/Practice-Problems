#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include <linux/can.h>
#include <linux/can/raw.h>
// #include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
//#include <sys/select.h>
#include <time.h>
#include <stdint.h>
#include <signal.h>
#include <err.h>
#include <errno.h>
#include <math.h>

#define PRIORITY_MASK  0x1C000000
#define EDP_MASK       0x02000000
#define DP_MASK        0x01000000
#define PF_MASK        0x00FF0000
#define PS_MASK        0x0000FF00
#define SA_MASK        0x000000FF
#define PDU1_PGN_MASK  0x03FF0000
#define PDU2_PGN_MASK  0x03FFFF00

#define SERVER_SA        0   // This is the source address for the server
#define CLIENT_SA        120 // These are where requests come from

#define RESULTS_PGN      0xFF15 //
#define REQUEST_PGN      0xFF14 //
#define DEFAULT_PRIORITY 6 
#define GLOBAL_ADDRESS   255

#define pi M_PI

int init_can(int *sock, char *interface);
void usage(char * program);
// Installs signals for use cases such as the timer
static void install_signal(int sig);
static void sighandler(int sig, siginfo_t *info, void *vp);

int Keplers_Law(
        float mean_anomaly,
        float eccentricity,
        float initial_guess,
        float * mean_anomaly_output,
        //int * iterations_output,
        float * error_output);

uint32_t get_j1939_id(uint8_t priority, uint32_t pgn, uint8_t sa, uint8_t da);
void parse_J1939(uint32_t can_id, uint8_t *priority, uint32_t *pgn, uint8_t *da, uint8_t *sa);
float formula(float ecc_anomaly, float eccentricity, float mean_anomaly);
float formula_derivative(float ecc_anomaly, float eccentricity);

#endif /* SRC_MAIN_H_ */
