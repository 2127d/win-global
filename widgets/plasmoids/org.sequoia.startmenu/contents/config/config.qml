import org.kde.plasma.configuration

ConfigModel {
    ConfigCategory { name: i18n("Start menu"); icon: "start-here-kde"; source: "ConfigGeneral.qml" }
    ConfigCategory { name: i18n("Pinned apps"); icon: "pin"; source: "ConfigPinned.qml" }
    ConfigCategory { name: i18n("Animations"); icon: "preferences-desktop-effects"; source: "ConfigAnimations.qml" }
}
