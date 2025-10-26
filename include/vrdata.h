#ifndef RIFTDATA_H
#define RIFTDATA_H

//------------------------------------------------------------------------------

#if OCULUS_RIFT
#	include "OVR.h"
#	include "timeout.h"
#endif

class CRiftData {
	public:
#if OCULUS_RIFT
		OVR::Ptr<OVR::DeviceManager>			m_pManager;
		OVR::Ptr<OVR::HMDDevice>				m_pHMD;
		OVR::Ptr<OVR::SensorDevice>			m_pSensorP;
		OVR::HMDInfo								m_hmdInfo;
		OVR::SensorFusion*						m_pSensorFusion;
		OVR::Util::Render::StereoConfig		m_stereoConfig;
		OVR::Util::Render::StereoEyeParams	m_eyes [2];
#	if 0 // manual calibration removed from Rift SDK since v0.25
		OVR::Util::MagCalibration				m_magCal;
		CTimeout			m_magCalTO;
		bool				m_bCalibrating;
#	endif
#endif

		float				m_renderScale;
		float				m_fov;
		float				m_projectionCenterOffset;
		int32_t			m_ipd;
		int32_t			m_nResolution;
		int32_t			m_bUse;
		int32_t			m_bAvailable;
		CFloatVector	m_center;

		CRiftData () : m_renderScale (1.0f), m_fov (125.0f), m_projectionCenterOffset (0.0f), m_ipd (0), m_nResolution (0), m_bUse (0), m_bAvailable (false) {}
		~CRiftData () { Destroy (); }
		bool Create (void);
		void Destroy (void);
		inline int32_t Available (void) { return m_bAvailable; }
		inline int32_t Resolution (void) { return m_nResolution; }
		int32_t GetViewMatrix (CFixMatrix& m);
		int32_t GetHeadAngles (CAngleVector* angles);
		void AutoCalibrate (void);
		inline void SetCenter (void) { GetHeadAngles (NULL); }
#if OCULUS_RIFT
		inline int32_t HResolution (void) { return (Available () &&  m_hmdInfo.HResolution) ? m_hmdInfo.HResolution : 1920; }
		inline int32_t VResolution (void) { return (Available () && m_hmdInfo.VResolution) ? m_hmdInfo.VResolution : 1200; }
#else
		inline int32_t HResolution (void) { return 1920; }
		inline int32_t VResolution (void) { return 1200; }
#endif
	};

#endif
