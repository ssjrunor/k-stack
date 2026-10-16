/*
 * Author: Runor Ewhro
 * Description: keeps a borderless material window attached just below a
 *              transparent Qt Quick window, using Liquid Glass when built
 *              with a macOS 26 SDK on macOS 26 or later and a behind-window
 *              blur otherwise, so Qt's own view hierarchy is never touched;
 *              QML drives its opacity and scale during open and close.
 */
#include "NativeMaterial.hpp"

#import <AppKit/AppKit.h>

#include <QSurfaceFormat>
#include <QWindow>

#include <memory>
#include <unordered_map>

namespace kstack::desktop {

namespace {
struct Backdrop {
    NSPanel* panel = nil;
    qreal opacity = 1.0;
    qreal scale = 1.0;
};

std::unordered_map<QWindow*, std::shared_ptr<Backdrop>>& backdrops() {
    static std::unordered_map<QWindow*, std::shared_ptr<Backdrop>> registry;
    return registry;
}

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
NSView* makeMaterialView(NSRect bounds, CGFloat radius, NativeMaterial* material) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
    if (Class glassClass = NSClassFromString(@"NSGlassEffectView")) {
        NSView* glass = [[glassClass alloc] initWithFrame:bounds];
        [glass setValue:@(radius) forKey:@"cornerRadius"];
        *material = NativeMaterial::Glass;
        return glass;
    }
#endif
    NSVisualEffectView* blur = [[NSVisualEffectView alloc] initWithFrame:bounds];
    blur.material = NSVisualEffectMaterialPopover;
    blur.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    blur.state = NSVisualEffectStateActive;
    blur.maskImage = roundedMask(radius);
    *material = NativeMaterial::Blur;
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

NativeMaterial attachNativeMaterial(QWindow* window, qreal cornerRadius) {
    if (!window) {
        return NativeMaterial::None;
    }
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
    NativeMaterial material = NativeMaterial::None;
    panel.contentView = makeMaterialView(bounds, cornerRadius, &material);

    const auto follow = [window, backdrop] {
        if (window->isVisible()) {
            attachBelow(window, backdrop);
        }
    };
    QObject::connect(window, &QWindow::visibleChanged, window, [window, backdrop](bool visible) {
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
    QObject::connect(window, &QWindow::xChanged, window, follow);
    QObject::connect(window, &QWindow::yChanged, window, follow);
    QObject::connect(window, &QWindow::widthChanged, window, follow);
    QObject::connect(window, &QWindow::heightChanged, window, follow);
    QObject::connect(window, &QObject::destroyed, [window, backdrop] {
        detach(backdrop);
        backdrops().erase(window);
    });
    backdrops()[window] = backdrop;
    return material;
}

NativeMaterialBridge::NativeMaterialBridge(QWindow* window, QObject* parent)
    : QObject(parent), window_(window) {}

void NativeMaterialBridge::setReveal(qreal opacity, qreal scale) {
    const auto found = backdrops().find(window_);
    if (found == backdrops().end()) {
        return;
    }
    found->second->opacity = opacity;
    found->second->scale = scale;
    if (window_->isVisible()) {
        if (NSWindow* host = hostWindowOf(window_)) {
            applyReveal(host, found->second);
        }
    }
}

bool NativeMaterialBridge::reduceMotion() const {
    return NSWorkspace.sharedWorkspace.accessibilityDisplayShouldReduceMotion;
}

} // namespace kstack::desktop
