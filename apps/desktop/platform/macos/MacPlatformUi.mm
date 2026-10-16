/*
 * Author: Runor Ewhro
 * Description: keeps a borderless material window attached just below the
 *              transparent panel, using Liquid Glass when built with a macOS
 *              26 SDK on macOS 26 or later and a behind-window blur otherwise,
 *              so Qt's own view hierarchy is never touched; the panel's open
 *              and close animation scales and fades it.
 */
#include "MacPlatformUi.hpp"

#import <AppKit/AppKit.h>

#include <QSurfaceFormat>
#include <QWindow>

namespace kstack::desktop {

struct MacPlatformUi::Backdrop {
    NSPanel* panel = nil;
    qreal opacity = 1.0;
    qreal scale = 1.0;
};

namespace {
using Backdrop = MacPlatformUi::Backdrop;

// Qt may replace the native window behind a QWindow, so look it up each time
NSWindow* hostWindowOf(QWindow* window) {
    NSView* view = (__bridge NSView*)reinterpret_cast<void*>(window->winId());
    return view.window;
}

// the behind-window blur ignores layer corner radii, so its shape comes from
// a stretchable rounded mask image instead
NSImage* roundedMask(CGFloat radius) {
    const CGFloat edge = radius * 2 + 1;
    NSImage* mask = [NSImage imageWithSize:NSMakeSize(edge, edge)
                                   flipped:NO
                            drawingHandler:^BOOL(NSRect rect) {
                                [NSColor.blackColor setFill];
                                [[NSBezierPath bezierPathWithRoundedRect:rect
                                                                 xRadius:radius
                                                                 yRadius:radius] fill];
                                return YES;
                            }];
    mask.capInsets = NSEdgeInsetsMake(radius, radius, radius, radius);
    mask.resizingMode = NSImageResizingModeStretch;
    return mask;
}

// NSGlassEffectView arrived in macOS 26, but macOS runs apps linked against an
// older SDK in compatibility mode, where the glass draws nothing; use it only
// when this build saw a macOS 26 SDK and the running system has it
NSView* makeMaterialView(NSRect bounds, CGFloat radius) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
    if (Class glassClass = NSClassFromString(@"NSGlassEffectView")) {
        NSView* glass = [[glassClass alloc] initWithFrame:bounds];
        [glass setValue:@(radius) forKey:@"cornerRadius"];
        // regular glass with a window-colored tint keeps text readable over
        // busy wallpapers; the system color follows light and dark on its own
        [glass setValue:@0 forKey:@"style"];
        [glass setValue:[NSColor.windowBackgroundColor colorWithAlphaComponent:0.55]
                 forKey:@"tintColor"];
        return glass;
    }
#endif
    NSVisualEffectView* blur = [[NSVisualEffectView alloc] initWithFrame:bounds];
    blur.material = NSVisualEffectMaterialPopover;
    blur.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    blur.state = NSVisualEffectStateActive;
    blur.maskImage = roundedMask(radius);
    return blur;
}

// size the material to the host frame scaled about its centre, matching the
// QML content scale, and refresh the shadow once the motion settles
void applyReveal(NSWindow* host, const std::shared_ptr<Backdrop>& backdrop) {
    const NSRect frame = host.frame;
    const CGFloat dx = frame.size.width * (1.0 - backdrop->scale) / 2.0;
    const CGFloat dy = frame.size.height * (1.0 - backdrop->scale) / 2.0;
    NSPanel* panel = backdrop->panel;
    panel.alphaValue = backdrop->opacity;
    [panel setFrame:NSInsetRect(frame, dx, dy) display:YES];
    if (backdrop->scale == 1.0) {
        [panel invalidateShadow];
    }
}

// match the host's frame and level, then hang the backdrop just below it; the
// host gives up its own shadow, which would trace the transparent content
void attachBelow(QWindow* window, const std::shared_ptr<Backdrop>& backdrop) {
    NSWindow* host = hostWindowOf(window);
    if (!host) {
        return;
    }
    host.opaque = NO;
    host.backgroundColor = NSColor.clearColor;
    host.hasShadow = NO;
    // the panel animates itself out when the app loses focus, so AppKit must
    // not snap it away first
    if ([host isKindOfClass:NSPanel.class]) {
        static_cast<NSPanel*>(host).hidesOnDeactivate = NO;
    }
    NSPanel* panel = backdrop->panel;
    panel.level = host.level;
    if (panel.parentWindow != host) {
        [panel.parentWindow removeChildWindow:panel];
        [host addChildWindow:panel ordered:NSWindowBelow];
        [panel orderWindow:NSWindowBelow relativeTo:host.windowNumber];
    }
    applyReveal(host, backdrop);
}

void detach(const std::shared_ptr<Backdrop>& backdrop) {
    [backdrop->panel.parentWindow removeChildWindow:backdrop->panel];
    [backdrop->panel orderOut:nil];
}
} // namespace

MacPlatformUi::MacPlatformUi(QObject* parent) : PlatformUi(parent) {}

MacPlatformUi::~MacPlatformUi() {
    if (backdrop_) {
        detach(backdrop_);
    }
}

bool MacPlatformUi::attachMaterial(QWindow* window, qreal cornerRadius) {
    if (!window || backdrop_) {
        return false;
    }
    window_ = window;
    // Qt Quick only clears to transparent when the surface carries alpha
    QSurfaceFormat format = window->format();
    format.setAlphaBufferSize(8);
    window->setFormat(format);

    const NSRect bounds = NSMakeRect(0, 0, window->width(), window->height());
    auto backdrop = std::make_shared<Backdrop>();
    backdrop->panel = [[NSPanel alloc]
        initWithContentRect:bounds
                  styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                    backing:NSBackingStoreBuffered
                      defer:NO];
    NSPanel* panel = backdrop->panel;
    panel.releasedWhenClosed = NO;
    panel.opaque = NO;
    panel.backgroundColor = NSColor.clearColor;
    panel.hasShadow = YES;
    panel.ignoresMouseEvents = YES;
    panel.hidesOnDeactivate = NO;
    panel.contentView = makeMaterialView(bounds, cornerRadius);
    backdrop_ = backdrop;

    const auto follow = [window, backdrop] {
        if (window->isVisible()) {
            attachBelow(window, backdrop);
        }
    };
    connect(window, &QWindow::visibleChanged, this, [window, backdrop](bool visible) {
        if (!visible) {
            detach(backdrop);
            return;
        }
        attachBelow(window, backdrop);
        // the native window is ordered front after this signal, so attach
        // again once AppKit has finished showing it
        dispatch_async(dispatch_get_main_queue(), ^{
            if (window->isVisible()) {
                attachBelow(window, backdrop);
            }
        });
    });
    connect(window, &QWindow::xChanged, this, follow);
    connect(window, &QWindow::yChanged, this, follow);
    connect(window, &QWindow::widthChanged, this, follow);
    connect(window, &QWindow::heightChanged, this, follow);
    connect(window, &QObject::destroyed, this, [this] {
        detach(backdrop_);
        backdrop_.reset();
        window_ = nullptr;
    });
    setNativeMaterial(true);
    return true;
}

void MacPlatformUi::setReveal(qreal opacity, qreal scale) {
    if (!backdrop_ || !window_) {
        return;
    }
    backdrop_->opacity = opacity;
    backdrop_->scale = scale;
    if (window_->isVisible()) {
        if (NSWindow* host = hostWindowOf(window_)) {
            applyReveal(host, backdrop_);
        }
    }
}

bool MacPlatformUi::reduceMotion() const {
    return NSWorkspace.sharedWorkspace.accessibilityDisplayShouldReduceMotion;
}

// Qt's palette reports the pale text-selection blue, not the accent itself
QColor MacPlatformUi::accentColor() const {
    NSColor* accent = [NSColor.controlAccentColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    return QColor::fromRgbF(static_cast<float>(accent.redComponent),
                            static_cast<float>(accent.greenComponent),
                            static_cast<float>(accent.blueComponent));
}

} // namespace kstack::desktop
