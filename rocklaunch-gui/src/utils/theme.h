#pragma once

#include <QColor>
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

// Single source of truth for UI colors. Official Catppuccin names keep hexes
// out of QML; NOTIFY flavorChanged lets a future selector swap the palette at
// runtime without reloading components.
class Theme final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString flavor READ flavor NOTIFY flavorChanged)

    // Accents (official Catppuccin order)
    Q_PROPERTY(QColor rosewater READ rosewater NOTIFY flavorChanged)
    Q_PROPERTY(QColor flamingo READ flamingo NOTIFY flavorChanged)
    Q_PROPERTY(QColor pink READ pink NOTIFY flavorChanged)
    Q_PROPERTY(QColor mauve READ mauve NOTIFY flavorChanged)
    Q_PROPERTY(QColor red READ red NOTIFY flavorChanged)
    Q_PROPERTY(QColor maroon READ maroon NOTIFY flavorChanged)
    Q_PROPERTY(QColor peach READ peach NOTIFY flavorChanged)
    Q_PROPERTY(QColor yellow READ yellow NOTIFY flavorChanged)
    Q_PROPERTY(QColor green READ green NOTIFY flavorChanged)
    Q_PROPERTY(QColor teal READ teal NOTIFY flavorChanged)
    Q_PROPERTY(QColor sky READ sky NOTIFY flavorChanged)
    Q_PROPERTY(QColor sapphire READ sapphire NOTIFY flavorChanged)
    Q_PROPERTY(QColor blue READ blue NOTIFY flavorChanged)
    Q_PROPERTY(QColor lavender READ lavender NOTIFY flavorChanged)

    // Monochrome (official Catppuccin order)
    Q_PROPERTY(QColor text READ text NOTIFY flavorChanged)
    Q_PROPERTY(QColor subtext1 READ subtext1 NOTIFY flavorChanged)
    Q_PROPERTY(QColor subtext0 READ subtext0 NOTIFY flavorChanged)
    Q_PROPERTY(QColor overlay2 READ overlay2 NOTIFY flavorChanged)
    Q_PROPERTY(QColor overlay1 READ overlay1 NOTIFY flavorChanged)
    Q_PROPERTY(QColor overlay0 READ overlay0 NOTIFY flavorChanged)
    Q_PROPERTY(QColor surface2 READ surface2 NOTIFY flavorChanged)
    Q_PROPERTY(QColor surface1 READ surface1 NOTIFY flavorChanged)
    Q_PROPERTY(QColor surface0 READ surface0 NOTIFY flavorChanged)
    Q_PROPERTY(QColor base READ base NOTIFY flavorChanged)
    Q_PROPERTY(QColor mantle READ mantle NOTIFY flavorChanged)
    Q_PROPERTY(QColor crust READ crust NOTIFY flavorChanged)

public:
    explicit Theme(QObject *parent);

    static Theme *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(jsEngine);
        Q_ASSERT(s_instance);
        Q_ASSERT(qmlEngine->thread() == s_instance->thread());
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }

    static void setInstance(Theme *instance)
    {
        s_instance = instance;
    }

    QString flavor() const;

    // Test hook until the theme selector lands; unknown names fall back to mocha.
    void setFlavor(const QString &flavor);

    // Selector options, in display order.
    static QStringList Flavors();

    QColor rosewater() const;
    QColor flamingo() const;
    QColor pink() const;
    QColor mauve() const;
    QColor red() const;
    QColor maroon() const;
    QColor peach() const;
    QColor yellow() const;
    QColor green() const;
    QColor teal() const;
    QColor sky() const;
    QColor sapphire() const;
    QColor blue() const;
    QColor lavender() const;
    QColor text() const;
    QColor subtext1() const;
    QColor subtext0() const;
    QColor overlay2() const;
    QColor overlay1() const;
    QColor overlay0() const;
    QColor surface2() const;
    QColor surface1() const;
    QColor surface0() const;
    QColor base() const;
    QColor mantle() const;
    QColor crust() const;

signals:
    void flavorChanged();

private:
    enum class Flavor { Latte, Frappe, Macchiato, Mocha, System };

    // Field order must match the initializer lists in theme.cpp.
    struct Palette
    {
        QColor rosewater, flamingo, pink, mauve, red, maroon, peach;
        QColor yellow, green, teal, sky, sapphire, blue, lavender;
        QColor text, subtext1, subtext0, overlay2, overlay1, overlay0;
        QColor surface2, surface1, surface0, base, mantle, crust;
    };

    static const Palette kMocha;
    static const Palette kLatte;
    static const Palette &PaletteFor(Flavor flavor);
    static QString FlavorName(Flavor flavor);
    static Flavor FlavorFromName(const QString &name);

    Flavor m_flavor = Flavor::Mocha;
    inline static Theme *s_instance = nullptr;
};