#ifndef __INPUT_H
#define __INPUT_H

#include "kconfig.h"

//extern tControlInfo controls [4];

class CControlsManager {
	private:
		tControlInfo	m_info [4];

	public:
		int ReadJoystick (int* joyAxisP);
		void ReadFCS (int nRawAxis);
		int ReadAll (void);
		void FlushInput (void);
		void ResetCruise (void);
		char GetKeyValue (char key);
		void SetType (void);
		int CalcDeadzone (int d, int nDeadzone);
		void Reset (void);
		int Read (void);

		inline void StopPrimaryFire (void) {
			m_info [0].firePrimaryState = 0;
			m_info [0].firePrimaryDownCount = 0;
			}

		inline void StopSecondaryFire (void) {
			m_info [0].fireSecondaryState = 0;
			m_info [0].fireSecondaryDownCount = 0;
			}

		inline tControlInfo& operator[] (int i) { return m_info [i]; }

	private:
		int AllowToToggle (int i);
		int ReadKeyboard (void);
		int ReadMouse (int * mouseAxis, int * nMouseButtons);
		int AttenuateAxis (double a, int nAxis);
		int ReadJoyAxis (int i, int rawJoyAxis []);
		void SetFCSButton (int btn, int button);
		int ReadCyberman (int *mouseAxis, int *nMouseButtons);
		int LimitTurnRate (int bUseMouse);
		int CapSampleRate (void);

		void DoD2XKeys (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank);
		void DoKeyboard (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank);
		void DoJoystick (int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank);
		void DoMouse (int *mouseAxis, int nMouseButtons, int *bSlideOn, int *bBankOn, fix *pitchTimeP, fix *headingTimeP, int *nCruiseSpeed, int bGetSlideBank);
		int ReadTrackIR (void);
		void DoTrackIR (void);
		void DoSlideBank (int bSlideOn, int bBankOn, fix pitchTime, fix headingTime);
		void CybermouseAdjust (void);
};

extern CControlsManager controls;


#endif //__INPUT_H
