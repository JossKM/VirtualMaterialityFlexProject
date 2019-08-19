Custom UE 4.21.2 branch with GameWorks
==========================

Check the traditional steps on how to compile this from [UE4 README](README_UE4.md).

This repository contains following NVIDIA GameWorks technologies:

* [Blast 1.1.4](README_Blast.md)
* [FleX 1.2](README_FleX.md)
* [Flow 1.0.1](README_Flow.md)
* [HairWorks 1.4](README_HairWorks.md)
* [HBAO+ 4](README_VXGI2_HBAOPlus.md)
* TXAA 3
* [VXGI 2.0.1](README_VXGI2_HBAOPlus.md)

###### Known issues:

* FleX: Had to disable Uniform Buffer safety checks so these buffers are probably not properly setup for FleX but it still appears to work
* Flow: Flow's DX12 mode will probably not work properly on multi-GPU system due to the hacky port on this part
* HBAO+: SSAO and HBAO+ do not work simultaneously so make sure to disable built-in SSAO first if you want to use HBAO+

###### Credits/Thanks: 

* Nvidia - Original UE4 GameWorks integrations (used as basis for this merged branch)
* kostenickj - HairWorks fix for 4.20 (used as basis for this branches HW)
* trashbyte, karma4jake - FleX fixes for 4.21 (used some of them here)

-0lento