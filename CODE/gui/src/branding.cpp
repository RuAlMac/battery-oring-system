#include "branding.h"

#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>

namespace branding {

/*******renderLogo*******
 * Description
        > renders an SVG graphic as a square pixmap on a white rounded tile (so
            dark artwork stays visible on light and dark themes); the empty
            margin around the artwork is trimmed so it fills the tile
 * Arguments
        > const QString& resourcePath: resource path, for example ":/mil_logo_light.svg"
        > int sizePx: side length in device-independent pixels
        > qreal devicePixelRatio: screen scale factor, so the result is sharp on high-DPI screens
 * Returns
        > QPixmap: rendered graphic (null pixmap if the SVG could not be loaded)
*/
QPixmap renderLogo(const QString& resourcePath, int sizePx, qreal devicePixelRatio){
    QSvgRenderer renderer(resourcePath);
    if (!renderer.isValid()){return QPixmap();}

    // the supplied SVGs are 4950 x 4950 with the artwork in the middle; crop to a centered square around it
    renderer.setViewBox(QRectF(560, 560, 3830, 3830));

    int px = qMax(1, (int)(sizePx * devicePixelRatio));
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(devicePixelRatio);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF tile(0, 0, sizePx, sizePx);
    QPainterPath path;
    path.addRoundedRect(tile, sizePx * 0.16, sizePx * 0.16);
    p.fillPath(path, Qt::white);

    renderer.render(&p, tile.adjusted(sizePx * 0.04, sizePx * 0.04, -sizePx * 0.04, -sizePx * 0.04));
    return pm;
}



/*******renderThemedLogo*******
 * Description
        > renders the light-mode or dark-mode version of a team graphic (for example ":/mil_logo_light.svg")
            as a square, transparent pixmap. These versions already include their own frame and are drawn
            to fit the light or dark window, so no background tile is added
 * Arguments
        > const QString& baseName: resource name without theme and extension, such as "mil_logo" or "radm_sprite"
        > bool dark: true=dark-mode version, false=light-mode version
        > int sizePx: side length in device-independent pixels
        > qreal devicePixelRatio: screen scale factor, so the result is sharp on high-DPI screens
 * Returns
        > QPixmap: rendered graphic (null pixmap if the SVG could not be loaded)
*/
QPixmap renderThemedLogo(const QString& baseName, bool dark, int sizePx, qreal devicePixelRatio){
    QSvgRenderer renderer(QStringLiteral(":/%1_%2.svg").arg(baseName, dark ? QStringLiteral("dark") : QStringLiteral("light")));
    if (!renderer.isValid()){return QPixmap();}

    int px = qMax(1, (int)(sizePx * devicePixelRatio));
    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(devicePixelRatio);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&p, QRectF(0, 0, sizePx, sizePx));
    return pm;
}



/*******isDarkTheme*******
 * Description
        > tells whether a palette belongs to a dark theme (window background darker than the text)
 * Arguments
        > const QPalette& palette: palette to check, normally the window's
 * Returns
        > bool: true=dark theme, false=light theme
*/
bool isDarkTheme(const QPalette& palette){
    return palette.color(QPalette::Window).lightness() < palette.color(QPalette::WindowText).lightness();
}



/*******appIcon*******
 * Description
        > builds the application / window icon from mil_logo_light.svg at several sizes
 * Arguments
        > N/A
 * Returns
        > QIcon: icon (empty if the SVG could not be loaded)
*/
QIcon appIcon(){
    QIcon icon;

    for (int size : {16, 24, 32, 48, 64, 128, 256}){
        QPixmap pm = renderLogo(QStringLiteral(":/mil_logo_light.svg"), size, 1.0);
        if (!pm.isNull()){icon.addPixmap(pm);}
    }

    return icon;
}

}   // namespace branding
