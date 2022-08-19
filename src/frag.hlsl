struct VertexOut {
    float4 pos: SV_Position;
    float2 uv: TEXCOORD;
};

SamplerState sampState {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
    AddressW = Wrap;
};

Texture2D tex;

float ltos(float c) {
    if ( c <= .0031308f ) {
        c *= 12.92f;
    } else {
        float t = pow(c, 0.416667f);
        c = 1.055f * t - .055f;
    }
    return c;
}

float stol(float c) {
    if ( c <= .04045 ) {
        c /= 12.92f;
    } else {
        c = pow((c + .055f) / 1.055f, 2.4f);
    }
    return c;
}

float3 ltos(float3 c) {
    return float3(ltos(c.r), ltos(c.g), ltos(c.b));
}

float4 ltos(float4 c) {
    return float4(ltos(c.rgb), c.a);
}

float4 fragmentMain(VertexOut input) : SV_Target {
    float4 color = tex.Sample(sampState, input.uv);
    color.rgb = color.rgb * color.a;
    return color;
}
