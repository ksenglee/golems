/**
 * @file calibration_v0.cpp
 * @brief Grab RGB, depth and kinematics, spits out calibration
 * @author A. Huaman Q.
 */
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>

#include <Eigen/Core>

#include <aoi/utils/robotData.h>
#include <aoi/utils/globalStructs.h>
#include <dart/utils/urdf/DartLoader.h>
#include <motion_control/DualLimb_Interface.h>

#include <dart/dynamics/Skeleton.h>
#include <dart/dynamics/BodyNode.h>

#include <marker_detection/crichton_markerDetector.h>
#include <calibration/calibration_utils.h>

struct calib_pair{
  Eigen::Vector3d pk;
  Eigen::Vector3d pr;
  int id;
};

cv::Mat mRgbImg;
cv::Mat mDepthImg;
std::string mWindowName( "Calibration v0" );

std::vector<Eigen::Isometry3d> mTjm; // Tf from joint to marker
std::vector<std::string> mJointNames;
Eigen::Vector3d mPam[2][4];
int mId[2][4];

subject_t mS;
DualLimb_Interface mDli;
ach_channel_t mBiTraj_chan;
ach_channel_t mBiHand_chan;

dart::simulation::World* mWorld;
crichton_markerDetector mMd;

std::vector<calib_pair> mP[2];

std::vector<Eigen::Vector3d> mPk[2];
std::vector<Eigen::Vector3d> mPr[2];

int counter = 0;

/***********************/
/** FUNCTIONS          */
/***********************/
void grabSnapshot( int state, void* userdata );
void showSnapshot( int state, void* userdata );
void storeSnapshot( int state, void* userdata );
void discardSnapshot( int state, void* userdata );
void calibrate( int state, void* userdata );

static void onMouse( int event, int x, int y, int, void* );

bool setKinematics();
void openComm();
void loadDefault();
void updateKinematics();
void updateTf();

/**
 * @function main
 */
int main( int argc, char* argv[] ) {
  
  // Set kinematics
  setKinematics();

  // Open Communication
  openComm();
  
  // Marker detector
  mMd.setMarkerSize(5);
  
  // Open device
  cv::VideoCapture capture( cv::CAP_OPENNI2 ); 
  
  if( !capture.isOpened() ) {
    std::cout << "/t * Could not open the capture object"<<std::endl;
    return -1;
  }
  printf("\t * Opened device \n");

  capture.set( cv::CAP_PROP_OPENNI2_MIRROR, 0.0 ); // off
  capture.set( cv::CAP_PROP_OPENNI_REGISTRATION, -1.0 ); // on

  printf("\t * RGB dimensions: (%f,%f), Depth dimensions: (%f,%f) \n",
	 capture.get( cv::CAP_OPENNI_IMAGE_GENERATOR + cv::CAP_PROP_FRAME_WIDTH ),
	 capture.get( cv::CAP_OPENNI_IMAGE_GENERATOR + cv::CAP_PROP_FRAME_HEIGHT ),
	 capture.get( cv::CAP_OPENNI_DEPTH_GENERATOR + cv::CAP_PROP_FRAME_WIDTH ),
	 capture.get( cv::CAP_OPENNI_DEPTH_GENERATOR + cv::CAP_PROP_FRAME_HEIGHT ) );
  
  // Set control panel
  int value;
  cv::namedWindow( mWindowName );
  // Set mouse callback 
  cv::setMouseCallback( mWindowName, onMouse, 0 );

  // Set buttons
  cv::createTrackbar( "**********", NULL, &value, 255, NULL );
  cv::createButton( "Grab snapshot", grabSnapshot, 
		    NULL, cv::QT_PUSH_BUTTON,
		    false );
  cv::createButton( "Show snapshot", showSnapshot, 
		    NULL, cv::QT_PUSH_BUTTON,
		    false );
  cv::createTrackbar( "----------", NULL, &value, 255, NULL );
  cv::createButton( "Store snapshot", storeSnapshot, 
		    NULL, cv::QT_PUSH_BUTTON,
		    false );
  cv::createButton( "Discard snapshot", discardSnapshot, 
		    NULL, cv::QT_PUSH_BUTTON,
		    false );
  cv::createTrackbar( "xxxxxxxxx", NULL, &value, 255, NULL );
  cv::createButton( "Calibrate", calibrate, 
		    NULL, cv::QT_PUSH_BUTTON,
		    false );
  
  // Loop
  for(;;) {
    if( !capture.grab() ) {
      std::cout << "\t * [DAMN] Could not grab a frame" << std::endl;
      return -1;
    }

    updateKinematics();
    updateTf();
    
    capture.retrieve( mRgbImg, cv::CAP_OPENNI_BGR_IMAGE );
    capture.retrieve( mDepthImg, cv::CAP_OPENNI_POINT_CLOUD_MAP );

    mMd.detect( mRgbImg );

    cv::imshow( mWindowName, mRgbImg );

    
    char k = cv::waitKey(30);
    if( k == 'q' ) {
      printf("\t * [PRESSED ESC] Finishing the program \n");
      break;
    }
    aa_mem_region_local_release();
    
  } // end for
  
  return 0;
}


/**
 * @brief Check depth with a click
 */
static void onMouse( int event, int x, int y, int, void* ) {
  
  if( event != cv::EVENT_LBUTTONDOWN ) {
    return;
  }

  // Get (X,Y,Z) from Kinect
  cv::Point3f p;
  p = mDepthImg.at<cv::Point3f>(y,x);
  printf("Depth: %f %f %f \n", p.x, p.y, p.z); 
}

/**
 * @function openComm
 */
void openComm() {
  
  // Start sns
  sns_init();
  sns_start();
  
  // Open channels to read motor states
  ach_channel_t* arm_state_chan[2];
  ach_channel_t* hand_state_chan[2];
  
  for( int i = 0; i < 2; ++i ) {
    arm_state_chan[i] = new ach_channel_t();
    hand_state_chan[i] = new ach_channel_t();
  }
  
  for( int i = 0; i < 2; ++i ) {
    sns_chan_open( arm_state_chan[i], mS.limb[i].arm_state_chan.c_str(), NULL );
    sns_chan_open( hand_state_chan[i], mS.limb[i].hand_state_chan.c_str(), NULL );
  }

  // BIMANUAL
  sns_chan_open( &mBiTraj_chan, "bimanual_chan", NULL );
  sns_chan_open( &mBiHand_chan, "bimanual_hand_chan", NULL );
  mDli.set_numJoints(7,7);
  mDli.set_hand_channels( hand_state_chan[0], hand_state_chan[1], &mBiHand_chan );
  mDli.set_arm_channels( arm_state_chan[0], arm_state_chan[1], &mBiTraj_chan );
  
  // Ready
  printf("\t * [SET_COMM] We should be ready to go \n");  
}

/**
 * @fuction grabSnapshot
 * @brief Store current information
 */
void grabSnapshot( int state, void* userdata ) {

  // Reset
  for( int i = 0; i < 2; ++i ) { mP[i].resize(0); }
  
  std::vector<calib_marker> markers;
  markers = mMd.getCalibMarkers();
  int ind;
  
  for( int i = 0; i < 2; ++i ) {
    for( int j = 0; j < 4; ++j ) {

      
      // Check if id was found
      ind = -1;
      for( int k = 0; k < markers.size(); ++k ) {
	if( mId[i][j] == markers[k].id ) {
	  ind = k; break;
	}
      }

      if( ind != -1 ) {
	// Get position in Kinect coordinates
	cv::Point3f p; p = mDepthImg.at<cv::Point3f>( markers[ind].py, markers[ind].px );
	// Check if it is valid
	if( p.x == 0 && p.y == 0 && p.z == 0 ) { continue; }
	
	// If valid, get position in robot coordinates and store
	calib_pair cp;
	cp.pk = Eigen::Vector3d( -p.x, p.y, p.z );
	cp.pr = mPam[i][j];
	cp.id = mId[i][j];
	mP[i].push_back( cp );
      }
      
      
    }
  }
  
}

/**
 * @fuction showSnapshot
 */
void showSnapshot( int state, void* userdata ) {

  for( int i = 0; i < 2; ++i ) {
    printf("Arm [%d] validmarkers detected \n", i);
    for( int j = 0; j < mP[i].size(); ++j ) {
      printf("Id: %d - Pk: %f %f %f - Pr: %f %f %f \n", mP[i][j].id,
	     mP[i][j].pk(0), mP[i][j].pk(1), mP[i][j].pk(2),
	     mP[i][j].pr(0), mP[i][j].pr(1), mP[i][j].pr(2) );
    }
  }
}

/**
 * @function storeSnapshot
 */
void storeSnapshot( int state, void* userdata ) {
  for( int i = 0; i < 2; ++i ) {
    for( int j = 0; j < mP[i].size(); ++j ) {
      mPk[i].push_back( mP[i][j].pk );
      mPr[i].push_back( mP[i][j].pr );
    }
    printf("Points stored so far for arm %d : %d \n", i, mPk[i].size() );
  }
}

/**
 * @function discardSnapshot
 */
void discardSnapshot( int state, void* userdata ) {
  // Uh? Well, don't store, that is it
}

/**
 * @function calibrate
 */
void calibrate( int state, void* userdata ) {

  Eigen::Isometry3d Tf[2];
  
  for( int i = 0; i < 2; ++i ) {
    svd_method( mPk[i], mPr[i], mPk[i].size(), Tf[i] );

    std::cout << "Trk ["<<i<<"] from "<<mPk[i].size() <<" points: "<< std::endl;
    std::cout << Tf[i].matrix() << std::endl;   
  }
  
}

/**
 * @function setKinematics
 */
bool setKinematics() {

  // Always set this first
  setHardCodedStructures();

  std::string path_world("/home/ana/Research/commonData/scenes/benchmark_tests/test0_followBall.urdf");
  
  // Load known knowledge
  loadDefault();
  
  // Load world
  dart::utils::DartLoader dl;
  mWorld = dl.parseWorld( path_world );
  
  if( !mWorld ) {
    printf("\t [ERROR] Was not able to load world \n");
    return false;
  }
  
  setSubject( mS, mWorld, LWA4_ROBOT );
  return true;
}


/**
 * @function loadDefault
 */
void loadDefault() {
    
  // Load default Tjoint_markers
  mTjm.resize(4);
  mJointNames.resize(4);
  for( int i = 0; i < 4; ++i ) { mTjm[i].setIdentity(); }
  
  // F22: Knuckle
  mTjm[0].translation() << 0, -0.0208, 0;
  mJointNames[0] = std::string("finger_22_link");
  // F23: Tip
  mTjm[1].translation() << 0, -0.015, 0;
  mJointNames[1] = std::string("finger_23_link");

  // F12: Knuckle
  mTjm[2].translation() << 0, 0.0275, 0;
  mJointNames[2] = std::string("finger_12_link");
  // F13: Tip
  mTjm[3].translation() << 0, 0.0176, 0;
  mJointNames[3] = std::string("finger_13_link");

  // mMarkerIndices
  mId[0][0] = 23; mId[0][1] = 24; mId[0][2] = 25; mId[0][3] = 26; // Left
  mId[1][0] = 2; mId[1][1] = 1; mId[1][2] = 8; mId[1][3] = 3; // Right
  
}

/**
 * @function updateKinematics
 */
void updateKinematics() {

  Eigen::VectorXd arm_q[2];
  Eigen::VectorXd arm_dq[2];
  
  Eigen::VectorXd hand_q[2];
  Eigen::VectorXd hand_dq[2];
  
  for( int i = 0; i < 2; ++i ) { 
    
    if( mDli.update(i) == true ) {
      
      // Get states
      mDli.get_arm_state( i, arm_q[i], arm_dq[i] );
      mDli.get_hand_state( i, hand_q[i], hand_dq[i] );
      
      // Update simulation      
      mS.limb[i].arm->setPositionSegment( mS.limb[i].armDofs, 
					  arm_q[i] );
      
      if( hand_dq[i].size() == 7 ) { 
	Eigen::VectorXd q(8); 
	q << hand_q[i](0), hand_q[i](1), hand_q[i](2), hand_q[i](0), hand_q[i](5), hand_q[i](6), hand_q[i](3), hand_q[i](4);
	mS.limb[i].hand->setPositionSegment( mS.limb[i].fingerDofs, q );
      } // end if hand_dq
      
    } // end if update
    
  }  // end for

  
}

/**
 * @function updateTf
 */
void updateTf() {

  Eigen::Isometry3d Twa, Taw, Twj, Twm, Tam;
  
  mS.update();
  
  // Get the poses of the finger markers
  for( int i = 0; i < 2; ++i ) {
    
    Twa = mS.limb[i].arm->getRootBodyNode()->getTransform();
    Taw = Twa.inverse();
    for( int j = 0; j < 4; ++j ) {
      
      Twj = mS.limb[i].hand->getBodyNode(mJointNames[j])->getTransform();
      Twm = Twj*mTjm[j];
      Tam = Taw* Twm;
      mPam[i][j] = Tam.translation();
    }
    
  }
  
  
}
