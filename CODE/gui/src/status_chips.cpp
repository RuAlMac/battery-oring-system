#include "status_chips.h"

#include <QHBoxLayout>

#include "gauges.h"
#include "protocol.h"

/*******StatusChips::StatusChips*******
 * Description
        > creates an empty, centered row of badges
 * Arguments
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
StatusChips::StatusChips(QWidget* parent) : QWidget(parent) {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(10);
    m_layout->addStretch(1);
    m_layout->addStretch(1);
}



/*******StatusChips::labelAt*******
 * Description
        > returns the badge label at an index, creating it if needed
 * Arguments
        > int index: badge position
 * Returns
        > QLabel*: the label
*/
QLabel* StatusChips::labelAt(int index){
    while (m_labels.size() <= index){
        auto* l = new QLabel(this);
        l->setAlignment(Qt::AlignCenter);
        m_layout->insertWidget(m_layout->count() - 1, l);
        m_labels.append(l);
    }
    return m_labels[index];
}



/*******StatusChips::chipStyle*******
 * Description
        > builds the style sheet of one badge
 * Arguments
        > int level: 0=good 1=warning 2=danger 3=shutdown, anything else = neutral
 * Returns
        > QString: style sheet text
*/
QString StatusChips::chipStyle(int level){
    QString fg = (level == 1) ? QStringLiteral("black") : QStringLiteral("white");
    return QStringLiteral("background-color: %1; color: %2; border-radius: 6px; padding: 3px 12px; font-weight: bold;")
        .arg(levelColor(level).name(), fg);
}



/*******StatusChips::setChips*******
 * Description
        > shows one colored badge per entry
 * Arguments
        > const QList<QPair<QString, int>>& chips: (name, level) pairs, in display order
 * Returns
        > N/A
*/
void StatusChips::setChips(const QList<QPair<QString, int>>& chips){
    for (int i = 0; i < chips.size(); ++i){
        QLabel* l = labelAt(i);
        l->setText(QStringLiteral("%1: %2").arg(chips[i].first, proto::levelName(chips[i].second)));
        l->setStyleSheet(chipStyle(chips[i].second));
        l->show();
    }
    for (int i = chips.size(); i < m_labels.size(); ++i){m_labels[i]->hide();}
}



/*******StatusChips::showText*******
 * Description
        > shows a single neutral badge with a message
 * Arguments
        > const QString& text: message
 * Returns
        > N/A
*/
void StatusChips::showText(const QString& text){
    QLabel* l = labelAt(0);
    l->setText(text);
    l->setStyleSheet(chipStyle(-1));
    l->show();
    for (int i = 1; i < m_labels.size(); ++i){m_labels[i]->hide();}
}



/*******StatusChips::clear*******
 * Description
        > hides all badges
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void StatusChips::clear(){
    for (QLabel* l : m_labels){l->hide();}
}
