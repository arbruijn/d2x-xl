#ifndef VRDATA_H
#define VRDATA_H

//------------------------------------------------------------------------------

#ifdef USE_OPENVR
#	include <openvr/openvr.h>
#endif

class CVRData {
	public:
#ifdef USE_OPENVR
		vr::IVRSystem*							m_pVRSystem;
		vr::IVRCompositor*						m_pVRCompositor;
		vr::TrackedDevicePose_t				m_trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::HmdMatrix34_t						m_mat4DevicePose[vr::k_unMaxTrackedDeviceCount];
		char										m_strDriver[1024];
		char										m_strDisplay[1024];
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
		inline void SetCenter (void) { GetHeadAngles (NULL); }
#ifdef USE_OPENVR
		inline int32_t HResolution (void) { return (Available () && m_pVRSystem) ? m_pVRSystem->GetRecommendedRenderTargetSize(&m_nResolution, nullptr), m_nResolution : 1920; }
		inline int32_t VResolution (void) { return (Available () && m_pVRSystem) ? m_pVRSystem->GetRecommendedRenderTargetSize(nullptr, &m_nResolution), m_nResolution : 1200; }
#else
		inline int32_t HResolution (void) { return 1920; }
		inline int32_t VResolution (void) { return 1200; }
#endif
	};

#endif
