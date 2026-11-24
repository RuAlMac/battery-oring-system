#include "protocol.h"

#include <QStringList>

namespace proto {

/*******levelName*******
 * Description
        > converts a battery level number into text
 * Arguments
        > int level: 0=good 1=warning 2=danger 3=shutdown
 * Returns
        > QString: "GOOD", "WARNING", "DANGER", "SHUTDOWN" (or "?")
*/
QString levelName(int level){
    switch (level){
        case 0:     {return QStringLiteral("GOOD");}
        case 1:     {return QStringLiteral("WARNING");}
        case 2:     {return QStringLiteral("DANGER");}
        case 3:     {return QStringLiteral("SHUTDOWN");}
        default:    {return QStringLiteral("?");}
    }
}



/*******parseKeyValues*******
 * Description
        > splits "key=value key2=value2" text into a map (words without '=' are skipped)
 * Arguments
        > const QString& text: text after "OK " in a reply line
 * Returns
        > QMap<QString, QString>: key -> value
*/
QMap<QString, QString> parseKeyValues(const QString& text){
    QMap<QString, QString> map;
    const QStringList words = text.split(' ', Qt::SkipEmptyParts);

    for (const QString& word : words){
        int eq = word.indexOf('=');
        if (eq > 0){map.insert(word.left(eq), word.mid(eq + 1));}
    }

    return map;
}



/*******parseFrame*******
 * Description
        > parses an S (status) or T (telemetry) frame line
 * Arguments
        > const QString& line: received line
        > BatteryFrame* out: filled in when the line is a valid frame
 * Returns
        > bool: true=valid frame, false=not a frame
*/
bool parseFrame(const QString& line, BatteryFrame* out){
    const QStringList f = line.split(',');
    bool ok = true;
    BatteryFrame r;

    auto num = [&](int i) -> double {
        bool good = false;
        double v = f.at(i).toDouble(&good);
        if (!good){ok = false;}
        return v;
    };

    if ((f.size() == 12 || f.size() == 14 || f.size() == 16) && f.at(0) == QLatin1String("S")){
        r.hasReal = false;
        r.tMs = (qint64)num(1);
        r.ageMs = (qint64)num(2);
        r.level = (int)num(3);
        r.effBatt = num(4);
        r.effTemp = num(5);
        for (int i = 0; i < kNumCells; i++){r.effCell[i] = num(6 + i);}
        r.ovrMask = (unsigned)num(11);
        r.cellLevel = r.battLevel = r.tempLevel = r.level;
        if (f.size() >= 14){r.cdActive = ((int)num(12)) != 0; r.cdMs = (qint64)num(13);}
        if (f.size() == 16){r.latched = ((int)num(14)) != 0; r.imbalance = ((int)num(15)) != 0;}
    }
    else if ((f.size() == 24 || f.size() == 26 || f.size() == 28) && f.at(0) == QLatin1String("T")){
        r.hasReal = true;
        r.tMs = (qint64)num(1);
        r.ageMs = (qint64)num(2);
        r.seq = (qint64)num(3);
        r.level = (int)num(4);
        r.cellLevel = (int)num(5);
        r.battLevel = (int)num(6);
        r.tempLevel = (int)num(7);
        r.ovrMask = (unsigned)num(8);
        r.unsaved = ((int)num(9)) != 0;
        r.effBatt = num(10);
        for (int i = 0; i < kNumCells; i++){r.effCell[i] = num(11 + i);}
        r.effTemp = num(16);
        r.realBatt = num(17);
        for (int i = 0; i < kNumCells; i++){r.realCell[i] = num(18 + i);}
        r.realTemp = num(23);
        if (f.size() >= 26){r.cdActive = ((int)num(24)) != 0; r.cdMs = (qint64)num(25);}
        if (f.size() == 28){r.latched = ((int)num(26)) != 0; r.imbalance = ((int)num(27)) != 0;}
    }
    else {return false;}

    if (!ok){return false;}

    *out = r;
    return true;
}



/*******parseSettingLine*******
 * Description
        > parses one "SET idx=.. name=.. ..." line from the "list" command
 * Arguments
        > const QString& line: received line
        > SettingRow* out: filled in when the line is valid
 * Returns
        > bool: true=valid, false=not a SET line or fields missing
*/
bool parseSettingLine(const QString& line, SettingRow* out){
    if (!line.startsWith(QLatin1String("SET "))){return false;}

    const QMap<QString, QString> kv = parseKeyValues(line.mid(4));
    const char* required[] = {"idx", "name", "units", "active", "pending", "default", "min", "max"};
    for (const char* key : required){
        if (!kv.contains(QLatin1String(key))){return false;}
    }

    bool ok1, ok2, ok3, ok4, ok5, ok6;
    SettingRow r;
    r.idx = kv["idx"].toInt(&ok1);
    r.name = kv["name"];
    r.units = kv["units"];
    r.active = kv["active"].toDouble(&ok2);
    r.pending = kv["pending"].toDouble(&ok3);
    r.def = kv["default"].toDouble(&ok4);
    r.min = kv["min"].toDouble(&ok5);
    r.max = kv["max"].toDouble(&ok6);

    if (!(ok1 && ok2 && ok3 && ok4 && ok5 && ok6)){return false;}

    *out = r;
    return true;
}



/*******formatSettingValue*******
 * Description
        > formats a setting value for display (whole numbers for ms, 3 decimals otherwise)
 * Arguments
        > double value: value to format
        > const QString& units: setting units
 * Returns
        > QString: formatted value
*/
QString formatSettingValue(double value, const QString& units){
    if (units == QLatin1String("ms") || units == QLatin1String("Hz") || units == QLatin1String("s") || units == QLatin1String("mV") || units == QLatin1String("bool")){return QString::number(value, 'f', 0);}
    return QString::number(value, 'f', 3);
}

}   // namespace proto
