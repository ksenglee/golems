/**
 * @file gatekeeper.cpp
 * @brief In charge of reading arm trajectories from client
 */
#include <sns.h>
#include <unistd.h>
#include <ach.h>

#include "msgs/bimanual_msgs.h"
#include "msgs/control_msgs.h"
#include "base_dual_control.h"
#include "global/crichton_global.h"

ach_channel_t bimanual_chan;
ach_channel_t bimanual_hand_chan;

/** Left limb */
ach_channel_t la_state_chan;
ach_channel_t la_ref_chan;
std::list<Eigen::VectorXd> la_path;

ach_channel_t lh_ref_chan;
std::list<Eigen::VectorXd> lh_path;


/** Right limb */
ach_channel_t ra_state_chan;
ach_channel_t ra_ref_chan;
std::list<Eigen::VectorXd> ra_path;

ach_channel_t rh_ref_chan;
std::list<Eigen::VectorXd> rh_path;

/** Gatekeeper feedback message */
ach_channel_t gatekeeper_msg_chan;

/** Dual control */
BaseDualControl bdc;

/** Additional data */
double mFreq = 100.0;
double mDt = 1.0 / mFreq;

double mMaxAccel = 0.4;
double mMaxVel = 0.3; // 0.4  23degrees/s

struct timespec t_now;
struct timespec t_timeout;
int mode;

/** Polling function */
bool poll_bimanual_chan( ach_channel_t* _chan,
			 int &_mode,
			 std::list<Eigen::VectorXd> &_left_path,
			 std::list<Eigen::VectorXd> &_right_path );

void send_gateekeper_msg( int _msg_type, bool _result );

/**
 * @function main
 */
int main( int argc, char* argv[] ) {
  
  Eigen::VectorXd maxVel, maxAccel;
  
  // Open channels
  ach_status r;
  r = ach_open( &bimanual_chan, "bimanual_chan", NULL );
  if( r!= ACH_OK ) { printf("[ERROR] bimanual_chan not created \n"); return 1; }
  ach_flush( &bimanual_chan ); 

 
  r = ach_open( &bimanual_hand_chan, "bimanual_hand_chan", NULL );
  if( r!= ACH_OK ) { printf("[ERROR] bimanual_hand_chan not created \n"); return 1; }
  ach_flush( &bimanual_hand_chan );

  r = ach_open( &la_state_chan, "state-left", NULL );
  if( r!= ACH_OK ) { printf("[ERROR] state-left not created \n"); return 1; }
  
  r = ach_open( &la_ref_chan, "ref-left", NULL );
  if( r!= ACH_OK ) { printf("[ERROR] ref-left not created \n"); return 1; }
  
  r = ach_open( &ra_state_chan, "state-right", NULL );
  if( r!= ACH_OK ) { printf("[ERROR] state-right not created \n"); return 1; }
  r = ach_open( &ra_ref_chan, "ref-right", NULL );
  if( r!= ACH_OK ) { printf("[ERROR] ref-right not created \n"); return 1; }

  r = ach_open( &lh_ref_chan, "sdhref-left", NULL );
  if( r!= ACH_OK ) { printf("[ERROR] sdhref-left not created \n"); return 1; }
  r = ach_open( &rh_ref_chan, "sdhref-right", NULL );
  if( r!= ACH_OK ) { printf("[ERROR] sdhref-right not created \n"); return 1; }

  r = ach_open( &gatekeeper_msg_chan, gGatekeeper_chan_name.c_str(), NULL );
  if( r!= ACH_OK ) { printf("[ERROR] Gatekeeper channel not created \n"); return 1; }

  // Set channels
  bdc.set_channels( 0, &la_ref_chan, &la_state_chan );
  bdc.set_channels( 1, &ra_ref_chan, &ra_state_chan );
  
  // Loop waiting for traj
  bool m;
  
  while( true ) {
    if( poll_bimanual_chan( &bimanual_chan,
			    mode,
			    la_path,
			    ra_path ) == true ) {
  printf("Received something! \n");

      int n_dof = -1;
      if( mode == 0 ) { n_dof = (*la_path.begin()).size(); }      
      else if( mode == 1 ) { n_dof = (*ra_path.begin()).size(); }
      else if( mode == 2 ) { n_dof = (*la_path.begin()).size(); } // Same as right
     bdc.set_numJoints( n_dof );
      maxVel = mMaxVel*Eigen::VectorXd::Ones( n_dof );
      maxAccel = mMaxAccel*Eigen::VectorXd::Ones( n_dof );

      if( mode == 0 ) {
	printf("Follow left path with %d points \n", la_path.size() );
	m = bdc.followTrajectory( 0, la_path, maxAccel, maxVel );
	send_gateekeper_msg( ARM_LEFT_END, m );
      } else if( mode == 1 ) {
	printf("Follow right path with %d points  \n", ra_path.size() );
	m = bdc.followTrajectory( 1, ra_path, maxAccel, maxVel );
	send_gateekeper_msg( ARM_RIGHT_END, m );
      } else if( mode == 2 ) {
	printf("Follow dual path with sizes %d and %d for left and right \n",
	       la_path.size(),
	       ra_path.size() );
	m = bdc.followDualTrajectory( la_path,
				      ra_path,
				      maxAccel, maxVel );
	send_gateekeper_msg( ARM_BOTH_END, m );
      } else {
	printf("Mode not valid \n");
	return 1;
      }
    } // poll_bimanual_chan

    /** HAND POLLING */
    int hand_side;
    if( poll_bimanual_chan( &bimanual_hand_chan,
			    mode,
			    lh_path,
			    rh_path ) == true ) {
      // This mode by now only works with one configuration, we don't need paths yet for hands
      Eigen::VectorXd config;
      ach_channel_t* chan;
      int n_dof;
      
      if( mode == 0 ) {
	config = *(lh_path.begin());
	chan = &lh_ref_chan;
	n_dof = config.size();
	hand_side = HAND_LEFT_END;
      }      
      else if( mode == 1 ) {
	config = *(rh_path.begin());
	chan = &rh_ref_chan;
	n_dof = config.size();
	hand_side = HAND_RIGHT_END;
      }
      else if( mode == 2 ) { printf("\t ! Not implemented yet \n"); break; } // Same as right
      else { printf("\t ! Mode not valid \n"); break; }

      BaseControl bc;
      double dt = 2.0;
      bc.control_n( n_dof,
		    config,
		    dt,
		    chan,
		    SNS_MOTOR_MODE_POS );     
      usleep(dt*1e6);
      send_gateekeper_msg( hand_side, m );
    }
    
    usleep(1e6*mDt);
    aa_mem_region_local_release();
  }

  return 0;
}


/**
 * @function poll_bimanual_chan
 * @brief Checks if a message is received through traj_chan
 */
bool poll_bimanual_chan( ach_channel_t* _chan,
			 int &_mode,
			 std::list<Eigen::VectorXd> &_leftPath,
			 std::list<Eigen::VectorXd> &_rightPath ) {
  
  _leftPath.resize(0);
  _rightPath.resize(0);
    
  if( clock_gettime( ACH_DEFAULT_CLOCK, &t_now ) ) {
    SNS_LOG( LOG_ERR, "clock_gettime failed: '%s' \n", strerror(errno) );
    return false;
  }

  t_timeout = sns_time_add_ns( t_now, 1000*1000*1 );
  
  size_t frame_size;
  void* buf = NULL;
  ach_status_t r = sns_msg_local_get( _chan,
				      &buf,
				      &frame_size,
				      &t_timeout,
				      ACH_O_LAST | ACH_O_WAIT );

  switch(r) {
    
  case ACH_OK:
  case ACH_MISSED_FRAME: {

    struct sns_msg_bimanual* msg = (struct sns_msg_bimanual*) buf;
    if( frame_size == sns_msg_bimanual_size(msg) ) {
     // Store trajectory
      int n_dofs, n_steps_left, n_steps_right, mode;
      n_dofs = msg->n_dof;
     n_steps_left = msg->n_steps_left;
      n_steps_right = msg->n_steps_right;
      _mode = msg->mode;
      printf("\t * [INFO] Received trajectory in mode (%d) with %d left and %d right points and %d dofs \n", _mode, n_steps_left, n_steps_right, n_dofs );
      
      if( _mode == 0 ) {
	int counter = 0;
	for( int i = 0; i < n_steps_left; ++i ) {
	  Eigen::VectorXd p(n_dofs);
	  for( int j = 0; j < n_dofs; ++j ) {
	    p(j) = msg->x[counter];
	    counter++;
	  }
  _leftPath.push_back( p );
	} // for i
      } else if( _mode == 1 ) {
	int counter = 0;
	for( int i = 0; i < n_steps_right; ++i ) {
	  Eigen::VectorXd p(n_dofs);
	  for( int j = 0; j < n_dofs; ++j ) {
	    p(j) = msg->x[counter];
	    counter++;
	  }
  _rightPath.push_back( p );
	} // for i
	
      } else if( _mode == 2 ) {

	int counter = 0;
	for( int i = 0; i < n_steps_left; ++i ) {
	  Eigen::VectorXd p(n_dofs);
	  for( int j = 0; j < n_dofs; ++j ) {
	    p(j) = msg->x[counter];
	    counter++;
	  }
  _leftPath.push_back( p );
	} // for i

	counter = n_steps_left * n_dofs;
	for( int i = 0; i < n_steps_right; ++i ) {
	  Eigen::VectorXd p(n_dofs);
	  for( int j = 0; j < n_dofs; ++j ) {
	    p(j) = msg->x[counter];
	    counter++;
	  }
  _rightPath.push_back( p );
	} // for i
	
	

      } else {
	printf("Error, mode incorrect \n");
	return false;
      }
      
      return true;
      
    } else {
      SNS_LOG( LOG_ERR, "\t [ERROR] Invalid path_dense msg \n" );
      return false;
    }

  } break;
  case ACH_TIMEOUT:
  case ACH_STALE_FRAMES:
  case ACH_CANCELED:
    break;
  default:
    SNS_LOG( LOG_ERR, "Failed ach_get: %s \n", ach_result_to_string(r) );
    
  } // switch r

  return false;
}


/**
 * @function send_gateekeper_msg
 */
void send_gateekeper_msg( int _msg_type, bool _result ) {

  gatekeeper_msg msg;
  sns_msg_header_fill( &msg.header );
  msg.type = _msg_type;
  msg.state = _result;
  ach_status r; 
 r = ach_put( &gatekeeper_msg_chan,
	   (void*)&msg, sizeof(msg) );
}
