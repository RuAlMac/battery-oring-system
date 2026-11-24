#include "simulate_panel.h"

#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <cmath>

/*******SimulatePanel::SimulatePanel*******
 * Description
        > builds the status banner and the table of readings / simulation controls
 * Arguments
        > SerialLink* link: serial link used to talk to the RP2350
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
SimulatePanel::SimulatePanel(SerialLink* link, QWidget* parent) : QWidget(parent), m_link(link) {
    auto* root = new QVBoxLayout(this);

    // level summary
        m_chips = new StatusChips(this);
        root->addWidget(m_chips);

        m_info = new QLabel(this);
        m_info->setAlignment(Qt::AlignCenter);
        root->addWidget(m_info);

    // simulation controls
        auto* box = new QGroupBox(tr("Battery conditions"), this);
        auto* boxLayout = new QHBoxLayout(box);

        // bordered table: header row with its own background, centered values, lines between all cells,
        // and it only takes the width it needs (the empty space stays to the right)
        auto* table = new QFrame(box);
        table->setObjectName(QStringLiteral("simTable"));
        table->setStyleSheet(QStringLiteral(
            "QFrame#simTable {border-top: 1px solid palette(mid); border-left: 1px solid palette(mid);}"
            "QFrame#simCell, QFrame#simHead {border-right: 1px solid palette(mid); border-bottom: 1px solid palette(mid);}"
            "QFrame#simHead {background-color: palette(button);}"));
        auto* grid = new QGridLayout(table);
        grid->setSpacing(0);
        grid->setContentsMargins(0, 0, 0, 0);

        // wraps content in a bordered cell, centered
        auto makeCell = [table](QWidget* content, bool header, QWidget* second = nullptr){
            auto* cell = new QFrame(table);
            cell->setObjectName(header ? QStringLiteral("simHead") : QStringLiteral("simCell"));
            auto* l = new QHBoxLayout(cell);
            l->setContentsMargins(12, 6, 12, 6);
            l->setSpacing(10);
            l->addStretch(1);
            l->addWidget(content);
            if (second != nullptr){l->addWidget(second);}
            l->addStretch(1);
            return cell;
        };

        const QStringList heads = {tr("Quantity"), tr("Measured"), tr("System is using"), tr("Simulate"), tr("Simulated value")};
        for (int c = 0; c < heads.size(); c++){
            auto* h = new QLabel(heads[c], table);
            QFont hf = h->font();
            hf.setBold(true);
            h->setFont(hf);
            h->setAlignment(Qt::AlignCenter);
            grid->addWidget(makeCell(h, true), 0, c);
        }

        for (int i = 0; i < proto::kNumCells; i++){
            bool populated = (i != proto::kUnpopulatedCell);
            QString label = populated ? tr("Cell %1").arg(i + 1) : tr("Cell %1 (not populated)").arg(i + 1);
            buildRow(i, label, QStringLiteral("cell %1").arg(i + 1), i, proto::kOvrCellMin, proto::kOvrCellMax, 3, QStringLiteral("V"), populated);
        }
        buildRow(proto::kNumCells, tr("Battery voltage"), QStringLiteral("batt"), 5, proto::kOvrBattMin, proto::kOvrBattMax, 3, QStringLiteral("V"), true);
        buildRow(proto::kNumCells + 1, tr("Temperature"), QStringLiteral("temp"), 6, proto::kOvrTempMin, proto::kOvrTempMax, 1, QStringLiteral("°C"), true);

        for (int i = 0; i < m_rows.size(); i++){
            Row& r = m_rows[i];
            auto* name = new QLabel(r.label, table);
            name->setAlignment(Qt::AlignCenter);
            r.measured->setAlignment(Qt::AlignCenter);
            r.used->setAlignment(Qt::AlignCenter);

            grid->addWidget(makeCell(name, false), i + 1, 0);
            grid->addWidget(makeCell(r.measured, false), i + 1, 1);
            grid->addWidget(makeCell(r.used, false), i + 1, 2);
            grid->addWidget(makeCell(r.sim, false), i + 1, 3);
            grid->addWidget(makeCell(r.slider, false, r.spin), i + 1, 4);
        }

        boxLayout->addWidget(table);
        boxLayout->addStretch(1);

        root->addWidget(box);

    // hint + buttons
        m_hint = new QLabel(this);
        m_hint->setWordWrap(true);
        root->addWidget(m_hint);

        auto* buttons = new QHBoxLayout();
        m_debugBtn = new QPushButton(tr("Switch RP2350 to Debug mode"), this);
        m_clearBtn = new QPushButton(tr("Clear all simulated values"), this);
        m_pollBtn = new QPushButton(tr("Read now"), this);
        m_pollBtn->setToolTip(tr("Ask the RP2350 for one reading (useful in Silent mode)."));
        buttons->addWidget(m_debugBtn);
        buttons->addWidget(m_clearBtn);
        buttons->addWidget(m_pollBtn);
        buttons->addStretch(1);
        root->addLayout(buttons);

        m_status = new QLabel(this);
        m_status->setWordWrap(true);
        root->addWidget(m_status);
        root->addStretch(1);

    connect(m_debugBtn, &QPushButton::clicked, this, &SimulatePanel::requestDebugMode);
    connect(m_pollBtn, &QPushButton::clicked, this, &SimulatePanel::requestPoll);
    connect(m_clearBtn, &QPushButton::clicked, this, &SimulatePanel::onClearAll);

    setConnectionState(false, QStringLiteral("normal"));
}



/*******SimulatePanel::buildRow*******
 * Description
        > creates the widgets for one quantity and wires them up
 * Arguments
        > int index: row number
        > const QString& label: name shown to the user
        > const QString& command: firmware command words ("cell 1", "batt", "temp")
        > int bit: bit of this quantity in ovr_mask
        > double lo: smallest simulated value
        > double hi: largest simulated value
        > int decimals: digits shown after the decimal point
        > const QString& unit: unit text
        > bool populated: false for the unpopulated cell (controls disabled)
 * Returns
        > N/A
*/
void SimulatePanel::buildRow(int index, const QString& label, const QString& command, int bit, double lo, double hi,
                            int decimals, const QString& unit, bool populated){
    Row r;
    r.label = label;
    r.command = command;
    r.bit = bit;
    r.lo = lo;
    r.hi = hi;
    r.decimals = decimals;
    r.unit = unit;
    r.populated = populated;

    r.measured = new QLabel(QStringLiteral("-"), this);
    r.used = new QLabel(QStringLiteral("-"), this);
    r.sim = new QCheckBox(tr("Override"), this);

    r.slider = new QSlider(Qt::Horizontal, this);
    r.slider->setRange((int)std::lround(lo * 100.0), (int)std::lround(hi * 100.0));
    r.slider->setValue((int)std::lround(((lo + hi) / 2.0) * 100.0));
    r.slider->setFixedWidth(220);

    r.spin = new QDoubleSpinBox(this);
    r.spin->setRange(lo, hi);
    r.spin->setDecimals(decimals);
    r.spin->setSingleStep(decimals == 3 ? 0.01 : 0.5);
    r.spin->setSuffix(QStringLiteral(" ") + unit);
    r.spin->setValue((lo + hi) / 2.0);

    r.sendTimer = new QTimer(this);
    r.sendTimer->setSingleShot(true);
    r.sendTimer->setInterval(80);

    m_rows.push_back(r);
    Row& stored = m_rows[index];

    if (!populated){
        stored.sim->setEnabled(false);
        stored.slider->setEnabled(false);
        stored.spin->setEnabled(false);
        return;
    }

    // slider <-> spin box
    connect(stored.slider, &QSlider::valueChanged, this, [this, index](int v){
        Row& row = m_rows[index];
        row.spin->blockSignals(true);
        row.spin->setValue(v / 100.0);
        row.spin->blockSignals(false);
        if (row.sim->isChecked()){row.sendTimer->start();}
    });
    connect(stored.spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, index](double v){
        Row& row = m_rows[index];
        row.slider->blockSignals(true);
        row.slider->setValue((int)std::lround(v * 100.0));
        row.slider->blockSignals(false);
        if (row.sim->isChecked()){row.sendTimer->start();}
    });

    connect(stored.sendTimer, &QTimer::timeout, this, [this, index](){sendOverride(index);});

    connect(stored.sim, &QCheckBox::toggled, this, [this, index](bool on){
        Row& row = m_rows[index];
        row.touched.start();
        row.touchedValid = true;
        refreshRowEnables();

        if (on){
            // start from the value the system is using right now, so enabling causes no jump
            double start = std::min(std::max(row.lastUsed, row.lo), row.hi);
            row.spin->setValue(start);
            sendOverride(index);
        }
        else {sendClear(index);}
    });
}



/*******SimulatePanel::sendOverride*******
 * Description
        > sends "ovr <quantity> <value>" for a row
 * Arguments
        > int index: row number
 * Returns
        > N/A
*/
void SimulatePanel::sendOverride(int index){
    Row& row = m_rows[index];
    row.touched.start();
    row.touchedValid = true;

    QString cmd = QStringLiteral("ovr %1 %2").arg(row.command, QString::number(row.spin->value(), 'f', row.decimals));

    m_link->sendCommand(cmd, SerialLink::Source::Gui, [this, index](bool ok, const QString& text){
        if (!ok){
            setStatus(tr("%1: %2").arg(m_rows[index].label, text), true);

            Row& r = m_rows[index];
            r.sim->blockSignals(true);
            r.sim->setChecked(false);
            r.sim->blockSignals(false);
            refreshRowEnables();
            return;
        }

        setStatus(tr("Simulating %1.").arg(m_rows[index].label), false);
        if (m_mode == QLatin1String("silent")){emit requestPoll();}
    });
}



/*******SimulatePanel::sendClear*******
 * Description
        > sends "ovr clear <quantity>" for a row
 * Arguments
        > int index: row number
 * Returns
        > N/A
*/
void SimulatePanel::sendClear(int index){
    Row& row = m_rows[index];
    QString cmd = QStringLiteral("ovr clear %1").arg(row.command);

    m_link->sendCommand(cmd, SerialLink::Source::Gui, [this, index](bool ok, const QString& text){
        if (!ok){setStatus(tr("%1: %2").arg(m_rows[index].label, text), true);}
        else {setStatus(tr("%1 is back to the real measurement.").arg(m_rows[index].label), false);}

        if (m_mode == QLatin1String("silent")){emit requestPoll();}
    });
}



/*******SimulatePanel::onClearAll*******
 * Description
        > "Clear all simulated values" button
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SimulatePanel::onClearAll(){
    for (Row& r : m_rows){
        r.sendTimer->stop();
        r.sim->blockSignals(true);
        r.sim->setChecked(false);
        r.sim->blockSignals(false);
        r.touched.start();
        r.touchedValid = true;
    }
    refreshRowEnables();

    m_link->sendCommand(QStringLiteral("ovr clear"), SerialLink::Source::Gui, [this](bool ok, const QString& text){
        if (ok){setStatus(tr("All simulated values cleared."), false);}
        else {setStatus(text, true);}

        if (m_mode == QLatin1String("silent")){emit requestPoll();}
    });
}



/*******SimulatePanel::formatValue*******
 * Description
        > formats a reading with its unit
 * Arguments
        > const Row& row: row the value belongs to
        > double value: value to format
 * Returns
        > QString: for example "3.912 V"
*/
QString SimulatePanel::formatValue(const Row& row, double value) const {
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', row.decimals), row.unit);
}



/*******SimulatePanel::setStatus*******
 * Description
        > shows a message at the bottom of the panel
 * Arguments
        > const QString& text: message
        > bool error: true=red text
 * Returns
        > N/A
*/
void SimulatePanel::setStatus(const QString& text, bool error){
    m_status->setText(text);
    m_status->setStyleSheet(error ? QStringLiteral("color: #d32f2f; font-weight: bold;") : QString());
}



/*******SimulatePanel::setConnectionState*******
 * Description
        > tells the panel whether the RP2350 is connected and which mode it is in
 * Arguments
        > bool connected: true=port open
        > const QString& mode: "normal", "debug", or "silent"
 * Returns
        > N/A
*/
void SimulatePanel::setConnectionState(bool connected, const QString& mode){
    m_connected = connected;
    m_mode = mode;

    if (!connected){
        m_chips->showText(tr("Not connected"));
        m_info->clear();
    }

    applySimulationAvailability();
}



/*******SimulatePanel::refreshRowEnables*******
 * Description
        > enables each row's controls: the Override box whenever simulation is
            available, the slider and value box only while that row's Override
            box is ticked
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SimulatePanel::refreshRowEnables(){
    for (Row& r : m_rows){
        bool usable = m_available && r.populated;
        r.sim->setEnabled(usable);
        r.slider->setEnabled(usable && r.sim->isChecked());
        r.spin->setEnabled(usable && r.sim->isChecked());
    }
}



/*******SimulatePanel::applySimulationAvailability*******
 * Description
        > enables the simulation controls only when connected and in DEBUG or SILENT mode
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SimulatePanel::applySimulationAvailability(){
    bool available = m_connected && (m_mode == QLatin1String("debug") || m_mode == QLatin1String("silent"));
    m_available = available;
    refreshRowEnables();

    m_clearBtn->setEnabled(available);
    m_pollBtn->setEnabled(m_connected);
    m_debugBtn->setVisible(m_connected && !available);

    if (!m_connected){m_hint->setText(tr("Connect to the RP2350 to see live data."));}
    else if (!available){m_hint->setText(tr("Simulation is only allowed in Debug or Silent mode, so a simulated value can never hide a real fault while the RP2350 is installed in the robot."));}
    else {m_hint->setText(tr("Tick \"Override\" to feed a value to the RP2350 in place of the real measurement. Overrides are removed when you switch to Normal mode, unplug the RP2350, or the GUI stops talking to it for 30 s."));}
}



/*******SimulatePanel::onFrame*******
 * Description
        > updates the display from a received S or T frame
 * Arguments
        > const proto::BatteryFrame& f: frame
 * Returns
        > N/A
*/
void SimulatePanel::onFrame(const proto::BatteryFrame& f){
    m_chips->setChips({{tr("Overall"), f.level}, {tr("Cells"), f.cellLevel}, {tr("Battery"), f.battLevel}, {tr("Temperature"), f.tempLevel}});

    QString info = tr("RP2350 uptime %1 s   |   data age %2 ms").arg(f.tMs / 1000.0, 0, 'f', 1).arg(f.ageMs);
    if (f.hasReal){info += tr("   |   update #%1").arg(f.seq);}
    if (f.latched){info += tr("   |   LATCHED IN SHUTDOWN");}
    if (f.imbalance){info += tr("   |   CELL IMBALANCE");}
    if (f.cdActive){info += tr("   |   SHUTDOWN IN %1 s").arg(f.cdMs / 1000.0, 0, 'f', 1);}
    if (f.ovrMask != 0){info += tr("   |   SIMULATED VALUES ACTIVE");}
    m_info->setText(info);

    for (int i = 0; i < m_rows.size(); i++){
        Row& r = m_rows[i];

        double used, measured;
        if (i < proto::kNumCells){used = f.effCell[i]; measured = f.realCell[i];}
        else if (i == proto::kNumCells){used = f.effBatt; measured = f.realBatt;}
        else {used = f.effTemp; measured = f.realTemp;}

        r.lastUsed = used;

        bool overridden = (f.ovrMask & (1u << r.bit)) != 0;

        if (!r.populated){
            r.measured->setText(QStringLiteral("-"));
            r.used->setText(QStringLiteral("-"));
            continue;
        }

        r.measured->setText(f.hasReal ? formatValue(r, measured) : QStringLiteral("-"));
        r.used->setText(formatValue(r, used) + (overridden ? tr("  (simulated)") : QString()));
        r.used->setStyleSheet(overridden ? QStringLiteral("color: #8e24aa; font-weight: bold;") : QStringLiteral("font-weight: bold;"));

        // keep the check boxes in step with the RP2350 (e.g. after an automatic clear),
        // but not right after the user touched them (the RP2350 has not answered yet)
        bool recentlyTouched = r.touchedValid && r.touched.isValid() && r.touched.elapsed() < 800;
        if (!recentlyTouched && r.sim->isChecked() != overridden){
            r.sim->blockSignals(true);
            r.sim->setChecked(overridden);
            r.sim->blockSignals(false);
            refreshRowEnables();

            if (overridden){
                r.spin->blockSignals(true);
                r.slider->blockSignals(true);
                r.spin->setValue(std::min(std::max(used, r.lo), r.hi));
                r.slider->setValue((int)std::lround(r.spin->value() * 100.0));
                r.slider->blockSignals(false);
                r.spin->blockSignals(false);
            }
        }
    }
}
