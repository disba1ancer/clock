struct VertexIn {
    float4 posUV: POSITION;
    uint index: BLENDINDICES;
};

struct VertexOut {
    float4 pos: SV_Position;
    float2 uv: TEXCOORD;
};

cbuffer Hands {
    float4 hands;
};

VertexOut vertexMain(VertexIn vert, unsigned int vidx: SV_VertexID) {
    VertexOut output;
    float a = hands[vert.index];
    float2x2 rot = {
         cos(a), sin(a),
        -sin(a), cos(a)
    };
    output.pos = float4(mul(rot, vert.posUV.xy), 0.f, 1.f);
    output.uv = vert.posUV.zw;
    return output;
}
