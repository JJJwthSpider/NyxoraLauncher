#import "NyxoraLook.h"

static NSString *const MOD_FILE = @"vellora-cosmetics-1.20.1.jar";

@implementation NyxoraLook

+ (NSString *)folder {
    NSString *folder = [@(getenv("AME_HOME")) stringByAppendingPathComponent:@"nyxora"];
    [NSFileManager.defaultManager createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:nil];
    return folder;
}

+ (NSString *)lookFolder {
    NSString *folder = [self.folder stringByAppendingPathComponent:@"look"];
    [NSFileManager.defaultManager createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:nil];
    return folder;
}

+ (NSDictionary *)worn {
    NSData *data = [NSData dataWithContentsOfFile:[self.folder stringByAppendingPathComponent:@"worn.json"]];
    id worn = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    return [worn isKindOfClass:NSDictionary.class] ? worn : @{};
}

+ (BOOL)saveWorn:(NSDictionary *)worn {
    if (![worn isKindOfClass:NSDictionary.class]) {
        return NO;
    }
    // only the item ids, one per slot
    NSMutableDictionary *clean = [NSMutableDictionary new];
    for (id slot in worn) {
        if ([slot isKindOfClass:NSString.class] && [worn[slot] isKindOfClass:NSString.class]) {
            clean[slot] = worn[slot];
        }
    }
    NSData *data = [NSJSONSerialization dataWithJSONObject:clean options:0 error:nil];
    return [data writeToFile:[self.folder stringByAppendingPathComponent:@"worn.json"] atomically:YES];
}

// "data:image/png;base64,...": the PNG, or nil
+ (NSData *)pngOf:(id)url {
    if (![url isKindOfClass:NSString.class]) {
        return nil;
    }
    NSRange comma = [url rangeOfString:@","];
    if (![url hasPrefix:@"data:image/png;base64"] || comma.location == NSNotFound) {
        return nil;
    }
    NSData *png = [[NSData alloc] initWithBase64EncodedString:[url substringFromIndex:comma.location + 1]
                                                      options:NSDataBase64DecodingIgnoreUnknownCharacters];
    static const unsigned char signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    if (png.length < 8 || memcmp(png.bytes, signature, 8) != 0) {
        return nil;
    }
    return png;
}

+ (BOOL)saveLook:(NSString *)look skin:(NSString *)skin cape:(NSString *)cape {
    if (![look isKindOfClass:NSString.class]) {
        return NO;
    }
    NSData *json = [look dataUsingEncoding:NSUTF8StringEncoding];
    if (![[NSJSONSerialization JSONObjectWithData:json options:0 error:nil] isKindOfClass:NSDictionary.class]) {
        return NO;
    }
    NSString *folder = self.lookFolder;
    NSFileManager *fm = NSFileManager.defaultManager;
    NSDictionary<NSString *, id> *images = @{@"skin.png": [self pngOf:skin] ?: NSNull.null,
                                          @"cape.png": [self pngOf:cape] ?: NSNull.null};
    for (NSString *name in images) {
        NSString *path = [folder stringByAppendingPathComponent:name];
        if ([images[name] isKindOfClass:NSData.class]) {
            [images[name] writeToFile:path atomically:YES];
        } else {
            [fm removeItemAtPath:path error:nil];
        }
    }
    return [json writeToFile:[folder stringByAppendingPathComponent:@"look.json"] atomically:YES];
}

+ (BOOL)fitsVersion:(NSString *)versionId {
    if (![versionId isKindOfClass:NSString.class]) {
        return NO;
    }
    NSString *version = versionId.lowercaseString;
    if (![version containsString:@"fabric"] && ![version containsString:@"quilt"]) {
        return NO;
    }
    // "fabric-loader-0.16.10-1.20.1": 1.20.1 itself, not 1.20.10 or 11.20.1
    NSRegularExpression *mc = [NSRegularExpression regularExpressionWithPattern:@"(^|[^0-9.])1\\.20\\.1($|[^0-9.])"
                                                                        options:0 error:nil];
    return [mc numberOfMatchesInString:version options:0 range:NSMakeRange(0, version.length)] > 0;
}

+ (void)prepareGameDir:(NSString *)gameDir versionId:(NSString *)versionId {
    if (gameDir.length == 0) {
        return;
    }
    NSFileManager *fm = NSFileManager.defaultManager;
    NSString *mods = [gameDir stringByAppendingPathComponent:@"mods"];
    NSString *jar = [mods stringByAppendingPathComponent:MOD_FILE];
    if (![self fitsVersion:versionId]) {
        if ([fm fileExistsAtPath:jar]) {
            [fm removeItemAtPath:jar error:nil];
        }
        return;
    }
    NSString *bundled = [[NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"nyxora"]
                         stringByAppendingPathComponent:MOD_FILE];
    NSError *error;
    [fm createDirectoryAtPath:mods withIntermediateDirectories:YES attributes:nil error:nil];
    if (![fm contentsEqualAtPath:bundled andPath:jar]) {
        [fm removeItemAtPath:jar error:nil];
        if (![fm copyItemAtPath:bundled toPath:jar error:&error]) {
            NSLog(@"[Nyxora] The look mod was not put into the pack: %@", error.localizedDescription);
            return;
        }
    }
    NSString *config = [gameDir stringByAppendingPathComponent:@"config/vellora-cosmetics"];
    [fm createDirectoryAtPath:config withIntermediateDirectories:YES attributes:nil error:nil];
    NSString *look = self.lookFolder;
    for (NSString *name in @[@"look.json", @"skin.png", @"cape.png"]) {
        NSString *from = [look stringByAppendingPathComponent:name], *to = [config stringByAppendingPathComponent:name];
        [fm removeItemAtPath:to error:nil];
        if ([fm fileExistsAtPath:from] && ![fm copyItemAtPath:from toPath:to error:&error]) {
            NSLog(@"[Nyxora] %@ was not put into the pack: %@", name, error.localizedDescription);
        }
    }
    NSLog(@"[Nyxora] The look for the game is in %@", config);
}

@end
