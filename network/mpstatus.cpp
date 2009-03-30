
#include "descent.h"
#include "config.h"
#include "menu.h"

#define DBG_MPSTATUS 0

extern int*	mpStatus;
extern bool (*mpActivate) (void);
extern int (*mpCheck) (int);
extern void (*mpNotify) (void);

// ----------------------------------------------------------------------------

class CMicroPayments {
	public:
		static int m_nStatus;

		static void Init (void);
		static bool Activate (void);
		static int Check (int nMode);
		static void Notify (void);
};

int CMicroPayments::m_nStatus;

class CMicroPayments microPayments;

// ----------------------------------------------------------------------------

void CMicroPayments::Init (void)
{
#if DBG_MPSTATUS
m_nStatus = -1;
#else
m_nStatus = 0;
#endif
mpStatus = &m_nStatus;
mpActivate = &Activate;
mpCheck = &Check;
mpNotify = &Notify;
}

// ----------------------------------------------------------------------------

bool CMicroPayments::Activate (void)
{
#if DBG_MPSTATUS
return true;
#else
   time_t      t;
   struct tm*	h;

time (&t);
h = localtime (&t);
return ((h->tm_mon == 3) && (h->tm_mday == 1));
#endif
}

//-----------------------------------------------------------------------------

int CMicroPayments::Check (int nMode)
{
if (!m_nStatus)
	return 1;
if (nMode == 1)
	return (gameStates.app.nSDLTicks - gameData.time.xGameStart >= gameData.time.xMaxOnline);
if (IsMultiGame && (gameStates.app.nSDLTicks - gameData.time.xGameStart >= gameData.time.xMaxOnline)) {
	messageBox.Show ("Your available online time has expired.");
	G3_SLEEP (4000);
	messageBox.Clear ();
	gameData.time.xGameStart = gameStates.app.nSDLTicks;
	return 0;
	}
LOCALPLAYER.primaryWeaponFlags &= LASER_INDEX;
LOCALPLAYER.laserLevel = 0;
LOCALPLAYER.secondaryWeaponFlags &= CONCUSSION_INDEX;
memset (&LOCALPLAYER.primaryAmmo [1], 0, sizeof (LOCALPLAYER.primaryAmmo) - sizeof (LOCALPLAYER.primaryAmmo [0]));
memset (&LOCALPLAYER.secondaryAmmo [1], 0, sizeof (LOCALPLAYER.secondaryAmmo) - sizeof (LOCALPLAYER.secondaryAmmo [0]));
return -1;
}

// ----------------------------------------------------------------------------

void CMicroPayments::Notify (void)
{
	CMenu	m (30);

gameData.menu.colorOverride = RGB_PAL (63, 47, 0);

m.AddText ("\x01\xFF\xC0\x80" "Preamble" "\x04");
m.AddText ("");
m.AddText ("Since donations have never really worked with D2X-XL, I have");
m.AddText ("decided to stay up-to-date and implement a micro-payment");
m.AddText ("system as a new, effective business model.  Going forward,");
m.AddText ("level 1 lasers and concussion missiles will be default");
m.AddText ("equipment, and small, reasonable fees will be charged for");
m.AddText ("each other missile and gun you collect.");
m.AddText ("");
m.AddText ("\x01\xFF\xC0\x80" "Fees" "\x04");
m.AddText ("");
m.AddText ("Following is a list of fees you can expect to be charged when");
m.AddText ("using the newest version of D2X-XL:");
m.AddText ("");
m.AddText ("   \x01\xE0\xE0\xE0 Earth Shaker: $0.05" "\x04");
m.AddText ("   \x01\xE0\xE0\xE0 Mega Missile: $0.03" "\x04");
m.AddText ("   \x01\xE0\xE0\xE0 Smart Missile/Mine: $0.02" "\x04");
m.AddText ("   \x01\xE0\xE0\xE0 Other missiles/mines: $0.01" "\x04");
m.AddText ("   \x01\xE0\xE0\xE0 Tier 1 guns: $0.01" "\x04");
m.AddText ("   \x01\xE0\xE0\xE0 Tier 2 guns: $0.02" "\x04");
m.AddText ("   \x01\xE0\xE0\xE0 Gauss gun: $0.10 (ammo will however be free)" "\x04");
m.AddText ("");
m.AddText ("All other equipment will be free.");
m.AddText ("");
m.AddText ("");
m.AddText ("");
m.AddText ("\x01\xC0\xC0\xC0" "[1/4]" "\x04");
m.Top ()->m_x = short (0x8000);
m.TinyMenu (NULL, "New Business Model");

m.Destroy ();
m.Create (30);
m.AddText ("\x01\xFF\xC0\x80" "Online Services" "\x04");
m.AddText ("");
m.AddText ("Online gaming will be supported by a new subscription model");
m.AddText ("aptly named \"D2X Live\", with a $15 monthly fee. Lifetime");
m.AddText ("subscriptions are available for $999; please contact me for");
m.AddText ("details. You can win annual and lifetime subscriptions in");
m.AddText ("lotteries that will be held on a monthly basis; see the");
m.AddText ("D2X-XL forum for more information.");
m.AddText ("");
m.AddText ("\x01\xFF\xC0\x80" "Support" "\x04");
m.AddText ("");
m.AddText ("Support will be free. Bug reports lacking the information");
m.AddText ("necessary and required to process the report, as well");
m.AddText ("as support requests suggesting the user didn't follow the");
m.AddText ("guidelines  for posting these or didn't read the installation");
m.AddText ("instructions and FAQ will cause extra support fees to be");
m.AddText ("charged the amount of which are entirely up to my disposition.");
m.AddText ("");
m.AddText ("");
m.AddText ("");
m.AddText ("");
m.AddText ("");
m.AddText ("");
m.AddText ("");
m.AddText ("");
m.AddText ("");
m.AddText ("\x01\xC0\xC0\xC0" "[2/4]" "\x04");
m.Top ()->m_x = short (0x8000);
m.TinyMenu (NULL, "New Business Model");

m.Destroy ();
m.Create (30);
m.AddText ("\x01\xFF\xC0\x80" "Exchange Rates" "\x04");
m.AddText ("");
m.AddText ("As I am not Valve Software and this is not Steam, users from");
m.AddText ("the Euro zone will not be charged more and thus actually");
m.AddText ("benefit from their currency exchange rate. :)");
m.AddText ("");
m.AddText ("\x01\xFF\xC0\x80" "Mac OS X" "\x04");
m.AddText ("");
m.AddText ("Due to the additional debugging time, man power and more");
m.AddText ("expensive hardware required for the Mac version, Mac users");
m.AddText ("will be charged 50% more. This  will allow the Mac port to");
m.AddText ("continue to be developed; You as a Mac user are certainly");
m.AddText ("able and willing to pay a slightly increased price for an");
m.AddText ("unparalleled D2X-XL experience; above which you will be able");
m.AddText ("to feel as an elite in this area too.");
m.AddText ("");
m.AddText ("\x01\xFF\xC0\x80" "Linux" "\x04");
m.AddText ("");
m.AddText ("Since D2X-XL is delivered as source code to Linux, and Linux");
m.AddText ("users would hence find a way to circumvent this new system,");
m.AddText ("I have had no choice but to remove the related code from the");
m.AddText ("source (I made sure it cannot be retrieved from the source");
m.AddText ("repository either, I am not that dumb), and Linux users will");
m.AddText ("either have to buy an OS that deserves the name or settle for");
m.AddText ("less as they're used to anyway.");
m.AddText ("");
m.AddText ("\x01\xC0\xC0\xC0" "[3/4]" "\x04");
m.Top ()->m_x = short (0x8000);
m.TinyMenu (NULL, "New Business Model");

m.Destroy ();
m.Create (30);
m.AddText ("\x01\xFF\xC0\x80" "Conclusion" "\x04");
m.AddText ("");
m.AddText ("The next dialog will ask you for your credit card number. If");
m.AddText ("you don't have a credit card, ask your parents or friends");
m.AddText ("for theirs. If your friends won't hand it to you, you may");
m.AddText ("want to find yourself better friends.");
m.AddText ("");
m.AddText ("\x01\xFF\xC0\x80" "Legal Information" "\x04");
m.AddText ("");
m.AddText ("Once entered, D2X-XL will automatically charge your credit");
m.AddText ("card with all fees that you have accumulated on a month-to-");
m.AddText ("month basis.  You will be sent a monthly receipt detailing");
m.AddText ("the items you have used and the fees assessed; no refunds");
m.AddText ("will be made. You agree to accept all charges the moment you");
m.AddText ("pick up any primary weapons other than level 1 lasers or any");
m.AddText ("secondary weapon other than concussion missiles. There is no");
m.AddText ("warranty on the utility or accuracy of any weapons, whether");
m.AddText ("expressed or implied.  All legal affairs related to the new");
m.AddText ("business model will be handled in Karlsruhe, Germany.");
m.AddText ("");
m.AddText ("\x01\xFF\xC0\x80" "Contact And Feedback" "\x04");
m.AddText ("");
m.AddText ("For feedback please visit http://www.descent2.de/forum.");
m.AddText ("");
m.AddText ("");
m.AddText ("\x01\xC0\xC0\xC0" "[4/4]" "\x04");
m.Top ()->m_x = short (0x8000);
m.TinyMenu (NULL, "New Business Model");

gameData.menu.colorOverride = RGB_PAL (63, 47, 0);

m.Destroy ();
m.Create (30);
m.AddText ("If you agree press Escape to be forwarded");
m.Top ()->m_x = short (0x8000);
m.AddText ("to the credit card data input form.");
m.Top ()->m_x = short (0x8000);
m.AddText ("");
m.AddText ("Otherwise " "\x01\xFF\xC0\x80" "please reboot now" "\x01\xFF\xE0\x80" " to avoid additionally");
m.Top ()->m_x = short (0x8000);
m.AddText ("required support modules being installed.");
m.Top ()->m_x = short (0x8000);
m.TinyMenu (NULL, "New Business Model");

char szCCInfo [25];
*szCCInfo = '\0';

gameData.menu.colorOverride = RGB_PAL (63, 47, 0);

m.Destroy ();
m.Create (30);
m.AddText ("Please enter your credit card information");
m.AddText ("below. Leaving the field empty or entering");
m.AddText ("invalid credit card information will disable");
m.AddText ("all advanced guns and missiles and limit");
m.AddText ("online gameplay time to three minutes.");
m.AddText ("");
m.AddInput (szCCInfo, sizeof (szCCInfo) - 1);
m.AddText ("");
m.AddText ("Press Escape to continue.");
m.Top ()->m_x = short (0x8000);
m.TinyMenu (NULL, "Credit Card Information");

gameData.menu.colorOverride = 0;

m_nStatus = 1;
}

// ----------------------------------------------------------------------------

void InitMPStatus (void)
{
microPayments.Init ();
}

// ----------------------------------------------------------------------------

