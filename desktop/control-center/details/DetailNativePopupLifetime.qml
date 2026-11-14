// SPDX-License-Identifier: MIT
import QtQuick
import "NativeMenus" as Native

DetailPopupLifetime {
    id: lifetime
    dismissNative: () => nativeCloser.closeFor(lifetime.ownerWindow)
    property Native.MenuCloser nativeCloser: Native.MenuCloser {}
}
