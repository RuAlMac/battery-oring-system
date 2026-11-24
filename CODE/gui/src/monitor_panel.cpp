#include "monitor_panel.h"

#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

/*******MonitorPanel::MonitorPanel*******
 * Description
        > builds the banner, the large voltage readout, the battery icon, and the thermometer
 * Arguments
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
MonitorPanel::MonitorPanel(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    // overall status
        m_banner = new QLabel(this);
        m_banner->setAlignment(Qt::AlignCenter);
        QFont bf = m_banner->font();
        bf.setPointSize(bf.pointSize() + 8);
        bf.setBold(true);
        m_banner->setFont(bf);
        m_banner->setMinimumHeight(56);
        root->addWidget(m_banner);

        m_chips = new StatusChips(this);
        root->addWidget(m_chips);

    // shown only while the RP2350 reports it is latched in shutdown
        m_unlatchBtn = new QPushButton(tr("Override shutdown latch (re-check battery once)"), this);
        m_unlatchBtn->setToolTip(tr("Lets the RP2350 leave the latched shutdown once. If the battery is still at shutdown levels "
                                    "the shutdown countdown restarts (or the system shuts down again right away if there is no delay)."));
        m_unlatchBtn->hide();
        connect(m_unlatchBtn, &QPushButton::clicked, this, &MonitorPanel::unlatchRequested);
        root->addWidget(m_unlatchBtn);

    // large battery voltage
        m_voltage = new QLabel(this);
        m_voltage->setAlignment(Qt::AlignCenter);
        QFont vf = m_voltage->font();
        vf.setPointSize(vf.pointSize() + 30);
        vf.setBold(true);
        m_voltage->setFont(vf);
        m_voltage->setMinimumWidth(0);
        root->addWidget(makeBox(tr("Battery Voltage"), m_voltage));

    // gauges
        auto* row = new QHBoxLayout();
        m_battery = new BatteryWidget(this);
        m_thermo = new ThermometerWidget(this);
        row->addWidget(makeBox(tr("Cell Voltages"), m_battery), 5);
        row->addWidget(makeBox(tr("Battery Temperature"), m_thermo), 2);
        root->addLayout(row, 1);

        m_info = new QLabel(this);
        m_info->setAlignment(Qt::AlignCenter);
        root->addWidget(m_info);

    m_battery->setThresholds(m_th);
    m_thermo->setThresholds(m_th);
    setConnectionState(false);
}



/*******MonitorPanel::makeBox*******
 * Description
        > wraps a gauge in a framed box with a centered bold title above it
 * Arguments
        > const QString& title: text shown above the gauge
        > QWidget* content: the gauge widget (becomes a child of the box)
 * Returns
        > QWidget*: the box
*/
QWidget* MonitorPanel::makeBox(const QString& title, QWidget* content){
    auto* frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Plain);
    frame->setLineWidth(1);

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(22, 10, 22, 18);

    auto* label = new QLabel(title, frame);
    label->setAlignment(Qt::AlignCenter);
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * kBoxTitleScale);
    label->setFont(f);

    layout->addWidget(label);
    layout->addWidget(content, 1);
    return frame;
}



/*******MonitorPanel::badgeStyle*******
 * Description
        > builds the style sheet for a colored badge (readable on light and dark themes)
 * Arguments
        > int level: 0=good 1=warning 2=danger 3=shutdown, anything else = no data
 * Returns
        > QString: style sheet text
*/
QString MonitorPanel::badgeStyle(int level){
    QString fg = (level == 1) ? QStringLiteral("black") : QStringLiteral("white");
    return QStringLiteral("background-color: %1; color: %2; border-radius: 6px; padding: 4px;").arg(levelColor(level).name(), fg);
}



/*******MonitorPanel::showNoData*******
 * Description
        > blanks the display and shows a message in the banner
 * Arguments
        > const QString& text: banner text
 * Returns
        > N/A
*/
void MonitorPanel::showNoData(const QString& text){
    m_haveFrame = false;
    m_banner->setText(text);
    m_banner->setStyleSheet(badgeStyle(-1));
    m_voltage->setText(QStringLiteral("- V"));
    m_voltage->setStyleSheet(QString());
    m_chips->clear();
    m_unlatchBtn->hide();
    m_info->clear();

    const double zeros[proto::kNumCells] = {0, 0, 0, 0, 0};
    m_battery->setCells(zeros, 0, false);
    m_thermo->setTemperature(0.0, false, false);
}



/*******MonitorPanel::setReadOnly*******
 * Description
        > read-only connections cannot send commands, so the latch override button is never shown
 * Arguments
        > bool readOnly: true=read-only connection
 * Returns
        > N/A
*/
void MonitorPanel::setReadOnly(bool readOnly){
    m_readOnly = readOnly;
    if (readOnly){m_unlatchBtn->hide();}
}



/*******MonitorPanel::setConnectionState*******
 * Description
        > tells the panel whether the RP2350 is connected
 * Arguments
        > bool connected: true=port open
 * Returns
        > N/A
*/
void MonitorPanel::setConnectionState(bool connected){
    m_connected = connected;
    showNoData(connected ? tr("WAITING FOR DATA") : tr("NOT CONNECTED"));
}



/*******MonitorPanel::setActiveSettings*******
 * Description
        > takes the RP2350's active thresholds (from the settings list) for the colors and zones
 * Arguments
        > const QMap<QString, double>& values: setting name -> active value
 * Returns
        > N/A
*/
void MonitorPanel::setActiveSettings(const QMap<QString, double>& values){
    auto get = [&](const char* name, double fallback){return values.value(QLatin1String(name), fallback);};

    m_th.cellWarn = get("warn_cell_voltage", m_th.cellWarn);
    m_th.cellDanger = get("danger_cell_voltage", m_th.cellDanger);
    m_th.cellShutdown = get("shtdwn_cell_voltage", m_th.cellShutdown);
    m_th.tempWarn = get("warn_temp", m_th.tempWarn);
    m_th.tempDanger = get("danger_temp", m_th.tempDanger);
    m_th.tempShutdown = get("shtdwn_temp", m_th.tempShutdown);

    m_battery->setThresholds(m_th);
    m_thermo->setThresholds(m_th);
}



/*******MonitorPanel::onFrame*******
 * Description
        > updates the banner, voltage, battery icon, and thermometer from an S or T frame
 * Arguments
        > const proto::BatteryFrame& f: frame
 * Returns
        > N/A
*/
void MonitorPanel::onFrame(const proto::BatteryFrame& f){
    m_haveFrame = true;

    QString bannerText = proto::levelName(f.level);
    if (f.cdActive){bannerText += tr("  -  SHUTDOWN IN %1 s").arg(f.cdMs / 1000.0, 0, 'f', 1);}
    if (f.latched){bannerText += tr("  (LATCHED)");}
    m_banner->setText(bannerText);
    m_unlatchBtn->setVisible(f.latched && !m_readOnly);
    m_banner->setStyleSheet(badgeStyle(f.level));

    m_chips->setChips({{tr("Cells"), f.cellLevel}, {tr("Battery"), f.battLevel}, {tr("Temperature"), f.tempLevel}});

    bool battSim = (f.ovrMask & (1u << 5)) != 0;
    bool tempSim = (f.ovrMask & (1u << 6)) != 0;

    m_voltage->setText(QStringLiteral("%1 V").arg(f.effBatt, 0, 'f', 2) + (battSim ? tr("  (SIM)") : QString()));
    m_voltage->setStyleSheet(QStringLiteral("color: %1;").arg(levelColor(f.battLevel).name()));

    m_battery->setCells(f.effCell, f.ovrMask, true);
    m_thermo->setTemperature(f.effTemp, tempSim, true);

    QString info = tr("RP2350 uptime %1 s   |   data age %2 ms").arg(f.tMs / 1000.0, 0, 'f', 1).arg(f.ageMs);
    if (f.imbalance){info += tr("   |   CELL VOLTAGE IMBALANCE");}
    if (f.ovrMask != 0){info += tr("   |   SIMULATED VALUES ACTIVE");}
    m_info->setText(info);
    m_info->setStyleSheet(f.ovrMask != 0 ? QStringLiteral("color: #8e24aa; font-weight: bold;") : QString());
}
