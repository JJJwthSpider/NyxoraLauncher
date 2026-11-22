#import <WebKit/WebKit.h>
#import "authenticator/BaseAuthenticator.h"
#import "LauncherMenuViewController.h"
#import "NyxoraHomeViewController.h"
#import "NyxoraLook.h"
#import "PLProfiles.h"

// the menu's requests come in through window.webkit.messageHandlers.nyxora.postMessage({id, action, data}); the answer
// goes back through window.nyxoraReply(id, result). The web view keeps its handler strongly, so it gets this go-between.
@interface NyxoraMessages : NSObject <WKScriptMessageHandler>
@property(nonatomic, weak) id<WKScriptMessageHandler> target;
@end

@implementation NyxoraMessages
- (void)userContentController:(WKUserContentController *)controller didReceiveScriptMessage:(WKScriptMessage *)message {
    [self.target userContentController:controller didReceiveScriptMessage:message];
}
@end

static id only(id value, Class kind) {
    return [value isKindOfClass:kind] ? value : nil;
}

@interface NyxoraHomeViewController () <WKScriptMessageHandler, WKNavigationDelegate>
@property(nonatomic) WKWebView *webView;
// the skin and the cape of each Microsoft profile: {skin, cape: PNG data URLs, slim}
@property(nonatomic) NSMutableDictionary<NSString *, NSDictionary *> *textures;
@end

@implementation NyxoraHomeViewController

- (id)init {
    self = [super init];
    self.title = @"Nyxora";
    return self;
}

- (NSString *)imageName {
    return @"cube.fill";
}

- (void)viewDidLoad {
    [super viewDidLoad];
    UIColor *night = [UIColor colorWithRed:4 / 255.0 green:4 / 255.0 blue:14 / 255.0 alpha:1];
    self.view.backgroundColor = night;
    self.textures = [NSMutableDictionary new];

    WKWebViewConfiguration *config = [WKWebViewConfiguration new];
    config.allowsInlineMediaPlayback = YES;
    config.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;
    NyxoraMessages *messages = [NyxoraMessages new];
    messages.target = self;
    [config.userContentController addScriptMessageHandler:messages name:@"nyxora"];

    self.webView = [[WKWebView alloc] initWithFrame:self.view.bounds configuration:config];
    self.webView.translatesAutoresizingMaskIntoConstraints = NO;
    self.webView.navigationDelegate = self;
    self.webView.opaque = NO;
    self.webView.backgroundColor = night;
    self.webView.scrollView.backgroundColor = night;
    self.webView.scrollView.scrollEnabled = NO;
    self.webView.scrollView.bounces = NO;
    self.webView.scrollView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    [self.view addSubview:self.webView];
    // between the bar with the account and the bar with the profile and Play
    UILayoutGuide *safe = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [self.webView.topAnchor constraintEqualToAnchor:safe.topAnchor],
        [self.webView.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor],
        [self.webView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [self.webView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
    ]];
    NSString *folder = [NSBundle.mainBundle.bundlePath stringByAppendingPathComponent:@"nyxora"];
    [self.webView loadFileURL:[NSURL fileURLWithPath:[folder stringByAppendingPathComponent:@"index.html"]]
      allowingReadAccessToURL:[NSURL fileURLWithPath:folder isDirectory:YES]];

    self.navigationItem.leftBarButtonItem = self.splitViewController.displayModeButtonItem;
    self.navigationItem.rightBarButtonItem = [sidebarViewController drawAccountButton];
    self.navigationItem.leftItemsSupplementBackButton = YES;

    __weak NyxoraHomeViewController *weakSelf = self;
    [NSNotificationCenter.defaultCenter addObserverForName:NyxoraChangedNotification object:nil
                                                     queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
        [weakSelf script:@"window.nyxoraChanged && window.nyxoraChanged()"];
    }];
    [NSNotificationCenter.defaultCenter addObserverForName:NyxoraGameStartsNotification object:nil
                                                     queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
        [weakSelf script:@"window.nyxoraHidden && window.nyxoraHidden(true)"];
    }];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    // back from the profiles, the settings or the game: the account and the profile may be new
    [self script:@"window.nyxoraChanged && window.nyxoraChanged()"];
    [self script:@"window.nyxoraHidden && window.nyxoraHidden(false)"];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self script:@"window.nyxoraHidden && window.nyxoraHidden(true)"];
}

- (void)script:(NSString *)script {
    [self.webView evaluateJavaScript:script completionHandler:nil];
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)action
decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    // the menu is the app's own page; a link away from it opens in Safari
    NSURL *url = action.request.URL;
    if (url.isFileURL || [url.scheme isEqualToString:@"about"]) {
        decisionHandler(WKNavigationActionPolicyAllow);
        return;
    }
    if ([url.scheme isEqualToString:@"https"]) {
        [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
    }
    decisionHandler(WKNavigationActionPolicyCancel);
}

#pragma mark - the menu's requests

- (void)userContentController:(WKUserContentController *)controller didReceiveScriptMessage:(WKScriptMessage *)message {
    NSDictionary *body = only(message.body, NSDictionary.class);
    NSNumber *requestId = only(body[@"id"], NSNumber.class);
    NSString *action = only(body[@"action"], NSString.class);
    NSDictionary *data = only(body[@"data"], NSDictionary.class) ?: @{};
    if ([action isEqualToString:@"state"]) {
        [self stateWithReply:^(NSDictionary *state) {
            [self reply:requestId result:state];
        }];
        return;
    }
    BOOL ok = YES;
    if ([action isEqualToString:@"saveWorn"]) {
        ok = [NyxoraLook saveWorn:only(data[@"worn"], NSDictionary.class)];
    } else if ([action isEqualToString:@"saveLook"]) {
        ok = [NyxoraLook saveLook:only(data[@"look"], NSString.class) skin:only(data[@"skin"], NSString.class)
                             cape:only(data[@"cape"], NSString.class)];
    } else if ([action isEqualToString:@"play"]) {
        ok = [self play];
    } else if ([action isEqualToString:@"open"]) {
        if ([only(data[@"screen"], NSString.class) isEqualToString:@"accounts"]) {
            [sidebarViewController selectAccount:sidebarViewController.accountButton];
        }
    } else {
        ok = NO;
    }
    [self reply:requestId result:@{@"ok": @(ok)}];
}

- (void)reply:(NSNumber *)requestId result:(NSDictionary *)result {
    if (requestId == nil) {
        return;
    }
    NSData *json = [NSJSONSerialization dataWithJSONObject:result ?: @{} options:0 error:nil];
    NSString *text = json ? [[NSString alloc] initWithData:json encoding:NSUTF8StringEncoding] : @"null";
    [self script:[NSString stringWithFormat:@"window.nyxoraReply(%ld, %@)", requestId.longValue, text]];
}

// Play: the same as the Play button under the menu (Amethyst asks for the profile or the account when there is none)
- (BOOL)play {
    UINavigationController *launcher = self.navigationController;
    SEL perform = NSSelectorFromString(@"performInstallOrShowDetails:");
    if (![launcher respondsToSelector:perform]) {
        return NO;
    }
    id button = nil;
    for (NSString *key in @[@"buttonInstall", @"buttonInstallItem"]) {
        @try {
            button = [launcher valueForKey:key];
        } @catch (NSException *e) {
            button = nil;
        }
        if (button) {
            break;
        }
    }
    if (!button || ![button isEnabled]) {
        return NO;  // the versions are still loading
    }
    ((void (*)(id, SEL, id))[launcher methodForSelector:perform])(launcher, perform, button);
    return YES;
}

// {account: {name, demo, kind, skin, cape, slim}, profile: {name, version, look}, worn}
- (void)stateWithReply:(void (^)(NSDictionary *))reply {
    NSMutableDictionary *state = [NSMutableDictionary new];
    state[@"worn"] = NyxoraLook.worn;
    PLProfiles *profiles = PLProfiles.current;
    NSString *profileName = only(profiles.selectedProfileName, NSString.class);
    if (profileName.length) {
        NSString *version = only(only(profiles.profiles[profileName], NSDictionary.class)[@"lastVersionId"], NSString.class) ?: @"";
        state[@"profile"] = @{@"name": profileName, @"version": version, @"look": @([NyxoraLook fitsVersion:version])};
    }
    NSDictionary *auth = BaseAuthenticator.current.authData;
    NSString *username = only(auth[@"username"], NSString.class);
    if (!username.length) {
        reply(state);
        return;
    }
    // Amethyst's accounts: Microsoft with the game, or the demo ("Demo.<name>") for an account without it
    BOOL demo = [username hasPrefix:@"Demo."];
    NSString *kind = demo ? @"demo" : auth[@"xboxGamertag"] ? @"microsoft" : @"local";
    NSMutableDictionary *account = [@{@"name": demo ? [username substringFromIndex:5] : username,
                                      @"demo": @(demo), @"kind": kind} mutableCopy];
    state[@"account"] = account;
    NSString *uuid = [only(auth[@"profileId"], NSString.class) stringByReplacingOccurrencesOfString:@"-" withString:@""];
    if (![kind isEqualToString:@"microsoft"] || uuid.length != 32 || [uuid isEqualToString:@"00000000000000000000000000000000"]) {
        reply(state);
        return;
    }
    [self texturesOf:uuid reply:^(NSDictionary *textures) {
        [account addEntriesFromDictionary:textures];
        reply(state);
    }];
}

#pragma mark - the skin and the cape of a Microsoft profile

- (NSString *)texturesFile:(NSString *)uuid {
    NSString *folder = [NyxoraLook.folder stringByAppendingPathComponent:@"skins"];
    [NSFileManager.defaultManager createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:nil];
    return [folder stringByAppendingPathComponent:[uuid stringByAppendingPathExtension:@"json"]];
}

// from Mojang's session server once per start of the app; without the internet (JIT is turned on in flight mode) the
// last ones saved
- (void)texturesOf:(NSString *)uuid reply:(void (^)(NSDictionary *))reply {
    NSDictionary *known = self.textures[uuid];
    if (known) {
        reply(known);
        return;
    }
    NSString *address = [@"https://sessionserver.mojang.com/session/minecraft/profile/" stringByAppendingString:uuid];
    [[NSURLSession.sharedSession dataTaskWithURL:[NSURL URLWithString:address]
                               completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        NSDictionary *textures = nil;
        NSDictionary *profile = data ? only([NSJSONSerialization JSONObjectWithData:data options:0 error:nil], NSDictionary.class) : nil;
        for (NSDictionary *property in only(profile[@"properties"], NSArray.class)) {
            if ([only(property, NSDictionary.class)[@"name"] isEqual:@"textures"] && only(property[@"value"], NSString.class)) {
                NSData *decoded = [[NSData alloc] initWithBase64EncodedString:property[@"value"]
                                                                      options:NSDataBase64DecodingIgnoreUnknownCharacters];
                NSDictionary *value = decoded ? only([NSJSONSerialization JSONObjectWithData:decoded options:0 error:nil], NSDictionary.class) : nil;
                textures = only(value[@"textures"], NSDictionary.class);
            }
        }
        if (!profile) {
            // no answer: the saved ones
            NSData *saved = [NSData dataWithContentsOfFile:[self texturesFile:uuid]];
            NSDictionary *found = saved ? only([NSJSONSerialization JSONObjectWithData:saved options:0 error:nil], NSDictionary.class) : nil;
            dispatch_async(dispatch_get_main_queue(), ^{
                reply(found ?: @{});
            });
            return;
        }
        NSDictionary *skin = only(textures[@"SKIN"], NSDictionary.class), *cape = only(textures[@"CAPE"], NSDictionary.class);
        NSMutableDictionary *found = [NSMutableDictionary new];
        found[@"slim"] = @([only(skin[@"metadata"], NSDictionary.class)[@"model"] isEqual:@"slim"]);
        NSString *skinPng = [self dataURLOf:only(skin[@"url"], NSString.class)];
        NSString *capePng = [self dataURLOf:only(cape[@"url"], NSString.class)];
        if (skinPng) {
            found[@"skin"] = skinPng;
        }
        if (capePng) {
            found[@"cape"] = capePng;
        }
        if (skin && !skinPng) {
            // the profile came but its skin did not: the saved ones, and another try next time
            NSData *saved = [NSData dataWithContentsOfFile:[self texturesFile:uuid]];
            NSDictionary *last = saved ? only([NSJSONSerialization JSONObjectWithData:saved options:0 error:nil], NSDictionary.class) : nil;
            dispatch_async(dispatch_get_main_queue(), ^{
                reply(last ?: found);
            });
            return;
        }
        [[NSJSONSerialization dataWithJSONObject:found options:0 error:nil] writeToFile:[self texturesFile:uuid] atomically:YES];
        dispatch_async(dispatch_get_main_queue(), ^{
            self.textures[uuid] = found;
            reply(found);
        });
    }] resume];
}

// a texture of textures.minecraft.net as a data URL the menu can draw (called off the main thread)
- (NSString *)dataURLOf:(NSString *)address {
    if (!address.length) {
        return nil;
    }
    NSURLComponents *parts = [NSURLComponents componentsWithString:address];
    if (![parts.host isEqualToString:@"textures.minecraft.net"]) {
        return nil;
    }
    parts.scheme = @"https";
    NSData *png = [NSData dataWithContentsOfURL:parts.URL];
    if (png.length < 8 || png.length > 1024 * 1024) {
        return nil;
    }
    return [@"data:image/png;base64," stringByAppendingString:[png base64EncodedStringWithOptions:0]];
}

@end
