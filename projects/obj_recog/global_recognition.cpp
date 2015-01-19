/**
 * @file global_recognition.cpp
 */
#include <pcl/io/pcd_io.h>
#include <tabletop_segmentation/tabletop_segmentation.h>
#include "GlobalRecognizer.h"

GlobalRecognizer mGr;
std::string objects_input("/home/ana/Research/golems/projects/obj_recog/models_input.json");
pcl::PointCloud<PointType>::Ptr scene( new pcl::PointCloud<PointType>() );

int main( int argc, char* argv[] ) {

  if( argc != 2 ) {
    printf("Syntax: %s scene.pcd", argv[0] );
    return -1;
  }

  // Load scene
  pcl::io::loadPCDFile( argv[1], *scene );

  // Load models
  mGr.load( objects_input );
  mGr.prepareModels();
  mGr.printInfo();
  printf("\t ** Prepare models \n");


  // View keypoints
  //mLr.viewModels();

  // Load scene & set descriptors
  mGr.setScene( scene );

  // Matching
  mGr.matching();

  // Alignment
  mGr.correspondenceGrouping();

  // Absolute transform
  
  mGr.viewRecognition();

  return 0;
}
