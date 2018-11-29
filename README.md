Custom UE 4.21 branch with GameWorks
==========================

Check the traditional steps on how to compile this from [UE4 README](README_UE4.md).

This repository contains following NVIDIA GameWorks technologies:

* [Blast 1.1.4](README_Blast.md)
* [Flow 1.0.1](README_Flow.md)
* [HairWorks 1.4](README_HairWorks.md)
* [HBAO+ 4](README_VXGI2_HBAOPlus.md)
* TXAA 3
* [VXGI 2.0.1](README_VXGI2_HBAOPlus.md)

###### Known issues:

* Flow: Flow's DX12 mode will probably not work properly on multi-GPU system due to the hacky port on this part
* HBAO+: SSAO and HBAO+ do not work simultaneously so make sure to disable built-in SSAO first if you want to use HBAO+

-0lento