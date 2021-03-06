#!/bin/bash


DIR="/home/ana/Research/commonData/objects/ch3_9/meshes"
PLY2WRL_SCRIPT="/home/ana/Research/golems/scripts/ply2wrl_mm.py"

cd $DIR;
# Use script to wrl
for PLY_FILE in `find . -type f -name "*.ply"`; do
  blender --background --python ${PLY2WRL_SCRIPT} -- ${PLY_FILE}
done
