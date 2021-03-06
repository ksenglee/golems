/**
 * @file read_rightArm_state.cpp
 * @brief Print states of right arm only
 */
#include <unistd.h>
#include <sns.h>
#include <ach.h>

#define ARM_AXES 7

//******************************
// Global variables
//******************************
ach_channel_t chan_state_right;

double qr[ARM_AXES]; double dqr[ARM_AXES];
struct timespec now;

//*************************
// Function prototypes
//*************************
static int update(void);
static int update_n( size_t n,
		     double *q, double *dq,
		     ach_channel_t *chan,
		     struct timespec *ts );
void printState( double *q, double *dq, int n,
		 char* label );

/**************************
 * @function main
 ***************************/
int main( int argc, char* argv[] ) {
    
    // Call sns
    sns_init();
    sns_start();

    // 2. Open channels
    sns_chan_open( &chan_state_right, "state-right", NULL );

    {
	ach_channel_t *chans[] = {&chan_state_right, NULL};
	sns_sigcancel( chans, sns_sig_term_default );
    }

    // 3. Run
    while( !sns_cx.shutdown ) {
	update();
	aa_mem_region_local_release();
	usleep(0.5*1e6);
    }

    // 4. Leave
    sns_end();
    return 0;
}

/**
 * @function update_n
 */
static int update_n( size_t n,
		     double *q,
		     double *dq,
		     ach_channel_t *chan, 
		     struct timespec *ts ) {

    size_t frame_size;
    void *buf = NULL;
    ach_status_t r = sns_msg_local_get( chan, &buf,
					&frame_size,
					ts, ACH_O_LAST | (ts ? ACH_O_WAIT : 0 ) );
    switch(r) {
    case ACH_OK:
    case ACH_MISSED_FRAME: {
	struct sns_msg_motor_state *msg = (struct sns_msg_motor_state*)buf;
	if( n == msg->header.n &&
	    frame_size == sns_msg_motor_state_size_n((uint32_t)n) ) {
	    
	    for( size_t j = 0; j < n; ++j ) {
		q[j] = msg->X[j].pos;
		dq[j] = msg->X[j].vel;
	    }
	    return 1;

	} else {
	    SNS_LOG( LOG_ERR, "Invalid motor_state message \n" );
	}
	break;
    }
    case ACH_TIMEOUT:
    case ACH_STALE_FRAMES:
    case ACH_CANCELED:
	break;
    default:
	SNS_LOG( LOG_ERR, "Failed ach_get: %s \n", ach_result_to_string(r) );

    } // end switch
    
    return 0;
}


/**
 * @function update
 */
static int update(void) {

    if( clock_gettime( ACH_DEFAULT_CLOCK, &now ) ) {
	SNS_LOG( LOG_ERR, "clock_gettime failed: '%s' \n", strerror(errno) );
	return 0;
    }
	struct timespec timeout = sns_time_add_ns( now, 1000*1000*1 );
	
	// Arm Right
	int u_r = update_n( ARM_AXES, qr, dqr, &chan_state_right, &timeout );
	
	printState( qr, dqr, ARM_AXES, "Right arm" );

	// Both are updated, return 1, otherwise return 0
	return u_r;

}

/**
 * @function printState
 */
void printState( double *q, double *dq, int n,
		 char* label ) {

    printf("\t * %s Position: ", label);
    for( int i = 0; i < n; ++i ) {
	printf(" %f ", q[i]);
    } printf("\n");

    printf(" Velocity: ");
    for( int i = 0; i < n; ++i ) {
	printf(" %f ", dq[i]);
    } printf("\n");

}
