static const char* shader_model_vs = R"(
#if STEREOMODE==2
#extension GL_ARB_shader_viewport_layer_array : enable
#endif

layout (std140, row_major, binding=0) uniform FrameState
{
    float       mTime;
    int         mFrame;
}frame;

layout (std140, row_major, binding=3) uniform LayersState
{
	mat4x4 mLayerToViewer;
	float  mLayerToViewerScale;
	float  mOpacity;
	float  mkUnused;
	float  mDrawInTime;
	vec4   mAnimParams; // draw-in and other effect parameters
	vec4   mKeepAlive[2];        // note, can't pack in an array of float[8] due to granularity of GLSL array types
    uint   mID;
}layer;


layout (std140, row_major, binding=4) uniform DisplayState
{
    struct
    {
        //mat4x4      mMatrix_Prj;
        //mat4x4      mMatrix_Cam;
        mat4x4      mMatrix_CamPrj;
        //mat4x4      mInvMatrix_Prj;
        //mat4x4      mInvMatrix_Cam;
        //mat4x4      mInvMatrix_CamPrj;
    }mEye[2];
    vec2        mResolution;
}display;

layout (std140, row_major, binding=5) uniform PassState
{
	int mID;
	int kk1;
	int kk2;
	int kk3;
}pass;

layout (location=0) in vec3 inVertex;
layout (location=1) in vec3 inColor;
layout (location=2) in vec3 inNormal;

out V2FData
{
    vec3 WPos;
    vec3 OPos;
    vec3 Col;
}vo;


void main()
{
    vec3 wpos = (layer.mLayerToViewer * vec4(inVertex,1.0)).xyz;
    vo.WPos = wpos;
    vo.OPos = inVertex;
    vo.Col = inColor;

	#if STEREOMODE==0
    #define iid 0
	#endif
	#if STEREOMODE==1
    #define iid pass.mID
	#endif
	#if STEREOMODE==2
	#define iid gl_InstanceID
    gl_ViewportIndex = gl_InstanceID;
	#endif

    gl_Position = display.mEye[iid].mMatrix_CamPrj * vec4(wpos,1.0);

}
)";
