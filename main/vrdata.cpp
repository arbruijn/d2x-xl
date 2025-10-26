#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "descent.h"

// ----------------------------------------------------------------------------

bool CVRData::Create (void)
{
#if OCULUS_RIFT
gameData.renderData.vr.m_bAvailable = 0;
if (gameOpts->render.bUseVR) {
	OVR::System::Init (OVR::Log::ConfigureDefaultLog (OVR::LogMask_All));
	m_pManager = *OVR::DeviceManager::Create();
	if (m_pManager) {
		//m_pManager->SetMessageHandler(this);

		// Release Sensor/HMD in case this is a retry.
		m_pSensorP.Clear ();
		m_pHMD.Clear ();
		// RenderParams.MonitorName.Clear();
		m_pHMD = *m_pManager->EnumerateDevices<OVR::HMDDevice> ().CreateDevice ();
		if (m_pHMD) {
			m_pSensorP = *m_pHMD->GetSensor ();

			// This will initialize m_hmdInfo with information about configured IPD,
			// screen size and other variables needed for correct projection.
			// We pass HMD DisplayDeviceName into the renderer to select the
			// correct monitor in full-screen mode.
			if (m_pHMD->GetDeviceInfo (&m_hmdInfo))	{            
				// RenderParams.MonitorName = m_hmdInfo.DisplayDeviceName;
				// RenderParams.DisplayId = m_hmdInfo.DisplayId;
				m_stereoConfig.SetHMDInfo (m_hmdInfo);
				m_stereoConfig.SetDistortionFitPointVP (-1, 0);
				m_renderScale = m_stereoConfig.GetDistortionScale (); 
				m_eyes [0] = m_stereoConfig.GetEyeRenderParams (OVR::Util::Render::StereoEye_Left);
				m_eyes [1] = m_stereoConfig.GetEyeRenderParams (OVR::Util::Render::StereoEye_Right);
				m_fov = m_stereoConfig.GetYFOVDegrees ();
				float viewCenter = m_hmdInfo.HScreenSize * 0.25f;
				float eyeProjectionShift = viewCenter - m_hmdInfo.LensSeparationDistance * 0.5f;
				m_projectionCenterOffset = 4.0f * eyeProjectionShift / m_hmdInfo.HScreenSize;
				}
			}
		else {            
			// If we didn't detect an HMD, try to create the sensor directly.
			// This is useful for debugging sensor interaction; it is not needed in
			// a shipping app.
			m_pSensorP = *m_pManager->EnumerateDevices<OVR::SensorDevice> ().CreateDevice ();
			}
		}
	m_nResolution = HResolution () > 1280;
	// If there was a problem detecting the VR, display appropriate message.

	const char* detectionMessage = NULL;

	if (!m_pManager)
		detectionMessage = "Cannot initialize VR system.";
	if (!m_pHMD && !m_pSensorP)
		detectionMessage = "VR not detected.";
	else if (!m_pHMD)
		detectionMessage = "Oculus Sensor detected; HMD Display not detected.\n";
	else if (m_hmdInfo.DisplayDeviceName [0] == '\0')
		detectionMessage = "Oculus Sensor detected; HMD display EDID not detected.\n";
	else if (!m_pSensorP) {
		m_bAvailable = 1;
		detectionMessage = "Oculus HMD Display detected; Sensor not detected.\n";
		}
	else {
		m_pSensorFusion = NEW OVR::SensorFusion;
		m_pSensorFusion->AttachToSensor (m_pSensorP);
		m_pSensorFusion->SetYawCorrectionEnabled (true);
#if 0
		m_magCalTO.Setup (60000); // 1 minute
		m_magCalTO.Start (-1, true);
		m_bCalibrating = false;
#endif
		m_bAvailable = 2;
		}
if (detectionMessage) 
		PrintLog (0, detectionMessage);
	}
#endif
return m_bAvailable != 0;
}

// ----------------------------------------------------------------------------

int32_t CVRData::GetViewMatrix (CFixMatrix& mOrient)
{
#if OCULUS_RIFT
if (Available () < 2)
	return 0;
OVR::Quatf q = m_pSensorFusion->GetOrientation ();
OVR::Matrix4f m (q);
for (int32_t i = 0; i < 3; i++)
	for (int32_t j = 0; j < 3; j++)
		mOrient.m.vec [i * 3 + j] = F2X (m.M [i][j]);
#endif
return 1;
}

// ----------------------------------------------------------------------------

#if OCULUS_RIFT
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
#if OCULUS_RIFT
if (Available () < 2)
	return 0;
OVR::Quatf q = m_pSensorFusion->GetOrientation ();
float yaw, pitch, roll;
q.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&yaw, &pitch, &roll);
if (!angles) 
	m_center.Set (pitch, roll, yaw);
else {
	pitch -= m_center.v.coord.x;
	roll -= m_center.v.coord.y;
	yaw -= m_center.v.coord.z;
	angles->Set (-F2X (AddDeadzone (pitch) * 0.5f), F2X (AddDeadzone (roll) * 0.5f), -F2X (AddDeadzone (yaw) * 0.5f));
	}
#endif
return 1;
}

// ----------------------------------------------------------------------------

void CVRData::AutoCalibrate (void)
{
#if 0
if (Available () > 1) {
	if (m_bCalibrating) {
		if (!m_magCal.IsAutoCalibrating ()) {
			m_bCalibrating = false;
			m_magCalTO.Start (-1, true);
			}
		else {
			m_magCal.UpdateAutoCalibration (*m_pSensorFusion);
			if (m_magCal.IsCalibrated ()) {
				m_bCalibrating = false;
				m_magCalTO.Start (-1);
				}
			}
		}
	else if (m_magCalTO.Expired (false))
		m_magCal.BeginAutoCalibration (*m_pSensorFusion);
	}
#endif
}

// ----------------------------------------------------------------------------

void CVRData::Destroy (void)
{
#if OCULUS_RIFT
if (m_pSensorFusion) {
	delete m_pSensorFusion;
	m_pSensorFusion = NULL;
	}
#endif
}

