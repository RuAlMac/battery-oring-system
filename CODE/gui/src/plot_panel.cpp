#include "plot_panel.h"

#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>
#include <cmath>
#include <limits>

#include "gauges.h"

namespace {
constexpr int kMaxSamples = 200000;     // oldest samples are dropped beyond this (about 5.5 h at 10 samples/s)
}

/*******PlotPanel::PlotPanel*******
 * Description
        > builds the control row, the overlay plot with its value selection, and the six small plots
 * Arguments
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
PlotPanel::PlotPanel(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    // controls
        auto* controls = new QHBoxLayout();
        m_recordBtn = new QPushButton(tr("Start recording"), this);
        m_recordBtn->setCheckable(true);
        m_recordBtn->setToolTip(tr("While recording, every frame received from the RP2350 is added to the plots.\n"
                                   "Frames arrive in every mode (use Read now in Silent mode)."));
        m_clearBtn = new QPushButton(tr("Clear plotted data"), this);
        m_saveBtn = new QPushButton(tr("Save as CSV..."), this);

        m_window = new QComboBox(this);
        m_window->addItem(tr("Everything"), 0);
        m_window->addItem(tr("Last 30 s"), 30);
        m_window->addItem(tr("Last 1 min"), 60);
        m_window->addItem(tr("Last 5 min"), 300);
        m_window->addItem(tr("Last 15 min"), 900);
        m_window->addItem(tr("Last 1 h"), 3600);

        m_status = new QLabel(this);

        controls->addWidget(m_recordBtn);
        controls->addWidget(m_clearBtn);
        controls->addWidget(m_saveBtn);
        controls->addSpacing(16);
        controls->addWidget(new QLabel(tr("Time shown:"), this));
        controls->addWidget(m_window);
        controls->addStretch(1);
        controls->addWidget(m_status);
        root->addLayout(controls);

    // overlay plot + selection
        m_overlay = new PlotWidget(&m_data, this);
        auto* overlayBox = new QWidget(this);
        auto* ov = new QVBoxLayout(overlayBox);
        ov->setContentsMargins(0, 0, 0, 0);
        auto* checks = new QHBoxLayout();
        checks->addWidget(new QLabel(tr("Show:"), overlayBox));
        for (int s = 0; s < plotdata::kSeriesCount; ++s){
            const auto& inf = plotdata::info(s);
            auto* cb = new QCheckBox(QString::fromLatin1(inf.name), overlayBox);
            cb->setChecked(true);
            cb->setStyleSheet(QStringLiteral("QCheckBox {color: %1; font-weight: bold;}").arg(inf.color.name()));
            m_overlayChecks[s] = cb;
            checks->addWidget(cb);
            connect(cb, &QCheckBox::toggled, this, &PlotPanel::updateOverlaySeries);
        }
        checks->addStretch(1);
        auto* note = new QLabel(tr("each trace uses its own scale (see min / max in the legend boxes)"), overlayBox);
        note->setEnabled(false);
        checks->addWidget(note);
        ov->addLayout(checks);
        ov->addWidget(m_overlay, 1);
        root->addWidget(makeBox(tr("All Values"), overlayBox), 5);

    // one plot per value
        auto* grid = new QGridLayout();
        for (int s = 0; s < plotdata::kSeriesCount; ++s){
            m_plots[s] = new PlotWidget(&m_data, this);
            m_plots[s]->setSeries({s});
            auto* frame = new QFrame(this);
            frame->setFrameShape(QFrame::StyledPanel);
            frame->setFrameShadow(QFrame::Plain);
            auto* fl = new QVBoxLayout(frame);
            fl->setContentsMargins(4, 2, 4, 2);
            fl->addWidget(m_plots[s]);
            grid->addWidget(frame, s / 3, s % 3);
        }
        root->addLayout(grid, 4);

    connect(m_recordBtn, &QPushButton::toggled, this, &PlotPanel::onRecordToggled);
    connect(m_clearBtn, &QPushButton::clicked, this, &PlotPanel::clearData);
    connect(m_saveBtn, &QPushButton::clicked, this, &PlotPanel::saveCsv);
    connect(m_window, &QComboBox::currentIndexChanged, this, &PlotPanel::updateWindow);

    updateOverlaySeries();
    refreshStatus();
}



/*******PlotPanel::makeBox*******
 * Description
        > wraps a widget in a framed box with a centered bold title (same title size as the Monitor page)
 * Arguments
        > const QString& title: text shown above the content
        > QWidget* content: the content (becomes a child of the box)
 * Returns
        > QWidget*: the box
*/
QWidget* PlotPanel::makeBox(const QString& title, QWidget* content){
    auto* frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Plain);
    frame->setLineWidth(1);

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 6, 12, 8);

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



/*******PlotPanel::setConnectionState*******
 * Description
        > remembers whether the port is open (shown in the status text; recording continues across reconnects)
 * Arguments
        > bool connected: true=port open
 * Returns
        > N/A
*/
void PlotPanel::setConnectionState(bool connected){
    m_connected = connected;
    if (!connected){m_gapPending = true;}       // unplugged: break the lines when data returns
    refreshStatus();
}



/*******PlotPanel::onRecordToggled*******
 * Description
        > starts or stops adding frames to the plots
 * Arguments
        > bool on: true=start recording
 * Returns
        > N/A
*/
void PlotPanel::onRecordToggled(bool on){
    m_recording = on;
    m_recordBtn->setText(on ? tr("Stop recording") : tr("Start recording"));
    if (on){m_gapPending = true;}               // a pause is shown as a gap, not as a straight line
    refreshStatus();
}



/*******PlotPanel::clearData*******
 * Description
        > deletes all plotted data (recording state is unchanged, a running recording starts again at 0:00)
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void PlotPanel::clearData(){
    m_data.clear();
    m_lastDeviceMs = -1;
    m_gapPending = false;
    refreshPlots();
    refreshStatus();
}



/*******PlotPanel::onFrame*******
 * Description
        > adds the values of a received S or T frame to the plots (only while recording)
 * Arguments
        > const proto::BatteryFrame& f: frame
 * Returns
        > N/A
*/
void PlotPanel::onFrame(const proto::BatteryFrame& f){
    if (!m_recording){return;}
    if (f.tMs == m_lastDeviceMs){return;}       // the same frame twice (e.g. S plus a reply to "status")

    const double nan = std::numeric_limits<double>::quiet_NaN();

    // time axis follows the RP2350 uptime, so the spacing of samples is exact
    if (m_data.size() == 0){
        m_timeBaseMs = static_cast<double>(f.tMs);
    }
    else if (f.tMs < m_lastDeviceMs){
        // the RP2350 restarted: continue the time axis where it stopped and break the lines
        m_timeBaseMs = static_cast<double>(f.tMs) - m_data.t.last() * 1000.0;
        m_gapPending = true;
    }
    m_lastDeviceMs = f.tMs;
    const double t = (static_cast<double>(f.tMs) - m_timeBaseMs) / 1000.0;

    if (m_gapPending && m_data.size() > 0){
        m_data.t.append(t);
        for (auto& s : m_data.v){s.append(nan);}
    }
    m_gapPending = false;

    m_data.t.append(t);
    m_data.v[0].append(f.effCell[0]);
    m_data.v[1].append(f.effCell[1]);
    m_data.v[2].append(f.effCell[2]);
    m_data.v[3].append(f.effCell[4]);       // cell 4 is not populated
    m_data.v[4].append(f.effBatt);
    m_data.v[5].append(f.effTemp);

    if (m_data.size() > kMaxSamples){
        const int drop = kMaxSamples / 10;
        m_data.t.remove(0, drop);
        for (auto& s : m_data.v){s.remove(0, drop);}
    }

    refreshPlots();
    refreshStatus();
}



/*******PlotPanel::refreshPlots*******
 * Description
        > repaints every plot
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void PlotPanel::refreshPlots(){
    m_overlay->update();
    for (auto* p : m_plots){p->update();}
}



/*******PlotPanel::refreshStatus*******
 * Description
        > updates the status text and the enabled state of the buttons
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void PlotPanel::refreshStatus(){
    QString text;
    if (m_recording){
        text = tr("Recording - %1 samples").arg(m_data.size());
        if (!m_connected){text += tr(" (waiting for the RP2350)");}
    }
    else {
        text = m_data.size() > 0 ? tr("Paused - %1 samples").arg(m_data.size()) : tr("Not recording");
    }
    if (m_data.size() > 1){
        const long long s = static_cast<long long>(m_data.t.last() - m_data.t.first());
        text += tr(" (%1:%2)").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
    }
    m_status->setText(text);
    m_status->setStyleSheet(m_recording ? QStringLiteral("color: #d32f2f; font-weight: bold;") : QString());
    m_clearBtn->setEnabled(m_data.size() > 0);
    m_saveBtn->setEnabled(m_data.size() > 0);
}



/*******PlotPanel::updateOverlaySeries*******
 * Description
        > applies the ticked checkboxes to the large plot
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void PlotPanel::updateOverlaySeries(){
    QList<int> chosen;
    for (int s = 0; s < plotdata::kSeriesCount; ++s){
        if (m_overlayChecks[s]->isChecked()){chosen.append(s);}
    }
    m_overlay->setSeries(chosen);
}



/*******PlotPanel::updateWindow*******
 * Description
        > applies the "Time shown" selection to every plot
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void PlotPanel::updateWindow(){
    const double w = m_window->currentData().toDouble();
    m_overlay->setWindow(w);
    for (auto* p : m_plots){p->setWindow(w);}
}



/*******PlotPanel::saveCsv*******
 * Description
        > writes the recorded samples to a CSV file chosen by the user
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void PlotPanel::saveCsv(){
    const QString path = QFileDialog::getSaveFileName(this, tr("Save recorded data"), QStringLiteral("battery_log.csv"), tr("CSV files (*.csv)"));
    if (path.isEmpty()){return;}

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::warning(this, tr("Save"), tr("Could not write %1").arg(path));
        return;
    }
    QTextStream out(&file);
    out << "time_s";
    for (int s = 0; s < plotdata::kSeriesCount; ++s){
        out << ',' << plotdata::info(s).name << " (" << QString::fromUtf8(plotdata::info(s).units) << ')';
    }
    out << '\n';
    for (int i = 0; i < m_data.size(); ++i){
        if (std::isnan(m_data.v[0][i])){continue;}      // gap marker
        out << QString::number(m_data.t[i], 'f', 3);
        for (int s = 0; s < plotdata::kSeriesCount; ++s){out << ',' << QString::number(m_data.v[s][i], 'f', 3);}
        out << '\n';
    }
}
