

template<typename PointT>
CrichtonView<PointT>::CrichtonView() {
  mShowSegmentation = false;
  mMirror = true;
  mWindowName = std::string("Perception Pick and Recognition"); 
  mGrabber = 0;
  mChanReady = false;
}

template<typename PointT>
void CrichtonView<PointT>::mirrorState(int _state, void* data) {
  mMirror = _state;
}

template<typename PointT>
void CrichtonView<PointT>::setGrabber() {

  this->mGrabber = new pcl::io::OpenNI2Grabber("", pcl::io::OpenNI2Grabber::OpenNI_Default_Mode, 
					 pcl::io::OpenNI2Grabber::OpenNI_Default_Mode );

  boost::function<void (const CloudConstPtr&)> f;
  f = boost::bind( &CrichtonView::grabber_callback, this, _1 );
  this->mGrabber->registerCallback(f);

}

template<typename PointT>
void CrichtonView<PointT>::setGUI() {

  cv::namedWindow( mWindowName, cv::WINDOW_AUTOSIZE );
 
  boost::shared_ptr<pcl::io::openni2::OpenNI2Device> device = mGrabber->getDevice();
  mF = device->getDepthFocalLength(); // Same as color length, by the way
  printf("FOcal length: %f \n", mF);

}

/**
 * @function grabber_callback
 */
template<typename PointT>
void CrichtonView<PointT>::grabber_callback( const CloudConstPtr& _cloud ) {
  mMutex.lock();
  double dt; clock_t ts, tf;
  // Segment the new input
  mTts.process( _cloud, mShowSegmentation );
   
  // Show it
  mRgbImg = mTts.getRgbImg();
  mPclMap = mTts.getXyzImg();
  // Draw bounding box
  this->drawBoundingBox();
  mMutex.unlock();
}


/**
 * @function startComm
 */
template<typename PointT>
void CrichtonView<PointT>::startComm( int state, void* userdata ) {

  sns_init();
  sns_start();
  ach_status_t r;
  
  r = ach_open( &mObj_param_chan, gObj_param_chan_name.c_str(), NULL );
  if( r != ACH_OK ) { printf("[ERROR] COULD NOT OPEN OBJ PARAM CHAN"); return; }
  r = ach_open( &mServer2Module_chan, gServer2Module_chan_name.c_str(), NULL );
  if( r != ACH_OK ) { printf("[ERROR] COULD NOT OPEN SERVER-2-SEE CHAN"); return; }
  r = ach_open( &mModule2Server_chan, gModule2Server_chan_name.c_str(), NULL );
  if( r != ACH_OK ) { printf("[ERROR] COULD NOT SEE-2-SERVER CHAN"); return; }
    
  printf("\t [OK] Communication stablished and ready to go \n");
  mChanReady = true;

}


/**
 * @function send
 */
template<typename PointT>
void CrichtonView<PointT>::send( int state, void* userData ) {

  mMutex.lock(); // Don't process anything while we are sending
  printf("Sending \n");
  for( int i = 0; i < 4; ++i ) { printf("**Table coeffs [%d]: %f \n", i, mTts.mTableCoeffs[i]);}

  printf("Clusters size: %d \n", mTts.getNumClusters() );
  if( mSelectedCluster < 0 || mSelectedCluster >= mTts.getNumClusters() ) {
    printf("--> ERROR: Did not select a cloud? \n");
    return;
  }
  
  // 0. Set message alloc
  sns_msg_tabletop_sq* msg = sns_msg_tabletop_sq_alloc( mTts.getNumClusters() );
  sns_msg_header_fill( &msg->header );
  msg->header.n = (uint32_t)mTts.getNumClusters();
  msg->n = (uint32_t) mTts.getNumClusters();
  msg->i = (uint32_t) mSelectedCluster;

  // Variables 
  pcl::PolygonMesh mesh;
  std::vector<SQ_parameters> ps;
  
  
  // 1. Save SQs
  for( int i = 0; i < mTts.getNumClusters(); ++i ) {  

    // Recognize 
    int xmin, ymin, xmax, ymax; int index; std::string label;
    mTts.getClusterBB( i, xmin, ymin, xmax, ymax );
    mOd.classify( cv::Mat( mRgbImg, cv::Rect(xmin,ymin, xmax-xmin, ymax-ymin) ),
		  index, label );
    // Send this info for fitting
    //index = 5; // BOTTLE BY NOW
    printf("*** Label!!: %s index: %d \n", label.c_str(), index );
    ObjectEntry entry; entry = mOd.getEntry( index );
    fit_SQ( mTts.getCluster(i), i, ps, entry );
    
    copy_SQparam_msg( ps[0], msg->u[i] );    
    std::cout << "Dim ps 0: "<< ps[0].dim[0]<<", "<< ps[0].dim[1]<<", "<< ps[0].dim[2]<<std::endl;
    //////////////////////////////
    // Debug Store mesh
    char debug_name[100];
    if( mMirror ) { sprintf( debug_name, "%s_mirrored_%d.ply", (char*)label.c_str(), i ); }
    else { sprintf( debug_name, "%s_no_mirror_%d.ply", (char*)label.c_str(), i ); }
    SQ_utils::create_SQ_mesh( ps[0], 25, debug_name, false );
    /////////////////////////

    char mesh_name[50]; sprintf( mesh_name, "%s/mesh_%d.ply", gPicturesPath.c_str(), i );
    if( ps.size() == 1 ) {
      SQ_utils::create_SQ_mesh( ps[0], 25, mesh_name, false );
    } else {
      SQ_utils::convertMeshes( ps, mesh_name );
      std::cout << "Mesh name: "<< mesh_name << std::endl;
    }
    msg->u[i].mesh_generated = true;
    strcpy( msg->u[i].mesh_filename, mesh_name );      
  }
  
  // 2. Get the table
  char tableName[50]; sprintf( tableName, "%s/tableMesh.ply", gPicturesPath.c_str() );
  create_table_mesh( mesh, tableName );

  pcl::io::savePLYFile( tableName, mesh ); 

  for( int i = 0; i < 4; ++i ) { msg->table_coeffs[i] = mTts.mTableCoeffs[i]; }
  sprintf( msg->table_meshfile, "%s", tableName );

  ach_status_t r = ach_put( &mObj_param_chan, msg, sns_msg_tabletop_sq_size(msg) );
  if( r != ACH_OK ) {
    printf("\t [BAD] Error sending message \n");
  } else {
    printf("\t [GOOD] Message sent all right \n ");
  }
  
  msg = 0;
    
  mMutex.unlock(); // Let the processing go on with its life
}


/**
 * @function create_table_mesh
 */
template<typename PointT>
void CrichtonView<PointT>::create_table_mesh( pcl::PolygonMesh &_table_mesh,
		   char _table_name_mesh[50] ) {
  
  
  pcl::PointCloud<pcl::PointNormal>::Ptr pnt( new pcl::PointCloud<pcl::PointNormal>() );
  CloudPtr table = mTts.getTable();
  for( int i = 0; i < table->points.size(); ++i ) {
    pcl::PointNormal p;
    p.x = table->points[i].x; p.y = table->points[i].y; p.z = table->points[i].z;
    p.normal_x = mTts.mTableCoeffs[0]; p.normal_y = mTts.mTableCoeffs[1]; p.normal_z = mTts.mTableCoeffs[2];	
    pnt->points.push_back(p);
  }
  pnt->height = 1; pnt->width = pnt->points.size();


  // Convex Hull
  pcl::ConvexHull<pcl::PointNormal> chull;
  pcl::PointCloud<pcl::PointNormal> points;
  std::vector<pcl::Vertices> polygons;
  
  chull.setInputCloud( pnt );
  chull.reconstruct( _table_mesh );

  pcl::io::savePLYFile( _table_name_mesh, _table_mesh ); 
}

/**
 * @function storeCloud
 */

template<typename PointT>
void CrichtonView<PointT>::storeCloud( char* _name, int _index, CloudPtr _cloud ) {
    char complete_name[100]; 
    sprintf(complete_name, "%s_%d.pcd", _name, _index );
    pcl::io::savePCDFile( complete_name, *_cloud );
}

/**
 * @function fit_SQ
 */
template<typename PointT>
void CrichtonView<PointT>::fit_SQ( CloudPtr _cluster, int _index,
				   std::vector<SQ_parameters> &_ps, 
				   ObjectEntry _oe  ) {

  CloudPtr filtered( new Cloud() );  
  CloudPtr completed( new Cloud() );  

  // Filter
  pcl::StatisticalOutlierRemoval<PointT> sor;
  sor.setInputCloud (_cluster);
  sor.setMeanK (50);
  sor.setStddevMulThresh (1.0);
  sor.filter (*filtered);

  Eigen::Isometry3d Tsymm; Eigen::Vector3d Bb;
  *completed = *filtered;

    // Debug: Store raw cloud
  char name[100]; sprintf( name, "%s_original", _oe.name.c_str() );
   storeCloud( name, _index, completed );    

  // Mirror
   clock_t ts, tf; double dt;

  if( mMirror ) {
    ts = clock();
    mindGapper<PointT> mg;
    std::vector<double> tc(4); for( int i = 0; i < 4; ++i ) { tc[i] = mTts.mTableCoeffs[i]; }
    mg.setTablePlane( tc );
    mg.setFittingParams();
    mg.setDeviceParams();
    mg.setFocalDist(mF);

    mg.reset();
    if( mg.complete( completed, true ) < 0 ) { printf("No mirror was valid \n");     sprintf( name, "%s_no_mirrored", _oe.name.c_str() ); }
    else { printf("Mirroring was effectively applied \n");      sprintf( name, "%s_mirrored", _oe.name.c_str() ); }
    tf = clock();
    dt = (tf-ts)/(double) CLOCKS_PER_SEC;
    printf("* Mirroring time: %f \n", dt );
    // Debug: Store mirrored cloud
    storeCloud( name, _index, completed );
  }
  
  // Fit superquadric
  SQ_parameters p; 
  int type = SQ_FX_ICHIM; // MULTIPLE WE USE DIFFERENT
  ts = clock();
  switch( _oe.sq_type ) {
  case REGULAR: {
    printf("Type: regular \n");
    SQ_fitter<PointT> fitter;
    fitter.setInputCloud( completed );
    fitter.fit( type, 0.03, 0.005, 5, 0.005 );
    fitter.getFinalParams( p ); _ps.push_back(p);
    break;
  }
  case TAMPERED: {
    printf("Type: tampered \n");
    SQ_fitter_t<PointT> fitter;
    fitter.setInputCloud( completed );
    fitter.fit( type, 0.03, 0.005, 5, 0.005 );
    fitter.getFinalParams( p ); _ps.push_back(p);
    break;
  }
  case BENT: {
    printf("Type: bent \n");
    SQ_fitter_b<PointT> fitter;
    fitter.setInputCloud( completed );
    fitter.fit( type, 0.03, 0.005, 5, 0.005 );
    fitter.getFinalParams( p ); _ps.push_back(p);
    break;
  }
  case MULTIPLE: {
    printf("Type: multiple \n");
    SQ_fitter_m<PointT> fitter;
    fitter.setInputCloud( completed );
    fitter.fit( SQ_FX_RADIAL, _oe.part_type, _oe.num_parts, _oe.hint_search, 0.03, 0.005, 5, 0.005 );
    fitter.getFinalParams( _ps );
    
    break;
  }

  }
  tf = clock();
  dt = (tf-ts)/(double)CLOCKS_PER_SEC;
  printf("Fitting time");
  if( mMirror ) { printf(" with mirror "); } else { printf(" with NO mirror: \n"); }
  printf(" %f \n", dt );

  if( _oe.sq_type != MULTIPLE ) {
    printParamsInfo(_ps[0]);
    
    pcl::PointCloud<pcl::PointXYZ>::Ptr sqp( new pcl::PointCloud<pcl::PointXYZ>() );
    char sqname[75]; sprintf( sqname, "%s_sq.pcd", _oe.name.c_str() ); 
    sqp = sampleSQ_uniform<pcl::PointXYZ>( _ps[0] ); 
    pcl::io::savePCDFile( sqname, *sqp );
    
    }
}

/**
 * @function onMouse
 * @brief Tells the point location
 */
template<typename PointT>
void CrichtonView<PointT>::onMouse( int event, int x, int y, int flags, void* userdata ) {
  
  if( event != cv::EVENT_LBUTTONDOWN ) {
    return;
  }
  mMutex.lock();
  cv::Point3f p; Eigen::Vector3d currentPoint;
  p = mPclMap.at<cv::Point3f>(y,x);
  currentPoint << (double)p.x, (double)p.y, (double)p.z;
  std::cout << "\t * Mouse pressed. Current point: "<< currentPoint.transpose() << std::endl;
  
  double minDist; int minIndex; double dist;

  mSelectedCluster = -1;

    for( int i = 0; i < mClustersBB.size(); ++i ) {
      int cx, cy;
      cx = (mClustersBB[i](0) + mClustersBB[i](2) )/2;
      cy = (mClustersBB[i](1) + mClustersBB[i](3) )/2;
      dist = (x-cx)*(x-cx) + (y-cy)*(y-cy);
      if( i == 0 ) { minIndex = 0; minDist = dist; }
      else { if( dist < minDist ) { minIndex = i; minDist = dist; } }
    }

    mSelectedCluster = minIndex;
    printf("\t * Selected cluster: %d \n", mSelectedCluster );
    mMutex.unlock();
}


/**
 * @function drawingBoundingBox
 * @brief BB is a bit larger than it should ( pixels)
 */
template<typename PointT>
void CrichtonView<PointT>::drawBoundingBox() {

  int xmin, ymin, xmax, ymax;

  cv::Vec3b colors; colors(0) = 255; colors(1) = 0; colors(2) = 0;
  mClustersBB.resize(0);

  for( int i = 0; i < mTts.getNumClusters(); ++i ) {
    mTts.getClusterBB( i, xmin, ymin, xmax, ymax );
    // Draw a bit bigger
    int dx = 2; int dy = 2;
    if( xmin - dx > 0 ) { xmin = xmin - dx; }
    if( xmax + dx < 640 - 1 ) { xmax = xmax + dx; }
    if( ymin -dy > 0 ) { ymin = ymin - dy; }
    if( ymax + dy < 480 - 1 ) { ymax = ymax + dy; }
    cv::rectangle( mRgbImg, cv::Point( xmin, ymin), cv::Point(xmax, ymax), colors, 2 );    
    mClustersBB.push_back( Eigen::Vector4d(xmin+1,ymin+1,xmax-1,ymax-1) );
  }

}
