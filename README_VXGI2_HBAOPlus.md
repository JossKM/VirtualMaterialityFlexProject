Unreal Engine with VXGI and HBAO+
=================================

Introduction
------------

VXGI stands for [Voxel Global Illumination](http://www.geforce.com/hardware/technology/vxgi), and it's an advanced rendering technique for real-time indirect illumination.

Global illumination (GI) is a way of computing lighting in the scene that includes indirect illumination, i.e. simulating objects that are lit by other objects as well as ideal light sources. Adding GI to the scene greatly improves the realism of the rendered images. Modern real-time rendering engines simulate indirect illumination using different approaches, which include precomputed light maps (offline GI), local light sources placed by artists, and simple ambient light.

HBAO+ stands for [Horizon-Based Ambient Occlusion Plus](http://www.geforce.com/hardware/technology/hbao-plus), and it's a fast and relatively stable screen-space ambient occlusion solution.

This HBAO+ integration is modified to target SSAO rendertarget instead of just overwriting the scene color with HBAO+. This will allow use of HBAO+ on Unreals forward renderer and it also makes AO take lighting into account just like built-in SSAO does. Additionally this lets you use built-in Ambient Occlusion buffer view to visualize AO.

Unreals SSAO overwrites HBAO+ on this config so if you use HBAO+, make sure built-in SSAO is disabled or you will not see HBAO+.