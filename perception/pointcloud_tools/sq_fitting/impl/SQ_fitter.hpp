/**
 * @file SQ_fitter.hpp
 * @brief Implementation
 */
#pragma once

#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/common/centroid.h>
#include <pcl/common/pca.h>
#include <pcl/common/common.h>
#include <pcl/filters/voxel_grid.h>

#include "../SQ_fitter.h"
#include "../SQ_utils.h"
#include "../SQ_structs.h"
#include "../analytic_equations.h"

#include "../levmar/levmar.h"

/**
 * @function SQ_fitter
 * @brief Constructor. Create initial pointers for cloud and its normalized version
 */
template<typename PointT>
SQ_fitter<PointT>::SQ_fitter() :
  cloud_( new pcl::PointCloud<PointT>() ) {
}

/**
 * @function ~SQ_fitter
 * @brief Destructor
 */
template<typename PointT>
SQ_fitter<PointT>::~SQ_fitter() {
}

/**
 * @function setInputCloud
 * @brief Set segmented cloud to be fitted
 */
template<typename PointT>
void SQ_fitter<PointT>::setInputCloud( const PointCloudPtr &_cloud ) {
  cloud_ = _cloud;
}

/**
 * @function fit
 * @brief Fit using Levenberg-Marquadt with box constraints
 */
template<typename PointT>
bool SQ_fitter<PointT>::fit( const int &_type,
			     const double &_smax,
			     const double &_smin,
			     const int &_N,
			     const double &_thresh ) {
    
  // 0. Store parameters
  smax_ = _smax;
  smin_ = _smin;
  N_ = _N;
  thresh_ = _thresh;

  double ds; double error_i; double error_i_1;
  double s_i; bool fitted; 
  SQ_parameters par_i, par_i_1;
  
  ds = (smax_ - smin_) / (double) (N_-1);
  
  // 1. Initialize par_in_ with bounding box values
  getBoundingBox( cloud_, 
		  par_in_.dim,
		  par_in_.trans,
		  par_in_.rot );

  // 1.1. Set e1 and e2 to middle value in range
  par_in_.e[0] = 0.5; par_in_.e[1] = 1.0;

  // Run loop
  par_i = par_in_;
  error_i = error_metric( par_i, cloud_ );
  fitted = false;


  /////////////////////////////////////////////
  //std::cout << "\t ****************************"<<std::endl;
  //std::cout << "\t ITERATION 0 (init params): " << std::endl;
  //std::cout<<"\t "; printParamsInfo(par_in_);
  //std::cout << "\t * Error metric: "<< error_i << " cloud size: "<< cloud_->points.size() << std::endl;
  ////////////////////////////////////////////


  for( int i = 0; i < N_; ++i ) {

    s_i = smax_ - (i)*ds;
    par_i_1 = par_i;
    error_i_1 = error_i;

    PointCloudPtr cloud_i( new pcl::PointCloud<PointT>() );
    downsample( cloud_,
		s_i,
		cloud_i );

    minimize( cloud_i,
	      par_i_1,
	      par_i,
	      error_i,
	      _type );

    ///////////////////////////////////////////////////////////
    //std::cout << "\t ITERATION "<<i+1<< std::endl;
    //std::cout << "\t * Voxel size: "<< s_i << ", Downsampled cloud points: "<< cloud_i->points.size() << std::endl;
    //std::cout << "\t "; printParamsInfo( par_i );
    //std::cout << "\t Error metric: "<< error_i << std::endl;
    ///////////////////////////////////////////////////////////
    
    // [CONDITION]
    double de = (error_i_1 - error_i);
    //std::cout << "\t ** Diff of errors at iter "<<i<<": "<<de << std::endl;
    if(  de < thresh_ ) {
	//std::cout << "\t DIFF OF ERRORS LESS THAN THRESH, OH HAPPY DAY! BREAK LOOP"<< std::endl;
      fitted = true;
      break;
    } 

  }
 
  par_out_ = par_i;

  return fitted;
}


/**
 * @function getBoundingBox
 * @brief Set segmented cloud to be fitted
 */
template<typename PointT>
void SQ_fitter<PointT>::getBoundingBox(const PointCloudPtr &_cloud,
				       double _dim[3],
				       double _trans[3],
				       double _rot[3],
				       bool _debug ) {

  // 1. Compute the bounding box center
  Eigen::Vector4d centroid;
  pcl::compute3DCentroid( *_cloud, centroid );
  _trans[0] = centroid(0);
  _trans[1] = centroid(1); 
  _trans[2] = centroid(2);

  // 2. Compute main axis orientations
  pcl::PCA<PointT> pca;
  pca.setInputCloud( _cloud );
  Eigen::Vector3f eigVal = pca.getEigenValues();
  Eigen::Matrix3f eigVec = pca.getEigenVectors();
  // Make sure 3 vectors are normal w.r.t. each other
  
  eigVec.col(2) = eigVec.col(0); // Z
  Eigen::Vector3f v3 = (eigVec.col(1)).cross( eigVec.col(2) );
  eigVec.col(0) = v3; 
  
  /*
  Eigen::Vector3f v3 = (eigVec.col(0)).cross( eigVec.col(1) );
  eigVec.col(2) = v3;
  */

  Eigen::Vector3f rpy = eigVec.eulerAngles(2,1,0);
 
  _rot[0] = (double)rpy(2);
  _rot[1] = (double)rpy(1);
  _rot[2] = (double)rpy(0);

  // Transform _cloud
  Eigen::Matrix4f transf = Eigen::Matrix4f::Identity();
  transf.block(0,3,3,1) << (float)centroid(0), (float)centroid(1), (float)centroid(2);
  transf.block(0,0,3,3) = eigVec;

  Eigen::Matrix4f tinv; tinv = transf.inverse();
  PointCloudPtr cloud_temp( new pcl::PointCloud<PointT>() );
  pcl::transformPointCloud( *_cloud, *cloud_temp, tinv );

  // Get maximum and minimum
  PointT minPt; PointT maxPt;
  pcl::getMinMax3D( *cloud_temp, minPt, maxPt );
  
  _dim[0] = ( maxPt.x - minPt.x ) / 2.0;
  _dim[1] = ( maxPt.y - minPt.y ) / 2.0;
  _dim[2] = ( maxPt.z - minPt.z ) / 2.0;


    ///////////////////////////////////////////////////////////
  if( _debug ) {
    std::cout << "\t [DEBUG] Bounding Box "<< std::endl;
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer( new pcl::visualization::PCLVisualizer("Debug Bounding Box") );
    viewer->addCoordinateSystem(_dim[2], 0);
    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> col(cloud_temp, 125,125,0);
    viewer->addPointCloud( cloud_temp, col, "Normalized cloud" );
    
    viewer->addCube( Eigen::Vector3f(0,0,0),
		     Eigen::Quaternionf(1,0,0,0),
		     _dim[0]*2, _dim[1]*2, _dim[2]*2, 
		     "OBB");
    
    std::cout << "* Dimensions: "<< _dim[0]<<", "<<_dim[1]<<", "<<_dim[2]<<std::endl;
    std::cout << "* Rot, trans: "<< _rot[0]<<", "<<_rot[1]<<", "<<", "<<_rot[2]<<std::endl;
    
    while( !viewer->wasStopped() ) {
      viewer->spinOnce(100);
      boost::this_thread::sleep( boost::posix_time::microseconds(100000) );
    }
  } // end if _debug
    ///////////////////////////////////////////////////////////


}

/**
 * @function downsample
 * @brief Fit using Levenberg-Marquadt with box constraints
 */
template<typename PointT>
void SQ_fitter<PointT>::downsample( const PointCloudPtr &_cloud,
				    const double &_voxelSize,
				    PointCloudPtr &_cloud_downsampled ) {
    
  // Create the filtering object
  pcl::VoxelGrid< PointT > downsampler;
  // Set input cloud
  downsampler.setInputCloud( _cloud );
  // Set size of voxel
  downsampler.setLeafSize( _voxelSize, _voxelSize, _voxelSize );
  // Downsample
  downsampler.filter( *_cloud_downsampled );
  
}


/**
 * @function minimize
 * @brief Apply bounded Levenberg-Marquadt with _in initial parameters
 * @output _out
 */
template<typename PointT>
bool SQ_fitter<PointT>::minimize( const PointCloudPtr &_cloud, 
				  const SQ_parameters &_in,
				  SQ_parameters &_out,
				  double &_error,
				  int _type ) {

    switch( _type ) {
	
    case CERES_MINIMIZER: {
	printf("Getting rid of CERES! \n");
	return false;
    } break;

    case LEVMAR_MINIMIZER: {
	return minimize_levmar( _cloud, _in, _out, _error );
    } break; 
    
    }

}
 



/**
 * @function minimize_ceres
 */
template<typename PointT>
bool SQ_fitter<PointT>::minimize_levmar( const PointCloudPtr &_cloud,
					 const SQ_parameters &_in,
					 SQ_parameters &_out,
					 double &_error ) {
    
    // Parameters initially _in:
    _out = _in; 

    // Set necessary parameters
    int n = _cloud->points.size();
    int m = 11; 
    double p[m]; // Parameters of SQ
    double y[n]; // Values we want to achieve

    double opts[LM_OPTS_SZ];
    double info[LM_INFO_SZ];
    
    opts[0] = LM_INIT_MU;
    opts[1] = 1E-15;
    opts[2] = 1E-15;
    opts[3] = 1E-20;
    opts[4] = LM_DIFF_DELTA;

    struct levmar_data data;
    data.x = new double[n];
    data.y = new double[n];
    data.z = new double[n];
    data.num = n;


    int i; int ret;
    typename pcl::PointCloud<PointT>::iterator pit;
    for( pit = _cloud->begin(), i = 0; pit != _cloud->end(); ++pit, ++i ) {
	data.x[i] = (*pit).x;
	data.y[i] = (*pit).y;
	data.z[i] = (*pit).z;
    }

    // Set minimizer value to zero (could be 1, depending of what equation you are minimizing)
    for( i = 0; i < n; ++i ) { y[i] = 0.0; }
  
    // Initialize values for parameters p
    for( i = 0; i < 3; ++i ) { p[i] = _in.dim[i]; }
    for( i = 0; i < 2; ++i ) { p[i+3] = _in.e[i]; }
    for( i = 0; i < 3; ++i ) { p[i+5] = _in.trans[i]; }
    for( i = 0; i < 3; ++i ) { p[i+8] = _in.rot[i]; }
    
    // Set limits
    double ub[m], lb[m];
    for( i = 0; i < 3; ++i ) { lb[i] = 0.01; ub[i] = 1.0; }
    for( i = 0; i < 2; ++i ) { lb[i+3] = 0.1; ub[i+3] = 1.9; }
    for( i = 0; i < 3; ++i ) { lb[i+5] = -2.0; ub[i+5] = 2.0; }
    for( i = 0; i < 3; ++i ) { lb[i+8] = -M_PI; ub[i+8] = M_PI; }
    
    ret = dlevmar_bc_der( levmar_fx,
			  levmar_jac,
			  p, y, m, n,
			  lb, ub,
			  NULL,
			  1000,
			  opts, info,
			  NULL, NULL, (void*)&data );

    // Fill _out
    for( i = 0; i < 3; ++i ) { _out.dim[i] = p[i]; }
    for( i = 0; i < 2; ++i ) { _out.e[i] = p[i+3]; }
    for( i = 0; i < 3; ++i ) { _out.trans[i] = p[i+5]; }
    for( i = 0; i < 3; ++i ) { _out.rot[i] = p[i+8]; }
    

    // Return status and error
    _error = error_metric( _out, cloud_ );
    
    // If stopped by invalid (TODO: Add other reasons)
    if( info[6] == 7 ) {
	return false;
    } else {
	return true;
    }
}


/**
 * @function error_metric
 * @brief Calculates the error 
 */
template<typename PointT>
double SQ_fitter<PointT>::error_metric( SQ_parameters _par,
					const PointCloudPtr &_cloud ) {

  double err = 0;
  typename pcl::PointCloud<PointT>::iterator it;
  for( it = _cloud->begin(); it != _cloud->end(); ++it ) {
    err += error_SQ( _par, it->x, it->y, it->z );
  }

  return err / (double) _cloud->points.size();
}

/**
 * @function printResults
 * @brief Print results of optimization (initial vs final fitted parameters)
 */
template<typename PointT>
void SQ_fitter<PointT>::printResults() {
  
  // 1. Print initial parameters information
  std::cout << "\t *******************************"<<std::endl;
  std::cout << "\t [SQ_fitter] INITIAL PARAMETERS: "<< std::endl;
  std::cout << "\t *******************************"<<std::endl;
  printParamsInfo( par_in_ );
  // 2. Print final parameters information
  std::cout << "\t *******************************"<<std::endl;
  std::cout << "\t [SQ_fitter] FINAL PARAMETERS: "<< std::endl;
  std::cout << "\t *******************************"<<std::endl;
  printParamsInfo( par_out_ );

}

/**
 * @function visualize
 * @brief Visualize pointcloud and final fit
 */
template<typename PointT>
void SQ_fitter<PointT>::visualize() {

  // 1. Create viewer
  boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer( new pcl::visualization::PCLVisualizer("SQ_fitter Viewer") );
  viewer->addCoordinateSystem(1.0, 0);
  
  // 2. Visualize input pointcloud (GREEN)
  pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> col(cloud_, 0,255,0);
  viewer->addPointCloud( cloud_, "Input cloud" );
  
  Eigen::Matrix3f rot; rot = Eigen::AngleAxisf( (float)par_in_.rot[2], Eigen::Vector3f::UnitZ() )*
			 Eigen::AngleAxisf( (float)par_in_.rot[1], Eigen::Vector3f::UnitY() )*
			 Eigen::AngleAxisf( (float)par_in_.rot[0], Eigen::Vector3f::UnitX() );
  viewer->addCube( Eigen::Vector3f( (float)par_in_.trans[0],
				    (float)par_in_.trans[1],
				    (float)par_in_.trans[2]),
		   Eigen::Quaternionf(rot),
		   par_in_.dim[0]*2, par_in_.dim[1]*2, par_in_.dim[2]*2, 
		   "OBB");
  
  // 3. Visualize output fitted pointcloud (RED)
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_out( new pcl::PointCloud<pcl::PointXYZ>() );
  cloud_out = sampleSQ_uniform( par_out_ );

  pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> col_out(cloud_out, 255,0,0);
  viewer->addPointCloud( cloud_out, col_out, "Cloud fitted"  );

  // Visualize initial guess (BLUE)
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_in( new pcl::PointCloud<pcl::PointXYZ>() );
  cloud_in = sampleSQ_naive( par_in_ );
  
  pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> col_in(cloud_in, 0,0,255);
  viewer->addPointCloud( cloud_in, col_in, "Cloud initial"  );
  
  while( !viewer->wasStopped() ) {
    viewer->spinOnce(100);
    boost::this_thread::sleep( boost::posix_time::microseconds(100000) );
  }

  
}

/**
 * @function getSampledOutput
 */
template<typename PointT>
typename SQ_fitter<PointT>::PointCloudPtr SQ_fitter<PointT>::getSampledOutput() {
    
    PointCloudPtr output( new pcl::PointCloud<PointT>() );
    output = sampleSQ_uniform( par_out_ );

    // Downsample
    PointCloudPtr cloud_out( new pcl::PointCloud<PointT>() );
    this->downsample( output,
		      0.01,
		      cloud_out );

    return cloud_out;
}