// Nyxora for the iPhone: what is worn in the wardrobe and the look for the game (the Nyxora Cosmetics mod).
// The menu (ios/nyxora/) sends both; this keeps them in <AME_HOME>/nyxora/ and, before every start, puts the mod and the
// look into the game folder of a Fabric or Quilt 1.20.1 profile (the same files as Nyxora on the PC and on Android:
// mods/vellora-cosmetics-1.20.1.jar, config/vellora-cosmetics/look.json, skin.png, cape.png).
#import <Foundation/Foundation.h>

// posted when the account or the profile changes, so the menu shows the new ones
#define NyxoraChangedNotification @"NyxoraChanged"
// posted when the game is about to start (the menu's music stops)
#define NyxoraGameStartsNotification @"NyxoraGameStarts"

@interface NyxoraLook : NSObject

+ (NSString *)folder;

+ (NSDictionary *)worn;
+ (BOOL)saveWorn:(NSDictionary *)worn;

// look: the look.json text; skin and cape: PNG data URLs, or nil when there is none
+ (BOOL)saveLook:(NSString *)look skin:(NSString *)skin cape:(NSString *)cape;

// Fabric or Quilt on Minecraft 1.20.1, the only packs the mod is made for
+ (BOOL)fitsVersion:(NSString *)versionId;

// before a start: the mod and the look in the game folder, or the mod out of a pack it does not fit
+ (void)prepareGameDir:(NSString *)gameDir versionId:(NSString *)versionId;

@end
