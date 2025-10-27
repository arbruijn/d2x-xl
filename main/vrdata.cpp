#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"

// ----------------------------------------------------------------------------

bool CVRData::Create (void)
{
#ifdef USE_OPENVR
gameData.renderData.vr.m_bAvailable = 0;
if (gameOpts->render.bUseVR) {
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pVRSystem = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None) {
		m_pVRSystem = nullptr;
		PrintLog(0, "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		return false;
	}

	m_pVRCompositor = vr::VRCompositor();
	if (!m_pVRCompositor) {
		PrintLog(0, "Compositor initialization failed.");
		vr::VR_Shutdown();
		m_pVRSystem = nullptr;
		return false;
	}

	m_pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, m_strDriver, sizeof(m_strDriver));
	m_pVRSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String, m_strDisplay, sizeof(m_strDisplay));

	uint32_t nWidth, nHeight;
	m_pVRSystem->GetRecommendedRenderTargetSize(&nWidth, &nHeight);
	m_nResolution = nWidth > 1280;

	m_bAvailable = 2;  // Fully available
	PrintLog(0, "VR initialized: %s %s", m_strDriver, m_strDisplay);
}
#endif
return m_bAvailable != 0;
}

// ----------------------------------------------------------------------------

int32_t CVRData::GetViewMatrix (CFixMatrix& mOrient)
{
#ifdef USE_OPENVR
if (Available () < 2)
	return 0;
m_pVRCompositor->WaitGetPoses(m_trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
if (m_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
	vr::HmdMatrix34_t mat = m_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
	for (int32_t i = 0; i < 3; i++)
		for (int32_t j = 0; j < 3; j++)
			mOrient.m.vec[i * 3 + j] = F2X(mat.m[i][j]);
}
#endif
return 1;
}

// ----------------------------------------------------------------------------

#ifdef USE_OPENVR
static inline float AddDeadzone (float v)
{
float deadzone = float (gameOpts->input.vr.nDeadzone) * 0.5f;

if ((deadzone <= 0.0f) || (deadzone >= 1.0f))
	return v;

	float h = 1.0f / (1.0f - deadzone) - 1.0f;

if (v < -deadzone)
	return (v + deadzone) * (1.0f + h * fabs (v));
if (v > deadzone)
	return (v - deadzone) * (1.0f + h * fabs (v));
return 0.0f;
}
#endif

int32_t CVRData::GetHeadAngles (CAngleVector* angles)
{
#ifdef USE_OPENVR
if (Available () < 2)
	return 0;
m_pVRCompositor->WaitGetPoses(m_trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
if (m_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
	vr::HmdMatrix34_t mat = m_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
	// Extract Euler angles from the matrix (simplified; you may need a proper conversion)
	float yaw = atan2(mat.m[0][2], mat.m[2][2]);
	float pitch = asin(-mat.m[1][2]);
	float roll = atan2(mat.m[1][0], mat.m[1][1]);
	if (!angles)
		m_center.Set(pitch, roll, yaw);
	else {
		pitch -= m_center.v.coord.x;
		roll -= m_center.v.coord.y;
		yaw -= m_center.v.coord.z;
		angles->Set(-F2X(AddDeadzone(pitch) * 0.5f), F2X(AddDeadzone(roll) * 0.5f), -F2X(AddDeadzone(yaw) * 0.5f));
	}
}
#endif
return 1;
}

// ----------------------------------------------------------------------------

void CVRData::AutoCalibrate (void)
{
#if 0  // OpenVR handles calibration internally; no manual calibration needed
#endif
}

// ----------------------------------------------------------------------------
void CVRData::Submit (int32_t nEye, GLuint hTexture)
{
#ifdef USE_OPENVR
if (m_pVRSystem) {
	vr::Texture_t vrTexture = {(void*)(uintptr_t)hTexture, vr::TextureType_OpenGL, vr::ColorSpace_Gamma};
	vr::VRCompositor ()->Submit (nEye ? vr::Eye_Right : vr::Eye_Left, &vrTexture);
}
#endif
}


// ----------------------------------------------------------------------------

void CVRData::Destroy (void)
{
#ifdef USE_OPENVR
if (m_pVRSystem) {
	vr::VR_Shutdown();
	m_pVRSystem = nullptr;
	m_pVRCompositor = nullptr;
}
#endif
}

