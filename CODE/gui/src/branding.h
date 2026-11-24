#ifndef BRANDING_H
#define BRANDING_H

/******************<Summary>*****************
 * Name: branding.h
 * Purpose:
 *      > Renders the two team graphics (SVG, built into the program as Qt
 *          resources) for the top-right corner of the window, and builds the
 *          application icon from mil_logo_light.svg
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QIcon>
#include <QPalette>
#include <QPixmap>
#include <QString>

namespace branding {

QPixmap renderLogo(const QString& resourcePath, int sizePx, qreal devicePixelRatio);
QPixmap renderThemedLogo(const QString& baseName, bool dark, int sizePx, qreal devicePixelRatio);
bool isDarkTheme(const QPalette& palette);
QIcon appIcon();

}   // namespace branding

#endif // BRANDING_H
