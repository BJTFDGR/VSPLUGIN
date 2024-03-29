//////////////////////////////////////////
// This file contains a simple invert
// filter that's commented to show
// the basics of the filter api.
// This file may make more sense when
// read from the bottom and up.

#include <stdlib.h>
#include "VapourSynth.h"
#include "VSHelper.h"

typedef struct {
	VSNodeRef *node;
	const VSVideoInfo *vi;
	int enabled;
	double  key;
} InvertData;

// This function is called immediately after vsapi->createFilter(). This is the only place where the video
// properties may be set. In this case we simply use the same as the input clip. You may pass an array
// of VSVideoInfo if the filter has more than one output, like rgb+alpha as two separate clips.
static void VS_CC invertInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
	InvertData *d = (InvertData *)* instanceData;
	vsapi->setVideoInfo(d->vi, 1, node);
}

// This is the main function that gets called when a frame should be produced. It will, in most cases, get
// called several times to produce one frame. This state is being kept track of by the value of
// activationReason. The first call to produce a certain frame n is always arInitial. In this state
// you should request all the input frames you need. Always do it in ascending order to play nice with the
// upstream filters.
// Once all frames are ready, the filter will be called with arAllFramesReady. It is now time to
// do the actual processing.
static const VSFrameRef *VS_CC invertGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) {
	InvertData *d = (InvertData *)* instanceData;
	int64_t key = d->key;
	if (activationReason == arInitial) {
		// Request the source frame on the first call
		vsapi->requestFrameFilter(n, d->node, frameCtx);
	}

	else if (activationReason == arAllFramesReady) {
		const VSFrameRef *src = vsapi->getFrameFilter(n, d->node, frameCtx);
		// The reason we query this on a per frame basis is because we want our filter
		// to accept clips with varying dimensions. If we reject such content using d->vi
		// would be better.
		const VSFormat *fi = d->vi->format;
		int height = vsapi->getFrameHeight(src, 0);
		int width = vsapi->getFrameWidth(src, 0);


		// When creating a new frame for output it is VERY EXTREMELY SUPER IMPORTANT to
		// supply the "dominant" source frame to copy properties from. Frame props
		// are an essential part of the filter chain and you should NEVER break it.
		VSFrameRef *dst = vsapi->newVideoFrame(fi, width, height, src, core);

		// It's processing loop time!
		// Loop over all the planes
		int plane;

	/*	
		uint8_t *r_dstp = new uint8_t[500000];
		uint8_t *g_dstp = new uint8_t[500000];
		uint8_t *b_dstp = new uint8_t[500000];
		uint8_t *a_dstp = new uint8_t[500000];
		/*
		for (plane = 0; plane < fi->numPlanes; plane++) {
			const uint8_t *srcp = vsapi->getReadPtr(src, plane);//Returns a read-only pointer to a plane of a frame.
			int src_stride = vsapi->getStride(src, plane);//Returns the distance in bytes between two consecutive lines of a plane of a frame. The stride is always positive.
			uint8_t *dstp = vsapi->getWritePtr(dst, plane);//Returns a read/write pointer to a plane of a frame.
			int dst_stride = vsapi->getStride(dst, plane);
			// note that if a frame has the same dimensions and format, 
			//the stride is guaranteed to be the same. int dst_stride = src_stride would be fine too in this filter.
			// Since planes may be subsampled you have to query the height of them individually
			int h = vsapi->getFrameHeight(src, plane);
			int y;
			int w = vsapi->getFrameWidth(src, plane);
			int x;

			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++)
					//dstp[x] = ~srcp[x];
					if (plane)
						dstp[x] = 127;
					else
						dstp[x] = srcp[x];

				dstp += dst_stride;
				srcp += src_stride;
			}
		}


		
		for (plane = 0; plane < fi->numPlanes; plane++) {
			const uint8_t *srcp = vsapi->getReadPtr(src, plane);//Returns a read-only pointer to a plane of a frame.
			int src_stride = vsapi->getStride(src, plane);//Returns the distance in bytes between two consecutive lines of a plane of a frame. The stride is always positive.

			// note that if a frame has the same dimensions and format, 
			//the stride is guaranteed to be the same. int dst_stride = src_stride would be fine too in this filter.
			// Since planes may be subsampled you have to query the height of them individually
			int h = vsapi->getFrameHeight(src, plane);
			int y;
			int w = vsapi->getFrameWidth(src, plane);
			int x;

			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					switch (plane) {
					case 0:r_dstp[x] = srcp[x];
						break;
					case 1:g_dstp[x] = srcp[x];
						break;
					case 2:b_dstp[x] = srcp[x];
						break;
					default:a_dstp[x] = srcp[x];
					}
				}
				switch (plane) {
				case 0:r_dstp += src_stride;
					break;
				case 1:g_dstp += src_stride;
					break;
				case 2:b_dstp += src_stride;
					break;
				default:a_dstp += src_stride;
				}
				srcp += src_stride;
			}
		}

		for (plane = 0; plane < fi->numPlanes; plane++) {
			const uint8_t *srcp = vsapi->getReadPtr(src, plane);//Returns a read-only pointer to a plane of a frame.
			int src_stride = vsapi->getStride(src, plane);//Returns the distance in bytes between two consecutive lines of a plane of a frame. The stride is always positive.
			uint8_t *dstp = vsapi->getWritePtr(dst, plane);//Returns a read/write pointer to a plane of a frame.
			int dst_stride = vsapi->getStride(dst, plane);
			// note that if a frame has the same dimensions and format, 
			//the stride is guaranteed to be the same. int dst_stride = src_stride would be fine too in this filter.
			// Since planes may be subsampled you have to query the height of them individually
			int h = vsapi->getFrameHeight(src, plane);
			int y;
			int w = vsapi->getFrameWidth(src, plane);
			int x;

			int dr = 0; int r = 0;
			int dg = 0; int g = 0;
			int db = 0; int b = 0;

			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					g = g_dstp[x];
					r = r_dstp[x];
					b = b_dstp[x];

					dr = 0.393*r + 0.769*g + 0.189*b;
					dg = 0.349*r + 0.686*g + 0.168*b;
					db = 0.272*r + 0.534*g + 0.131*b;

					switch (plane) {
					case 0:r_dstp[x] = r_dstp[x];
						break;
					case 1:g_dstp[x] = 127;
						break;
					case 2:b_dstp[x] = 127;
						break;
					default:a_dstp[x] = a_dstp[x];
					}
				}
				dstp += dst_stride;
				r_dstp += src_stride;
				g_dstp += src_stride;
				b_dstp += src_stride;
				a_dstp += src_stride;
			}
		}
		delete a_dstp;
		delete g_dstp;
		delete b_dstp;
		delete r_dstp;
*/

			
//start			
			const uint8_t *srcp_r = vsapi->getReadPtr(src, 0);
			const uint8_t *srcp_g = vsapi->getReadPtr(src, 1);
			const uint8_t *srcp_b = vsapi->getReadPtr(src, 2);

			uint8_t *dstp_r = vsapi->getWritePtr(dst, 0);
			uint8_t *dstp_g = vsapi->getWritePtr(dst, 1);
			uint8_t *dstp_b = vsapi->getWritePtr(dst, 2);

			int src_stride_r = vsapi->getStride(src, 0);
			int src_stride_g = vsapi->getStride(src, 1);
			int src_stride_b = vsapi->getStride(src, 2);


			int dst_stride_r = vsapi->getStride(dst, 0);
			int dst_stride_g = vsapi->getStride(dst, 1);
			int dst_stride_b = vsapi->getStride(dst, 2);

			int h = vsapi->getFrameHeight(src, 0);
			int y;
			int w = vsapi->getFrameWidth(src, 0);
			int x;
			int i = 0;
			int bgr[3];
			int yIdx, uIdx, vIdx, idx;

			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					bgr[0] = (int)(srcp_r[x] + 1.370705 * (srcp_b[x/2] - 128)); // r分量                                 
					bgr[1] = (int)(srcp_r[x] - 0.698001 * (srcp_g[x/2] - 128) - 0.703125 * (srcp_b[x/2] - 128));// g分量               
					bgr[2] = (int)(srcp_r[x] + 1.732446 * (srcp_g[x/2] - 128)); // b分量 

					int avg = (bgr[0] + bgr[1] + bgr[2]) / 3;
					//求平均值的去色滤镜
					switch (key) {
					case 0:
						bgr[0] = avg;
						bgr[1] = avg;
						bgr[2] = avg;
						break;
					case 1:
						bgr[0] = (0.393 * bgr[0] + 0.769 * bgr[1] + 0.189 * bgr[2]);
						bgr[1] = (0.349 * bgr[0] + 0.686 * bgr[1] + 0.168 * bgr[2]);
						bgr[2] = (0.272 * bgr[0] + 0.534 * bgr[2] + 0.131 * bgr[2]);
						break;
					case 2:
						bgr[0] = abs(bgr[1] - bgr[2] + bgr[1] + bgr[0]) *bgr[0] / 256;
						bgr[1] = abs(bgr[2] - bgr[1] + bgr[2] + bgr[0]) *bgr[0] / 256;
						bgr[2] = abs(bgr[2] - bgr[1] + bgr[2] + bgr[0]) *bgr[1] / 256;
						break;
					case 3:
						bgr[0] = 256 - bgr[0];
						bgr[1] = 256 - bgr[1];
						bgr[2] = 256 - bgr[2];
						break;
					case 4:
						bgr[0] = bgr[0];
						bgr[1] = 0;
						bgr[2] = 0;
						break;
					default:
						bgr[0] = bgr[0];
						bgr[1] = bgr[1];
						bgr[2] = bgr[2];
						
					}
					/*
					if (key == 0) {
						bgr[0] = avg;
						bgr[1] = avg;
						bgr[2] = avg;
					}
					//怀旧滤镜
					if (key == 1) {
						bgr[0] = (0.393 * bgr[0] + 0.769 * bgr[1] + 0.189 * bgr[2]);
						bgr[1] = (0.349 * bgr[0] + 0.686 * bgr[1] + 0.168 * bgr[2]);
						bgr[2] = (0.272 * bgr[0] + 0.534 * bgr[2] + 0.131 * bgr[2]);
					}
						//连环画滤镜
					if (key == 2) {
						bgr[0] = abs(bgr[1] - bgr[2] + bgr[1] + bgr[0]) *bgr[0] / 256;
						bgr[1] = abs(bgr[2] - bgr[1] + bgr[2] + bgr[0]) *bgr[0] / 256;
						bgr[2] = abs(bgr[2] - bgr[1] + bgr[2] + bgr[0]) *bgr[1] / 256;
					}
					if (key == 3) {
						bgr[0] = 256 - bgr[0];
						bgr[1] = 256 - bgr[1];
						bgr[2] = 256 - bgr[2];
					}
					if (key == 4) {

					}
					*/
					dstp_r[x] = (0.257 * bgr[0]) + (0.504 * bgr[1]) + (0.098 * bgr[2]) + 16;
					dstp_b[x/2] = (0.439 * bgr[0]) - (0.368 * bgr[1]) - (0.071 * bgr[2]) + 128;
					dstp_g[x/2] = -(0.148 * bgr[0]) - (0.291 * bgr[1]) + (0.439 * bgr[2]) + 128;


				}
				dstp_r += dst_stride_r;
				srcp_r += src_stride_r;
				
				if (y%2==1) {
					dstp_g += dst_stride_g;
					srcp_g += src_stride_g;
					srcp_b += src_stride_b;
					dstp_b += dst_stride_b;
				}



			}

//finish 
/*
			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					dstp_r[x] = srcp_r[x];
				}
				dstp_r += dst_stride_r;
				srcp_r += src_stride_r;
			}
*/

					//dstp[x] = ~srcp[x];
/*
MATLAB
clear;
M=[0.2126 0.7152 0.0722 ;
-0.11457 -0.38543 0.5 ;
0.5 -0.45415 -0.04585];
newM=[0.393 0.769 0.189;
0.349 0.686 0.168;
0.272 0.534 0.131];%怀旧
FM=[1 0 1.5748 -128*1.5748;
1 -0.18732 -0.46812 0.18732*128+0.46812*128;
1 1.8556 0 -1.8556*128]
H=M*newM*FM
%FH= +128+128

%M*newM*FM*YUV1
*/
/*
					dstp_r[x] = 1.2153*srcp_r[x] + 0.1853*srcp_g[x / 2] + 0.2313*srcp_b[x / 2] - 53.3306;
					dstp_g[x / 2] = 128 + 6.5684 - 0.1500*srcp_r[x] - 0.0228*srcp_g[x / 2] + 0.0285*srcp_b[x / 2];
					dstp_b[x / 2] = 128 - 3.9768 + 0.0862*srcp_r[x] + 0.0136*srcp_g[x / 2] + 0.0175*srcp_b[x / 2];

/*
					dstp_r[x] = srcp_r[x] ;
					dstp_g[x] = 0.6*srcp_g[x/2] + 0.4*srcp_b[x/2];
					dstp_b[x/2] = 0.5*srcp_g[x/2] + 0.5*srcp_b[x/2];

					dstp_r[x] = srcp_r[x];
*/	/*				
				}
				dstp_r += dst_stride_r;
				srcp_r += src_stride_r;

				if (i % 2 == 1) {
					dstp_g += dst_stride_g;
					dstp_b += dst_stride_b;
					srcp_g += src_stride_g;
					srcp_b += src_stride_b;
					i=(i+1)%2;
				}

				
			}
			*//*
			int h1 = vsapi->getFrameHeight(src, 1);
			int w1 = vsapi->getFrameWidth(src, 1);

			for (y = 0; y < h1; y++) {
				for (x = 0; x < w1; x++) {

					//dstp[x] = ~srcp[x];
					/*
					dstp_r[x] = 0.393*srcp_r[x] + 0.769*srcp_g[x/4] + 0.189*srcp_b[x/4];
					dstp_g[x/4] = 0.349*srcp_r[x] + 0.686*srcp_g[x/4] + 0.168*srcp_b[x/4];
					dstp_b[x/4] = 0.272*srcp_r[x] + 0.534*srcp_g[x/4] + 0.131*srcp_b[x/4];

					
					dstp_g[x] = 0.6*srcp_g[x/4] + 0.4*srcp_b[x/4];
					dstp_b[x] = 0.5*srcp_g[x/4] + 0.5*srcp_b[x/4];*/
					//dstp_r[x] = srcp_r[x];如果是一半的话 结果是四分之一就是画中画
/*					if (w1 == w / 2 && h1 == h / 2) {
						dstp_g[x] = 0.6*srcp_g[x] + 0.4*srcp_b[x];
						dstp_b[x] = 0.5*srcp_g[x] + 0.5*srcp_b[x];
					}
					else {
						dstp_g[x] = 127;
						dstp_b[x] = 127;
					}

				}

				dstp_g += dst_stride_g;
				dstp_b += dst_stride_b;

				
				srcp_g += src_stride_g;
				srcp_b += src_stride_b;

			}
*/	
			

		// Release the source frame
		vsapi->freeFrame(src);

		// A reference is consumed when it is returned, so saving the dst reference somewhere
		// and reusing it is not allowed.
		return dst;
	}

	return 0;
}



// Free all allocated data on filter destruction
static void VS_CC invertFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
	InvertData *d = (InvertData *)instanceData;
	vsapi->freeNode(d->node);
	free(d);
}

// This function is responsible for validating arguments and creating a new filter
static void VS_CC invertCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
	InvertData d;
	InvertData *data;
	int err;

	// Get a clip reference from the input arguments. This must be freed later.
	d.node = vsapi->propGetNode(in, "clip", 0, 0);
	d.vi = vsapi->getVideoInfo(d.node);

	// In this first version we only want to handle 8bit integer formats. Note that
	// vi->format can be 0 if the input clip can change format midstream.
	if (!isConstantFormat(d.vi) || d.vi->format->sampleType != stInteger || d.vi->format->bitsPerSample != 8) {
		vsapi->setError(out, "Invert: only constant format 8bit integer input supported");
		vsapi->freeNode(d.node);
		return;
	}

	// If a property read fails for some reason (index out of bounds/wrong type)
	// then err will have flags set to indicate why and 0 will be returned. This
	// can be very useful to know when having optional arguments. Since we have
	// strict checking because of what we wrote in the argument string, the only
	// reason this could fail is when the value wasn't set by the user.
	// And when it's not set we want it to default to enabled.
	d.key = vsapi->propGetFloat(in, "key", 0, 0);
	d.enabled = !!vsapi->propGetInt(in, "enable", 0, &err);
	if (err)
		d.enabled = 1;

	// Let's pretend the only allowed values are 1 or 0...
	if (d.enabled < 0 || d.enabled > 1) {
		vsapi->setError(out, "Invert: enabled must be 0 or 1");
		vsapi->freeNode(d.node);
		return;
	}



	// I usually keep the filter data struct on the stack and don't allocate it
	// until all the input validation is done.
	data = (InvertData*)malloc(sizeof(d));
	*data = d;

	// Creates a new filter and returns a reference to it. Always pass on the in and out
	// arguments or unexpected things may happen. The name should be something that's
	// easy to connect to the filter, like its function name.
	// The three function pointers handle initialization, frame processing and filter destruction.
	// The filtermode is very important to get right as it controls how threading of the filter
	// is handled. In general you should only use fmParallel whenever possible. This is if you
	// need to modify no shared data at all when the filter is running.
	// For more complicated filters, fmParallelRequests is usually easier to achieve as it can
	// be prefetched in parallel but the actual processing is serialized.
	// The others can be considered special cases where fmSerial is useful to source filters and
	// fmUnordered is useful when a filter's state may change even when deciding which frames to
	// prefetch (such as a cache filter).
	// If your filter is really fast (such as a filter that only resorts frames) you should set the
	// nfNoCache flag to make the caching work smoother.
	vsapi->createFilter(in, out, "Invert", invertInit, invertGetFrame, invertFree, fmParallel, 0, data, core);
}

//////////////////////////////////////////
// Init

// This is the entry point that is called when a plugin is loaded. You are only supposed
// to call the two provided functions here.
// configFunc sets the id, namespace, and long name of the plugin (the last 3 arguments
// never need to be changed for a normal plugin).
//
// id: Needs to be a "reverse" url and unique among all plugins.
//   It is inspired by how android packages identify themselves.
//   If you don't own a domain then make one up that's related
//   to the plugin name.
//
// namespace: Should only use [a-z_] and not be too long.
//
// full name: Any name that describes the plugin nicely.
//
// registerFunc is called once for each function you want to register. Function names
// should be PascalCase. The argument string has this format:
// name:type; or name:type:flag1:flag2....;
// All argument name should be lowercase and only use [a-z_].
// The valid types are int,float,data,clip,frame,func. [] can be appended to allow arrays
// of type to be passed (numbers:int[])
// The available flags are opt, to make an argument optional, empty, which controls whether
// or not empty arrays are accepted

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin) {
	configFunc("com.example.invert", "invert", "VapourSynth Invert Example", VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("Filter", "clip:clip;enabled:int;key:float", invertCreate, 0, plugin);
}
