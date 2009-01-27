#ifndef _HUDICONS_H
#define _HUDICONS_H

class CHUDIcons {
	private:
		float	xScale;
		float	yScale;
		int	nLineSpacing;

	public:
		int EquipmentActive (int bFlag);
		int LoadInventoryIcons (void);
		void DestroyInventoryIcons (void);
		int LoadTallyIcons (void);
		void DestroyTallyIcons (void);
		void DrawTally (void);
		void ToggleWeaponIcons (void);
		void DrawWeapons (void);
		void DrawInventory (void);
		void Render (void);
		void Destroy (void);
	};

extern CHUDIcons hudIcons;

//	-----------------------------------------------------------------------------

#endif //_HUDICONS_H
