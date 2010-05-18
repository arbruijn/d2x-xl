#ifndef __INPUT_H
#define __INPUT_H

#include "kconfig.h"

//extern tControlInfo Controls [4];

class Controls {
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
		void ResetControls (void);
		int Read (void);

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



#endif //__INPUT_H
