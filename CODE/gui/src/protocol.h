#ifndef PROTOCOL_H
#define PROTOCOL_H

/******************<Summary>*****************
 * Name: protocol.h
 * Purpose:
 *      > Constants and parsers for the RP2350 serial protocol
 *          (see communication/serial_interface.h in the firmware project)
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QMap>
#include <QString>

namespace proto {

/******************<Constants>*****************/
constexpr int kNumCells = 5;
constexpr int kUnpopulatedCell = 3;         // index of cell 4, not populated on the 4S battery

constexpr double kOvrCellMin = 0.0;         // must match OVR_* in battery_monitoring.h
constexpr double kOvrCellMax = 6.0;
constexpr double kOvrBattMin = 0.0;
constexpr double kOvrBattMax = 30.0;
constexpr double kOvrTempMin = -60.0;
constexpr double kOvrTempMax = 150.0;

constexpr int kStreamMinMs = 20;            // must match SERIAL_STREAM_* in serial_interface.h
constexpr int kStreamMaxMs = 10000;
constexpr int kDefaultDebugStreamMs = 100;
constexpr int kRateMinMs = 50;              // MIN_BATT_CHECK_INTERVAL
/******************</Constants>*****************/



/******************<Types>*****************/
struct BatteryFrame {
    bool hasReal = false;       // true for T frames (real values present)
    qint64 tMs = 0;             // device uptime when sent
    qint64 ageMs = 0;           // age of the underlying measurement
    qint64 seq = 0;             // T frames only
    int level = 0;              // 0=good 1=warning 2=danger 3=shutdown
    int cellLevel = 0;          // T frames only
    int battLevel = 0;          // T frames only
    int tempLevel = 0;          // T frames only
    unsigned ovrMask = 0;       // bits 0-4 cells, 5 battery, 6 temperature
    bool unsaved = false;       // T frames only
    bool cdActive = false;      // shutdown countdown running (protocol v2+; level stays 2=danger)
    qint64 cdMs = 0;            // time left until shutdown, ms
    bool latched = false;       // latched in shutdown (protocol v3+), needs "unlatch" or a reset
    bool imbalance = false;     // a cell differs too much from the others (protocol v3+)
    double effBatt = 0.0;
    double effCell[kNumCells] = {0, 0, 0, 0, 0};
    double effTemp = 0.0;
    double realBatt = 0.0;
    double realCell[kNumCells] = {0, 0, 0, 0, 0};
    double realTemp = 0.0;
};

struct SettingRow {
    int idx = 0;
    QString name;
    QString units;
    double active = 0.0;
    double pending = 0.0;
    double def = 0.0;
    double min = 0.0;
    double max = 0.0;
};
/******************</Types>*****************/



/******************<Function Declarations>*****************/
QString levelName(int level);
bool parseFrame(const QString& line, BatteryFrame* out);
bool parseSettingLine(const QString& line, SettingRow* out);
QMap<QString, QString> parseKeyValues(const QString& text);
QString formatSettingValue(double value, const QString& units);
/******************</Function Declarations>*****************/

}   // namespace proto

#endif // PROTOCOL_H
