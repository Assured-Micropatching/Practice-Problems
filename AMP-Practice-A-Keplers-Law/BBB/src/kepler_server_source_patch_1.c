#include <stdio.h>
#include "kepler_server.h"


float tol = 10*__FLT_EPSILON__;

void usage(char * program) {
    fprintf(stderr,"Usage %s [can_interface] \n", program);
}


/**
  * @brief  Handles the initialization of the  CAN socket. 
  * It also binds the socket.
  *
  * @param  sock: a pointer to a socket, that will be initialized
  * @retval 0 on successful bind, -1 on failure
  */
int init_can( int *sock, char *interface ) {
    struct ifreq ifr;
    struct sockaddr_can addr;
    printf("Interface : %s\n",interface);
    strcpy(ifr.ifr_name, interface);
    // Retrieve the interface index of the interface into ifr_ifindex
    ioctl(*sock, SIOCGIFINDEX, &ifr);  

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // We're going to be listening
    if (bind(*sock,(struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("Failed to bind socket.");
        return -1;
    }
    return 0;
}


/**
  * @brief  Parses out the J1939 priority, pgn, destination address, and source address
  *         from a can_id.
  *
  * @param can_id: J1939 Can ID to parse
  * @param priority: returned priority value
  * @param pgn: returned Parameter Group Number
  * @param da: returned Destination Address
  * @param sa: returned Source Address
  */
void parse_J1939(uint32_t can_id, uint8_t *priority, uint32_t *pgn, uint8_t *da, uint8_t *sa) {
    // Parse J1939
    uint8_t pf, ps;
    // Priority
    *priority = (PRIORITY_MASK & can_id) >> 26;
    // Protocol Data Unit (PDU) Format
    pf = (PF_MASK & can_id) >> 16;
    // Protocol Data Unit (PDU) Specific
    ps = (PS_MASK & can_id) >> 8;
    // Determine the Parameter Group Number and Destination Address
    if (pf >= 0xF0) {
        // PDU 2 format, include the PS as a group extension
        *da = 255;
        *pgn = (can_id & PDU2_PGN_MASK) >> 8;
    } else {
        *da = ps;
        *pgn = (can_id & PDU1_PGN_MASK) >> 8;
    }
    // source address
    *sa = (can_id & SA_MASK);
}

/**
  * @brief  Combines J1939 id elements into a 29-bit CAN id.
  *
  * @param can_id: J1939 Can ID to generate
  * @param priority: input priority value
  * @param pgn: input Parameter Group Number
  * @param da: input Destination Address
  * @param sa: input Source Address
  */
uint32_t get_j1939_id(uint8_t priority, uint32_t pgn, uint8_t sa, uint8_t da){
    uint32_t can_id = sa;
    if (pgn >= 0xF000) {
        // PDU 2 format, include the PS as a group extension
        can_id += (0x3FFFF & pgn) << 8;
    } else {
        can_id += (0x3FF00 & pgn) << 8;
        can_id += da << 8;
    }
    can_id += priority << 26;
    can_id |= 0x80000000;
    return can_id;
}

/**
  * @brief  On getting a sig to kill program, set exit status to gracefully close program. sig_alrm is used for
  *         handling address claim timeouts
  *
  * @param  sig:  Signal given
  * @param  info: Additional signal info
  * @param  vp:   signal context structure
  */
static void sighandler(int sig, siginfo_t *info, void *vp) {
    switch (sig) {
        case SIGINT:
            exit(1);
            break;
        case SIGTERM:  // gracefully exit (close socket)
            exit(1);
            break;
        case SIGALRM:
            break;
    }
}


/**
  * @brief  Installs signals with the sighandler function
  *
  * @param  sig:  Signal to install
  */
static void install_signal(int sig) {
    int ret;
    struct sigaction sigact = {
            .sa_sigaction = sighandler,
            .sa_flags = SA_SIGINFO,
    };

    sigfillset(&sigact.sa_mask);
    ret = sigaction(sig, &sigact, NULL);
    if (ret < 0)
        err(1, "sigaction for signal %i", sig);
}



int Keplers_Law(
        float mean_anomaly,
        float eccentricity,
        float initial_guess,
        float * eccentric_anomaly_output,
        //int * iterations_output,
        float * error_output){
    printf("Using Newton-Rahpson to calculate eccentric anomaly based on Kepler's Law\n");
    printf("Inputs to determine eccentric anomaly:\n\teccentricity (e)= %0.6f\n\tmean anomaly (M) = %0.6f\n\tinitial guess = %0.6f\n", eccentricity,mean_anomaly,initial_guess);
    float error;
    float E;
    // Add this iteration counter as part of the patch
    // int n;
    // n = 0;
    // int convergence_flag;
    // convergence_flag = 1;
    E = initial_guess;
    error = 1;
    while (fabsf(error) > tol){
        error = formula(E, eccentricity, mean_anomaly);
        printf("Error: %g\n", error);
        E = E - error/formula_derivative(E,eccentricity);
        // Add this loop protection as part of the patch
        // n++;
        // if (n >= MAX_ITERATIONS){
        //      convergence_flag = 0;    
        //      break;
        // }
    }
    *eccentric_anomaly_output = E;
    // *iterations_output = n + 1; // n indexes from zero, so we need to add 1 to get interations
    *error_output = error;
    // replace this with return convergence flag for the patch.
    return 1; 
}


float formula(float ecc_anomaly, float eccentricity, float mean_anomaly)
{
    return (ecc_anomaly - eccentricity*(sinf(ecc_anomaly)) - mean_anomaly);
}
float formula_derivative(float ecc_anomaly, float eccentricity)
{
    return (1-eccentricity*(cosf(ecc_anomaly)));
}


int main(int argc, char **argv) {
    // Accepts arguments for:
    // can channel
    
    if (argc < 2){
        usage(argv[0]); 
        exit(1);
    }
    printf("Starting the Kelper's Law Server.\n");
    printf("J1939 messages with PGN %04X are inputs.\n",REQUEST_PGN);
    printf("Results are output in J1939 PGN %04X.\n",RESULTS_PGN);
    // Install signals
    install_signal(SIGTERM);
    install_signal(SIGINT);
    install_signal(SIGALRM);

    // Set defaults
    char * iter_name = argv[1];

    // Init CAN Socket
    int s = socket (PF_CAN, SOCK_RAW, CAN_RAW);
    if (init_can(&s, iter_name) != 0) {
        printf("Failed to create socket\n");
        exit(1);
    }
    printf("CAN Socket created for %s\n", iter_name );

    char *channel = argv[1];
    struct can_frame cf;
    
    printf("Tol: %.10e\n", tol);
    int ret;
    float eccentricity;
    float mean_anomaly;
    uint16_t raw_eccent;
    uint32_t raw_mean_anomaly;
    uint32_t out_mean_anomaly_int;
    float out_mean_anomaly;
    float out_error;
                    

    //J1939 Data
    uint8_t priority;
    uint32_t pgn;
    uint8_t da;
    uint8_t sa;

    // We will use the mean anomaly as our initial guess
    while (1) {
        int nbytes = read(s, &cf, sizeof(struct can_frame));
        if (nbytes == 0){
            sleep(0.01);
            continue;
        }
        parse_J1939(cf.can_id, &priority, &pgn, &da, &sa);
        printf("Received J1939 Message with PGN %08X and SA %d\n", pgn,sa);
        if (pgn == REQUEST_PGN){
            printf("Received valid request.\n");
            // unsigned char data_array[8];
            raw_eccent = 0;
            raw_mean_anomaly=0;
            memcpy(&raw_mean_anomaly,&cf.data[0],4);
            memcpy(&raw_eccent,&cf.data[4],2); 
            // Apply the SLOT for eccentricity to get radians
            eccentricity = raw_eccent*0.0015625;
            if (eccentricity >= 1) eccentricity = 0.9999999;

            //Apply the SLOT to get radians for mean anomaly
            float anom_deg = raw_mean_anomaly*0.000001 - 210;
            //TODO: check to see if data is in bounds
            // ensure the mean anomaly is between 0 and 2*pi radians
            while (anom_deg > 360) anom_deg -= 360;
            while (anom_deg < 0) anom_deg += 360;
            mean_anomaly = pi*anom_deg/180.0;
            // int out_iterations; //
            float out_mean_anomaly = 0;
            float out_error = 0;
            ret = Keplers_Law(mean_anomaly,
                              eccentricity,
                              mean_anomaly,
                              &out_mean_anomaly,
                              &out_error);
            // Convert out_mean_anomaly from radians into the J1939 SLOT
            printf("Solution converges to %f radians with an error of %.10e\n", out_mean_anomaly, out_error);
            float deg = 180*out_mean_anomaly/pi;
            while (deg > 180) deg -= 360;
            while (deg < -180) deg += 360;
            printf("which is %f degrees.\n\n", deg);
            out_mean_anomaly_int = (uint32_t)((deg + 210)*1000000);
            
            cf.can_id = get_j1939_id(DEFAULT_PRIORITY,
                                     RESULTS_PGN,
                                     SERVER_SA,
                                     GLOBAL_ADDRESS);
            cf.can_dlc = 8;
            memset(cf.data, 0xff, 8);
            memcpy(&cf.data[0],&out_mean_anomaly_int,4);
            write(s, &cf, sizeof(struct can_frame));
            
        }
    }
}
