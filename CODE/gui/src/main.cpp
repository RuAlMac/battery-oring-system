#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QStringList>

#include "branding.h"
#include "main_window.h"

/*******main*******
 * Description
        > starts the Qt application and shows the main window
 * Arguments
        > int argc: argument count
        > char* argv[]: arguments
 * Returns
        > int: exit code
*/
int main(int argc, char* argv[]){
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MIL"));
    QCoreApplication::setApplicationName(QStringLiteral("SubjuGatorBatteryTool"));

    // Cascadia Mono (variable weight) is embedded in the executable and used for the whole GUI.
    // Some Qt versions do not expose the variable font's Bold instance, so the static Bold
    // is registered as a fallback only when Bold is missing.
    const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/CascadiaMono-VariableFont_wght.ttf"));
    if (fontId >= 0){
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()){
            const QString family = families.first();
            if (!QFontDatabase::styles(family).contains(QStringLiteral("Bold"))){
                QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/CascadiaMono-Bold.ttf"));
            }
            QFont f(family);
            f.setStretch(QFont::Unstretched);
            f.setPointSizeF(app.font().pointSizeF() > 0 ? app.font().pointSizeF() : 10.0);
            app.setFont(f);
        }
    }

    app.setWindowIcon(branding::appIcon());

    MainWindow window;
    window.show();

    return app.exec();
}
