#ifndef VRDATA_H
#define VRDATA_H

//------------------------------------------------------------------------------

#ifdef USE_OPENVR
#ifdef _MSC_VER
#	include <openvr.h>
#else
#	include <openvr/openvr.h>
#endif
#endif

struct DistortionConfig {
	float XCenterOffset;
	float YCenterOffset;
	float Scale;
	float K[4];
	float ChromaticAberration[4];
	DistortionConfig(float k0, float k1, float k2, float k3) {
		K[0] = k0;
		K[1] = k1;
		K[2] = k2;
		K[3] = k3;
	}
	DistortionConfig() {
	}
	void SetChromaticAberration(float v0, float v1, float v2, float v3) {
		ChromaticAberration[0] = v0;
		ChromaticAberration[1] = v1;
		ChromaticAberration[2] = v2;
		ChromaticAberration[3] = v3;
	}
};

class CVREyeInfo {
	public:
		DistortionConfig *pDistortion;
		CVREyeInfo() {
			pDistortion = NULL;
		}
};

class CVRData {
	public:
#ifdef USE_OPENVR
		vr::IVRSystem*							m_pVRSystem;
		vr::IVRCompositor*						m_pVRCompositor;
		vr::TrackedDevicePose_t				m_trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::HmdMatrix34_t						m_mat4DevicePose[vr::k_unMaxTrackedDeviceCount];
		char										m_strDriver[1024];
		char										m_strDisplay[1024];
		CVREyeInfo			m_eyes[2];
#endif

		float				m_renderScale;
		float				m_fov;
		float				m_projectionCenterOffset;
		int32_t			m_ipd;
		uint32_t			m_nResolution;
		int32_t			m_bUse;
		int32_t			m_bAvailable;
		CFloatVector	m_center;

		CVRData () : m_renderScale (1.0f), m_fov (125.0f), m_projectionCenterOffset (0.0f), m_ipd (0), m_nResolution (0), m_bUse (0), m_bAvailable (false) {}
		~CVRData () { Destroy (); }
		bool Create (void);
		void Destroy (void);
		inline int32_t Available (void) { return m_bAvailable; }
		inline int32_t Resolution (void) { return m_nResolution; }
		int32_t GetViewMatrix (CFixMatrix& m);
		int32_t GetHeadAngles (CAngleVector* angles);
		void AutoCalibrate (void);
		void Submit (int32_t nEye, GLuint hTexture);
		inline void SetCenter (void) { GetHeadAngles (NULL); }
#ifdef USE_OPENVR
		inline int32_t HResolution (void) {
			uint32_t w, h;
			return (Available () && m_pVRSystem) ? m_pVRSystem->GetRecommendedRenderTargetSize(&w, &h), w : 1920;
		}
		inline int32_t VResolution (void) {
			uint32_t w, h;
			return (Available () && m_pVRSystem) ? m_pVRSystem->GetRecommendedRenderTargetSize(&w, &h), h : 1080;
		}
#else
		inline int32_t HResolution (void) { return 1920; }
		inline int32_t VResolution (void) { return 1200; }
#endif
	};

#endif
