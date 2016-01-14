
#include <pcl/features/integral_image_normal.h>
#include <pcl/segmentation/organized_multi_plane_segmentation.h>
#include <pcl/segmentation/organized_connected_component_segmentation.h>
#include <pcl/segmentation/euclidean_cluster_comparator.h>
#include <pcl/io/pcd_io.h>

#include <opencv2/core/core.hpp>


/**
 * @class Fast_Tabletop_Segmentation
 */
template<typename PointT>
class Fast_Tabletop_Segmentation {

  typedef pcl::PointCloud<PointT> Cloud;
  typedef typename Cloud::Ptr CloudPtr;
  typedef typename Cloud::ConstPtr CloudConstPtr;
  typedef pcl::PointCloud<pcl::Label> LabelCloud;
  typedef typename LabelCloud::Ptr LabelCloudPtr;

 public:
  Fast_Tabletop_Segmentation();
  void process( CloudConstPtr _cloud, bool _showSegmentation = true );
  cv::Mat getRgbImg() { return mRgbImg; }
  cv::Mat getXyzImg() { return mXyzImg; }
  int getNumClusters() { return mClusters.size(); }
  bool getClusterBB( int _i, int &_xmin, int &_ymin,
		     int &_xmax, int &_ymax );

private:

  void getSegmentedImg( CloudConstPtr _cloud, bool _showSegmentation );
  std::vector<CloudPtr> getClusters() { return mClusters; }
  CloudPtr getCluster( int _i ) {
    if( _i >= 0 && _i < mClusters.size() ) {return mClusters[_i]; }
    else { return NULL; }
  }

  
  CloudPtr mCloud;
  float mThreshold;
  bool mDepthDependent;
  bool mPolygonRefinement;
  int mMinPlaneInliers;
  cv::Mat mRgbImg;
  cv::Mat mXyzImg;
  
  // Operators
  pcl::IntegralImageNormalEstimation<PointT, pcl::Normal> mNe;
  pcl::OrganizedMultiPlaneSegmentation<PointT, pcl::Normal, pcl::Label> mMps;
  typename pcl::EuclideanClusterComparator<PointT, pcl::Normal, pcl::Label>::Ptr mEcc;
  
public:
  pcl::PointCloud<pcl::Normal>::Ptr mNormalCloud;
  pcl::PointIndices mPlaneIndices;
  std::vector<pcl::PointIndices> mClustersIndices;
  std::vector<CloudPtr> mClusters;
  std::vector<int> mLabels; // 0: table numbers > 0: label (-): Njo label
  
};
