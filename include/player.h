/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _PLAYER_H
#define _PLAYER_H

#include "fix.h"
#include "vecmat.h"
#include "weapon.h"
#include "timeout.h"

#define MAX_PLAYERS_D2		8
#define MAX_PLAYERS_D2X		16
#define MAX_PLAYERS			16
#define MAX_COOP_PLAYERS	3
#define MAX_MULTI_PLAYERS (MAX_PLAYERS + MAX_COOP_PLAYERS)
#define MAX_PLAYER_COLORS	8

#define MAX_SHIP_TYPES		3

// Initial player stat values
#define INITIAL_ENERGY  I2X(100)    // 100% energy to start
#define INITIAL_SHIELD	I2X(100)    // 100% shield to start

#define MAX_ENERGY      I2X(200)    // go up to 200
#define MAX_SHIELD		I2X(200)

#define INITIAL_LIVES               3   // start off with 3 lives

// Values for special flags
#define PLAYER_FLAGS_INVULNERABLE   1       // Player is invulnerable
#define PLAYER_FLAGS_BLUE_KEY       2       // Player has blue key
#define PLAYER_FLAGS_RED_KEY        4       // Player has red key
#define PLAYER_FLAGS_GOLD_KEY       8       // Player has gold key
#define PLAYER_FLAGS_FLAG           16      // Player has his team's flag
#define PLAYER_FLAGS_UNUSED         32      //
#define PLAYER_FLAGS_FULLMAP        64      // Player can see unvisited areas on map
#define PLAYER_FLAGS_AMMO_RACK      128     // Player has ammo rack
#define PLAYER_FLAGS_CONVERTER      256     // Player has energy->shield converter
#define PLAYER_FLAGS_FULLMAP_CHEAT  512     // Player can see unvisited areas on map normally
#define PLAYER_FLAGS_QUAD_LASERS    1024    // Player shoots 4 at once
#define PLAYER_FLAGS_CLOAKED        2048    // Player is cloaked for awhile
#define PLAYER_FLAGS_AFTERBURNER    4096    // Player's afterburner is engaged
#define PLAYER_FLAGS_HEADLIGHT      8192    // Player has headlight boost
#define PLAYER_FLAGS_HEADLIGHT_ON   16384   // is headlight on or off?
#define PLAYER_FLAGS_SLOWMOTION		32768
#define PLAYER_FLAGS_BULLETTIME		65536

#define PLAYER_FLAGS_ALL_KEYS			(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY)

#define AFTERBURNER_MAX_TIME    		I2X (5)    // Max time afterburner can be on.
#define CALLSIGN_LEN                8       // so can use as filename (was: 12)

// Amount of time player is cloaked.
#define CLOAK_TIME_MAX          I2X (30)
#define INVULNERABLE_TIME_MAX   I2X (30)

#define PLAYER_STRUCT_VERSION   17  // increment this every time player struct changes

void NetworkFlushData (void);

// defines for teams
#define TEAM_BLUE   0
#define TEAM_RED    1

typedef struct tPlayerHostages {
	uint16_t  nRescued;		// Total number of hostages rescued.
	uint16_t  nTotal;      // Total number of hostages.
	uint8_t   nOnBoard;    // Number of hostages on ship.
	uint8_t   nLevel;      // Number of hostages on this level.
} __pack__ tPlayerHostages;
// When this structure changes, increment the constant
// SAVE_FILE_VERSION in playsave.c


typedef union tShipModifier {
	struct {
		float shield;
		float	energy;
		float	speed;
		} v;
	float a [3];
} __pack__ tShipModifier;


#define RECHARGE_DELAY_COUNT	10

class CShipEnergy {
	private:
		int32_t	m_type;
		int32_t	m_index;
		fix		m_init;
		fix		m_max;
		fix*		m_current;
		CTimeout	m_toRecharge;

		static time_t m_nRechargeDelays [RECHARGE_DELAY_COUNT];

	public:
		//default c-tor
		static int32_t RechargeDelayCount (void);
		static time_t RechargeDelay (uint8_t i);

		CShipEnergy () { Reset (); }

		void Reset (void)  { Setup (0, 0, I2X (100), NULL); }
		// return ship type dependent scaling value
		float Scale (void);
		// get initial value (which is identical to the max value w/o overcharge)
		inline fix Initial (void) { return (fix) FRound (m_init * Scale ()); }
		// get max value
		inline fix Max (void) { return (fix) FRound (m_max * Scale ()); }
		// get current value
		inline fix Get (bool bScale = true) { return bScale ? *m_current : (fix) FRound (*m_current / Scale ()); }
		// set by fixed value
		bool Set (fix e, bool bScale = true);
		// change by some value
		inline fix Update (fix delta) { 
			if (delta)
				Set (Get () + delta, false);
			return Get (); 
			}	
		// fill up
		inline bool Reset (fix e) {	
			e = (fix) FRound (e * Scale ());
			return (Get () < e) ? Set (e, false) : false; 
			}
		// fill rate in percent
		inline int32_t Level (void) { return (int32_t) FRound (100.0f * float (Get ()) / float (Initial ())); }
		// initialize
		void Setup (int32_t type, int32_t index, fix init, fix* current);

		void Recharge (void);

		inline time_t RechargeDelay (void) { return m_toRecharge.Duration (); }

		inline void SetRechargeDelay (time_t delay) { m_toRecharge.Setup (delay); }
	};


#define MAX_LASER_LEVEL					3   // Note, laser levels are numbered from 0.
#define MAX_SUPERLASER_LEVEL			5   // Note, laser levels are numbered from 0.

class __pack__ CPlayerInfo {
	public:
		// Who am I data
		char			callsign [CALLSIGN_LEN+1];  // The callsign of this player, for net purposes.
		uint8_t		netAddress[6];					// The network address of the player.
		int8_t		connected;						// Is the player connected or not?
		int32_t     nObject;						   // What CObject number this player is. (made an int32_t by mk because it's very often referenced)
		int32_t     nPacketsGot;					   // How many packets we got from them
		int32_t     nPacketsSent;					// How many packets we sent to them

		//  -- make sure you're 4 byte aligned now!

		// Game data
		uint32_t    flags;							// Powerup flags, see below...
		fix			energy;						// Amount of energy remaining.
		fix			shield;						// shield remaining (protection)
		uint8_t		lives;							// Lives remaining, 0 = game over.
		int8_t		level;							// Current level player is playing. (must be signed for secret levels)
		uint8_t		laserLevel;					// Current level of the laser.
		int8_t		startingLevel;				// What level the player started on.
		int16_t		nKillerObj;					// Who killed me.... (-1 if no one)
		uint16_t		primaryWeaponFlags;		// bit set indicates the player has this weapon.
		uint16_t		secondaryWeaponFlags;		// bit set indicates the player has this weapon.
		uint16_t		primaryAmmo [MAX_PRIMARY_WEAPONS]; // How much ammo of each nType.
		uint16_t		secondaryAmmo [MAX_SECONDARY_WEAPONS]; // How much ammo of each nType.
	#if 1 //for inventory system
		uint8_t		nInvuls;
		uint8_t		nCloaks;
	#else
		uint16_t  pad; // Pad because increased weaponFlags from byte to int16_t -YW 3/22/95
	#endif
		//  -- make sure you're 4 byte aligned now

		// Statistics...
		int32_t     lastScore;            // Score at beginning of current level.
		int32_t     score;                // Current score.
		fix			timeLevel;            // Level time played
		fix			timeTotal;            // Game time played (high word = seconds)

		fix			cloakTime;            // Time cloaked
		fix			invulnerableTime;     // Time invulnerable

		int16_t		nScoreGoalCount;       // Num of players killed this level
		int16_t		netKilledTotal;			// Number of times killed total
		int16_t		netKillsTotal;        // Number of net kills total
		int16_t		numKillsLevel;        // Number of kills this level
		int16_t		numKillsTotal;        // Number of kills total
		int16_t		numRobotsLevel;       // Number of initial robots this level
		int16_t		numRobotsTotal;       // Number of robots total
		tPlayerHostages	hostages;
		fix			homingObjectDist;     // Distance of nearest homing CObject.
		int8_t		hoursLevel;           // Hours played (since timeTotal can only go up to 9 hours)
		int8_t		hoursTotal;           // Hours played (since timeTotal can only go up to 9 hours)

	public:
		inline bool HasLeft (void) { return (connected == 0) && (*callsign == '\0'); }
		inline bool IsConnected (void) { return (connected != 0) && (*callsign != '\0'); }
		inline bool Connected (int32_t nState) { return (abs (connected) == nState) && (*callsign != '\0'); }
		inline void SetConnected (int32_t nState) { connected = nState; }
		inline int32_t GetConnected (void) { return connected; }
	};

#include "flightpath.h"

class CPlayerData : public CPlayerInfo {
	public:
		CFlightPath	m_flightPath;
		CShipEnergy	m_shield;
		CShipEnergy	m_energy;
		int8_t		m_nObservedPlayer;
		uint8_t		m_laserLevels [2];
		uint8_t		m_bExploded;
		uint8_t		m_nLevel;
		int32_t		m_tDisconnect;
		int32_t		m_tDeath;
		int32_t		m_tWeaponInfo;

	public:
		CPlayerData () { 
			Reset ();
			Setup (); 
			}

		void Reset (void) { 
			m_shield.Reset ();
			m_energy.Reset ();
			m_flightPath.Reset (-1, -1);
			m_nObservedPlayer = 0;
			m_laserLevels [0] = m_laserLevels [1] = 0;
			m_bExploded = 0;
			m_nLevel = 0;
			m_tDisconnect = 0;
			m_tDeath = 0;
			m_tWeaponInfo = 0;
			}

		void Setup (void) {
			m_shield.Setup (0, Index (), INITIAL_SHIELD, &shield);
			m_energy.Setup (1, Index (), INITIAL_ENERGY, &energy);
			}

		void Connect (int8_t nStatus);

		bool TimedOut (void) { return m_tDisconnect > 0; }

		bool WaitingForExplosion (void);
		bool WaitingForWeaponInfo (void);

		inline CFlightPath& FlightPath (void) { return m_flightPath; }
		inline int8_t ObservedPlayer (void) { return m_nObservedPlayer; }
		inline void SetObservedPlayer (int8_t nPlayer) { m_nObservedPlayer = nPlayer; }

#if 1
		inline fix InitialShield (void) { return m_shield.Initial (); }
		inline fix InitialEnergy (void) { return m_energy.Initial (); }
		inline fix Shield (bool bScale = true) { return m_shield.Get (bScale); }
		inline fix Energy (bool bScale = true) { return m_energy.Get (bScale); }
		inline fix MaxShield (void) { return m_shield.Max (); }
		inline fix MaxEnergy (void) { return m_energy.Max (); }
		inline int32_t ShieldRechargeDelay (void) { return (int32_t) m_shield.RechargeDelay (); }
		inline int32_t EnergyRechargeDelay (void) { return (int32_t) m_energy.RechargeDelay (); }
		inline void SetShieldRechargeDelay (int32_t delay) { m_shield.SetRechargeDelay (delay); }
		inline void SetEnergyRechargeDelay (int32_t delay) { m_energy.SetRechargeDelay (delay); }
		fix SetShield (fix s, bool bScale = true);
		fix SetEnergy (fix e, bool bScale = true);
		void Recharge (void) {
			m_energy.Recharge ();
			m_shield.Recharge (); 
			}
		void UpdateDeathTime (void);
		inline fix ResetShield (fix s) { return m_shield.Reset (s); }
		inline fix ResetEnergy (fix e) { return m_energy.Reset (e); }
		inline fix UpdateShield (fix delta) { 
			if (delta) {
				m_shield.Update (delta); 
				NetworkFlushData (); // will send position, shield and weapon info
				UpdateDeathTime ();
				}
			return m_shield.Get ();
			}
		inline fix UpdateEnergy (fix delta) { return m_energy.Update (delta); }
		inline int32_t ShieldLevel (void) { return m_shield.Level (); }
		inline int32_t EnergyLevel (void) { return m_energy.Level (); }
		inline float ShieldScale (void) { return m_shield.Scale (); }
		inline float EnergyScale (void) { return m_energy.Scale (); }
		inline int32_t RemainingRobots (void) { return numRobotsLevel - numKillsLevel; }

		inline void UpdateLaserLevel (void) { laserLevel = LaserLevel (); }
		inline uint8_t LaserLevel (int32_t bSuperLaser = -1) { return (bSuperLaser < 0) ? m_laserLevels [1] ? MAX_LASER_LEVEL + m_laserLevels [1] : m_laserLevels [0] : m_laserLevels [bSuperLaser]; }
		inline bool AddLaser (int32_t bSuperLaser) { 
			if (m_laserLevels [bSuperLaser] >= (bSuperLaser ? MAX_SUPERLASER_LEVEL - MAX_LASER_LEVEL : MAX_LASER_LEVEL)) 
				return false;
			++m_laserLevels [bSuperLaser];
			UpdateLaserLevel ();
			return true;
			}
		inline bool AddStandardLaser (void) { return HasSuperLaser () ? false : AddLaser (0); }
		inline bool AddSuperLaser (void) { return AddLaser (1); }
		inline uint8_t HasSuperLaser (void) { return uint8_t (m_laserLevels [1] > 0); }
		inline uint8_t HasStandardLaser (void) { return uint8_t (m_laserLevels [1] == 0); }
		inline bool DropSuperLaser (void) { 
			if (!m_laserLevels [1])
				return false;
			--m_laserLevels [1]; 
			return true;
			}
		inline bool DropStandardLaser (void) { 
			if (!m_laserLevels [0])
				return false;
			--m_laserLevels [0]; 
			return true;
			}
		inline bool DropLaser (void) { 
			if (!(DropSuperLaser () || DropStandardLaser ()))
				return false;
			UpdateLaserLevel ();
			return true;
			}
		inline void SetStandardLaser (uint8_t nLevel) { m_laserLevels [0] = nLevel; }
		inline void SetSuperLaser (uint8_t nLevel) { m_laserLevels [1] = nLevel; }
		inline void SetLaserLevels (uint8_t nStandard, uint8_t nSuper) {
			m_laserLevels [0] = nStandard;
			m_laserLevels [1] = nSuper;
			UpdateLaserLevel ();
			}
		inline void ComputeLaserLevels (uint8_t nLevel) { 
			if (nLevel > MAX_LASER_LEVEL) 
				SetLaserLevels (0, nLevel - MAX_LASER_LEVEL);
			else
				SetLaserLevels (nLevel, 0);
			}
#endif
		CObject* Object (void);
		void SetObject (int16_t n);
		bool IsLocalPlayer (void);

	private:
		int32_t Index (void);
};


//version 16 structure

#define MAX_PRIMARY_WEAPONS16   5
#define MAX_SECONDARY_WEAPONS16 5

typedef struct player16 {
	// Who am I data
	char    callsign[CALLSIGN_LEN+1]; // The callsign of this player, for net purposes.
	uint8_t   netAddress[6];         // The network address of the player.
	int8_t   connected;              // Is the player connected or not?
	int32_t     nObject;                 // What CObject number this player is. (made an int32_t by mk because it's very often referenced)
	int32_t     nPacketsGot;          // How many packets we got from them
	int32_t     nPacketsSent;         // How many packets we sent to them

	//  -- make sure you're 4 byte aligned now!

	// Game data
	uint32_t    flags;                  // Powerup flags, see below...
	fix     energy;                 // Amount of energy remaining.
	fix     shield;                // shield remaining (protection)
	uint8_t   lives;                  // Lives remaining, 0 = game over.
	int8_t   level;                  // Current level player is playing. (must be signed for secret levels)
	uint8_t   laserLevel;            // Current level of the laser.
	int8_t   startingLevel;         // What level the player started on.
	int16_t   nKillerObj;          // Who killed me.... (-1 if no one)
	uint8_t   primaryWeaponFlags;   // bit set indicates the player has this weapon.
	uint8_t   secondaryWeaponFlags; // bit set indicates the player has this weapon.
	uint16_t  primaryAmmo[MAX_PRIMARY_WEAPONS16];    // How much ammo of each nType.
	uint16_t  secondaryAmmo[MAX_SECONDARY_WEAPONS16];// How much ammo of each nType.

	//  -- make sure you're 4 byte aligned now

	// Statistics...
	int32_t     last_score;             // Score at beginning of current level.
	int32_t     score;                  // Current score.
	fix     timeLevel;             // Level time played
	fix     timeTotal;             // Game time played (high word = seconds)

	fix     cloakTime;             // Time cloaked
	fix     invulnerableTime;      // Time invulnerable

	int16_t   netKilledTotal;       // Number of times killed total
	int16_t   netKillsTotal;        // Number of net kills total
	int16_t   numKillsLevel;        // Number of kills this level
	int16_t   numKillsTotal;        // Number of kills total
	int16_t   numRobotsLevel;       // Number of initial robots this level
	int16_t   numRobotsTotal;       // Number of robots total
	tPlayerHostages	hostages;
	fix     homingObjectDist;     // Distance of nearest homing CObject.
	int8_t   hoursLevel;            // Hours played (since timeTotal can only go up to 9 hours)
	int8_t   hoursTotal;            // Hours played (since timeTotal can only go up to 9 hours)
} __pack__ player16;

//------------------------------------------------------------------------------

#define N_PLAYER_GUNS 8

class __pack__ CPlayerShip {
	public:
		int32_t		nModel;
		int32_t		nExplVClip;
		fix			mass;
		fix			drag;
		fix			maxThrust;
		fix			reverseThrust;
		fix			brakes;
		fix			wiggle;
		fix			maxRotThrust;
		CFixVector	gunPoints [N_PLAYER_GUNS];

	public:
		CPlayerShip () { memset (this, 0, sizeof (*this)); }

};

/*
 * reads a CPlayerShip structure from a CFILE
 */
void PlayerShipRead(CPlayerShip *ps, CFile& cf);
int32_t EquippedPlayerGun (CObject *objP);
int32_t EquippedPlayerBomb (CObject *objP);
int32_t EquippedPlayerMissile (CObject *objP, int32_t *nMissiles);
void UpdatePlayerWeaponInfo (void);
void UpdatePlayerEffects (void);

extern tShipModifier shipModifiers [MAX_SHIP_TYPES];

#endif
