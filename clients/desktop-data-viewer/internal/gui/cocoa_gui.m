#import <Cocoa/Cocoa.h>
#import "cocoa_gui.h"

static NSTextField *globalLabel = nil;

// ColorFromHex converts a 3 or 6-digit hex string into an NSColor.
static NSColor* ColorFromHex(const char* hexStr) {
    NSString *hex = [NSString stringWithUTF8String:hexStr];
    if ([hex hasPrefix:@"#"]) {
        hex = [hex substringFromIndex:1];
    }
    
    unsigned int rgb = 0;
    NSScanner *scanner = [NSScanner scannerWithString:hex];
    [scanner scanHexInt:&rgb];
    
    float r, g, b;
    if ([hex length] == 3) {
        r = ((rgb & 0xF00) >> 8) / 15.0;
        g = ((rgb & 0x0F0) >> 4) / 15.0;
        b = (rgb & 0x00F) / 15.0;
    } else {
        r = ((rgb & 0xFF0000) >> 16) / 255.0;
        g = ((rgb & 0x00FF00) >> 8) / 255.0;
        b = (rgb & 0x0000FF) / 255.0;
    }
    return [NSColor colorWithSRGBRed:r green:g blue:b alpha:1.0];
}

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (assign) int width;
@property (assign) int height;
@property (assign) int x;
@property (assign) int y;
@property (assign) int fontSize;
@property (strong) NSString *colorHex;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
    int windowX = self.x;
    int windowY = self.y;
    
    if (self.x <= 0 && self.y <= 0) {
        int margin = 20;
        windowX = screenFrame.origin.x + screenFrame.size.width - self.width - margin;
        windowY = screenFrame.origin.y + screenFrame.size.height - self.height - margin;
    }
    
    NSRect frame = NSMakeRect(windowX, windowY, self.width, self.height);
    
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:NSWindowStyleMaskBorderless
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [window setOpaque:NO];
    [window setBackgroundColor:[NSColor clearColor]];
    [window setMovableByWindowBackground:YES];
    [window setHasShadow:NO];
    [window setLevel:NSNormalWindowLevel];
    [window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                                   NSWindowCollectionBehaviorStationary |
                                   NSWindowCollectionBehaviorIgnoresCycle];
    
    NSTextField *label = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, self.width, self.height)];
    [label setEditable:NO];
    [label setSelectable:NO];
    [label setBordered:NO];
    [label setBezeled:NO];
    [label setDrawsBackground:NO];
    [label setAlignment:NSTextAlignmentCenter];
    
    [label setFont:[NSFont systemFontOfSize:self.fontSize weight:NSFontWeightBold]];
    [label setTextColor:ColorFromHex([self.colorHex UTF8String])];
    [label setStringValue:@"Connecting..."];
    
    globalLabel = label;
    
    [[window contentView] addSubview:label];
    [window makeKeyAndOrderFront:nil];
}

@end

void StartCocoaApp(int width, int height, int x, int y, int fontSize, const char* colorHex) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        
        AppDelegate *delegate = [[AppDelegate alloc] init];
        delegate.width = width;
        delegate.height = height;
        delegate.x = x;
        delegate.y = y;
        delegate.fontSize = fontSize;
        delegate.colorHex = [NSString stringWithUTF8String:colorHex];
        
        [app setDelegate:delegate];
        [app run];
    }
}

void UpdateWidgetText(const char* text) {
    if (globalLabel == nil) return;
    NSString *nsText = [NSString stringWithUTF8String:text];
    dispatch_async(dispatch_get_main_queue(), ^{
        [globalLabel setStringValue:nsText];
    });
}
