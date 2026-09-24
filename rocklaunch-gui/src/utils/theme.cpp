#include "theme.h"

// Catppuccin Mocha values (https://catppuccin.com, dark).
const Theme::Palette Theme::kMocha = {
    /* rosewater */ QColor(0xf5, 0xe0, 0xdc),
    /* flamingo  */ QColor(0xf2, 0xcd, 0xcd),
    /* pink      */ QColor(0xf5, 0xc2, 0xe7),
    /* mauve     */ QColor(0xcb, 0xa6, 0xf7),
    /* red       */ QColor(0xf3, 0x8b, 0xa8),
    /* maroon    */ QColor(0xeb, 0xa0, 0xac),
    /* peach     */ QColor(0xfa, 0xb3, 0x87),
    /* yellow    */ QColor(0xf9, 0xe2, 0xaf),
    /* green     */ QColor(0xa6, 0xe3, 0xa1),
    /* teal      */ QColor(0x94, 0xe2, 0xd5),
    /* sky       */ QColor(0x89, 0xdc, 0xeb),
    /* sapphire  */ QColor(0x74, 0xc7, 0xec),
    /* blue      */ QColor(0x89, 0xb4, 0xfa),
    /* lavender  */ QColor(0xb4, 0xbe, 0xfe),
    /* text      */ QColor(0xcd, 0xd6, 0xf4),
    /* subtext1  */ QColor(0xba, 0xc2, 0xde),
    /* subtext0  */ QColor(0xa6, 0xad, 0xc8),
    /* overlay2  */ QColor(0x93, 0x99, 0xb2),
    /* overlay1  */ QColor(0x7f, 0x84, 0x9c),
    /* overlay0  */ QColor(0x6c, 0x70, 0x86),
    /* surface2  */ QColor(0x58, 0x5b, 0x70),
    /* surface1  */ QColor(0x45, 0x47, 0x5a),
    /* surface0  */ QColor(0x31, 0x32, 0x44),
    /* base      */ QColor(0x1e, 0x1e, 0x2e),
    /* mantle    */ QColor(0x18, 0x18, 0x25),
    /* crust     */ QColor(0x11, 0x11, 0x1b),
};

// Catppuccin Latte values (https://catppuccin.com, light).
const Theme::Palette Theme::kLatte = {
    /* rosewater */ QColor(0xdc, 0x8a, 0x78),
    /* flamingo  */ QColor(0xdd, 0x78, 0x78),
    /* pink      */ QColor(0xea, 0x76, 0xcb),
    /* mauve     */ QColor(0x88, 0x39, 0xef),
    /* red       */ QColor(0xd2, 0x0f, 0x39),
    /* maroon    */ QColor(0xe6, 0x45, 0x53),
    /* peach     */ QColor(0xfe, 0x64, 0x0b),
    /* yellow    */ QColor(0xdf, 0x8e, 0x1d),
    /* green     */ QColor(0x40, 0xa0, 0x2b),
    /* teal      */ QColor(0x17, 0x92, 0x99),
    /* sky       */ QColor(0x04, 0xa5, 0xe5),
    /* sapphire  */ QColor(0x20, 0x9f, 0xb5),
    /* blue      */ QColor(0x1e, 0x66, 0xf5),
    /* lavender  */ QColor(0x72, 0x87, 0xfd),
    /* text      */ QColor(0x4c, 0x4f, 0x69),
    /* subtext1  */ QColor(0x5c, 0x5f, 0x77),
    /* subtext0  */ QColor(0x6c, 0x6f, 0x85),
    /* overlay2  */ QColor(0x7c, 0x7f, 0x93),
    /* overlay1  */ QColor(0x8c, 0x8f, 0xa1),
    /* overlay0  */ QColor(0x9c, 0xa0, 0xb0),
    /* surface2  */ QColor(0xac, 0xb0, 0xbe),
    /* surface1  */ QColor(0xbc, 0xc0, 0xcc),
    /* surface0  */ QColor(0xcc, 0xd0, 0xda),
    /* base      */ QColor(0xef, 0xf1, 0xf5),
    /* mantle    */ QColor(0xe6, 0xe9, 0xef),
    /* crust     */ QColor(0xdc, 0xe0, 0xe8),
};

Theme::Theme(QObject *parent)
    : QObject(parent)
{
}

QString Theme::flavor() const
{
    return FlavorName(m_flavor);
}

void Theme::setFlavor(const QString &flavor)
{
    const Flavor next = FlavorFromName(flavor);
    if (m_flavor == next) {
        return;
    }
    m_flavor = next;
    emit flavorChanged();
}

QStringList Theme::Flavors()
{
    return {
        QStringLiteral("latte"),
        QStringLiteral("frappe"),
        QStringLiteral("macchiato"),
        QStringLiteral("mocha"),
        QStringLiteral("system"),
    };
}

QColor Theme::rosewater() const { return PaletteFor(m_flavor).rosewater; }
QColor Theme::flamingo() const { return PaletteFor(m_flavor).flamingo; }
QColor Theme::pink() const { return PaletteFor(m_flavor).pink; }
QColor Theme::mauve() const { return PaletteFor(m_flavor).mauve; }
QColor Theme::red() const { return PaletteFor(m_flavor).red; }
QColor Theme::maroon() const { return PaletteFor(m_flavor).maroon; }
QColor Theme::peach() const { return PaletteFor(m_flavor).peach; }
QColor Theme::yellow() const { return PaletteFor(m_flavor).yellow; }
QColor Theme::green() const { return PaletteFor(m_flavor).green; }
QColor Theme::teal() const { return PaletteFor(m_flavor).teal; }
QColor Theme::sky() const { return PaletteFor(m_flavor).sky; }
QColor Theme::sapphire() const { return PaletteFor(m_flavor).sapphire; }
QColor Theme::blue() const { return PaletteFor(m_flavor).blue; }
QColor Theme::lavender() const { return PaletteFor(m_flavor).lavender; }
QColor Theme::text() const { return PaletteFor(m_flavor).text; }
QColor Theme::subtext1() const { return PaletteFor(m_flavor).subtext1; }
QColor Theme::subtext0() const { return PaletteFor(m_flavor).subtext0; }
QColor Theme::overlay2() const { return PaletteFor(m_flavor).overlay2; }
QColor Theme::overlay1() const { return PaletteFor(m_flavor).overlay1; }
QColor Theme::overlay0() const { return PaletteFor(m_flavor).overlay0; }
QColor Theme::surface2() const { return PaletteFor(m_flavor).surface2; }
QColor Theme::surface1() const { return PaletteFor(m_flavor).surface1; }
QColor Theme::surface0() const { return PaletteFor(m_flavor).surface0; }
QColor Theme::base() const { return PaletteFor(m_flavor).base; }
QColor Theme::mantle() const { return PaletteFor(m_flavor).mantle; }
QColor Theme::crust() const { return PaletteFor(m_flavor).crust; }

const Theme::Palette &Theme::PaletteFor(Flavor flavor)
{
    switch (flavor) {
    case Flavor::Latte:
        return kLatte;
    // Theme selector: fill the remaining palettes when it lands, and map
    // System to QGuiApplication::palette() once FallbackStyle=Fusion is set.
    // case Flavor::Frappe:     return kFrappe;
    // case Flavor::Macchiato:  return kMacchiato;
    // case Flavor::System:     return SystemPalette();
    case Flavor::Mocha:
    default:
        return kMocha;
    }
}

QString Theme::FlavorName(Flavor flavor)
{
    switch (flavor) {
    case Flavor::Latte:     return QStringLiteral("latte");
    case Flavor::Frappe:    return QStringLiteral("frappe");
    case Flavor::Macchiato: return QStringLiteral("macchiato");
    case Flavor::System:    return QStringLiteral("system");
    case Flavor::Mocha:
    default:                return QStringLiteral("mocha");
    }
}

Theme::Flavor Theme::FlavorFromName(const QString &name)
{
    if (name == QStringLiteral("latte")) {
        return Flavor::Latte;
    }
    if (name == QStringLiteral("frappe")) {
        return Flavor::Frappe;
    }
    if (name == QStringLiteral("macchiato")) {
        return Flavor::Macchiato;
    }
    if (name == QStringLiteral("system")) {
        return Flavor::System;
    }
    return Flavor::Mocha; // unknown → dark safe default
}