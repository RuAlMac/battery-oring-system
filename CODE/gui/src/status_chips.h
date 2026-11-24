#ifndef STATUS_CHIPS_H
#define STATUS_CHIPS_H

/******************<Summary>*****************
 * Name: status_chips.h
 * Purpose:
 *      > A row of small colored badges ("Overall: GOOD", "Cells: WARNING", ...)
 *      > Each badge is colored by its own level (green / yellow / orange / red)
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QLabel>
#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

class QHBoxLayout;

class StatusChips : public QWidget {
    Q_OBJECT

public:
    explicit StatusChips(QWidget* parent = nullptr);

    void setChips(const QList<QPair<QString, int>>& chips);
    void showText(const QString& text);
    void clear();

private:
    QHBoxLayout* m_layout;
    QList<QLabel*> m_labels;

    QLabel* labelAt(int index);
    static QString chipStyle(int level);
};

#endif // STATUS_CHIPS_H
