#ifndef _HUDICONS_H
#define _HUDICONS_H

void FreeInventoryIcons (void);
void FreeObjTallyIcons (void);
void HUDShowIcons (void);

class CHUDIcons {
	private:
		float	xScale;
		float	yScale;
		int	nLineSpacing;

	public:
		int EquipmentActive (int bFlag);
		int LoadInventory (void);
		void DestroyInventory (void);
		int LoadTally (void);
		void DestroyTally (void);
		void DrawTally (void);
		void ToggleWeaponIcons (void);
		void DrawWeapons (void);
		void DrawInventory (void);
		void Render (void);
	};

extern CHUDIcons hudIcons;

//	-----------------------------------------------------------------------------

#endif //_HUDICONS_H
