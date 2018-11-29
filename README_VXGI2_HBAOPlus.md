Unreal Engine with VXGI and HBAO+
=================================

Introduction
------------

VXGI stands for [Voxel Global Illumination](http://www.geforce.com/hardware/technology/vxgi), and it's an advanced rendering technique for real-time indirect illumination.

Global illumination (GI) is a way of computing lighting in the scene that includes indirect illumination, i.e. simulating objects that are lit by other objects as well as ideal light sources. Adding GI to the scene greatly improves the realism of the rendered images. Modern real-time rendering engines simulate indirect illumination using different approaches, which include precomputed light maps (offline GI), local light sources placed by artists, and simple ambient light.

HBAO+ stands for [Horizon-Based Ambient Occlusion Plus](http://www.geforce.com/hardware/technology/hbao-plus), and it's a fast and relatively stable screen-space ambient occlusion solution.