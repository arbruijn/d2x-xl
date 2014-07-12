#ifndef __INPUT_H
#define __INPUT_H

#include "kconfig.h"

//extern tControlInfo controls [4];

class CControlsManager {
	private:
		tControlInfo	m_info [4];
		int32_t			m_frameCount;
		int32_t			m_maxTurnRate;
		float				m_slackTurnRate;
		time_t			m_pollTime;
		float				m_frameTime;
		float				m_lastTick;
		int32_t			m_joyAxis [JOY_MAX_AXES];

	public:
		CControlsManager () {
			m_frameCount = m_maxTurnRate = 0;
			m_lastTick = 0;
			m_slackTurnRate = 0;
			}
		int32_t ReadJoystick (int32_t* joyAxis);
		void ReadFCS (int32_t nRawAxis);
		int32_t ReadAll (void);
		void FlushInput (void);
		void ResetCruise (void);
		char GetKeyValue (char key);
		void SetType (void);
		int32_t CalcDeadzone (int32_t d, int32_t nDeadzone);
		void Reset (void);
		void ResetTriggers (void);
		int32_t Read (void);

		inline void StopPrimaryFire (void) {
			m_info [0].firePrimaryState = 0;
			m_info [0].firePrimaryDownCount = 0;
			}

		inline void StopSecondaryFire (void) {
			m_info [0].fireSecondaryState = 0;
			m_info [0].fireSecondaryDownCount = 0;
			}

		inline time_t PollTime (void) { return m_pollTime; }
		inline void SetPollTime (time_t pollTime) { m_pollTime = pollTime; }

		inline tControlInfo& operator[] (int32_t i) { return m_info [i]; }

		inline int32_t JoyAxis (int32_t i) { return m_joyAxis [i]; }

	private:
		int32_t AllowToToggle (int32_t i);
		int32_t ReadKeyboard (void);
		int32_t ReadMouse (int32_t * mouseAxis, int32_t * nMouseButtons);
		int32_t AttenuateAxis (double a, int32_t nAxis);
		int32_t ReadJoyAxis (int32_t i, int32_t rawJoyAxis []);
		void SetFCSButton (int32_t btn, int32_t button);
		int32_t ReadCyberman (int32_t *mouseAxis, int32_t *nMouseButtons);
#ifdef USE_SIXENSE
		int32_t ReadSixense (int32_t* joyAxis);
#endif
		int32_t LimitTurnRate (int32_t bUseMouse);
		int32_t CapSampleRate (void);

		void DoD2XKeys (int32_t *bSlideOn, int32_t *bBankOn, fix *pitchTimeP, fix *headingTimeP, int32_t *nCruiseSpeed, int32_t bGetSlideBank);
		void DoKeyboard (int32_t *bSlideOn, int32_t *bBankOn, fix *pitchTimeP, fix *headingTimeP, int32_t *nCruiseSpeed, int32_t bGetSlideBank);
		void DoJoystick (int32_t *bSlideOn, int32_t *bBankOn, fix *pitchTimeP, fix *headingTimeP, int32_t *nCruiseSpeed, int32_t bGetSlideBank);
		void DoMouse (int32_t *mouseAxis, int32_t nMouseButtons, int32_t *bSlideOn, int32_t *bBankOn, fix *pitchTimeP, fix *headingTimeP, int32_t *nCruiseSpeed, int32_t bGetSlideBank);
		int32_t ReadTrackIR (void);
		void DoTrackIR (void);
		int32_t ReadOculusRift (void);
		void DoOculusRift (void);
		void DoSlideBank (int32_t bSlideOn, int32_t bBankOn, fix pitchTime, fix headingTime);
		void CybermouseAdjust (void);
		int32_t DeltaAxis (int32_t v);
};

extern CControlsManager controls;


#endif //__INPUT_H
