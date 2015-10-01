/**
 * @file SQ_utils.h
 * @author A. Huaman Quispe <ahuaman3@gatech.edu>
 * @brief Utilities 
 */
#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <pcl/common/common.h>
#include <pcl/common/centroid.h>
#include <pcl/common/pca.h>

#include "SQ_parameters.h"

void printParamsInfo( const SQ_parameters &_par );

double dTheta_0( double K, double e,
		      double a1, double a2,
		      double t );

double dTheta( double K, double e,
		    double a1, double a2,
		    double t );

template<typename PointT>
void downsampling( const typename pcl::PointCloud<PointT>::Ptr &_cloud,
		   const double &_voxelSize,
		   typename pcl::PointCloud<PointT>::Ptr &_cloud_downsampled );


/**
 * @function DONE
 */
template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr sampleSQ_uniform( const double &_a1, 
							const double &_a2,
							const double &_a3,
							const double &_e1,
							const double &_e2,
							const int &_N=100);

/**
 * @function DONE
 */
template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr sampleSQ_uniform( double _dim[3],
							double _e[2],
							double _trans[3],
							double _rot[3] );

/**
 * @function DONE
 */
template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr sampleSQ_uniform( const SQ_parameters &_par );


/**
 * @function SOURCE CODE CPP
 */
void sampleSQ_uniform_pn( const double &_a1, 
			  const double &_a2,
			  const double &_a3,
			  const double &_e1,
			  const double &_e2,
			  const int &_N,
			  pcl::PointCloud<pcl::PointNormal>::Ptr &_pn );



template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr sampleSQ_uniform_t( const SQ_parameters &_par );

template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr sampleSQ_uniform_b( const SQ_parameters &_par );

template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr sampleSQ_uniform_b2( const SQ_parameters &_par ); // Solina


/**
 * @function DONE
 */
template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr sampleSE_uniform( const double &_a1, 
							const  double &_a2,
							const double &_e,
							const int &_N=100);

/**
 * @function DONE
 */
template<typename PointT>
void sampleSE_uniform_pcs( const double &_a1, 
			   const  double &_a2,
			   const double &_e,
			   const int &_N,
			   typename pcl::PointCloud<PointT>::Ptr &_points,
			   std::vector<double> &_ca,
			   std::vector<double> &_sa );

/**
 * @function DONE
 */
template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr sampleSQ_naive( const SQ_parameters &_par );


/////////////////////////////////////////////////////
#include "impl/SQ_utils.hpp"
