dxc -spirv -T vs_6_0 -E main ./vertex.hlsl -Fo ./vertex.spv
dxc -spirv -T ps_6_0 -E main ./fragment.hlsl -Fo ./fragment.spv