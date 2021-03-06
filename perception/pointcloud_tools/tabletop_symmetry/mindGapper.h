/**
 * @file mindGapper.h
 * @brief Generates an object's full pointcloud from a partial view using symmetries w.r.t. a given resting plane
 */
#pragma once

#include <vector>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

//-- OpenCV headers
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

/**
 * @class mindGapper
 */
template<typename PointT>
class mindGapper {

 public:
  
  typedef pcl::PointCloud<PointT> PointCloud;
  typedef typename PointCloud::Ptr PointCloudPtr;
  typedef typename PointCloud::ConstPtr PointCloudConstPtr;
  typedef typename PointCloud::iterator PointCloudIter;


  mindGapper();
  ~mindGapper();
  
  /**< Set table plane where the object rests and its partial cloud */
  void setTablePlane( std::vector<double> planeCoeff );


  /**< Set parameters for symmetry plane search */
  void setFittingParams( int _n = 11, int _m = 17, 
			 double _dj = 0.01, double _alpha = 45.0*M_PI / 180.0 );

  /**< Set Kinect params to calculate mirror errors */
  void setDeviceParams( int _width = 640, int _height = 480, 
			double _focal_length_in_pixels = 525,
			double _cx = 320, double _cy = 240 );
  void setFocalDist(double _f) { mF = _f; }

  /** Reset storage variables for new mirror operation */
  void reset();

  /**< Generates mirroring cloud */
  int complete( PointCloudPtr &_cloud,
		bool _completeCloud = true );

  Eigen::VectorXd getSymPlane() { return mSymPlane; } // call after complete
  Eigen::VectorXd getCenterPlane() { return mCenterPlane; } // call after complete
  void getProjectedInfo( Eigen::Vector3d &_center,
			 Eigen::Vector3d &_ea,
			 Eigen::Vector3d &_eb ) { _center = mC; _ea = mEa; _eb = mEb; }
  PointCloudPtr getProjected() { return mProjected; }

  /**< Get the symmetry Tf: Z is the table plane and the rest comes from the symmetry plane */
  void getSymmetryApprox( Eigen::Isometry3d &_Tf,

			  Eigen::Vector3d &_dim );

  void calculateSymmTf( const Eigen::Isometry3d &_Rt,
			const PointCloudPtr &_cloud );
  
  bool generate2DMask( PointCloudPtr _segmented_cloud,
		       cv::Mat &_markMask,
		       cv::Mat &_depthMask );
  cv::Mat get2DMask() { return mMarkMask; }

  cv::Mat printDebugMask();

  // Debug functions
  bool viewMirror( int _ind );
  void printMirror( int _ind );
  PointCloudPtr getCandidate(int _ind) { return mCandidates[_ind]; }
  bool viewInitialParameters();

  std::vector<Eigen::VectorXd> mSymPlanes;
  std::vector<Eigen::VectorXd> mCenterPlanes;


 private:
  Eigen::VectorXd mSymPlane; Eigen::VectorXd mCenterPlane;

  /** Helper functions for complete */
  PointCloudPtr mirrorFromPlane( PointCloudPtr _cloud,
				 Eigen::VectorXd _plane,
				 bool _joinMirrored = true );

  PointCloudPtr mProjected;
  std::vector<PointCloudPtr> mCandidates;
  std::vector<double> mDelta1;
  std::vector<double> mDelta2;
  std::vector<bool> mValidity;
  PointCloudPtr mCloud;
  Eigen::Isometry3d mSymmTf;
  Eigen::Vector3d mBBDim;

  /**< Constants for Limits */
  double mMax_Out_Mask_Ratio;
  double mUpper_Ratio_Delta;
  double mMax_Front_Dist_Avg;
  double mMax_Out_Pixel_Avg;
  double mCutoff_Pixel_MaxDist;
  
  /**< Variables */
  Eigen::VectorXd mPlaneCoeffs;
  int mN;
  int mM;
  double mDj;
  double mAlpha;
  
  // Mask variables
  double mF, mCx, mCy;
  cv::Mat mMarkMask; cv::Mat mDepthMask;
  cv::Mat mDTMask;
  int mWidth; int mHeight;

  Eigen::Vector3d mC;
  Eigen::Vector3d mEa, mEb;

 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  
};


#include "perception/pointcloud_tools/tabletop_symmetry/impl/mindGapper.hpp"
