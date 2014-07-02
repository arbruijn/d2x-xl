#ifndef _HUDICONS_H
#define _HUDICONS_H

extern int32_t bHaveInvBms;

class CHUDIcons {
	private:
		float	m_xScale;
		float	m_yScale;
		int32_t	m_nLineSpacing;

	public:
		int32_t EquipmentActive (int32_t bFlag);
		int32_t LoadInventoryIcons (void);
		void DestroyInventoryIcons (void);
		int32_t LoadTallyIcons (void);
		void DestroyTallyIcons (void);
		int32_t LoadGaugeIcons (void);
		void DestroyGaugeIcons (void);
		CBitmap& GaugeIcon (int32_t i);
		void DrawTally (void);
		void ToggleWeaponIcons (void);
		void DrawWeapons (void);
		void DrawInventory (void);
		void Render (void);
		void Destroy (void);
		int32_t LoadIcons (const char* pszIcons [], CBitmap* icons, int32_t nIcons, int32_t& bHaveIcons);
		void DestroyIcons (CBitmap* icons, int32_t nIcons, int32_t& bHaveIcons);

		inline bool Visible (void) { return !(gameStates.app.bNostalgia || gameStates.app.bEndLevelSequence || gameStates.render.bRearView); }
		inline bool Inventory (void) { return Visible () && gameOpts->render.weaponIcons.bEquipment && bHaveInvBms; }

	private:
		int32_t GetWeaponState (int32_t& bHave, int32_t& bAvailable, int32_t& bActive, int32_t i, int32_t j, int32_t l);
		int32_t GetAmmo (char* szAmmo, int32_t i, int32_t j, int32_t l);
		int32_t GetWeaponIndex (int32_t i, int32_t j, int32_t& nMaxAutoSelect);
		CBitmap* LoadWeaponIcon (int32_t i, int32_t l);
		void SetWeaponFillColor (int32_t bHave, int32_t bAvailable, float alpha);
		void SetWeaponFrameColor (int32_t bHave, int32_t bAvailable, int32_t bActive, float alpha);

	};

extern CHUDIcons hudIcons;

//	-----------------------------------------------------------------------------

#endif //_HUDICONS_H
