#include "Timetable.h"

#include <QRegularExpression>
#include <algorithm>
#include <set>

namespace Campus {

namespace {

QJsonArray intArray(const QList<int> &v)
{
    QJsonArray a;
    for (int n : v)
        a.append(n);
    return a;
}

QList<int> toIntList(const QJsonValue &v)
{
    QList<int> out;
    for (const QJsonValue &x : v.toArray())
        out << x.toInt();
    return out;
}

QDate parseDate(const QString &value)
{
    static const QRegularExpression re(QStringLiteral(R"(^(\d{4})-(\d{2})-(\d{2})$)"));
    const QRegularExpressionMatch m = re.match(value);
    if (!m.hasMatch())
        return {};
    return QDate(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
}

QString dateText(const QDate &d)
{
    return d.toString(QStringLiteral("yyyy-MM-dd"));
}

int floorDiv(int a, int b)
{
    int q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0)))
        --q;
    return q;
}

} // namespace

const QList<Period> &periods()
{
    static const QList<Period> p = {
        {QStringLiteral("上午"), QStringLiteral("第1、2节"), QStringLiteral("08:30–09:55")},
        {QStringLiteral("上午"), QStringLiteral("第3、4节"), QStringLiteral("10:15–11:40")},
        {QStringLiteral("下午"), QStringLiteral("第5、6节"), QStringLiteral("14:00–15:25")},
        {QStringLiteral("下午"), QStringLiteral("第7、8节"), QStringLiteral("15:45–17:10")},
        {QStringLiteral("晚上"), QStringLiteral("第9、10节"), QStringLiteral("18:30–19:55")},
        {QStringLiteral("晚上"), QStringLiteral("第11、12节"), QStringLiteral("20:00–21:25")},
    };
    return p;
}

const QStringList &dayNames()
{
    static const QStringList d = {QStringLiteral("星期一"), QStringLiteral("星期二"),
                                  QStringLiteral("星期三"), QStringLiteral("星期四"),
                                  QStringLiteral("星期五"), QStringLiteral("星期六"),
                                  QStringLiteral("星期日")};
    return d;
}

QString clean(const QString &s)
{
    QString t = s;
    t.replace(QChar(0x00a0), QLatin1Char(' '));
    static const QRegularExpression blanks(QStringLiteral(R"([ \t]+)"));
    t.replace(blanks, QStringLiteral(" "));
    return t.trimmed();
}

// ---- Course / Schedule JSON ---------------------------------------------

bool Course::inBlock(int block) const
{
    for (int n : sections)
        if ((n + 1) / 2 == block + 1)
            return true;
    return false;
}

Course Course::fromJson(const QJsonObject &o)
{
    Course c;
    c.id = o.value(QLatin1String("id")).toString();
    c.name = o.value(QLatin1String("name")).toString();
    c.raw = o.value(QLatin1String("raw")).toString();
    c.day = o.value(QLatin1String("day")).toInt(1);
    c.sections = toIntList(o.value(QLatin1String("slots")));
    const QJsonValue w = o.value(QLatin1String("weeks"));
    c.hasWeeks = w.isArray();
    if (c.hasWeeks)
        c.weeks = toIntList(w);
    c.manual = o.value(QLatin1String("manual")).toBool(false);
    return c;
}

QJsonObject Course::toJson() const
{
    QJsonObject o;
    if (!id.isEmpty())
        o.insert(QStringLiteral("id"), id);
    if (manual)
        o.insert(QStringLiteral("manual"), true);
    o.insert(QStringLiteral("name"), name);
    o.insert(QStringLiteral("raw"), raw);
    o.insert(QStringLiteral("day"), day);
    o.insert(QStringLiteral("slots"), intArray(sections));
    o.insert(QStringLiteral("weeks"), hasWeeks ? QJsonValue(intArray(weeks)) : QJsonValue());
    return o;
}

Schedule Schedule::fromJson(const QJsonObject &o)
{
    Schedule s;
    s.memos = o.value(QLatin1String("memos")).toObject();
    for (const QJsonValue &v : o.value(QLatin1String("courses")).toArray())
        s.courses << Course::fromJson(v.toObject());
    for (const QJsonValue &v : o.value(QLatin1String("manualCourses")).toArray()) {
        Course c = Course::fromJson(v.toObject());
        c.manual = true;
        s.manualCourses << c;
    }
    s.semester = o.value(QLatin1String("semester")).toString();
    s.source = o.value(QLatin1String("source")).toString();
    s.unknownRows = o.value(QLatin1String("unknownRows")).toInt();
    s.weekOneMonday = o.value(QLatin1String("weekOneMonday")).toString();
    s.selectedWeek = o.value(QLatin1String("selectedWeek")).toString();
    s.importedAt = o.value(QLatin1String("importedAt")).toString();
    return s;
}

QJsonObject Schedule::toJson() const
{
    QJsonObject o;
    QJsonArray cs, ms;
    o.insert(QStringLiteral("memos"), memos);
    for (const Course &c : courses)
        cs.append(c.toJson());
    for (const Course &c : manualCourses)
        ms.append(c.toJson());
    o.insert(QStringLiteral("courses"), cs);
    o.insert(QStringLiteral("manualCourses"), ms);
    o.insert(QStringLiteral("semester"), semester);
    o.insert(QStringLiteral("source"), source);
    o.insert(QStringLiteral("unknownRows"), unknownRows);
    o.insert(QStringLiteral("recognized"), true);
    if (!weekOneMonday.isEmpty())
        o.insert(QStringLiteral("weekOneMonday"), weekOneMonday);
    if (!selectedWeek.isEmpty())
        o.insert(QStringLiteral("selectedWeek"), selectedWeek);
    if (!importedAt.isEmpty())
        o.insert(QStringLiteral("importedAt"), importedAt);
    return o;
}

// ---- parsing ---------------------------------------------------------------

bool weekNumbers(const QString &raw, QList<int> *out)
{
    static const QRegularExpression pattern(QString::fromUtf8(
        R"(([\d\s,，、\-－~～至]+)\s*(?:\(\s*([单双])?\s*周\s*\)|（\s*([单双])?\s*周\s*）|周(?:\s*[（(]?([单双])(?:周)?[）)]?)?))"));
    static const QRegularExpression splitter(QString::fromUtf8(R"([,，、])"));
    static const QRegularExpression range(QString::fromUtf8(R"(^(\d+)(?:[-－~～至](\d+))?$)"));

    std::set<int> weeks;
    bool found = false;
    auto it = pattern.globalMatch(raw);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        found = true;
        QString parity = m.captured(2);
        if (parity.isEmpty())
            parity = m.captured(3);
        if (parity.isEmpty())
            parity = m.captured(4);
        if (parity.isEmpty()) {
            const QString whole = m.captured(0);
            if (whole.contains(QStringLiteral("单周")))
                parity = QStringLiteral("单");
            else if (whole.contains(QStringLiteral("双周")))
                parity = QStringLiteral("双");
        }
        QString list = m.captured(1);
        list.remove(QRegularExpression(QStringLiteral(R"(\s)")));
        for (const QString &item : list.split(splitter)) {
            const QRegularExpressionMatch n = range.match(item);
            if (!n.hasMatch())
                continue;
            const int start = n.captured(1).toInt();
            const int end = n.captured(2).isEmpty() ? start : n.captured(2).toInt();
            if (start < 1 || end > 40 || end < start)
                continue;
            for (int w = start; w <= end; ++w) {
                if (parity.isEmpty() || (parity == QStringLiteral("单") ? w % 2 == 1 : w % 2 == 0))
                    weeks.insert(w);
            }
        }
    }
    if (!found || weeks.empty())
        return false;
    if (out)
        *out = QList<int>(weeks.begin(), weeks.end());
    return true;
}

QList<int> slotsFrom(const QString &text)
{
    static const QRegularExpression clock(QString::fromUtf8(
        R"(\d{1,2}:\d{2}\s*[-–—~～]\s*\d{1,2}:\d{2})"));
    static const QRegularExpression big(QString::fromUtf8(R"(第([一二三四五六1-6])大节)"));
    static const QRegularExpression withUnit(QString::fromUtf8(
        R"((?:第|\[|【)?\s*(\d{1,2})\s*(?:[,，、\-－~～至]\s*(\d{1,2}))?\s*(?:[\]】]?\s*节|[\]】]))"));
    static const QRegularExpression simple(QString::fromUtf8(
        R"(^\s*(\d{1,2})(?:\s*[-－~～、,]\s*(\d{1,2}))?\s*$)"));

    QString s = clean(text);
    s.remove(clock);

    auto range = [](int a, int b) {
        QList<int> r;
        if (a < 1 || b > 12 || b < a)
            return r;
        for (int i = a; i <= b; ++i)
            r << i;
        return r;
    };

    const QRegularExpressionMatch b = big.match(s);
    if (b.hasMatch()) {
        const QString c = b.captured(1);
        const int block = c.at(0).isDigit() ? c.toInt()
                                            : QStringLiteral("一二三四五六").indexOf(c) + 1;
        return {block * 2 - 1, block * 2};
    }
    const QRegularExpressionMatch m = withUnit.match(s);
    if (!m.hasMatch()) {
        const QRegularExpressionMatch sm = simple.match(s);
        if (!sm.hasMatch())
            return {};
        const int a = sm.captured(1).toInt();
        const int e = sm.captured(2).isEmpty() ? a : sm.captured(2).toInt();
        return range(a, e);
    }
    const int a = m.captured(1).toInt();
    const int e = m.captured(2).isEmpty() ? a : m.captured(2).toInt();
    return range(a, e);
}

bool parseManualWeeks(const QString &input, QList<int> *out)
{
    const QString in = clean(input);
    static const QRegularExpression parityTail(QString::fromUtf8(R"([单双]\s*周?$)"));
    QString normalized;
    if (parityTail.match(in).hasMatch()) {
        normalized = in;
        normalized.replace(QRegularExpression(QString::fromUtf8(R"(([单双])\s*周?$)")),
                           QString::fromUtf8("(\\1周)"));
    } else if (in.contains(QStringLiteral("周"))) {
        normalized = in;
    } else {
        normalized = in + QStringLiteral("(周)");
    }
    return weekNumbers(normalized, out);
}

// ---- date algebra ----------------------------------------------------------

QString weekOneMonday(int referenceWeek, int referenceDay, const QString &referenceDate)
{
    const QDate d = parseDate(referenceDate);
    if (!d.isValid() || referenceWeek < 1 || referenceWeek > 40 || referenceDay < 1
        || referenceDay > 7)
        return {};
    return dateText(d.addDays(-((referenceWeek - 1) * 7 + referenceDay - 1)));
}

QString dateFor(const QString &anchor, int week, int day)
{
    const QDate monday = parseDate(anchor);
    if (!monday.isValid() || week < 1 || day < 1 || day > 7)
        return {};
    return dateText(monday.addDays((week - 1) * 7 + day - 1));
}

bool reminderFor(const QString &text, const QString &anchor, Reminder *out)
{
    static const QRegularExpression re(
        QStringLiteral(R"((\d{4})[/-](\d{1,2})[/-](\d{1,2})\s+(\d{1,2}):(\d{2}))"));
    const QRegularExpressionMatch m = re.match(text);
    const QDate monday = parseDate(anchor);
    if (!m.hasMatch() || !monday.isValid())
        return false;
    const QDate date(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
    if (!date.isValid())
        return false;
    const int diff = int(monday.daysTo(date));
    const int week = floorDiv(diff, 7) + 1;
    const int day = ((diff % 7) + 7) % 7 + 1;
    if (week < 1 || week > 40)
        return false;
    const int minute = m.captured(4).toInt() * 60 + m.captured(5).toInt();
    static const int starts[] = {10 * 60 + 15, 14 * 60, 15 * 60 + 45, 18 * 60 + 30, 20 * 60};
    int block = 0;
    for (int s : starts)
        if (minute >= s)
            ++block;
    if (out) {
        out->week = week;
        out->day = day;
        out->block = block;
        out->start = QStringLiteral("%1-%2-%3 %4:%5")
                         .arg(m.captured(1))
                         .arg(m.captured(2).toInt(), 2, 10, QLatin1Char('0'))
                         .arg(m.captured(3).toInt(), 2, 10, QLatin1Char('0'))
                         .arg(m.captured(4).toInt(), 2, 10, QLatin1Char('0'))
                         .arg(m.captured(5));
    }
    return true;
}

QList<Reminder> activityMoments(const Activity &a, const QString &anchor)
{
    QList<Reminder> result;
    Reminder r;
    if (reminderFor(a.registration, anchor, &r)) {
        r.kind = Reminder::Registration;
        r.activity = &a;
        result << r;
    }
    Reminder e;
    if (reminderFor(a.activityTime, anchor, &e)) {
        e.kind = Reminder::Event;
        e.activity = &a;
        result << e;
    }
    return result;
}

} // namespace Campus
