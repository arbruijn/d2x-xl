#ifndef _STATUSBAR_H
#define _STATUSBAR_H

void SBInitGaugeCanvases (void);
void SBCloseGaugeCanvases (void);
void SBGetHostageWindowCoords (int *x, int *y, int *w, int *h);
void SBShowScore (void);
void SBShowScoreAdded (void);
void SBShowHomingWarning (void);
void SBDrawPrimaryAmmoInfo (int ammoCount);
void SBDrawSecondaryAmmoInfo (int ammoCount);
void SBShowLives (void);
void SBDrawEnergyBar (int nEnergy);
void SBDrawAfterburner (void);
void SBDrawShieldNum (int shield);
void SBDrawShieldBar (int shield);
void SBDrawKeys (void);
void SBDrawInvulnerableShip (void);
void SBRenderGauges (void);

#endif //_STATUSBAR_H
